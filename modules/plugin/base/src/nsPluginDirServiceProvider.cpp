






































#include "nsPluginDirServiceProvider.h"

#include "nsCRT.h"
#include "nsILocalFile.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsDependentString.h"
#include "nsXPIDLString.h"
#include "prmem.h"
#include "nsArrayEnumerator.h"

typedef struct structVer
{
  WORD wMajor;
  WORD wMinor;
  WORD wRelease;
  WORD wBuild;
} verBlock;

static void
ClearVersion(verBlock *ver)
{
  ver->wMajor   = 0;
  ver->wMinor   = 0;
  ver->wRelease = 0;
  ver->wBuild   = 0;
}

static BOOL
FileExists(wchar_t* szFile)
{
  return GetFileAttributesW(szFile) != 0xFFFFFFFF;
}


static BOOL
GetFileVersion(wchar_t* szFile, verBlock *vbVersion)
{
  UINT              uLen;
  UINT              dwLen;
  BOOL              bRv;
  DWORD             dwHandle;
  LPVOID            lpData;
  LPVOID            lpBuffer;
  VS_FIXEDFILEINFO  *lpBuffer2;

  ClearVersion(vbVersion);
  if (FileExists(szFile)) {
    bRv    = TRUE;
    dwLen  = GetFileVersionInfoSizeW(szFile, &dwHandle);
    lpData = (LPVOID)malloc(dwLen);
    uLen   = 0;

    if (lpData && GetFileVersionInfoW(szFile, dwHandle, dwLen, lpData) != 0) {
      if (VerQueryValueW(lpData, L"\\", &lpBuffer, &uLen) != 0) {
        lpBuffer2 = (VS_FIXEDFILEINFO *)lpBuffer;

        vbVersion->wMajor   = HIWORD(lpBuffer2->dwFileVersionMS);
        vbVersion->wMinor   = LOWORD(lpBuffer2->dwFileVersionMS);
        vbVersion->wRelease = HIWORD(lpBuffer2->dwFileVersionLS);
        vbVersion->wBuild   = LOWORD(lpBuffer2->dwFileVersionLS);
      }
    }

    free(lpData);
  } else {
    
    bRv = FALSE;
  }

  return bRv;
}


static void
CopyVersion(verBlock *ver1, verBlock *ver2)
{
  ver1->wMajor   = ver2->wMajor;
  ver1->wMinor   = ver2->wMinor;
  ver1->wRelease = ver2->wRelease;
  ver1->wBuild   = ver2->wBuild;
}


static void
TranslateVersionStr(const char* szVersion, verBlock *vbVersion)
{
  char* szNum1 = NULL;
  char* szNum2 = NULL;
  char* szNum3 = NULL;
  char* szNum4 = NULL;
  char* szJavaBuild = NULL;

  char* strVer = nsnull;
  if (szVersion) {
    strVer = strdup(szVersion);
  }

  if (!strVer) {
    
    ClearVersion(vbVersion);
    return;
  }

  
  szJavaBuild = strchr(strVer, '_');
  if (szJavaBuild) {
    szJavaBuild[0] = '.';
  }

  szNum1 = strtok(strVer, ".");
  szNum2 = strtok(NULL,   ".");
  szNum3 = strtok(NULL,   ".");
  szNum4 = strtok(NULL,   ".");

  vbVersion->wMajor   = szNum1 ? atoi(szNum1) : 0;
  vbVersion->wMinor   = szNum2 ? atoi(szNum2) : 0;
  vbVersion->wRelease = szNum3 ? atoi(szNum3) : 0;
  vbVersion->wBuild   = szNum4 ? atoi(szNum4) : 0;

  free(strVer);
}


static int
CompareVersion(verBlock vbVersionOld, verBlock vbVersionNew)
{
  if (vbVersionOld.wMajor > vbVersionNew.wMajor) {
    return 4;
  } else if (vbVersionOld.wMajor < vbVersionNew.wMajor) {
    return -4;
  }

  if (vbVersionOld.wMinor > vbVersionNew.wMinor) {
    return 3;
  } else if (vbVersionOld.wMinor < vbVersionNew.wMinor) {
    return -3;
  }

  if (vbVersionOld.wRelease > vbVersionNew.wRelease) {
    return 2;
  } else if (vbVersionOld.wRelease < vbVersionNew.wRelease) {
    return -2;
  }

  if (vbVersionOld.wBuild > vbVersionNew.wBuild) {
    return 1;
  } else if (vbVersionOld.wBuild < vbVersionNew.wBuild) {
    return -1;
  }

  
  return 0;
}



