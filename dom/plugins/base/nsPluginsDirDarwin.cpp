














































#include "GeckoChildProcessHost.h"
#include "base/process_util.h"

#include "prlink.h"
#include "prnetdb.h"
#include "nsXPCOM.h"

#include "nsPluginsDir.h"
#include "nsNPAPIPlugin.h"
#include "nsPluginsDirUtils.h"

#include "nsILocalFileMac.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>

typedef NS_NPAPIPLUGIN_CALLBACK(const char *, NP_GETMIMEDESCRIPTION) ();
typedef NS_NPAPIPLUGIN_CALLBACK(OSErr, BP_GETSUPPORTEDMIMETYPES) (BPSupportedMIMETypes *mimeInfo, UInt32 flags);






static CFBundleRef getPluginBundle(const char* path)
{
  CFBundleRef bundle = NULL;
  CFStringRef pathRef = ::CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
  if (pathRef) {
    CFURLRef bundleURL = ::CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, true);
    if (bundleURL) {
      bundle = ::CFBundleCreate(NULL, bundleURL);
      ::CFRelease(bundleURL);
    }
    ::CFRelease(pathRef);
  }
  return bundle;
}

static nsresult toCFURLRef(nsIFile* file, CFURLRef& outURL)
{
  nsCOMPtr<nsILocalFileMac> lfm = do_QueryInterface(file);
  if (!lfm)
    return NS_ERROR_FAILURE;
  CFURLRef url;
  nsresult rv = lfm->GetCFURL(&url);
  if (NS_SUCCEEDED(rv))
    outURL = url;
  
  return rv;
}

PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
  nsCString fileName;
  file->GetNativeLeafName(fileName);
  



  if (!strcmp(fileName.get(), "VerifiedDownloadPlugin.plugin")) {
    NS_WARNING("Preventing load of VerifiedDownloadPlugin.plugin (see bug 436575)");
    return PR_FALSE;
  }
  return PR_TRUE;
}


static char* CFStringRefToUTF8Buffer(CFStringRef cfString)
{
  const char* buffer = ::CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8);
  if (buffer) {
    return PL_strdup(buffer);
  }

  int bufferLength =
    ::CFStringGetMaximumSizeForEncoding(::CFStringGetLength(cfString),
                                        kCFStringEncodingUTF8) + 1;
  char* newBuffer = static_cast<char*>(NS_Alloc(bufferLength));
  if (!newBuffer) {
    return nsnull;
  }

  if (!::CFStringGetCString(cfString, newBuffer, bufferLength,
                            kCFStringEncodingUTF8)) {
    NS_Free(newBuffer);
    return nsnull;
  }

  newBuffer = static_cast<char*>(NS_Realloc(newBuffer,
                                            PL_strlen(newBuffer) + 1));
  return newBuffer;
}

class AutoCFTypeObject {
public:
  AutoCFTypeObject(CFTypeRef object)
  {
    mObject = object;
  }
  ~AutoCFTypeObject()
  {
    ::CFRelease(mObject);
  }
private:
  CFTypeRef mObject;
};

static Boolean MimeTypeEnabled(CFDictionaryRef mimeDict) {
  if (!mimeDict) {
    return true;
  }
  
  CFTypeRef value;
  if (::CFDictionaryGetValueIfPresent(mimeDict, CFSTR("WebPluginTypeEnabled"), &value)) {
    if (value && ::CFGetTypeID(value) == ::CFBooleanGetTypeID()) {
      return ::CFBooleanGetValue(static_cast<CFBooleanRef>(value));
    }
  }
  return true;
}

