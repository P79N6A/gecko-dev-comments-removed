











































#include <windows.h>
#include <aygshell.h>

#include "nsSetupStrings.h"
#include "resource.h"

#define WM_DIALOGCREATED (WM_USER + 1)

const WCHAR c_sStringsFile[] = L"setup.ini";
nsSetupStrings Strings;

WCHAR g_sInstallPath[MAX_PATH];
WCHAR g_sUninstallPath[MAX_PATH];
HWND g_hDlg = NULL;

const DWORD c_nTempBufSize = MAX_PATH * 2;
const WCHAR c_sRemoveParam[] = L"[remove]";


int nTotalRetries = 0;
const int c_nMaxTotalRetries = 10;
const int c_nMaxOneFileRetries = 6;
const int c_nRetryDelay = 500; 

int g_nDirsDeleted = 0;
const int c_nMaxDirs = 25; 

enum {
  ErrOK = 0,
  ErrCancel = IDCANCEL,
  ErrNoStrings = -1,
  ErrInstallationNotFound = -2,
  ErrShutdownFailed = -3,
};

int g_nResult = ErrOK;


BOOL GetModulePath(WCHAR *sPath);
BOOL LoadStrings();
BOOL GetInstallPath(WCHAR *sPath);
BOOL DeleteShortcut(HWND hwndParent);
BOOL DeleteDirectory(const WCHAR* sPathToDelete);
BOOL DeleteRegistryKey();
BOOL CopyAndLaunch();
BOOL ShutdownFastStartService(const WCHAR *sInstallPath);

BOOL CALLBACK DlgUninstall(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL OnDialogInit(HWND hDlg);
BOOL OnDialogCreated(HWND hDlg);
int OnUninstall(HWND hDlg);
void UpdateProgress();




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
      return ErrOK;
  }
  
  

  if (!LoadStrings())
  {
    MessageBoxW(hWnd, L"Cannot find the strings file.", L"Uninstall", MB_OK|MB_ICONWARNING);
    return ErrNoStrings;
  }

  if (GetInstallPath(g_sInstallPath))
  {
    int nDlgResult = DialogBox(hInstance, (LPCTSTR)IDD_MAIN, NULL, (DLGPROC)DlgUninstall);
    if (nDlgResult != ErrOK)
      g_nResult = nDlgResult;
  }
  else
  {
    MessageBoxW(hWnd, Strings.GetString(StrID_InstallationNotFound),
                Strings.GetString(StrID_UninstallCaption), MB_OK|MB_ICONINFORMATION);
    return ErrInstallationNotFound;
  }

  return g_nResult;
}

int Uninstall(HWND hWnd)
{
  
  DeleteDirectory(g_sInstallPath);
  DeleteShortcut(hWnd);
  DeleteRegistryKey();

  
  
  return ErrOK;
}




BOOL CALLBACK DlgUninstall(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      return OnDialogInit(hDlg);

    case WM_DIALOGCREATED:
      return OnDialogCreated(hDlg);

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
          EndDialog(hDlg, ErrOK);
          return TRUE;

        case IDCANCEL:
          EndDialog(hDlg, ErrCancel);
          return TRUE;

        case IDC_BTN_UNINSTALL:
          g_nResult = OnUninstall(hDlg);
          return TRUE;
      }
      break;

    case WM_CLOSE:
      EndDialog(hDlg, ErrCancel);
      return TRUE;
  }
  return FALSE;
}

BOOL OnDialogCreated(HWND)
{
  ShutdownFastStartService(g_sInstallPath);
  
  return TRUE;
}

BOOL OnDialogInit(HWND hDlg)
{
  g_hDlg = hDlg;
  PostMessage(hDlg, WM_DIALOGCREATED, 0, 0);

  SetWindowText(hDlg, Strings.GetString(StrID_UninstallCaption));

  SHINITDLGINFO shidi;
  shidi.dwMask = SHIDIM_FLAGS;
  shidi.dwFlags = SHIDIF_SIPDOWN | SHIDIF_EMPTYMENU;
  shidi.hDlg = hDlg;
  SHInitDialog(&shidi);

  
  SHDoneButton(hDlg, SHDB_HIDE);

  
  WCHAR sMsg[c_nTempBufSize];
  _snwprintf(sMsg, c_nTempBufSize, L"%s %s", Strings.GetString(StrID_FilesWillBeRemoved), g_sInstallPath);
  SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TEXT), sMsg);
  SetWindowText(GetDlgItem(hDlg, IDC_QUESTION_TEXT), Strings.GetString(StrID_AreYouSure));
  SetWindowText(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), L"OK"); 
  SetWindowText(GetDlgItem(hDlg, IDCANCEL), Strings.GetString(StrID_Cancel));
  ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
  ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);

  return TRUE; 
}

int OnUninstall(HWND hDlg)
{
  SetWindowText(GetDlgItem(hDlg, IDC_QUESTION_TEXT), L"");
  ShowWindow(GetDlgItem(hDlg, IDC_BTN_UNINSTALL), SW_HIDE);
  ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
  ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_SHOW);
  SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE, 0, (LPARAM) MAKELPARAM (0, c_nMaxDirs));
  SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);

  UpdateWindow(hDlg); 
  
  int ret = Uninstall(hDlg);

  if (ret == ErrOK)
  {
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TEXT), Strings.GetString(StrID_UninstalledSuccessfully));
  }
  else
  {
    
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TEXT), L"There were errors during uninstallation.");
  }

  ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
  ShowWindow(GetDlgItem(hDlg, IDOK), SW_SHOW);

  return ret;
}

