






































#include "uninstall.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"


HINSTANCE       hInst;

HANDLE          hAccelTable;

HWND            hDlgUninstall;
HWND            hDlgMessage;
HWND            hWndMain;

LPSTR           szEGlobalAlloc;
LPSTR           szEStringLoad;
LPSTR           szEDllLoad;
LPSTR           szEStringNull;
LPSTR           szTempSetupPath;

LPSTR           szClassName;
LPSTR           szUninstallDir;
LPSTR           szTempDir;
LPSTR           szOSTempDir;
LPSTR           szFileIniUninstall;
LPSTR           szFileIniDefaultsInfo;
LPSTR           gszSharedFilename;

ULONG           ulOSType;
DWORD           dwScreenX;
DWORD           dwScreenY;

DWORD           gdwWhatToDo;

BOOL            gbAllowMultipleInstalls = FALSE;

uninstallGen    ugUninstall;
diU             diUninstall;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
  
  
  
  
  
  

  MSG   msg;
  char  szBuf[MAX_BUF];
  int   iRv = WIZ_OK;
  HWND  hwndFW;

  if(!hPrevInstance)
  {
    hInst = GetModuleHandle(NULL);
    if(InitUninstallGeneral())
      PostQuitMessage(1);
    else if(ParseCommandLine(lpszCmdLine))
      PostQuitMessage(1);
    else if((hwndFW = FindWindow(CLASS_NAME_UNINSTALL_DLG, NULL)) != NULL && !gbAllowMultipleInstalls)
    {
    



      ShowWindow(hwndFW, SW_RESTORE);
      SetForegroundWindow(hwndFW);
      iRv = WIZ_SETUP_ALREADY_RUNNING;
      PostQuitMessage(1);
    }
    else if(Initialize(hInst))
    {
      PostQuitMessage(1);
    }
    else if(!InitApplication(hInst))
    {
      char szEFailed[MAX_BUF];

      if(NS_LoadString(hInst, IDS_ERROR_FAILED, szEFailed, MAX_BUF) == WIZ_OK)
      {
        wsprintf(szBuf, szEFailed, "InitApplication().");
        PrintError(szBuf, ERROR_CODE_SHOW);
      }
      PostQuitMessage(1);
    }
    else if(ParseUninstallIni())
    {
      PostQuitMessage(1);
    }
    else if(ugUninstall.bUninstallFiles == TRUE)
    {
      if(diUninstall.bShowDialog == TRUE)
        hDlgUninstall = InstantiateDialog(hWndMain, DLG_UNINSTALL, diUninstall.szTitle, DlgProcUninstall);
      
      else if((ugUninstall.mode == SHOWICONS) || (ugUninstall.mode == HIDEICONS))
        ParseDefaultsInfo();
      else if(ugUninstall.mode == SETDEFAULT)
        SetDefault();
      else
        ParseAllUninstallLogs();
    }
  }

  if((ugUninstall.bUninstallFiles == TRUE) && (diUninstall.bShowDialog == TRUE))
  {
    while(GetMessage(&msg, NULL, 0, 0))
    {
      if((!IsDialogMessage(hDlgUninstall, &msg)) && (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  
  DeInitUninstallGeneral();
  if(iRv != WIZ_SETUP_ALREADY_RUNNING)
    
    DeInitialize();

  return(msg.wParam);
} 