static CFDictionaryRef ParsePlistForMIMETypesFilename(CFBundleRef bundle)
{
  CFTypeRef mimeFileName = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("WebPluginMIMETypesFilename"));
  if (!mimeFileName || ::CFGetTypeID(mimeFileName) != ::CFStringGetTypeID()) {
    return NULL;
  }
  
  FSRef homeDir;
  if (::FSFindFolder(kUserDomain, kCurrentUserFolderType, kDontCreateFolder, &homeDir) != noErr) {
    return NULL;
  }
  
  CFURLRef userDirURL = ::CFURLCreateFromFSRef(kCFAllocatorDefault, &homeDir);
  if (!userDirURL) {
    return NULL;
  }
  
  AutoCFTypeObject userDirURLAutorelease(userDirURL);
  CFStringRef mimeFilePath = ::CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("Library/Preferences/%@"), static_cast<CFStringRef>(mimeFileName));
  if (!mimeFilePath) {
    return NULL;
  }
  
  AutoCFTypeObject mimeFilePathAutorelease(mimeFilePath);
  CFURLRef mimeFileURL = ::CFURLCreateWithFileSystemPathRelativeToBase(kCFAllocatorDefault, mimeFilePath, kCFURLPOSIXPathStyle, false, userDirURL);
  if (!mimeFileURL) {
    return NULL;
  }
  
  AutoCFTypeObject mimeFileURLAutorelease(mimeFileURL);
  SInt32 errorCode = 0;
  CFDataRef mimeFileData = NULL;
  Boolean result = ::CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, mimeFileURL, &mimeFileData, NULL, NULL, &errorCode);
  if (!result) {
    return NULL;
  }
  
  AutoCFTypeObject mimeFileDataAutorelease(mimeFileData);
  if (errorCode != 0) {
    return NULL;
  }
  
  CFPropertyListRef propertyList = ::CFPropertyListCreateFromXMLData(kCFAllocatorDefault, mimeFileData, kCFPropertyListImmutable, NULL);
  if (!propertyList) {
    return NULL;
  }
  
  AutoCFTypeObject propertyListAutorelease(propertyList);
  if (::CFGetTypeID(propertyList) != ::CFDictionaryGetTypeID()) {
    return NULL;
  }

  CFTypeRef mimeTypes = ::CFDictionaryGetValue(static_cast<CFDictionaryRef>(propertyList), CFSTR("WebPluginMIMETypes"));
  if (!mimeTypes || ::CFGetTypeID(mimeTypes) != ::CFDictionaryGetTypeID() || ::CFDictionaryGetCount(static_cast<CFDictionaryRef>(mimeTypes)) == 0) {
    return NULL;
  }
  
  return static_cast<CFDictionaryRef>(::CFRetain(mimeTypes));
}

