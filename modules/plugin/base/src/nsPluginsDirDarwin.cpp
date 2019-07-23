














































#include "prlink.h"
#include "prnetdb.h"

#include "nsPluginsDir.h"
#include "ns4xPlugin.h"
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

typedef NS_4XPLUGIN_CALLBACK(const char *, NP_GETMIMEDESCRIPTION) ();
typedef NS_4XPLUGIN_CALLBACK(OSErr, BP_GETSUPPORTEDMIMETYPES) (BPSupportedMIMETypes *mimeInfo, UInt32 flags);






static CFBundleRef getPluginBundle(const char* path)
{
    CFBundleRef bundle = NULL;
    CFStringRef pathRef = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    if (pathRef) {
        CFURLRef bundleURL = CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, true);
        if (bundleURL != NULL) {
            bundle = CFBundleCreate(NULL, bundleURL);
            CFRelease(bundleURL);
        }
        CFRelease(pathRef);
    }
    return bundle;
}

static OSErr toFSSpec(nsIFile* file, FSSpec& outSpec)
{
    nsCOMPtr<nsILocalFileMac> lfm = do_QueryInterface(file);
    if (!lfm)
        return -1;
    FSSpec foo;
    lfm->GetFSSpec(&foo);
    outSpec = foo;

    return NS_OK;
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




static short OpenPluginResourceFork(nsIFile *pluginFile)
{
    FSSpec spec;
    OSErr err = toFSSpec(pluginFile, spec);
    Boolean targetIsFolder, wasAliased;
    err = ::ResolveAliasFile(&spec, true, &targetIsFolder, &wasAliased);
    short refNum = ::FSpOpenResFile(&spec, fsRdPerm);
    if (refNum < 0) {
        nsCString path;
        pluginFile->GetNativePath(path);
        CFBundleRef bundle = getPluginBundle(path.get());
        if (bundle) {
            refNum = CFBundleOpenBundleResourceMap(bundle);
            CFRelease(bundle);
        }
    }
    
    return refNum;
}


static PRBool IsLoadablePlugin(CFURLRef aURL)
{
  if (!aURL)
    return PR_FALSE;
  
  PRBool isLoadable = PR_FALSE;
  char path[PATH_MAX];
  if (CFURLGetFileSystemRepresentation(aURL, TRUE, (UInt8*)path, sizeof(path))) {
    UInt32 magic;
    int f = open(path, O_RDONLY);
    if (f != -1) {
      
      
      
      
      if (read(f, &magic, sizeof(magic)) == sizeof(magic)) {
        if ((magic == MH_MAGIC) || (PR_ntohl(magic) == FAT_MAGIC))
          isLoadable = PR_TRUE;
#ifdef __POWERPC__
        
        if (isLoadable == PR_FALSE) {
          UInt32 magic2;
          if (read(f, &magic2, sizeof(magic2)) == sizeof(magic2)) {
            UInt32 cfm_header1 = 0x4A6F7921; 
            UInt32 cfm_header2 = 0x70656666; 
            if (cfm_header1 == magic && cfm_header2 == magic2)
              isLoadable = PR_TRUE;
          }
        }
#endif
      }
      close(f);
    }
  }
  return isLoadable;
}

PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
  CFURLRef pluginURL = NULL;
  if (NS_FAILED(toCFURLRef(file, pluginURL)))
    return PR_FALSE;
  
  PRBool isPluginFile = PR_FALSE;
  
  CFBundleRef pluginBundle = CFBundleCreate(kCFAllocatorDefault, pluginURL);
  if (pluginBundle) {
    UInt32 packageType, packageCreator;
    CFBundleGetPackageInfo(pluginBundle, &packageType, &packageCreator);
    if (packageType == 'BRPL' || packageType == 'IEPL' || packageType == 'NSPL') {
      CFURLRef executableURL = CFBundleCopyExecutableURL(pluginBundle);
      if (executableURL) {
        isPluginFile = IsLoadablePlugin(executableURL);
        CFRelease(executableURL);
      }
    }
  
    
    short refNum;
    if (isPluginFile) {
      refNum = OpenPluginResourceFork(file);
      if (refNum < 0) {
        isPluginFile = PR_FALSE;
      } else {
        ::CloseResFile(refNum); 
      }
    }
  
    CFRelease(pluginBundle);
  }
  else {
    LSItemInfoRecord info;
    if (LSCopyItemInfoForURL(pluginURL, kLSRequestTypeCreator, &info) == noErr) {
      if ((info.filetype == 'shlb' && info.creator == 'MOSS') ||
          info.filetype == 'NSPL' ||
          info.filetype == 'BRPL' ||
          info.filetype == 'IEPL') {
        isPluginFile = IsLoadablePlugin(pluginURL);
      }
    }
  }
  
  CFRelease(pluginURL);
  return isPluginFile;
}

