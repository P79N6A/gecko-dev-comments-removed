






































#include "setup.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"


HINSTANCE       hSetupRscInst;
HINSTANCE       hXPIStubInst;

HWND            hDlgCurrent;
HWND            hDlgMessage;
HWND            hWndMain;

LPSTR           szEGlobalAlloc;
LPSTR           szEStringLoad;
LPSTR           szEDllLoad;
LPSTR           szEStringNull;
LPSTR           szTempSetupPath;

LPSTR           szSetupDir;
LPSTR           szTempDir;
LPSTR           szOSTempDir;
LPSTR           szFileIniConfig;
LPSTR           szFileIniInstall;

LPSTR           szSiteSelectorDescription;

DWORD           ulWizardState;
ULONG           ulSetupType;
LONG            lScreenX;
LONG            lScreenY;

DWORD           ulTempSetupType;
DWORD           gulUpgradeValue;
DWORD           gulSiteSelectorStatus;

BOOL            bSDUserCanceled;
BOOL            bIdiArchivesExists;
BOOL            bCreateDestinationDir;
BOOL            bReboot;
BOOL            gbILUseTemp;
BOOL            gbPreviousUnfinishedDownload;
BOOL            gbPreviousUnfinishedInstallXpi;
BOOL            gbIgnoreRunAppX;
BOOL            gbIgnoreProgramFolderX;
BOOL            gbDownloadTriggered;

setupGen        sgProduct;
diS             diSetup;
diW             diWelcome;
diL             diLicense;
diQL            diQuickLaunch;
diST            diSetupType;
diSC            diSelectComponents;
diSC            diSelectAdditionalComponents;
diOI            diOS2Integration;
diPF            diProgramFolder;
diDO            diAdditionalOptions;
diAS            diAdvancedSettings;
diSI            diStartInstall;
diD             diDownload;
diR             diReboot;
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

int main(int argc, char *argv[], char *envp[])
{
  HAB hab;
  HMQ hmq;
  QMSG qmsg;
  char  szBuf[MAX_BUF];
  int   iRv = WIZ_OK;
  HWND  hwndFW;
  int rc = 0;
  ATOM atom;

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  atom = WinAddAtom(WinQuerySystemAtomTable(), CLASS_NAME_SETUP_DLG);

  



  


  if((hwndFW = FindWindow(CLASS_NAME_SETUP_DLG)) != NULL)
  {
    WinSetActiveWindow(HWND_DESKTOP, hwndFW);
    iRv = WIZ_SETUP_ALREADY_RUNNING;
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else if(Initialize(0, argv[0]))
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  else if(!InitApplication())
  {
    char szEFailed[MAX_BUF];

    if(GetPrivateProfileString("Messages", "ERROR_FAILED", "", szEFailed, sizeof(szEFailed), szFileIniInstall))
    {
      sprintf(szBuf, szEFailed, "InitApplication().");
      PrintError(szBuf, ERROR_CODE_SHOW);
    }
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else if(!InitInstance())
  {
    char szEFailed[MAX_BUF];

    if(GetPrivateProfileString("Messages", "ERROR_FAILED", "", szEFailed, sizeof(szEFailed), szFileIniInstall))
    {
      sprintf(szBuf, szEFailed, "InitInstance().");
      PrintError(szBuf, ERROR_CODE_SHOW);
    }
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else if(GetInstallIni())
  {
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else if(ParseInstallIni())
  {
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else if(GetConfigIni())
  {
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else if(ParseConfigIni(argc, argv))
  {
    WinPostQueueMsg(0, WM_QUIT, 1, 0);
  }
  else
  {
    DlgSequence(NEXT_DLG);
  }

  while ( WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0 ) )
    WinDispatchMsg( hab, &qmsg );

  if(iRv != WIZ_SETUP_ALREADY_RUNNING)
    
    DeInitialize();

  WinDeleteAtom(WinQuerySystemAtomTable(), atom);

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab ); 

  
  DosSetExtLIBPATH("F", LIBPATHSTRICT);
}