static void ParsePlistPluginInfo(nsPluginInfo& info, CFBundleRef bundle)
{
  CFDictionaryRef mimeDict = ParsePlistForMIMETypesFilename(bundle);
  
  if (!mimeDict) {
    CFTypeRef mimeTypes = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("WebPluginMIMETypes"));
    if (!mimeTypes || ::CFGetTypeID(mimeTypes) != ::CFDictionaryGetTypeID() || ::CFDictionaryGetCount(static_cast<CFDictionaryRef>(mimeTypes)) == 0)
      return;
    mimeDict = static_cast<CFDictionaryRef>(::CFRetain(mimeTypes));
  }
  
  AutoCFTypeObject mimeDictAutorelease(mimeDict);
  int mimeDictKeyCount = ::CFDictionaryGetCount(mimeDict);

  
  int mimeDataArraySize = mimeDictKeyCount * sizeof(char*);
  info.fMimeTypeArray = static_cast<char**>(NS_Alloc(mimeDataArraySize));
  if (!info.fMimeTypeArray)
    return;
  memset(info.fMimeTypeArray, 0, mimeDataArraySize);
  info.fExtensionArray = static_cast<char**>(NS_Alloc(mimeDataArraySize));
  if (!info.fExtensionArray)
    return;
  memset(info.fExtensionArray, 0, mimeDataArraySize);
  info.fMimeDescriptionArray = static_cast<char**>(NS_Alloc(mimeDataArraySize));
  if (!info.fMimeDescriptionArray)
    return;
  memset(info.fMimeDescriptionArray, 0, mimeDataArraySize);

  
  nsAutoArrayPtr<CFTypeRef> keys(new CFTypeRef[mimeDictKeyCount]);
  if (!keys)
    return;
  nsAutoArrayPtr<CFTypeRef> values(new CFTypeRef[mimeDictKeyCount]);
  if (!values)
    return;
  
  info.fVariantCount = 0;

  ::CFDictionaryGetKeysAndValues(mimeDict, keys, values);
  for (int i = 0; i < mimeDictKeyCount; i++) {
    CFTypeRef mimeString = keys[i];
    if (!mimeString || ::CFGetTypeID(mimeString) != ::CFStringGetTypeID()) {
      continue;
    }
    CFTypeRef mimeDict = values[i];
    if (mimeDict && ::CFGetTypeID(mimeDict) == ::CFDictionaryGetTypeID()) {
      if (!MimeTypeEnabled(static_cast<CFDictionaryRef>(mimeDict))) {
        continue;
      }
      info.fMimeTypeArray[info.fVariantCount] = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(mimeString));
      if (!info.fMimeTypeArray[info.fVariantCount]) {
        continue;
      }
      CFTypeRef extensions = ::CFDictionaryGetValue(static_cast<CFDictionaryRef>(mimeDict), CFSTR("WebPluginExtensions"));
      if (extensions && ::CFGetTypeID(extensions) == ::CFArrayGetTypeID()) {
        int extensionCount = ::CFArrayGetCount(static_cast<CFArrayRef>(extensions));
        CFMutableStringRef extensionList = ::CFStringCreateMutable(kCFAllocatorDefault, 0);
        for (int j = 0; j < extensionCount; j++) {
          CFTypeRef extension = ::CFArrayGetValueAtIndex(static_cast<CFArrayRef>(extensions), j);
          if (extension && ::CFGetTypeID(extension) == ::CFStringGetTypeID()) {
            if (j > 0)
              ::CFStringAppend(extensionList, CFSTR(","));
            ::CFStringAppend(static_cast<CFMutableStringRef>(extensionList), static_cast<CFStringRef>(extension));
          }
        }
        info.fExtensionArray[info.fVariantCount] = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(extensionList));
        ::CFRelease(extensionList);
      }
      CFTypeRef description = ::CFDictionaryGetValue(static_cast<CFDictionaryRef>(mimeDict), CFSTR("WebPluginTypeDescription"));
      if (description && ::CFGetTypeID(description) == ::CFStringGetTypeID())
        info.fMimeDescriptionArray[info.fVariantCount] = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(description));
    }
    info.fVariantCount++;
  }
}

nsPluginFile::nsPluginFile(nsIFile *spec)
    : mPlugin(spec)
{
}

nsPluginFile::~nsPluginFile() {}

nsresult nsPluginFile::LoadPlugin(PRLibrary **outLibrary)
{
  if (!mPlugin)
    return NS_ERROR_NULL_POINTER;

  
  
  
  
  
  
  
#ifdef __LP64__
  char executablePath[PATH_MAX];
  executablePath[0] = '\0';
  nsCAutoString bundlePath;
  mPlugin->GetNativePath(bundlePath);
  CFStringRef pathRef = ::CFStringCreateWithCString(NULL, bundlePath.get(), kCFStringEncodingUTF8);
  if (pathRef) {
    CFURLRef bundleURL = ::CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, true);
    if (bundleURL) {
      CFBundleRef bundle = ::CFBundleCreate(NULL, bundleURL);
      if (bundle) {
        CFURLRef executableURL = ::CFBundleCopyExecutableURL(bundle);
        if (executableURL) {
          if (!::CFURLGetFileSystemRepresentation(executableURL, true, (UInt8*)&executablePath, PATH_MAX))
            executablePath[0] = '\0';
          ::CFRelease(executableURL);
        }
        ::CFRelease(bundle);
      }
      ::CFRelease(bundleURL);
    }
    ::CFRelease(pathRef); 
  }