nsPluginFile::nsPluginFile(nsIFile *spec)
    : mPlugin(spec)
{
}

nsPluginFile::~nsPluginFile() {}





nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
    const char* path;

    if (!mPlugin)
        return NS_ERROR_NULL_POINTER;

    nsCAutoString temp;
    mPlugin->GetNativePath(temp);
    path = temp.get();

    outLibrary = PR_LoadLibrary(path);
    pLibrary = outLibrary;
    if (!outLibrary) {
        return NS_ERROR_FAILURE;
    }
#ifdef DEBUG
    printf("[loaded plugin %s]\n", path);
#endif
    return NS_OK;
}

static char* p2cstrdup(StringPtr pstr)
{
    int len = pstr[0];
    char* cstr = new char[len + 1];
    if (cstr != NULL) {
        ::BlockMoveData(pstr + 1, cstr, len);
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

static char* GetPluginString(short id, short index)
{
    Str255 str;
    ::GetIndString(str, id, index);
    return p2cstrdup(str);
}

short nsPluginFile::OpenPluginResource()
{
    return OpenPluginResourceFork(mPlugin);
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
  
  memset(&info.fName, 0, sizeof(info) - sizeof(PRUint32));
  
  if (info.fPluginInfoSize < sizeof(nsPluginInfo))
    return NS_ERROR_FAILURE;
  
  
  short refNum = OpenPluginResource();
  if (refNum < 0)
    return NS_ERROR_FAILURE;
  
  
  info.fName = GetPluginString(126, 2);
  
  
  info.fDescription = GetPluginString(126, 1);
  
  nsCString path;
  mPlugin->GetNativePath(path);
  
  FSSpec spec;
  toFSSpec(mPlugin, spec);
  info.fFileName = p2cstrdup(spec.name);
  
  info.fFullPath = PL_strdup(path.get());
  CFBundleRef bundle = getPluginBundle(path.get());
  if (bundle) {
    info.fBundle = PR_TRUE;
    CFRelease(bundle);
  }
  else {
    info.fBundle = PR_FALSE;
  }

  
  
  
  
  BPSupportedMIMETypes mi = {kBPSupportedMIMETypesStructVers_1, NULL, NULL};
  if (pLibrary) {
    
    NP_GETMIMEDESCRIPTION pfnGetMimeDesc = 
    (NP_GETMIMEDESCRIPTION)PR_FindSymbol(pLibrary, NP_GETMIMEDESCRIPTION_NAME); 
    if (pfnGetMimeDesc) {
      nsresult rv = ParsePluginMimeDescription(pfnGetMimeDesc(), info);
      if (NS_SUCCEEDED(rv)) {    
        ::CloseResFile(refNum);  
        return rv;
      }
    }
    
    
    BP_GETSUPPORTEDMIMETYPES pfnMime = 
      (BP_GETSUPPORTEDMIMETYPES)PR_FindSymbol(pLibrary, "BP_GetSupportedMIMETypes");
    if (pfnMime && noErr == pfnMime(&mi, 0) && mi.typeStrings) {        
      info.fVariantCount = (**(short**)mi.typeStrings) / 2;
      ::HLock(mi.typeStrings);
      if (mi.infoStrings)  
        ::HLock(mi.infoStrings);
    }
  }
  
  
  
  if (!info.fVariantCount) {
    mi.typeStrings = ::Get1Resource('STR#', 128);
    if (mi.typeStrings) {
      info.fVariantCount = (**(short**)mi.typeStrings) / 2;
      ::DetachResource(mi.typeStrings);
      ::HLock(mi.typeStrings);
    } else {
      
      ::CloseResFile(refNum);
      return NS_ERROR_FAILURE;
    }
    
    mi.infoStrings = ::Get1Resource('STR#', 127);
    if (mi.infoStrings) {
      ::DetachResource(mi.infoStrings);
      ::HLock(mi.infoStrings);
    }
  }
  
  
  int variantCount = info.fVariantCount;
  info.fMimeTypeArray = new char*[variantCount];
  info.fExtensionArray = new char*[variantCount];
  if (mi.infoStrings)
    info.fMimeDescriptionArray = new char*[variantCount];
  
  short mimeIndex = 2, descriptionIndex = 2;
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

  ::CloseResFile(refNum);

  return NS_OK;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
  if (info.fPluginInfoSize <= sizeof(nsPluginInfo)) {
    delete[] info.fName;
    delete[] info.fDescription;
    int variantCount = info.fVariantCount;
    for (int i = 0; i < variantCount; i++) {
      delete[] info.fMimeTypeArray[i];
      delete[] info.fExtensionArray[i];
      delete[] info.fMimeDescriptionArray[i];
    }
    delete[] info.fMimeTypeArray;
    delete[] info.fMimeDescriptionArray;
    delete[] info.fExtensionArray;
    delete[] info.fFileName;
    delete[] info.fFullPath;
  }

  return NS_OK;
}