static PRBool
TryToUseNPRuntimeJavaPlugIn(const wchar_t* javaVersion)
{
  HKEY javaKey = NULL;
  wchar_t keyName[_MAX_PATH];
  
  wcsncpy(keyName, L"Software\\JavaSoft\\Java Plug-in\\", wcslen(L"Software\\JavaSoft\\Java Plug-in\\"));
  wcscpy(keyName, javaVersion);

  DWORD val;
  DWORD valSize = sizeof(DWORD);
    
  if (ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                      keyName, 0, KEY_READ, &javaKey)) {
    return FALSE;
  }

  
  if (ERROR_SUCCESS != ::RegQueryValueExW(javaKey,
                                          L"UseNewJavaPlugin",
                                          NULL, NULL,
                                          (LPBYTE) &val,
                                          &valSize)) {
    val = 0;
  }

  ::RegCloseKey(javaKey);
  return (val == 0) ? PR_FALSE : PR_TRUE;
}






nsPluginDirServiceProvider::nsPluginDirServiceProvider()
{
}

nsPluginDirServiceProvider::~nsPluginDirServiceProvider()
{
}





NS_IMPL_THREADSAFE_ISUPPORTS1(nsPluginDirServiceProvider,
                              nsIDirectoryServiceProvider)





NS_IMETHODIMP
nsPluginDirServiceProvider::GetFile(const char *prop, PRBool *persistant,
                                    nsIFile **_retval)
{
  nsCOMPtr<nsILocalFile>  localFile;
  nsresult rv = NS_ERROR_FAILURE;

  NS_ENSURE_ARG(prop);
  *_retval = nsnull;
  *persistant = PR_FALSE;

  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs) {
    return rv;
  }

  if (strcmp(prop, NS_WIN_4DOTX_SCAN_KEY) == 0) {
    
    
    
    PRBool bScan4x;
    if (NS_SUCCEEDED(prefs->GetBoolPref(NS_WIN_4DOTX_SCAN_KEY, &bScan4x)) &&
        !bScan4x) {
      return rv;
    }

    
    
    HKEY keyloc;
    long result;
    DWORD type;
    wchar_t szKey[_MAX_PATH] = L"Software\\Netscape\\Netscape Navigator";
    wchar_t path[_MAX_PATH];

    result = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &keyloc);

    if (result == ERROR_SUCCESS) {
      wchar_t current_version[80];
      DWORD length = sizeof(current_version);

      result = ::RegQueryValueExW(keyloc, L"CurrentVersion", NULL, &type,
                                 (LPBYTE)&current_version, &length);

      ::RegCloseKey(keyloc);
      wcscat(szKey, L"\\");
      wcscat(szKey, current_version);
      wcscat(szKey, L"\\Main");
      result = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &keyloc);

      if (result == ERROR_SUCCESS) {
        DWORD pathlen = sizeof(path);

        result = ::RegQueryValueExW(keyloc, L"Plugins Directory", NULL, &type,
                                   (LPBYTE)&path, &pathlen);
        if (result == ERROR_SUCCESS) {
          rv = NS_NewLocalFile(nsDependentString(path), PR_TRUE,
                               getter_AddRefs(localFile));
        }

        ::RegCloseKey(keyloc);
      }
    }
  } else if (strcmp(prop, NS_WIN_JRE_SCAN_KEY) == 0) {
    nsXPIDLCString strVer;
#ifdef OJI
    if (NS_FAILED(prefs->GetCharPref(prop, getter_Copies(strVer))))
#endif 
      return NS_ERROR_FAILURE;
    verBlock minVer;
    TranslateVersionStr(strVer.get(), &minVer);

    
    HKEY baseloc;
    HKEY keyloc;
    HKEY entryloc;
    FILETIME modTime;
    DWORD type;
    DWORD index = 0;
    DWORD numChars = _MAX_PATH;
    DWORD pathlen;
    verBlock maxVer;
    ClearVersion(&maxVer);
    wchar_t curKey[_MAX_PATH] = L"Software\\JavaSoft\\Java Runtime Environment";
    wchar_t path[_MAX_PATH];

    
    
#define JAVA_PATH_SIZE _MAX_PATH + 15
    wchar_t newestPath[JAVA_PATH_SIZE];
    const wchar_t mozPath[_MAX_PATH] = L"Software\\mozilla.org\\Mozilla";
    wchar_t browserJavaVersion[_MAX_PATH];
    PRBool tryNPRuntimeJavaPlugIn = PR_FALSE;

    newestPath[0] = 0;
    LONG result = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, curKey, 0, KEY_READ,
                                 &baseloc);
    if (ERROR_SUCCESS != result)
      return NS_ERROR_FAILURE;

    
    if (ERROR_SUCCESS != ::RegQueryValueExW(baseloc, L"BrowserJavaVersion", NULL,
                                           NULL, (LPBYTE)&browserJavaVersion,
                                           &numChars))
      browserJavaVersion[0] = 0;

    
    
    do {
      path[0] = 0;
      numChars = _MAX_PATH;
      pathlen = sizeof(path);
      result = ::RegEnumKeyExW(baseloc, index, curKey, &numChars, NULL, NULL,
                              NULL, &modTime);
      index++;

      
      numChars = 0;
      for (wchar_t *p = curKey; *p; p++) {  
        if (*p == '.') {
          numChars++;
        }
      }
      if (numChars < 2)
        continue;

      if (ERROR_SUCCESS == result) {
        if (ERROR_SUCCESS == ::RegOpenKeyExW(baseloc, curKey, 0,
                                            KEY_QUERY_VALUE, &keyloc)) {
          
          if (ERROR_SUCCESS == ::RegQueryValueExW(keyloc, L"JavaHome", NULL,
                                                 &type, (LPBYTE)&path,
                                                 &pathlen)) {
            verBlock curVer;
            TranslateVersionStr(NS_ConvertUTF16toUTF8(curKey).get(), &curVer);
            if (CompareVersion(curVer, minVer) >= 0) {
              if (!wcsncmp(browserJavaVersion, curKey, _MAX_PATH)) {
                wcscpy(newestPath, path);
                tryNPRuntimeJavaPlugIn = TryToUseNPRuntimeJavaPlugIn(curKey);
                ::RegCloseKey(keyloc);
                break;
              }

              if (CompareVersion(curVer, maxVer) >= 0) {
                wcscpy(newestPath, path);
                CopyVersion(&maxVer, &curVer);
                tryNPRuntimeJavaPlugIn = TryToUseNPRuntimeJavaPlugIn(curKey);
              }
            }
          }
          ::RegCloseKey(keyloc);
        }
      }
    } while (ERROR_SUCCESS == result);

    ::RegCloseKey(baseloc);

    
    
    if (newestPath[0] != 0) {
      if (ERROR_SUCCESS == ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, mozPath, 0,
                                            NULL, REG_OPTION_NON_VOLATILE,
                                            KEY_SET_VALUE|KEY_QUERY_VALUE,
                                            NULL, &entryloc, NULL)) {
        if (ERROR_SUCCESS != ::RegQueryValueExW(entryloc, L"CurrentVersion", 0,
                                               NULL, NULL, NULL)) {
          ::RegSetValueExW(entryloc, L"CurrentVersion", 0, REG_SZ,
                          (const BYTE*)MOZILLA_VERSION,
                          sizeof(MOZILLA_VERSION));
        }
        ::RegCloseKey(entryloc);
      }

      wcscat(newestPath,L"\\bin");

      
      
      
      
      
      
      if (tryNPRuntimeJavaPlugIn) {
        
        wchar_t tmpPath[JAVA_PATH_SIZE];
        wcscpy(tmpPath, newestPath);
        wcscat(tmpPath, L"\\new_plugin");
        nsCOMPtr<nsILocalFile> tmpFile;
        if (NS_SUCCEEDED(NS_NewLocalFile(nsDependentString(tmpPath),
                                         PR_TRUE,
                                         getter_AddRefs(tmpFile))) &&
            tmpFile) {
          PRBool exists = PR_FALSE;
          PRBool isDir = PR_FALSE;
          if (NS_SUCCEEDED(tmpFile->Exists(&exists)) && exists &&
              NS_SUCCEEDED(tmpFile->IsDirectory(&isDir)) && isDir) {
            
            
            
            wcscpy(newestPath, tmpPath);
          }
        }
      }

      rv = NS_NewLocalFile(nsDependentString(newestPath), PR_TRUE,
                           getter_AddRefs(localFile));
    }
  } else if (strcmp(prop, NS_WIN_QUICKTIME_SCAN_KEY) == 0) {
    nsXPIDLCString strVer;
    if (NS_FAILED(prefs->GetCharPref(prop, getter_Copies(strVer))))
      return NS_ERROR_FAILURE;
    verBlock minVer;
    TranslateVersionStr(strVer.get(), &minVer);

    
    HKEY keyloc;
    long result;
    DWORD type;
    verBlock qtVer;
    ClearVersion(&qtVer);
    wchar_t path[_MAX_PATH];
    DWORD pathlen = sizeof(path);

    
    
    if (ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\QuickTimePlayer.exe", 0, KEY_READ, &keyloc)) {
      if (ERROR_SUCCESS == ::RegQueryValueExW(keyloc, NULL, NULL, &type,
                                              (LPBYTE)&path, &pathlen)) {
        GetFileVersion(path, &qtVer);
      }
      ::RegCloseKey(keyloc);
    }
    if (CompareVersion(qtVer, minVer) < 0)
      return rv;

    if (ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"software\\Apple Computer, Inc.\\QuickTime", 0, KEY_READ, &keyloc)) {
      DWORD pathlen = sizeof(path);

      result = ::RegQueryValueExW(keyloc, L"InstallDir", NULL, &type,
                                 (LPBYTE)&path, &pathlen);
      wcscat(path, L"\\Plugins");
      if (result == ERROR_SUCCESS)
        rv = NS_NewLocalFile(nsDependentString(path), PR_TRUE,
                             getter_AddRefs(localFile));
      ::RegCloseKey(keyloc);
    }
  } else if (strcmp(prop, NS_WIN_WMP_SCAN_KEY) == 0) {
    nsXPIDLCString strVer;
    if (NS_FAILED(prefs->GetCharPref(prop, getter_Copies(strVer))))
      return NS_ERROR_FAILURE;
    verBlock minVer;
    TranslateVersionStr(strVer.get(), &minVer);

    
    HKEY keyloc;
    DWORD type;
    verBlock wmpVer;
    ClearVersion(&wmpVer);
    wchar_t path[_MAX_PATH];
    DWORD pathlen = sizeof(path);

    
    if (ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wmplayer.exe", 0, KEY_READ, &keyloc)) {
      if (ERROR_SUCCESS == ::RegQueryValueExW(keyloc, NULL, NULL, &type,
                                              (LPBYTE)&path, &pathlen)) {
        GetFileVersion(path, &wmpVer);
      }
      ::RegCloseKey(keyloc);
    }
    if (CompareVersion(wmpVer, minVer) < 0)
      return rv;

    if (ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                        L"software\\Microsoft\\MediaPlayer", 0,
                                        KEY_READ, &keyloc)) {
      if (ERROR_SUCCESS == ::RegQueryValueExW(keyloc, L"Installation Directory",
                                             NULL, &type, (LPBYTE)&path,
                                             &pathlen)) {
        rv = NS_NewLocalFile(nsDependentString(path), PR_TRUE,
                             getter_AddRefs(localFile));
      }

      ::RegCloseKey(keyloc);
    }
  } else if (strcmp(prop, NS_WIN_ACROBAT_SCAN_KEY) == 0) {
    nsXPIDLCString strVer;
    if (NS_FAILED(prefs->GetCharPref(prop, getter_Copies(strVer)))) {
      return NS_ERROR_FAILURE;
    }

    verBlock minVer;
    TranslateVersionStr(strVer.get(), &minVer);

    
    HKEY baseloc;
    HKEY keyloc;
    FILETIME modTime;
    DWORD type;
    DWORD index = 0;
    DWORD numChars = _MAX_PATH;
    DWORD pathlen;
    verBlock maxVer;
    ClearVersion(&maxVer);
    wchar_t curKey[_MAX_PATH] = L"software\\Adobe\\Acrobat Reader";
    wchar_t path[_MAX_PATH];
    
    wchar_t newestPath[_MAX_PATH + 8];

    newestPath[0] = 0;
    if (ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, curKey, 0,
                                        KEY_READ, &baseloc)) {
      wcscpy(curKey, L"software\\Adobe\\Adobe Acrobat");
      if (ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, curKey, 0,
                                          KEY_READ, &baseloc)) {
        return NS_ERROR_FAILURE;
      }
    }

    
    
    LONG result = ERROR_SUCCESS;
    while (ERROR_SUCCESS == result) {
      path[0] = 0;
      numChars = _MAX_PATH;
      pathlen = sizeof(path);
      result = ::RegEnumKeyExW(baseloc, index, curKey, &numChars, NULL, NULL,
                              NULL, &modTime);
      index++;

      if (ERROR_SUCCESS == result) {
        verBlock curVer;
        TranslateVersionStr(NS_ConvertUTF16toUTF8(curKey).get(), &curVer);
        wcscat(curKey, L"\\InstallPath");
        if (ERROR_SUCCESS == ::RegOpenKeyExW(baseloc, curKey, 0,
                                            KEY_QUERY_VALUE, &keyloc)) {
          
          if (ERROR_SUCCESS == ::RegQueryValueExW(keyloc, NULL, NULL, &type,
                                                 (LPBYTE)&path, &pathlen)) {
            if (CompareVersion(curVer, maxVer) >= 0 &&
                CompareVersion(curVer, minVer) >= 0) {
              wcscpy(newestPath, path);
              CopyVersion(&maxVer, &curVer);
            }
          }

          ::RegCloseKey(keyloc);
        }
      }
    }

    ::RegCloseKey(baseloc);

    if (newestPath[0] != 0) {
      wcscat(newestPath, L"\\browser");
      rv = NS_NewLocalFile(nsDependentString(newestPath), PR_TRUE,
                           getter_AddRefs(localFile));
    }

  }

  if (localFile && NS_SUCCEEDED(rv))
    return CallQueryInterface(localFile, _retval);

  return rv;
}

