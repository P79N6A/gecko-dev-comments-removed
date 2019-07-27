












#include "mozilla/ArrayUtils.h" 
#include "mozilla/DebugOnly.h"

#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"

#include "windows.h"
#include "winbase.h"

#include "nsString.h"
#include "nsIFile.h"
#include "nsUnicharUtils.h"

#include <shlwapi.h>
#define SHOCKWAVE_BASE_FILENAME L"np32dsw"






bool
ShouldProtectPluginCurrentDirectory(char16ptr_t pluginFilePath)
{
  LPCWSTR passedInFilename = PathFindFileName(pluginFilePath);
  if (!passedInFilename) {
    return true;
  }

  
  
  if (!wcsicmp(passedInFilename, SHOCKWAVE_BASE_FILENAME L".dll")) {
    return false;
  }

  
  const uint64_t kFixedShockwaveVersion = 1202122;
  uint64_t version;
  int found = swscanf(passedInFilename, SHOCKWAVE_BASE_FILENAME L"_%llu.dll",
                      &version);
  if (found && version < kFixedShockwaveVersion) {
    return false;
  }

  
  return true;
}

using namespace mozilla;



static char* GetKeyValue(void* verbuf, const WCHAR* key,
                         UINT language, UINT codepage)
{
  WCHAR keybuf[64]; 
                    
  const WCHAR keyFormat[] = L"\\StringFileInfo\\%04X%04X\\%ls";
  WCHAR *buf = nullptr;
  UINT blen;

  if (_snwprintf_s(keybuf, ArrayLength(keybuf), _TRUNCATE,
                   keyFormat, language, codepage, key) < 0)
  {
    NS_NOTREACHED("plugin info key too long for buffer!");
    return nullptr;
  }

  if (::VerQueryValueW(verbuf, keybuf, (void **)&buf, &blen) == 0 ||
      buf == nullptr || blen == 0)
  {
    return nullptr;
  }

  return PL_strdup(NS_ConvertUTF16toUTF8(buf, blen).get());
}

static char* GetVersion(void* verbuf)
{
  VS_FIXEDFILEINFO *fileInfo;
  UINT fileInfoLen;

  ::VerQueryValueW(verbuf, L"\\", (void **)&fileInfo, &fileInfoLen);

  if (fileInfo) {
    return PR_smprintf("%ld.%ld.%ld.%ld",
                       HIWORD(fileInfo->dwFileVersionMS),
                       LOWORD(fileInfo->dwFileVersionMS),
                       HIWORD(fileInfo->dwFileVersionLS),
                       LOWORD(fileInfo->dwFileVersionLS));
  }

  return nullptr;
}

static uint32_t CalculateVariantCount(char* mimeTypes)
{
  uint32_t variants = 1;

  if (!mimeTypes)
    return 0;

  char* index = mimeTypes;
  while (*index) {
    if (*index == '|')
      variants++;

    ++index;
  }
  return variants;
}

static char** MakeStringArray(uint32_t variants, char* data)
{
  
  
  
  
  

  if ((variants <= 0) || !data)
    return nullptr;

  char ** array = (char **)PR_Calloc(variants, sizeof(char *));
  if (!array)
    return nullptr;

  char * start = data;

  for (uint32_t i = 0; i < variants; i++) {
    char * p = PL_strchr(start, '|');
    if (p)
      *p = 0;

    array[i] = PL_strdup(start);

    if (!p) {
      
      
      while(++i < variants)
        array[i] = PL_strdup("");

      break;
    }

    start = ++p;
  }
  return array;
}

static void FreeStringArray(uint32_t variants, char ** array)
{
  if ((variants == 0) || !array)
    return;

  for (uint32_t i = 0; i < variants; i++) {
    if (array[i]) {
      PL_strfree(array[i]);
      array[i] = nullptr;
    }
  }
  PR_Free(array);
}

static bool CanLoadPlugin(char16ptr_t aBinaryPath)
{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64)
  bool canLoad = false;

  HANDLE file = CreateFileW(aBinaryPath, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file != INVALID_HANDLE_VALUE) {
    HANDLE map = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0,
                                    GetFileSize(file, nullptr), nullptr);
    if (map != nullptr) {
      LPVOID mapView = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
      if (mapView != nullptr) {
        if (((IMAGE_DOS_HEADER*)mapView)->e_magic == IMAGE_DOS_SIGNATURE) {
          long peImageHeaderStart = (((IMAGE_DOS_HEADER*)mapView)->e_lfanew);
          if (peImageHeaderStart != 0L) {
            DWORD arch = (((IMAGE_NT_HEADERS*)((LPBYTE)mapView + peImageHeaderStart))->FileHeader.Machine);
#ifdef _M_IX86
            canLoad = (arch == IMAGE_FILE_MACHINE_I386);
#elif defined(_M_X64)
            canLoad = (arch == IMAGE_FILE_MACHINE_AMD64);
#elif defined(_M_IA64)
            canLoad = (arch == IMAGE_FILE_MACHINE_IA64);
#endif
          }
        }
        UnmapViewOfFile(mapView);
      }
      CloseHandle(map);
    }
    CloseHandle(file);
  }

  return canLoad;
#else
  
  return true;
#endif
}