void UpdateProgress()
{
  SendMessage(GetDlgItem(g_hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM)g_nDirsDeleted, 0);
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
  BOOL            bResult = TRUE;
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
          
          if (!DeleteDirectory(sFilePath))
            bResult = FALSE;
        }
        else
        {
          
          if (SetFileAttributes(sFilePath, FILE_ATTRIBUTE_NORMAL))
          {
            
            if (!DeleteFile(sFilePath) && nTotalRetries < c_nMaxTotalRetries)
            {
              BOOL bDeleted = FALSE;
              nTotalRetries++;
              for (int nRetry = 0; nRetry < c_nMaxOneFileRetries; nRetry++)
              {
                Sleep(c_nRetryDelay);
                if (DeleteFile(sFilePath))
                {
                  bDeleted = TRUE;
                  break;
                }
              }
              if (!bDeleted)
                bResult = FALSE;
            }
          }
        }
      }
    } while (FindNextFile(hFile, &findData));

    
    FindClose(hFile);
  }

  
  if (SetFileAttributes(sPathToDelete, FILE_ATTRIBUTE_NORMAL))
  {
    g_nDirsDeleted++;
    UpdateProgress();
    
    if (!RemoveDirectory(sPathToDelete) && nTotalRetries < c_nMaxTotalRetries)
    {
      BOOL bDeleted = FALSE;
      nTotalRetries++;
      for (int nRetry = 0; nRetry < c_nMaxOneFileRetries; nRetry++)
      {
        Sleep(c_nRetryDelay);
        if (RemoveDirectory(sPathToDelete))
        {
          bDeleted = TRUE;
          break;
        }
      }
      if (!bDeleted)
        bResult = FALSE;
    }
  }

  return bResult;
}



BOOL DeleteShortcut(HWND hwndParent)
{
  BOOL result = FALSE;

  WCHAR sProgramsPath[MAX_PATH];
  if (SHGetSpecialFolderPath(hwndParent, sProgramsPath, CSIDL_PROGRAMS, FALSE))
  {
    WCHAR sShortcutPath[MAX_PATH];
    _snwprintf(sShortcutPath, MAX_PATH, L"%s\\%s.lnk", sProgramsPath, Strings.GetString(StrID_AppShortName));


    if(SetFileAttributes(sShortcutPath, FILE_ATTRIBUTE_NORMAL))
      result = DeleteFile(sShortcutPath);
  }

  
  WCHAR sStartupPath[MAX_PATH];
  if (SHGetSpecialFolderPath(hwndParent, sStartupPath, CSIDL_STARTUP, FALSE))
  {
    WCHAR sStartupShortcutPath[MAX_PATH];
    _snwprintf(sStartupShortcutPath, MAX_PATH, L"%s\\%sFastStart.lnk", sStartupPath, Strings.GetString(StrID_AppShortName));

    
    if(SetFileAttributes(sStartupShortcutPath, FILE_ATTRIBUTE_NORMAL))
      DeleteFile(sStartupShortcutPath);
  }

  return result;
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
    WCHAR sParam[c_nTempBufSize];
    _snwprintf(sParam, c_nTempBufSize, L"%s %s", c_sRemoveParam, g_sUninstallPath);

    
    return CreateProcess(sNewName, sParam, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);
  }
  else
    return FALSE;
}

BOOL ConvertToChar(const WCHAR *wstr, char *str, DWORD bufSize)
{
  return 0 != WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, bufSize, NULL, NULL);
}

BOOL ShutdownFastStartService(const WCHAR *wsInstallPath)
{
  BOOL result = TRUE;

  
  WCHAR sClassName[c_nTempBufSize];
  _snwprintf(sClassName, c_nTempBufSize, L"%s%s", Strings.GetString(StrID_AppShortName), L"MessageWindow");

  HWND handle = ::FindWindowW(sClassName, NULL);
  if (handle)
  {
    char sPath[MAX_PATH];
    ConvertToChar(wsInstallPath, sPath, MAX_PATH);
    size_t pathLen = strlen(sPath);
    if (pathLen > 0 && sPath[pathLen-1] != '\\')
    {
      strcat(sPath, "\\");
      pathLen++;
    }

    char sCopyData[MAX_PATH * 3];
    _snprintf(sCopyData, MAX_PATH * 2, "\"%s%S.exe\" -shutdown-faststart", sPath, Strings.GetString(StrID_AppShortName));
    size_t cmdLineLen = strlen(sCopyData);

    char *sRest = sCopyData + cmdLineLen + 1;
    strcpy(sRest, sPath);

    COPYDATASTRUCT cds = {
      1,
      pathLen + 1 + cmdLineLen,
      (void*) sCopyData
    };
    ::SendMessage(handle, WM_COPYDATA, 0, (LPARAM)&cds);

    
    for (int i = 0; i < 20 && handle; i++)
    {
      Sleep(500);
      handle = ::FindWindowW(sClassName, NULL);
    }

    
    result = (handle == NULL);
  }
  return result;
}