nsresult
nsPluginDirServiceProvider::GetPLIDDirectories(nsISimpleEnumerator **aEnumerator)
{
  NS_ENSURE_ARG_POINTER(aEnumerator);
  *aEnumerator = nsnull;

  nsCOMArray<nsILocalFile> dirs;

  GetPLIDDirectoriesWithHKEY(HKEY_CURRENT_USER, dirs);
  GetPLIDDirectoriesWithHKEY(HKEY_LOCAL_MACHINE, dirs);

  return NS_NewArrayEnumerator(aEnumerator, dirs);
}

nsresult
nsPluginDirServiceProvider::GetPLIDDirectoriesWithHKEY(HKEY aKey, nsCOMArray<nsILocalFile> &aDirs)
{
  wchar_t subkey[_MAX_PATH] = L"Software\\MozillaPlugins";
  HKEY baseloc;

  if (ERROR_SUCCESS != ::RegOpenKeyExW(aKey, subkey, 0, KEY_READ, &baseloc))
    return NS_ERROR_FAILURE;

  DWORD index = 0;
  DWORD subkeylen = _MAX_PATH;
  FILETIME modTime;
  while (ERROR_SUCCESS == ::RegEnumKeyExW(baseloc, index++, subkey, &subkeylen,
                                         NULL, NULL, NULL, &modTime)) {
    subkeylen = _MAX_PATH;
    HKEY keyloc;

    if (ERROR_SUCCESS == ::RegOpenKeyExW(baseloc, subkey, 0, KEY_QUERY_VALUE,
                                        &keyloc)) {
      DWORD type;
      wchar_t path[_MAX_PATH];
      DWORD pathlen = sizeof(path);

      if (ERROR_SUCCESS == ::RegQueryValueExW(keyloc, L"Path", NULL, &type,
                                             (LPBYTE)&path, &pathlen)) {
        nsCOMPtr<nsILocalFile> localFile;
        if (NS_SUCCEEDED(NS_NewLocalFile(nsDependentString(path),
                                         PR_TRUE,
                                         getter_AddRefs(localFile))) &&
            localFile) {
          
          
          PRBool isDir = PR_FALSE;
          if (NS_SUCCEEDED(localFile->IsDirectory(&isDir)) && !isDir) {
            nsCOMPtr<nsIFile> temp;
            localFile->GetParent(getter_AddRefs(temp));
            if (temp)
              localFile = do_QueryInterface(temp);
          }

          
          
          PRBool isFileThere = PR_FALSE;
          PRBool isDupEntry = PR_FALSE;
          if (NS_SUCCEEDED(localFile->Exists(&isFileThere)) && isFileThere) {
            PRInt32 c = aDirs.Count();
            for (PRInt32 i = 0; i < c; i++) {
              nsIFile *dup = static_cast<nsIFile*>(aDirs[i]);
              if (dup &&
                  NS_SUCCEEDED(dup->Equals(localFile, &isDupEntry)) &&
                  isDupEntry) {
                break;
              }
            }

            if (!isDupEntry) {
              aDirs.AppendObject(localFile);
            }
          }
        }
      }
      ::RegCloseKey(keyloc);
    }
  }
  ::RegCloseKey(baseloc);
  return NS_OK;
}

