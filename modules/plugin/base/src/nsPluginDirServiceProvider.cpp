






































#include "nsPluginDirServiceProvider.h"

#include "nsCRT.h"
#include "nsILocalFile.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsDependentString.h"
#include "nsXPIDLString.h"
#include "prmem.h"
#include "nsArrayEnumerator.h"

#if defined (XP_WIN)

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
FileExists(LPCSTR szFile)
{
  return GetFileAttributes(szFile) != 0xFFFFFFFF;
}


static BOOL
GetFileVersion(LPSTR szFile, verBlock *vbVersion)
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
    dwLen  = GetFileVersionInfoSize(szFile, &dwHandle);
    lpData = (LPVOID)malloc(dwLen);
    uLen   = 0;

    if (lpData && GetFileVersionInfo(szFile, dwHandle, dwLen, lpData) != 0) {
      if (VerQueryValue(lpData, "\\", &lpBuffer, &uLen) != 0) {
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
  LPSTR szNum1 = NULL;
  LPSTR szNum2 = NULL;
  LPSTR szNum3 = NULL;
  LPSTR szNum4 = NULL;
  LPSTR szJavaBuild = NULL;

  char *strVer = nsnull;
  if (szVersion) {
    strVer = PL_strdup(szVersion);
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

  PL_strfree(strVer);
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
#endif





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
  *persistant = PR_TRUE;

#if defined(XP_WIN)
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs) {
    return rv;
  }

  if (nsCRT::strcmp(prop, NS_WIN_4DOTX_SCAN_KEY) == 0) {
    
    
    
    PRBool bScan4x;
    if (NS_SUCCEEDED(prefs->GetBoolPref(NS_WIN_4DOTX_SCAN_KEY, &bScan4x)) &&
        !bScan4x) {
      return rv;
    }

    
    
    HKEY keyloc;
    long result;
    DWORD type;
    char szKey[_MAX_PATH] = "Software\\Netscape\\Netscape Navigator";
    char path[_MAX_PATH];

    result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &keyloc);

    if (result == ERROR_SUCCESS) {
      char current_version[80];
      DWORD length = sizeof(current_version);

      result = ::RegQueryValueEx(keyloc, "CurrentVersion", NULL, &type,
                                 (LPBYTE)&current_version, &length);

      ::RegCloseKey(keyloc);
      PL_strcat(szKey, "\\");
      PL_strcat(szKey, current_version);
      PL_strcat(szKey, "\\Main");
      result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &keyloc);

      if (result == ERROR_SUCCESS) {
        DWORD pathlen = sizeof(path);

        result = ::RegQueryValueEx(keyloc, "Plugins Directory", NULL, &type,
                                   (LPBYTE)&path, &pathlen);
        if (result == ERROR_SUCCESS) {
          rv = NS_NewNativeLocalFile(nsDependentCString(path), PR_TRUE,
                                     getter_AddRefs(localFile));
        }

        ::RegCloseKey(keyloc);
      }
    }
  } else if (nsCRT::strcmp(prop, NS_WIN_JRE_SCAN_KEY) == 0) {
    PRBool isJavaEnabled;
    nsXPIDLCString strVer;
#ifdef OJI
    if ((NS_FAILED(prefs->GetBoolPref("security.enable_java", &isJavaEnabled))
         || !isJavaEnabled) ||
        NS_FAILED(prefs->GetCharPref(prop, getter_Copies(strVer))))
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
    char curKey[_MAX_PATH] = "Software\\JavaSoft\\Java Runtime Environment";
    char path[_MAX_PATH];
    
    char newestPath[_MAX_PATH + 4];
    const char mozPath[_MAX_PATH] = "Software\\mozilla.org\\Mozilla";
    char browserJavaVersion[_MAX_PATH];

    newestPath[0] = 0;
    LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, curKey, 0, KEY_READ,
                                 &baseloc);
    if (ERROR_SUCCESS != result)
      return NS_ERROR_FAILURE;

    
    if (ERROR_SUCCESS != ::RegQueryValueEx(baseloc, "BrowserJavaVersion", NULL,
                                           NULL, (LPBYTE)&browserJavaVersion,
                                           &numChars))
      browserJavaVersion[0] = 0;

    
    
    do {
      path[0] = 0;
      numChars = _MAX_PATH;
      pathlen = sizeof(path);
      result = ::RegEnumKeyEx(baseloc, index, curKey, &numChars, NULL, NULL,
                              NULL, &modTime);
      index++;

      
      numChars = 0;
      for (char *p = curKey; *p; p++) {
        if (*p == '.') {
          numChars++;
        }
      }
      if (numChars < 2)
        continue;

      if (ERROR_SUCCESS == result) {
        if (ERROR_SUCCESS == ::RegOpenKeyEx(baseloc, curKey, 0,
                                            KEY_QUERY_VALUE, &keyloc)) {
          
          if (ERROR_SUCCESS == ::RegQueryValueEx(keyloc, "JavaHome", NULL,
                                                 &type, (LPBYTE)&path,
                                                 &pathlen)) {
            verBlock curVer;
            TranslateVersionStr(curKey, &curVer);
            if (CompareVersion(curVer, minVer) >= 0) {
              if (!strncmp(browserJavaVersion, curKey, _MAX_PATH)) {
                PL_strcpy(newestPath, path);
                ::RegCloseKey(keyloc);
                break;
              }

              if (CompareVersion(curVer, maxVer) >= 0) {
                PL_strcpy(newestPath, path);
                CopyVersion(&maxVer, &curVer);
              }
            }
          }
          ::RegCloseKey(keyloc);
        }
      }
    } while (ERROR_SUCCESS == result);

    ::RegCloseKey(baseloc);

    
    
    if (newestPath[0] != 0) {
      if (ERROR_SUCCESS == ::RegCreateKeyEx(HKEY_LOCAL_MACHINE, mozPath, 0,
                                            NULL, REG_OPTION_NON_VOLATILE,
                                            KEY_SET_VALUE|KEY_QUERY_VALUE,
                                            NULL, &entryloc, NULL)) {
        if (ERROR_SUCCESS != ::RegQueryValueEx(entryloc, "CurrentVersion", 0,
                                               NULL, NULL, NULL)) {
          ::RegSetValueEx(entryloc, "CurrentVersion", 0, REG_SZ,
                          (const BYTE*)MOZILLA_VERSION,
                          sizeof(MOZILLA_VERSION));
        }
        ::RegCloseKey(entryloc);
      }

      PL_strcat(newestPath,"\\bin");
      rv = NS_NewNativeLocalFile(nsDependentCString(newestPath), PR_TRUE,
                                 getter_AddRefs(localFile));
    }
  } else if (nsCRT::strcmp(prop, NS_WIN_QUICKTIME_SCAN_KEY) == 0) {
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
    char path[_MAX_PATH];
    DWORD pathlen = sizeof(path);

    
    
    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\QuickTimePlayer.exe", 0, KEY_READ, &keyloc)) {
      if (ERROR_SUCCESS == ::RegQueryValueEx(keyloc, NULL, NULL, &type,
                                             (LPBYTE)&path, &pathlen)) {
        GetFileVersion((char*)path, &qtVer);
      }
      ::RegCloseKey(keyloc);
    }
    if (CompareVersion(qtVer, minVer) < 0)
      return rv;

    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "software\\Apple Computer, Inc.\\QuickTime", 0, KEY_READ, &keyloc)) {
      DWORD pathlen = sizeof(path);

      result = ::RegQueryValueEx(keyloc, "InstallDir", NULL, &type,
                                 (LPBYTE)&path, &pathlen);
      PL_strcat(path, "\\Plugins");
      if (result == ERROR_SUCCESS)
        rv = NS_NewNativeLocalFile(nsDependentCString(path), PR_TRUE,
                                   getter_AddRefs(localFile));
      ::RegCloseKey(keyloc);
    }
  } else if (nsCRT::strcmp(prop, NS_WIN_WMP_SCAN_KEY) == 0) {
    nsXPIDLCString strVer;
    if (NS_FAILED(prefs->GetCharPref(prop, getter_Copies(strVer))))
      return NS_ERROR_FAILURE;
    verBlock minVer;
    TranslateVersionStr(strVer.get(), &minVer);

    
    HKEY keyloc;
    DWORD type;
    verBlock wmpVer;
    ClearVersion(&wmpVer);
    char path[_MAX_PATH];
    DWORD pathlen = sizeof(path);

    
    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wmplayer.exe", 0, KEY_READ, &keyloc)) {
      if (ERROR_SUCCESS == ::RegQueryValueEx(keyloc, NULL, NULL, &type,
                                             (LPBYTE)&path, &pathlen)) {
        GetFileVersion((char*)path, &wmpVer);
      }
      ::RegCloseKey(keyloc);
    }
    if (CompareVersion(wmpVer, minVer) < 0)
      return rv;

    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        "software\\Microsoft\\MediaPlayer", 0,
                                        KEY_READ, &keyloc)) {
      if (ERROR_SUCCESS == ::RegQueryValueEx(keyloc, "Installation Directory",
                                             NULL, &type, (LPBYTE)&path,
                                             &pathlen)) {
        rv = NS_NewNativeLocalFile(nsDependentCString(path), PR_TRUE,
                                   getter_AddRefs(localFile));
      }

      ::RegCloseKey(keyloc);
    }
  } else if (nsCRT::strcmp(prop, NS_WIN_ACROBAT_SCAN_KEY) == 0) {
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
    char curKey[_MAX_PATH] = "software\\Adobe\\Acrobat Reader";
    char path[_MAX_PATH];
    
    char newestPath[_MAX_PATH + 8];

    newestPath[0] = 0;
    if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, curKey, 0,
                                        KEY_READ, &baseloc)) {
      PL_strcpy(curKey, "software\\Adobe\\Adobe Acrobat");
      if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, curKey, 0,
                                          KEY_READ, &baseloc)) {
        return NS_ERROR_FAILURE;
      }
    }

    
    
    LONG result = ERROR_SUCCESS;
    while (ERROR_SUCCESS == result) {
      path[0] = 0;
      numChars = _MAX_PATH;
      pathlen = sizeof(path);
      result = ::RegEnumKeyEx(baseloc, index, curKey, &numChars, NULL, NULL,
                              NULL, &modTime);
      index++;

      if (ERROR_SUCCESS == result) {
        verBlock curVer;
        TranslateVersionStr(curKey, &curVer);
        PL_strcat(curKey, "\\InstallPath");
        if (ERROR_SUCCESS == ::RegOpenKeyEx(baseloc, curKey, 0,
                                            KEY_QUERY_VALUE, &keyloc)) {
          
          if (ERROR_SUCCESS == ::RegQueryValueEx(keyloc, NULL, NULL, &type,
                                                 (LPBYTE)&path, &pathlen)) {
            if (CompareVersion(curVer, maxVer) >= 0 &&
                CompareVersion(curVer, minVer) >= 0) {
              PL_strcpy(newestPath, path);
              CopyVersion(&maxVer, &curVer);
            }
          }

          ::RegCloseKey(keyloc);
        }
      }
    }

    ::RegCloseKey(baseloc);

    if (newestPath[0] != 0) {
      PL_strcat(newestPath,"\\browser");
      rv = NS_NewNativeLocalFile(nsDependentCString(newestPath), PR_TRUE,
                                 getter_AddRefs(localFile));
    }

  }