bool nsPluginsDir::IsPluginFile(nsIFile* file)
{
  nsAutoCString path;
  if (NS_FAILED(file->GetNativePath(path)))
    return false;

  const char *cPath = path.get();

  
  const char* filename = PL_strrchr(cPath, '\\');
  if (filename)
    ++filename;
  else
    filename = cPath;

  char* extension = PL_strrchr(filename, '.');
  if (extension)
    ++extension;

  uint32_t fullLength = strlen(filename);
  uint32_t extLength = extension ? strlen(extension) : 0;
  if (fullLength >= 7 && extLength == 3) {
    if (!PL_strncasecmp(filename, "np", 2) && !PL_strncasecmp(extension, "dll", 3)) {
      
      if (!PL_strncasecmp(filename, "npoji", 5) ||
          !PL_strncasecmp(filename, "npjava", 6))
        return false;
      return true;
    }
  }

  return false;
}



nsPluginFile::nsPluginFile(nsIFile* file)
: mPlugin(file)
{
  
}

nsPluginFile::~nsPluginFile()
{
  
}





nsresult nsPluginFile::LoadPlugin(PRLibrary **outLibrary)
{
  if (!mPlugin)
    return NS_ERROR_NULL_POINTER;

  bool protectCurrentDirectory = true;

  nsAutoString pluginFilePath;
  mPlugin->GetPath(pluginFilePath);
  protectCurrentDirectory =
    ShouldProtectPluginCurrentDirectory(pluginFilePath.BeginReading());

  nsAutoString pluginFolderPath = pluginFilePath;
  int32_t idx = pluginFilePath.RFindChar('\\');
  pluginFolderPath.SetLength(idx);

  BOOL restoreOrigDir = FALSE;
  WCHAR aOrigDir[MAX_PATH + 1];
  DWORD dwCheck = GetCurrentDirectoryW(MAX_PATH, aOrigDir);
  NS_ASSERTION(dwCheck <= MAX_PATH + 1, "Error in Loading plugin");

  if (dwCheck <= MAX_PATH + 1) {
    restoreOrigDir = SetCurrentDirectoryW(pluginFolderPath.get());
    NS_ASSERTION(restoreOrigDir, "Error in Loading plugin");
  }

  if (protectCurrentDirectory) {
    SetDllDirectory(nullptr);
  }

  nsresult rv = mPlugin->Load(outLibrary);
  if (NS_FAILED(rv))
      *outLibrary = nullptr;

  if (protectCurrentDirectory) {
    SetDllDirectory(L"");
  }

  if (restoreOrigDir) {
    DebugOnly<BOOL> bCheck = SetCurrentDirectoryW(aOrigDir);
    NS_ASSERTION(bCheck, "Error in Loading plugin");
  }

  return rv;
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info, PRLibrary **outLibrary)
{
  *outLibrary = nullptr;

  nsresult rv = NS_OK;
  DWORD zerome, versionsize;
  void* verbuf = nullptr;

  if (!mPlugin)
    return NS_ERROR_NULL_POINTER;

  nsAutoString fullPath;
  if (NS_FAILED(rv = mPlugin->GetPath(fullPath)))
    return rv;

  if (!CanLoadPlugin(fullPath.get()))
    return NS_ERROR_FAILURE;

  nsAutoString fileName;
  if (NS_FAILED(rv = mPlugin->GetLeafName(fileName)))
    return rv;

  LPCWSTR lpFilepath = fullPath.get();

  versionsize = ::GetFileVersionInfoSizeW(lpFilepath, &zerome);

  if (versionsize > 0)
    verbuf = PR_Malloc(versionsize);
  if (!verbuf)
    return NS_ERROR_OUT_OF_MEMORY;

  if (::GetFileVersionInfoW(lpFilepath, 0, versionsize, verbuf))
  {
    
    UINT lang = 1033; 
    UINT cp = 1252;   
    info.fName = GetKeyValue(verbuf, L"ProductName", lang, cp);
    info.fDescription = GetKeyValue(verbuf, L"FileDescription", lang, cp);
 
    char *mimeType = GetKeyValue(verbuf, L"MIMEType", lang, cp);
    char *mimeDescription = GetKeyValue(verbuf, L"FileOpenName", lang, cp);
    char *extensions = GetKeyValue(verbuf, L"FileExtents", lang, cp);

    info.fVariantCount = CalculateVariantCount(mimeType);
    info.fMimeTypeArray = MakeStringArray(info.fVariantCount, mimeType);
    info.fMimeDescriptionArray = MakeStringArray(info.fVariantCount, mimeDescription);
    info.fExtensionArray = MakeStringArray(info.fVariantCount, extensions);
    info.fFullPath = PL_strdup(NS_ConvertUTF16toUTF8(fullPath).get());
    info.fFileName = PL_strdup(NS_ConvertUTF16toUTF8(fileName).get());
    info.fVersion = GetVersion(verbuf);

    PL_strfree(mimeType);
    PL_strfree(mimeDescription);
    PL_strfree(extensions);
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  PR_Free(verbuf);

  return rv;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
  if (info.fName)
    PL_strfree(info.fName);

  if (info.fDescription)
    PL_strfree(info.fDescription);

  if (info.fMimeTypeArray)
    FreeStringArray(info.fVariantCount, info.fMimeTypeArray);

  if (info.fMimeDescriptionArray)
    FreeStringArray(info.fVariantCount, info.fMimeDescriptionArray);

  if (info.fExtensionArray)
    FreeStringArray(info.fVariantCount, info.fExtensionArray);

  if (info.fFullPath)
    PL_strfree(info.fFullPath);

  if (info.fFileName)
    PL_strfree(info.fFileName);

  if (info.fVersion)
    PR_smprintf_free(info.fVersion);

  ZeroMemory((void *)&info, sizeof(info));

  return NS_OK;
}