#else
  nsCAutoString bundlePath;
  mPlugin->GetNativePath(bundlePath);
  const char *executablePath = bundlePath.get();
#endif

  *outLibrary = PR_LoadLibrary(executablePath);
  pLibrary = *outLibrary;
  if (!pLibrary) {
    return NS_ERROR_FAILURE;
  }
#ifdef DEBUG
  printf("[loaded plugin %s]\n", bundlePath.get());
#endif
  return NS_OK;
}

static char* p2cstrdup(StringPtr pstr)
{
  int len = pstr[0];
  char* cstr = static_cast<char*>(NS_Alloc(len + 1));
  if (cstr) {
    memmove(cstr, pstr + 1, len);
    cstr[len] = '\0';
  }
  return cstr;
}

static char* GetNextPluginStringFromHandle(Handle h, short *index)
{
  char *ret = p2cstrdup((unsigned char*)(*h + *index));
  *index += (ret ? PL_strlen(ret) : 0) + 1;
  return ret;
}

static PRBool IsCompatibleArch(nsIFile *file)
{
  CFURLRef pluginURL = NULL;
  if (NS_FAILED(toCFURLRef(file, pluginURL)))
    return PR_FALSE;
  
  PRBool isPluginFile = PR_FALSE;

  CFBundleRef pluginBundle = ::CFBundleCreate(kCFAllocatorDefault, pluginURL);
  if (pluginBundle) {
    UInt32 packageType, packageCreator;
    ::CFBundleGetPackageInfo(pluginBundle, &packageType, &packageCreator);
    if (packageType == 'BRPL' || packageType == 'IEPL' || packageType == 'NSPL') {
      
      char executablePath[PATH_MAX];
      executablePath[0] = '\0';
      if (!::CFURLGetFileSystemRepresentation(pluginURL, true, (UInt8*)&executablePath, PATH_MAX)) {
        executablePath[0] = '\0';
      }

      uint32 pluginLibArchitectures;
      nsresult rv = mozilla::ipc::GeckoChildProcessHost::GetArchitecturesForBinary(executablePath, &pluginLibArchitectures);
      if (NS_FAILED(rv)) {
        return PR_FALSE;
      }

      uint32 containerArchitectures = mozilla::ipc::GeckoChildProcessHost::GetSupportedArchitecturesForProcessType(GeckoProcessType_Plugin);

      
      isPluginFile = !!(containerArchitectures & pluginLibArchitectures);
    }
    ::CFRelease(pluginBundle);
  }

  ::CFRelease(pluginURL);
  return isPluginFile;
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info, PRLibrary **outLibrary)
{
  *outLibrary = nsnull;

  nsresult rv = NS_OK;

  if (!IsCompatibleArch(mPlugin)) {
      return NS_ERROR_FAILURE;
  }

  
  memset(&info, 0, sizeof(info));

  
  nsCAutoString path;
  if (NS_FAILED(rv = mPlugin->GetNativePath(path)))
    return rv;
  CFBundleRef bundle = getPluginBundle(path.get());

  
  info.fFullPath = PL_strdup(path.get());

  
  nsCAutoString fileName;
  if (NS_FAILED(rv = mPlugin->GetNativeLeafName(fileName)))
    return rv;
  info.fFileName = PL_strdup(fileName.get());

  
  if (bundle) {
    CFTypeRef name = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("WebPluginName"));
    if (name && ::CFGetTypeID(name) == ::CFStringGetTypeID())
      info.fName = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(name));
  }

  
  if (bundle) {
    CFTypeRef description = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("WebPluginDescription"));
    if (description && ::CFGetTypeID(description) == ::CFStringGetTypeID())
      info.fDescription = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(description));
  }

  
  if (bundle) {
    
    CFTypeRef version = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleShortVersionString"));
    if (!version) 
      version = ::CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);
    if (version && ::CFGetTypeID(version) == ::CFStringGetTypeID())
      info.fVersion = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(version));
  }

  
  

  
  if (bundle) {
    ParsePlistPluginInfo(info, bundle);
    ::CFRelease(bundle);
    if (info.fVariantCount > 0)
      return NS_OK;
  }

  
  
  
  

  
  rv = LoadPlugin(outLibrary);
  if (NS_FAILED(rv))
    return rv;

  
  if (pLibrary) {
    NP_GETMIMEDESCRIPTION pfnGetMimeDesc = (NP_GETMIMEDESCRIPTION)PR_FindFunctionSymbol(pLibrary, NP_GETMIMEDESCRIPTION_NAME); 
    if (pfnGetMimeDesc)
      ParsePluginMimeDescription(pfnGetMimeDesc(), info);
    if (info.fVariantCount)
      return NS_OK;
  }

  
  BPSupportedMIMETypes mi = {kBPSupportedMIMETypesStructVers_1, NULL, NULL};

  
  if (pLibrary) {
    BP_GETSUPPORTEDMIMETYPES pfnMime = (BP_GETSUPPORTEDMIMETYPES)PR_FindFunctionSymbol(pLibrary, "BP_GetSupportedMIMETypes");
    if (pfnMime && noErr == pfnMime(&mi, 0) && mi.typeStrings) {
      info.fVariantCount = (**(short**)mi.typeStrings) / 2;
      ::HLock(mi.typeStrings);
      if (mi.infoStrings)  
        ::HLock(mi.infoStrings);
    }
  }

  
  int variantCount = info.fVariantCount;
  info.fMimeTypeArray = static_cast<char**>(NS_Alloc(variantCount * sizeof(char*)));
  if (!info.fMimeTypeArray)
    return NS_ERROR_OUT_OF_MEMORY;
  info.fExtensionArray = static_cast<char**>(NS_Alloc(variantCount * sizeof(char*)));
  if (!info.fExtensionArray)
    return NS_ERROR_OUT_OF_MEMORY;
  if (mi.infoStrings) {
    info.fMimeDescriptionArray = static_cast<char**>(NS_Alloc(variantCount * sizeof(char*)));
    if (!info.fMimeDescriptionArray)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  short mimeIndex = 2;
  short descriptionIndex = 2;
  for (int i = 0; i < variantCount; i++) {
    info.fMimeTypeArray[i] = GetNextPluginStringFromHandle(mi.typeStrings, &mimeIndex);
    info.fExtensionArray[i] = GetNextPluginStringFromHandle(mi.typeStrings, &mimeIndex);
    if (mi.infoStrings)
      info.fMimeDescriptionArray[i] = GetNextPluginStringFromHandle(mi.infoStrings, &descriptionIndex);
  }

  ::HUnlock(mi.typeStrings);
  ::DisposeHandle(mi.typeStrings);
  if (mi.infoStrings) {
    ::HUnlock(mi.infoStrings);
    ::DisposeHandle(mi.infoStrings);
  }

  return NS_OK;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
  NS_Free(info.fName);
  NS_Free(info.fDescription);
  int variantCount = info.fVariantCount;
  for (int i = 0; i < variantCount; i++) {
    NS_Free(info.fMimeTypeArray[i]);
    NS_Free(info.fExtensionArray[i]);
    NS_Free(info.fMimeDescriptionArray[i]);
  }
  NS_Free(info.fMimeTypeArray);
  NS_Free(info.fMimeDescriptionArray);
  NS_Free(info.fExtensionArray);
  NS_Free(info.fFileName);
  NS_Free(info.fFullPath);
  NS_Free(info.fVersion);

  return NS_OK;
}
