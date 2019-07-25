












































#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"

#include "windows.h"
#include "winbase.h"

#include "nsString.h"
#include "nsILocalFile.h"
#include "nsUnicharUtils.h"
#include "nsSetDllDirectory.h"



static char* GetKeyValue(void* verbuf, const WCHAR* key,
                         UINT language, UINT codepage)
{
  WCHAR keybuf[64]; 
                    
  const WCHAR keyFormat[] = L"\\StringFileInfo\\%04X%04X\\%s";
  WCHAR *buf = NULL;
  UINT blen;

  if (_snwprintf_s(keybuf, NS_ARRAY_LENGTH(keybuf), _TRUNCATE,
                   keyFormat, language, codepage, key) < 0)
  {
    NS_NOTREACHED("plugin info key too long for buffer!");
    return nsnull;
  }

  if (::VerQueryValueW(verbuf, keybuf, (void **)&buf, &blen) == 0 ||
      buf == nsnull || blen == 0)
  {
    return nsnull;
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

  return nsnull;
}

static PRUint32 CalculateVariantCount(char* mimeTypes)
{
  PRUint32 variants = 1;

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

static char** MakeStringArray(PRUint32 variants, char* data)
{
  
  
  
  
  

  if ((variants <= 0) || !data)
    return NULL;

  char ** array = (char **)PR_Calloc(variants, sizeof(char *));
  if (!array)
    return NULL;

  char * start = data;

  for (PRUint32 i = 0; i < variants; i++) {
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

static void FreeStringArray(PRUint32 variants, char ** array)
{
  if ((variants == 0) || !array)
    return;

  for (PRUint32 i = 0; i < variants; i++) {
    if (array[i]) {
      PL_strfree(array[i]);
      array[i] = NULL;
    }
  }
  PR_Free(array);
}

static PRBool CanLoadPlugin(const PRUnichar* aBinaryPath)
{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64)
  PRBool canLoad = PR_FALSE;

  HANDLE file = CreateFileW(aBinaryPath, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file != INVALID_HANDLE_VALUE) {
    HANDLE map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0,
                                    GetFileSize(file, NULL), NULL);
    if (map != NULL) {
      LPVOID mapView = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
      if (mapView != NULL) {
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
  
  return PR_TRUE;
#endif
}




PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
  nsCAutoString path;
  if (NS_FAILED(file->GetNativePath(path)))
    return PR_FALSE;

  const char *cPath = path.get();

  
  const char* filename = PL_strrchr(cPath, '\\');
  if (filename)
    ++filename;
  else
    filename = cPath;

  char* extension = PL_strrchr(filename, '.');
  if (extension)
    ++extension;

  PRUint32 fullLength = PL_strlen(filename);
  PRUint32 extLength = PL_strlen(extension);
  if (fullLength >= 7 && extLength == 3) {
    if (!PL_strncasecmp(filename, "np", 2) && !PL_strncasecmp(extension, "dll", 3)) {
      
      if (!PL_strncasecmp(filename, "npoji", 5) ||
          !PL_strncasecmp(filename, "npjava", 6))
        return PR_FALSE;
      return PR_TRUE;
    }
  }

  return PR_FALSE;
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
  nsCOMPtr<nsILocalFile> plugin = do_QueryInterface(mPlugin);

  if (!plugin)
    return NS_ERROR_NULL_POINTER;

  PRBool protectCurrentDirectory = PR_TRUE;

#ifndef WINCE
  nsAutoString pluginFolderPath;
  plugin->GetPath(pluginFolderPath);

  PRInt32 idx = pluginFolderPath.RFindChar('\\');
  if (kNotFound == idx)
    return NS_ERROR_FILE_INVALID_PATH;

  if (Substring(pluginFolderPath, idx).LowerCaseEqualsLiteral("\\np32dsw.dll")) {
    protectCurrentDirectory = PR_FALSE;
  }

  pluginFolderPath.SetLength(idx);

  BOOL restoreOrigDir = FALSE;
  WCHAR aOrigDir[MAX_PATH + 1];
  DWORD dwCheck = GetCurrentDirectoryW(MAX_PATH, aOrigDir);
  NS_ASSERTION(dwCheck <= MAX_PATH + 1, "Error in Loading plugin");

  if (dwCheck <= MAX_PATH + 1) {
    restoreOrigDir = SetCurrentDirectoryW(pluginFolderPath.get());
    NS_ASSERTION(restoreOrigDir, "Error in Loading plugin");
  }
#endif

  if (protectCurrentDirectory) {
    mozilla::NS_SetDllDirectory(NULL);
  }

  nsresult rv = plugin->Load(outLibrary);
  if (NS_FAILED(rv))
      *outLibrary = NULL;

  if (protectCurrentDirectory) {
    mozilla::NS_SetDllDirectory(L"");
  }

#ifndef WINCE    
  if (restoreOrigDir) {
    BOOL bCheck = SetCurrentDirectoryW(aOrigDir);
    NS_ASSERTION(bCheck, "Error in Loading plugin");
  }
#endif

  return rv;
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info, PRLibrary **outLibrary)
{
  *outLibrary = nsnull;

  nsresult rv = NS_OK;
  DWORD zerome, versionsize;
  void* verbuf = nsnull;

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

#ifdef WINCE
    
  LPWSTR lpFilepath = const_cast<LPWSTR>(fullPath.get());
#else
  LPCWSTR lpFilepath = fullPath.get();
#endif

  versionsize = ::GetFileVersionInfoSizeW(lpFilepath, &zerome);

  if (versionsize > 0)
    verbuf = PR_Malloc(versionsize);
  if (!verbuf)
    return NS_ERROR_OUT_OF_MEMORY;

  if (::GetFileVersionInfoW(lpFilepath, NULL, versionsize, verbuf))
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