#endif

  if (localFile && NS_SUCCEEDED(rv))
    return CallQueryInterface(localFile, _retval);

  return rv;
}

#ifdef XP_WIN
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
  char subkey[_MAX_PATH] = "Software\\MozillaPlugins";
  HKEY baseloc;

  if (ERROR_SUCCESS != ::RegOpenKeyEx(aKey, subkey, 0, KEY_READ, &baseloc))
    return NS_ERROR_FAILURE;

  DWORD index = 0;
  DWORD subkeylen = _MAX_PATH;
  FILETIME modTime;
  while (ERROR_SUCCESS == ::RegEnumKeyEx(baseloc, index++, subkey, &subkeylen,
                                         NULL, NULL, NULL, &modTime)) {
    subkeylen = _MAX_PATH;
    HKEY keyloc;

    if (ERROR_SUCCESS == ::RegOpenKeyEx(baseloc, subkey, 0, KEY_QUERY_VALUE,
                                        &keyloc)) {
      DWORD type;
      char path[_MAX_PATH];
      DWORD pathlen = sizeof(path);

      if (ERROR_SUCCESS == ::RegQueryValueEx(keyloc, "Path", NULL, &type,
                                             (LPBYTE)&path, &pathlen)) {
        nsCOMPtr<nsILocalFile> localFile;
        if (NS_SUCCEEDED(NS_NewNativeLocalFile(nsDependentCString(path),
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

#endif

