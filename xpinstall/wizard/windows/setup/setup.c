






































#include "setup.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"


HINSTANCE       hInst;
HINSTANCE       hSetupRscInst;
HINSTANCE       hSDInst;
HINSTANCE       hXPIStubInst;

HBITMAP         hbmpBoxChecked;
HBITMAP         hbmpBoxCheckedDisabled;
HBITMAP         hbmpBoxUnChecked;

HANDLE          hAccelTable;

HWND            hDlgCurrent;
HWND            hDlgMessage;
HWND            hWndMain;

LPSTR           szEGlobalAlloc;
LPSTR           szEStringLoad;
LPSTR           szEDllLoad;
LPSTR           szEStringNull;
LPSTR           szTempSetupPath;
LPSTR           szEOutOfMemory;

LPSTR           szSetupDir;
LPSTR           szTempDir;
LPSTR           szOSTempDir;
LPSTR           szFileIniConfig;
LPSTR           szFileIniInstall;

LPSTR           szSiteSelectorDescription;

DWORD           dwWizardState;
DWORD           dwSetupType;

DWORD           dwTempSetupType;
DWORD           gdwUpgradeValue;
DWORD           gdwSiteSelectorStatus;

BOOL            bSDUserCanceled;
BOOL            bIdiArchivesExists;
BOOL            bCreateDestinationDir;
BOOL            bReboot;
BOOL            gbILUseTemp;
BOOL            gbPreviousUnfinishedDownload;
BOOL            gbPreviousUnfinishedInstallXpi;
BOOL            gbIgnoreRunAppX;
BOOL            gbIgnoreProgramFolderX;
BOOL            gbRestrictedAccess;
BOOL            gbDownloadTriggered;
BOOL            gbAllowMultipleInstalls = FALSE;
BOOL            gbForceInstall = FALSE;
BOOL            gbForceInstallGre = FALSE;
BOOL            gShowBannerImage = TRUE;

setupGen        sgProduct;
diS             diSetup;
diW             diWelcome;
diL             diLicense;
diQL            diQuickLaunch;
diST            diSetupType;
diSC            diSelectComponents;
diSC            diSelectAdditionalComponents;
diWI            diWindowsIntegration;
diPF            diProgramFolder;
diDO            diAdditionalOptions;
diAS            diAdvancedSettings;
diSI            diStartInstall;
diD             diDownload;
diR             diReboot;
siSD            siSDObject;
siCF            siCFXpcomFile;
siC             *siComponents;
ssi             *ssiSiteSelector;
installGui      sgInstallGui;
sems            gErrorMessageStream;
sysinfo         gSystemInfo;
dsN             *gdsnComponentDSRequirement = NULL;




char *SetupFileList[] = {"setuprsc.dll",
                         "config.ini",
                         "setup.ini",
                         "install.ini",
                         "license.txt",
                         ""};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
  
  
  
  
  
  

  MSG   msg;
  char  szBuf[MAX_BUF];
  int   iRv = WIZ_OK;
  HWND  hwndFW;

  if(!hPrevInstance)
  {
    if(InitSetupGeneral())
      PostQuitMessage(1);
    else if(ParseForStartupOptions(lpszCmdLine))
      PostQuitMessage(1);
    else if(((hwndFW = FindWindow(CLASS_NAME_SETUP_DLG, NULL)) != NULL ||
            ((hwndFW = FindWindow(CLASS_NAME_SETUP, NULL)) != NULL)) &&
              !gbAllowMultipleInstalls)
    {
    



      ShowWindow(hwndFW, SW_RESTORE);
      SetForegroundWindow(hwndFW);
      iRv = WIZ_SETUP_ALREADY_RUNNING;
      PostQuitMessage(1);
    }
    else if(Initialize(hInstance))
      PostQuitMessage(1);
    else if(!InitApplication(hInstance, hSetupRscInst))
    {
      char szEFailed[MAX_BUF];

      if(GetPrivateProfileString("Messages", "ERROR_FAILED", "", szEFailed, sizeof(szEFailed), szFileIniInstall))
      {
        wsprintf(szBuf, szEFailed, "InitApplication().");
        PrintError(szBuf, ERROR_CODE_SHOW);
      }
      PostQuitMessage(1);
    }
    else if(!InitInstance(hInstance, nCmdShow))
    {
      char szEFailed[MAX_BUF];

      if(GetPrivateProfileString("Messages", "ERROR_FAILED", "", szEFailed, sizeof(szEFailed), szFileIniInstall))
      {
        wsprintf(szBuf, szEFailed, "InitInstance().");
        PrintError(szBuf, ERROR_CODE_SHOW);
      }
      PostQuitMessage(1);
    }
    else if(GetInstallIni())
    {
      PostQuitMessage(1);
    }
    else if(ParseInstallIni())
    {
      PostQuitMessage(1);
    }
    else if(GetConfigIni())
    {
      PostQuitMessage(1);
    }
    else if(ParseConfigIni(lpszCmdLine))
    {
      PostQuitMessage(1);
    }
    else
    {
      ShowMessage(NULL, FALSE);
      DlgSequence(NEXT_DLG);
    }
  }

  while(GetMessage(&msg, NULL, 0, 0))
  {
    if((!IsDialogMessage(hDlgCurrent, &msg)) && (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  if(iRv != WIZ_SETUP_ALREADY_RUNNING)
    
    DeInitialize();

  
  DeInitSetupGeneral();

  return(msg.wParam);
} 

