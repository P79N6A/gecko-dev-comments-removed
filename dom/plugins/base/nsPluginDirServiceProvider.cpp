




#include "nsPluginDirServiceProvider.h"

#include "nsCRT.h"
#include "nsIFile.h"
#include "nsDependentString.h"
#include "nsArrayEnumerator.h"
#include "mozilla/Preferences.h"

#include <windows.h>
#include "nsIWindowsRegKey.h"

using namespace mozilla;

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
FileExists(LPCWSTR szFile)
{
  return GetFileAttributesW(szFile) != 0xFFFFFFFF;
}


static BOOL
GetFileVersion(LPCWSTR szFile, verBlock *vbVersion)
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
    LPCWSTR lpFilepath = szFile;
    dwLen  = GetFileVersionInfoSizeW(lpFilepath, &dwHandle);
    lpData = (LPVOID)malloc(dwLen);
    uLen   = 0;

    if (lpData && GetFileVersionInfoW(lpFilepath, dwHandle, dwLen, lpData) != 0) {
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
TranslateVersionStr(const WCHAR* szVersion, verBlock *vbVersion)
{
  WCHAR* szNum1 = NULL;
  WCHAR* szNum2 = NULL;
  WCHAR* szNum3 = NULL;
  WCHAR* szNum4 = NULL;
  WCHAR* szJavaBuild = NULL;

  WCHAR *strVer = nullptr;
  if (szVersion) {
    strVer = wcsdup(szVersion);
  }

  if (!strVer) {
    
    ClearVersion(vbVersion);
    return;
  }

  
  szJavaBuild = wcschr(strVer, '_');
  if (szJavaBuild) {
    szJavaBuild[0] = '.';
  }

  szNum1 = wcstok(strVer, L".");
  szNum2 = wcstok(NULL,   L".");
  szNum3 = wcstok(NULL,   L".");
  szNum4 = wcstok(NULL,   L".");

  vbVersion->wMajor   = szNum1 ? (WORD) _wtoi(szNum1) : 0;
  vbVersion->wMinor   = szNum2 ? (WORD) _wtoi(szNum2) : 0;
  vbVersion->wRelease = szNum3 ? (WORD) _wtoi(szNum3) : 0;
  vbVersion->wBuild   = szNum4 ? (WORD) _wtoi(szNum4) : 0;

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





nsPluginDirServiceProvider::nsPluginDirServiceProvider()
{
}

nsPluginDirServiceProvider::~nsPluginDirServiceProvider()
{
}





NS_IMPL_THREADSAFE_ISUPPORTS1(nsPluginDirServiceProvider,
                              nsIDirectoryServiceProvider)





NS_IMETHODIMP
nsPluginDirServiceProvider::GetFile(const char *charProp, bool *persistant,
                                    nsIFile **_retval)
{
  nsCOMPtr<nsIFile>  localFile;
  nsresult rv = NS_ERROR_FAILURE;

  NS_ENSURE_ARG(charProp);

  *_retval = nullptr;
  *persistant = false;

  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_ENSURE_TRUE(regKey, NS_ERROR_FAILURE);

  if (nsCRT::strcmp(charProp, NS_WIN_QUICKTIME_SCAN_KEY) == 0) {
    nsAdoptingCString strVer = Preferences::GetCString(charProp);
    if (!strVer) {
      return NS_ERROR_FAILURE;
    }
    verBlock minVer;
    TranslateVersionStr(NS_ConvertASCIItoUTF16(strVer).get(), &minVer);

    
    verBlock qtVer;
    ClearVersion(&qtVer);

    
    
    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                      NS_LITERAL_STRING("software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\QuickTimePlayer.exe"),
                      nsIWindowsRegKey::ACCESS_READ);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString path;
      rv = regKey->ReadStringValue(NS_LITERAL_STRING(""), path);
      if (NS_SUCCEEDED(rv)) {
        GetFileVersion(path.get(), &qtVer);
      }
      regKey->Close();
    }
    if (CompareVersion(qtVer, minVer) < 0)
      return rv;

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                      NS_LITERAL_STRING("software\\Apple Computer, Inc.\\QuickTime"),
                      nsIWindowsRegKey::ACCESS_READ);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString path;
      rv = regKey->ReadStringValue(NS_LITERAL_STRING("InstallDir"), path);
      if (NS_SUCCEEDED(rv)) {
        path += NS_LITERAL_STRING("\\Plugins");
        rv = NS_NewLocalFile(path, true,
                             getter_AddRefs(localFile));
      }
    }
  } else if (nsCRT::strcmp(charProp, NS_WIN_WMP_SCAN_KEY) == 0) {
    nsAdoptingCString strVer = Preferences::GetCString(charProp);
    if (!strVer) {
      return NS_ERROR_FAILURE;
    }
    verBlock minVer;
    TranslateVersionStr(NS_ConvertASCIItoUTF16(strVer).get(), &minVer);

    
    verBlock wmpVer;
    ClearVersion(&wmpVer);

    
    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                      NS_LITERAL_STRING("software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wmplayer.exe"),
                      nsIWindowsRegKey::ACCESS_READ);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString path;
      rv = regKey->ReadStringValue(NS_LITERAL_STRING(""), path);
      if (NS_SUCCEEDED(rv)) {
        GetFileVersion(path.get(), &wmpVer);
      }
      regKey->Close();
    }
    if (CompareVersion(wmpVer, minVer) < 0)
      return rv;

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                      NS_LITERAL_STRING("software\\Microsoft\\MediaPlayer"),
                      nsIWindowsRegKey::ACCESS_READ);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString path;
      rv = regKey->ReadStringValue(NS_LITERAL_STRING("Installation Directory"),
                                   path);
      if (NS_SUCCEEDED(rv)) {
        rv = NS_NewLocalFile(path, true,
                             getter_AddRefs(localFile));
      }
    }
  } else if (nsCRT::strcmp(charProp, NS_WIN_ACROBAT_SCAN_KEY) == 0) {
    nsAdoptingCString strVer = Preferences::GetCString(charProp);
    if (!strVer) {
      return NS_ERROR_FAILURE;
    }

    verBlock minVer;
    TranslateVersionStr(NS_ConvertASCIItoUTF16(strVer).get(), &minVer);

    
    verBlock maxVer;
    ClearVersion(&maxVer);

    nsAutoString newestPath;

    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                      NS_LITERAL_STRING("software\\Adobe\\Acrobat Reader"),
                      nsIWindowsRegKey::ACCESS_READ);
    if (NS_FAILED(rv)) {
      rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                        NS_LITERAL_STRING("software\\Adobe\\Adobe Acrobat"),
                        nsIWindowsRegKey::ACCESS_READ);
      if (NS_FAILED(rv)) {
        return NS_ERROR_FAILURE;
      }
    }

    
    
    uint32_t childCount = 0;
    regKey->GetChildCount(&childCount);

    for (uint32_t index = 0; index < childCount; ++index) {
      nsAutoString childName;
      rv = regKey->GetChildName(index, childName);
      if (NS_SUCCEEDED(rv)) {
        verBlock curVer;
        TranslateVersionStr(childName.get(), &curVer);

        childName += NS_LITERAL_STRING("\\InstallPath");

        nsCOMPtr<nsIWindowsRegKey> childKey;
        rv = regKey->OpenChild(childName, nsIWindowsRegKey::ACCESS_QUERY_VALUE,
                               getter_AddRefs(childKey));
        if (NS_SUCCEEDED(rv)) {
          
          nsAutoString path;
          rv = childKey->ReadStringValue(NS_LITERAL_STRING(""), path);
          if (NS_SUCCEEDED(rv)) {
            if (CompareVersion(curVer, maxVer) >= 0 &&
                CompareVersion(curVer, minVer) >= 0) {
              newestPath = path;
              CopyVersion(&maxVer, &curVer);
            }
          }
        }
      }
    }

    if (!newestPath.IsEmpty()) {
      newestPath += NS_LITERAL_STRING("\\browser");
      rv = NS_NewLocalFile(newestPath, true,
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
  *aEnumerator = nullptr;

  nsCOMArray<nsIFile> dirs;

  GetPLIDDirectoriesWithRootKey(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER, dirs);
  GetPLIDDirectoriesWithRootKey(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE, dirs);

  return NS_NewArrayEnumerator(aEnumerator, dirs);
}

nsresult
nsPluginDirServiceProvider::GetPLIDDirectoriesWithRootKey(uint32_t aKey, nsCOMArray<nsIFile> &aDirs)
{
  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_ENSURE_TRUE(regKey, NS_ERROR_FAILURE);

  nsresult rv = regKey->Open(aKey,
                             NS_LITERAL_STRING("Software\\MozillaPlugins"),
                             nsIWindowsRegKey::ACCESS_READ);
  if (NS_FAILED(rv)) {
    return rv;
  }

  uint32_t childCount = 0;
  regKey->GetChildCount(&childCount);

  for (uint32_t index = 0; index < childCount; ++index) {
    nsAutoString childName;
    rv = regKey->GetChildName(index, childName);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIWindowsRegKey> childKey;
      rv = regKey->OpenChild(childName, nsIWindowsRegKey::ACCESS_QUERY_VALUE,
                             getter_AddRefs(childKey));
      if (NS_SUCCEEDED(rv) && childKey) {
        nsAutoString path;
        rv = childKey->ReadStringValue(NS_LITERAL_STRING("Path"), path);
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIFile> localFile;
          if (NS_SUCCEEDED(NS_NewLocalFile(path, true,
                                           getter_AddRefs(localFile))) &&
              localFile) {
            
            
            bool isDir = false;
            if (NS_SUCCEEDED(localFile->IsDirectory(&isDir)) && !isDir) {
              nsCOMPtr<nsIFile> temp;
              localFile->GetParent(getter_AddRefs(temp));
              if (temp)
                localFile = temp;
            }

            
            
            bool isFileThere = false;
            bool isDupEntry = false;
            if (NS_SUCCEEDED(localFile->Exists(&isFileThere)) && isFileThere) {
              int32_t c = aDirs.Count();
              for (int32_t i = 0; i < c; i++) {
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
      }
    }
  }
  return NS_OK;
}

