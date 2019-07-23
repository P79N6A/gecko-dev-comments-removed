











































#include <windows.h>
#include "nsSetupStrings.h"

const WCHAR c_sStringsFile[] = L"setup.ini";
nsSetupStrings Strings;

WCHAR g_sUninstallPath[MAX_PATH];

const WCHAR c_sRemoveParam[] = L"remove";


BOOL GetModulePath(WCHAR *sPath);
BOOL LoadStrings();
BOOL GetInstallPath(WCHAR *sPath);
BOOL DeleteShortcut(HWND hwndParent);
BOOL DeleteDirectory(const WCHAR* sPathToDelete);
BOOL DeleteRegistryKey();
BOOL CopyAndLaunch();


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
  HWND hWnd = GetForegroundWindow();
  *g_sUninstallPath = 0;

  WCHAR *sCommandLine = GetCommandLine();
  WCHAR *sRemoveParam = wcsstr(sCommandLine, c_sRemoveParam);
  if (sRemoveParam != NULL)
  {
    
    wcscpy(g_sUninstallPath, sRemoveParam + wcslen(c_sRemoveParam) + 1);
  }
  else
  {
    
    
    if (CopyAndLaunch())
      return 0;
  }
  
  

  if (!LoadStrings())
  {
    MessageBoxW(hWnd, L"Cannot find the strings file.", L"Uninstall", MB_OK|MB_ICONWARNING);
    return -1;
  }

  WCHAR sInstallPath[MAX_PATH];
  if (GetInstallPath(sInstallPath))
  {
    WCHAR sMsg[MAX_PATH+256];
    _snwprintf(sMsg, MAX_PATH+256, L"%s %s\n%s", Strings.GetString(StrID_FilesWillBeRemoved),
      sInstallPath, Strings.GetString(StrID_AreYouSure));
    if (MessageBoxW(hWnd, sMsg, Strings.GetString(StrID_UninstallCaption),
                    MB_YESNO|MB_ICONWARNING) == IDNO)
    {
      return -2;
    }

    
    DeleteDirectory(sInstallPath);
    DeleteShortcut(hWnd);
    DeleteRegistryKey();

    
    
    MessageBoxW(hWnd, Strings.GetString(StrID_UninstalledSuccessfully),
                Strings.GetString(StrID_UninstallCaption), MB_OK|MB_ICONINFORMATION);
  }
  else
  {
    MessageBoxW(hWnd, Strings.GetString(StrID_InstallationNotFound),
                Strings.GetString(StrID_UninstallCaption), MB_OK|MB_ICONINFORMATION);
    return -1;
  }

  return 0;
}

BOOL LoadStrings()
{
  
  if (*g_sUninstallPath == 0)
    GetModulePath(g_sUninstallPath);

  WCHAR sStringsFile[MAX_PATH];
  _snwprintf(sStringsFile, MAX_PATH, L"%s\\%s", g_sUninstallPath, c_sStringsFile);

  return Strings.LoadStrings(sStringsFile);
}

BOOL GetModulePath(WCHAR *sPath)
{
  if (GetModuleFileName(NULL, sPath, MAX_PATH) == 0)
    return FALSE;

  WCHAR *sSlash = wcsrchr(sPath, L'\\') + 1;
  *sSlash = L'\0'; 

  return TRUE;
}

BOOL GetInstallPath(WCHAR *sPath)
{
  HKEY hKey;
  WCHAR sRegFennecKey[MAX_PATH];
  _snwprintf(sRegFennecKey, MAX_PATH, L"Software\\%s", Strings.GetString(StrID_AppShortName));

  LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sRegFennecKey, 0, KEY_ALL_ACCESS, &hKey);
  if (result == ERROR_SUCCESS)
  {
    DWORD dwType = NULL;
    DWORD dwCount = MAX_PATH * sizeof(WCHAR);
    result = RegQueryValueEx(hKey, L"Path", NULL, &dwType, (LPBYTE)sPath, &dwCount);

    RegCloseKey(hKey);
  }

  return (result == ERROR_SUCCESS);
}

BOOL DeleteDirectory(const WCHAR* sPathToDelete)
{
  HANDLE          hFile;
  WCHAR           sFilePath[MAX_PATH];
  WCHAR           sPattern[MAX_PATH];
  WIN32_FIND_DATA findData;

  wcscpy(sPattern, sPathToDelete);
  wcscat(sPattern, L"\\*.*");
  hFile = FindFirstFile(sPattern, &findData);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (findData.cFileName[0] != L'.')
      {
        _snwprintf(sFilePath, MAX_PATH, L"%s\\%s", sPathToDelete, findData.cFileName);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          
          BOOL bRet = DeleteDirectory(sFilePath);
          if (!bRet)
          {
            
            
          }
        }
        else
        {
          
          if (SetFileAttributes(sFilePath, FILE_ATTRIBUTE_NORMAL))
          {
            
            if (!DeleteFile(sFilePath))
            {
              
              
            }
          }
        }
      }
    } while (FindNextFile(hFile, &findData));

    
    FindClose(hFile);

    DWORD dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
      return FALSE;
    else
    {
      
      if (SetFileAttributes(sPathToDelete, FILE_ATTRIBUTE_NORMAL))
      {
        
        if (!RemoveDirectory(sPathToDelete))
          return FALSE;
      }
    }
  }

  return TRUE;
}

BOOL DeleteShortcut(HWND hwndParent)
{
  WCHAR sProgramsPath[MAX_PATH];
  if (!SHGetSpecialFolderPath(hwndParent, sProgramsPath, CSIDL_PROGRAMS, FALSE))
    wcscpy(sProgramsPath, L"\\Windows\\Start Menu\\Programs");

  WCHAR sShortcutPath[MAX_PATH];
  _snwprintf(sShortcutPath, MAX_PATH, L"%s\\%s.lnk", sProgramsPath, Strings.GetString(StrID_AppShortName));


  if(SetFileAttributes(sShortcutPath, FILE_ATTRIBUTE_NORMAL))
    return DeleteFile(sShortcutPath);

  return FALSE;
}

BOOL DeleteRegistryKey()
{
  WCHAR sRegFennecKey[MAX_PATH];
  _snwprintf(sRegFennecKey, MAX_PATH, L"Software\\%s", Strings.GetString(StrID_AppShortName));
  LONG result = RegDeleteKey(HKEY_LOCAL_MACHINE, sRegFennecKey);
  return (result == ERROR_SUCCESS);
}


BOOL CopyAndLaunch()
{
  WCHAR sNewName[] = L"\\Temp\\uninstall.exe";
  WCHAR sModule[MAX_PATH];
  if (GetModuleFileName(NULL, sModule, MAX_PATH) == 0 || !GetModulePath(g_sUninstallPath))
    return FALSE;
  if (CopyFile(sModule, sNewName, FALSE))
  {
    PROCESS_INFORMATION pi;
    WCHAR sParam[MAX_PATH+20];
    _snwprintf(sParam, MAX_PATH+20, L"%s %s", c_sRemoveParam, g_sUninstallPath);

    
    return CreateProcess(sNewName, sParam, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);
  }
  else
    return FALSE;
}
