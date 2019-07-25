














































#ifdef MOZ_IPC
#include "GeckoChildProcessHost.h"
#include "base/process_util.h"
#endif

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
    
  CFURLRef pluginURL = NULL;
  if (NS_FAILED(toCFURLRef(file, pluginURL)))
    return PR_FALSE;
  
  PRBool isPluginFile = PR_FALSE;

  CFBundleRef pluginBundle = ::CFBundleCreate(kCFAllocatorDefault, pluginURL);
  if (pluginBundle) {
    UInt32 packageType, packageCreator;
    ::CFBundleGetPackageInfo(pluginBundle, &packageType, &packageCreator);
    if (packageType == 'BRPL' || packageType == 'IEPL' || packageType == 'NSPL') {
#ifdef MOZ_IPC
      
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
#else
      isPluginFile = !!::CFBundlePreflightExecutable(pluginBundle, NULL);
#endif
    }
    ::CFRelease(pluginBundle);
  }

  ::CFRelease(pluginURL);
  return isPluginFile;
}


static char* CFStringRefToUTF8Buffer(CFStringRef cfString)
{
  int bufferLength = ::CFStringGetLength(cfString) + 1;
  char* newBuffer = static_cast<char*>(NS_Alloc(bufferLength));
  if (newBuffer && !::CFStringGetCString(cfString, newBuffer, bufferLength, kCFStringEncodingUTF8)) {
    NS_Free(newBuffer);
    newBuffer = nsnull;
  }
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

static Boolean ShouldIgnoreMIMEType(CFBundleRef bundle, CFStringRef mimeString) {
  CFTypeRef bundleID = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleIdentifier"));
  if (!bundleID || ::CFGetTypeID(bundleID) != ::CFStringGetTypeID())
    return false;
    
  
  if ((::CFStringCompare(static_cast<CFStringRef>(bundleID), CFSTR("com.apple.QuickTime Plugin.plugin"), 0) == kCFCompareEqualTo) && 
      (::CFStringCompare(static_cast<CFStringRef>(mimeString), CFSTR("application/pdf"), 0) == kCFCompareEqualTo))
    return true;
  
  return false;
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
  
  CFTypeRef localizedName = ::CFDictionaryGetValue(static_cast<CFDictionaryRef>(propertyList), CFSTR("WebPluginLocalizationName"));
  if (!localizedName || ::CFGetTypeID(localizedName) != ::CFStringGetTypeID()) {
    return NULL;
  }
  
  CFLocaleRef currentLocale = ::CFLocaleCopyCurrent();
  if (!currentLocale) {
    return NULL;
  }
  
  AutoCFTypeObject currentLocaleAutorelease(currentLocale);
  if (::CFStringCompare(static_cast<CFStringRef>(localizedName), ::CFLocaleGetIdentifier(currentLocale), 0) != kCFCompareEqualTo) {
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
    if (mimeString && ::CFGetTypeID(mimeString) == ::CFStringGetTypeID() && !ShouldIgnoreMIMEType(bundle, static_cast<CFStringRef>(mimeString))) {
      info.fMimeTypeArray[info.fVariantCount] = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(mimeString));
    }
    else {
      continue;
    }
    CFTypeRef mimeDict = values[i];
    if (mimeDict && ::CFGetTypeID(mimeDict) == ::CFDictionaryGetTypeID()) {
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

#ifndef __LP64__
static char* GetPluginString(short id, short index)
{
  Str255 str;
  ::GetIndString(str, id, index);
  return p2cstrdup(str);
}



static short OpenPluginResourceFork(nsIFile *pluginFile)
{
  FSSpec spec;
  nsCOMPtr<nsILocalFileMac> lfm = do_QueryInterface(pluginFile);
  if (!lfm || NS_FAILED(lfm->GetFSSpec(&spec)))
    return -1;

  Boolean targetIsFolder, wasAliased;
  ::ResolveAliasFile(&spec, true, &targetIsFolder, &wasAliased);
  short refNum = ::FSpOpenResFile(&spec, fsRdPerm);
  if (refNum < 0) {
    nsCString path;
    pluginFile->GetNativePath(path);
    CFBundleRef bundle = getPluginBundle(path.get());
    if (bundle) {
      refNum = CFBundleOpenBundleResourceMap(bundle);
      ::CFRelease(bundle);
    }
  }
  return refNum;
}

short nsPluginFile::OpenPluginResource()
{
  return OpenPluginResourceFork(mPlugin);
}

class nsAutoCloseResourceObject {
public:
  nsAutoCloseResourceObject(nsIFile *pluginFile)
  {
    mRefNum = OpenPluginResourceFork(pluginFile);
  }
  ~nsAutoCloseResourceObject()
  {
    if (mRefNum > 0)
      ::CloseResFile(mRefNum);
  }
  PRBool ResourceOpened()
  {
    return (mRefNum > 0);
  }
private:
  short mRefNum;
};
#endif




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info, PRLibrary **outLibrary)
{
  *outLibrary = nsnull;

  nsresult rv = NS_OK;

  
  memset(&info, 0, sizeof(info));

#ifndef __LP64__
  
  nsAutoCloseResourceObject resourceObject(mPlugin);
  bool resourceOpened = resourceObject.ResourceOpened();
#endif

  
  nsCAutoString path;
  if (NS_FAILED(rv = mPlugin->GetNativePath(path)))
    return rv;
  CFBundleRef bundle = getPluginBundle(path.get());

  
  info.fFullPath = PL_strdup(path.get());

  
  nsCAutoString fileName;
  if (NS_FAILED(rv = mPlugin->GetNativeLeafName(fileName)))
    return rv;
  info.fFileName = PL_strdup(fileName.get());

  
  if (bundle)
    info.fBundle = PR_TRUE;

  
  if (bundle) {
    CFTypeRef name = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("WebPluginName"));
    if (name && ::CFGetTypeID(name) == ::CFStringGetTypeID())
      info.fName = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(name));
  }
#ifndef __LP64__
  if (!info.fName && resourceOpened) {
    
    info.fName = GetPluginString(126, 2);
  }
#endif

  
  if (bundle) {
    CFTypeRef description = ::CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("WebPluginDescription"));
    if (description && ::CFGetTypeID(description) == ::CFStringGetTypeID())
      info.fDescription = CFStringRefToUTF8Buffer(static_cast<CFStringRef>(description));
  }
#ifndef __LP64__
  if (!info.fDescription && resourceOpened) {
    
    info.fDescription = GetPluginString(126, 1);
  }
#endif

  
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

#ifndef __LP64__
  
  if (!info.fVariantCount && resourceObject.ResourceOpened()) {
    mi.typeStrings = ::Get1Resource('STR#', 128);
    if (mi.typeStrings) {
      info.fVariantCount = (**(short**)mi.typeStrings) / 2;
      ::DetachResource(mi.typeStrings);
      ::HLock(mi.typeStrings);
    } else {
      
      return NS_ERROR_FAILURE;
    }
    
    mi.infoStrings = ::Get1Resource('STR#', 127);
    if (mi.infoStrings) {
      ::DetachResource(mi.infoStrings);
      ::HLock(mi.infoStrings);
    }
  }
#endif

  
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
