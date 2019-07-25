











































#include <windows.h>
#include <ce_setup.h>

const WCHAR c_sAppRegKey[] = L"Software\\Mozilla\\" MOZ_APP_DISPLAYNAME;


BOOL GetInstallPath(WCHAR *sPath);
BOOL RunUninstall();




BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  return TRUE;
}

codeINSTALL_INIT Install_Init(HWND hwndParent, BOOL fFirstCall, BOOL fPreviouslyInstalled, LPCTSTR pszInstallDir)
{
  return codeINSTALL_INIT_CONTINUE;
}

codeINSTALL_EXIT Install_Exit(HWND hwndParent, LPCTSTR pszInstallDir,
                              WORD cFailedDirs, WORD cFailedFiles, WORD cFailedRegKeys,
                              WORD cFailedRegVals, WORD cFailedShortcuts)
{
  return codeINSTALL_EXIT_DONE;
}

codeUNINSTALL_INIT Uninstall_Init(HWND hwndParent, LPCTSTR pszInstallDir)
{
  RunUninstall();

  
  
  
  return codeUNINSTALL_INIT_CONTINUE;
}

codeUNINSTALL_EXIT Uninstall_Exit(HWND hwndParent)
{
  return codeUNINSTALL_EXIT_DONE;
}




BOOL GetInstallPath(WCHAR *sPath)
{
  HKEY hKey;

  LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_sAppRegKey, 0, KEY_ALL_ACCESS, &hKey);
  if (result == ERROR_SUCCESS)
  {
    DWORD dwType = NULL;
    DWORD dwCount = MAX_PATH * sizeof(WCHAR);
    result = RegQueryValueEx(hKey, L"Path", NULL, &dwType, (LPBYTE)sPath, &dwCount);

    RegCloseKey(hKey);
  }

  return (result == ERROR_SUCCESS);
}

BOOL RunUninstall()
{
  BOOL bResult = FALSE;
  WCHAR sUninstallPath[MAX_PATH];
  if (GetInstallPath(sUninstallPath))
  {
    if (wcslen(sUninstallPath) > 0 && sUninstallPath[wcslen(sUninstallPath)-1] != '\\')
      wcscat(sUninstallPath, L"\\");

    wcscat(sUninstallPath, L"uninstall.exe");

    PROCESS_INFORMATION pi;
    bResult = CreateProcess(sUninstallPath, L"[setup]",
                            NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);
    if (bResult)
    {
      
      WaitForSingleObject(pi.hProcess, INFINITE);
    }
  }
  return bResult;
}
