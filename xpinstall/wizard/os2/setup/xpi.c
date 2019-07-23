






































#include "extern.h"
#include "dialogs.h"
#include "extra.h"
#include "xpistub.h"
#include "xpi.h"
#include "xperr.h"
#include "logging.h"
#include "ifuncns.h"

#define BDIR_RIGHT 1
#define BDIR_LEFT  2

typedef HRESULT (_Optlink *XpiInit)(const char *, const char *aLogName, pfnXPIProgress);
typedef HRESULT (_Optlink *XpiInstall)(const char *, const char *, long);
typedef void    (_Optlink *XpiExit)(void);

static XpiInit          pfnXpiInit;
static XpiInstall       pfnXpiInstall;
static XpiExit          pfnXpiExit;

static DWORD            dwCurrentArchive;
static DWORD            dwTotalArchives;
char                    szStrProcessingFile[MAX_BUF];
char                    szStrCopyingFile[MAX_BUF];
char                    szStrInstalling[MAX_BUF];

static void UpdateGaugeArchiveProgressBar(unsigned value);

struct ExtractFilesDlgInfo
{
	HWND	hWndDlg;
	int		nMaxFileBars;	    
	int		nMaxArchiveBars;	
	int		nArchiveBars;		  
} dlgInfo;

HRESULT InitializeXPIStub()
{
  char szBuf[MAX_BUF];
  char szXPIStubFile[MAX_BUF];
  char szEDosQueryProcAddr[MAX_BUF];
  APIRET rc;

  hXPIStubInst = NULL;

  if(!GetPrivateProfileString("Messages", "ERROR_DOSQUERYPROCADDR", "", szEDosQueryProcAddr, sizeof(szEDosQueryProcAddr), szFileIniInstall))
    return(1);

  
  
  strcpy(szBuf, siCFXpcomFile.szDestination);
  AppendBackSlash(szBuf, sizeof(szBuf));
  strcat(szBuf, "bin");
  chdir(szBuf);

  
  DosSetExtLIBPATH("T", LIBPATHSTRICT);

  
  DosSetExtLIBPATH(szBuf, BEGIN_LIBPATH);

  
  strcpy(szXPIStubFile, szBuf);
  AppendBackSlash(szXPIStubFile, sizeof(szXPIStubFile));
  strcat(szXPIStubFile, "xpistub.dll");

  if(FileExists(szXPIStubFile) == FALSE)
    return(2);

  
  if (DosLoadModule(NULL, 0, szXPIStubFile, &hXPIStubInst) != NO_ERROR)
  {
    sprintf(szBuf, szEDllLoad, szXPIStubFile);
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  if(DosQueryProcAddr(hXPIStubInst, 0, "_XPI_Init", &pfnXpiInit) != NO_ERROR)
  {
    sprintf(szBuf, szEDosQueryProcAddr, "_XPI_Init");
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  if(DosQueryProcAddr(hXPIStubInst, 0, "_XPI_Install", &pfnXpiInstall) != NO_ERROR)
  {
    sprintf(szBuf, szEDosQueryProcAddr, "_XPI_Install");
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  if(DosQueryProcAddr(hXPIStubInst, 0, "_XPI_Exit", &pfnXpiExit) != NO_ERROR)
  {
    sprintf(szBuf, szEDosQueryProcAddr, "_XPI_Exit");
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }

  return(0);
}

HRESULT DeInitializeXPIStub()
{
  pfnXpiInit    = NULL;
  pfnXpiInstall = NULL;
  pfnXpiExit    = NULL;

  if(hXPIStubInst)
    DosFreeModule(hXPIStubInst);

  chdir(szSetupDir);
  return(0);
}

void GetTotalArchivesToInstall(void)
{
  DWORD     dwIndex0;
  siC       *siCObject = NULL;

  dwIndex0        = 0;
  dwTotalArchives = 0;
  siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  while(siCObject)
  {
    if((siCObject->dwAttributes & SIC_SELECTED) && !(siCObject->dwAttributes & SIC_LAUNCHAPP))
      ++dwTotalArchives;

    ++dwIndex0;
    siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  }
}

char *GetErrorString(DWORD dwError, char *szErrorString, DWORD dwErrorStringSize)
{
  int  i = 0;
  char szErrorNumber[MAX_BUF];

  memset(szErrorString, 0, dwErrorStringSize);
  _itoa(dwError, szErrorNumber, 10);

  
  while(TRUE)
  {
    if(*XpErrorList[i] == '\0')
      break;

    if(stricmp(szErrorNumber, XpErrorList[i]) == 0)
    {
      if(*XpErrorList[i + 1] != '\0')
        strcpy(szErrorString, XpErrorList[i + 1]);

      break;
    }

    ++i;
  }

  return(szErrorString);
}

HRESULT SmartUpdateJars()
{
  DWORD     dwIndex0;
  siC       *siCObject = NULL;
  HRESULT   hrResult;
  char      szBuf[MAX_BUF];
  char      szEXpiInstall[MAX_BUF];
  char      szArchive[MAX_BUF];
  char      szMsgSmartUpdateStart[MAX_BUF];
  char      szDlgExtractingTitle[MAX_BUF];

  if(!GetPrivateProfileString("Messages", "MSG_SMARTUPDATE_START", "", szMsgSmartUpdateStart, sizeof(szMsgSmartUpdateStart), szFileIniInstall))
    return(1);
  if(!GetPrivateProfileString("Messages", "DLG_EXTRACTING_TITLE", "", szDlgExtractingTitle, sizeof(szDlgExtractingTitle), szFileIniInstall))
    return(1);
  if(!GetPrivateProfileString("Messages", "STR_PROCESSINGFILE", "", szStrProcessingFile, sizeof(szStrProcessingFile), szFileIniInstall))
    exit(1);
  if(!GetPrivateProfileString("Messages", "STR_INSTALLING", "", szStrInstalling, sizeof(szStrInstalling), szFileIniInstall))
    exit(1);
  if(!GetPrivateProfileString("Messages", "STR_COPYINGFILE", "", szStrCopyingFile, sizeof(szStrCopyingFile), szFileIniInstall))
    exit(1);

  ShowMessage(szMsgSmartUpdateStart, TRUE);
  if(InitializeXPIStub() == WIZ_OK)
  {
    LogISXPInstall(W_START);
    strcpy(szBuf, sgProduct.szPath);
    if(*sgProduct.szSubPath != '\0')
    {
      AppendBackSlash(szBuf, sizeof(szBuf));
      strcat(szBuf, sgProduct.szSubPath);
    }
    hrResult = pfnXpiInit(szBuf, FILE_INSTALL_LOG, cbXPIProgress);

    
    DosSetExtLIBPATH("F", LIBPATHSTRICT);

    ShowMessage(szMsgSmartUpdateStart, FALSE);
    InitProgressDlg();
    GetTotalArchivesToInstall();
    WinSetWindowText(dlgInfo.hWndDlg, szDlgExtractingTitle);

    dwIndex0          = 0;
    dwCurrentArchive  = 0;
    siCObject         = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
    while(siCObject)
    {
      if(siCObject->dwAttributes & SIC_SELECTED)
        
         ProcessFileOps(T_PRE_ARCHIVE, siCObject->szReferenceName);

      
      if((siCObject->dwAttributes & SIC_SELECTED)   &&
        !(siCObject->dwAttributes & SIC_LAUNCHAPP) &&
        !(siCObject->dwAttributes & SIC_DOWNLOAD_ONLY))
      {
        strcpy(szArchive, sgProduct.szAlternateArchiveSearchPath);
        AppendBackSlash(szArchive, sizeof(szArchive));
        strcat(szArchive, siCObject->szArchiveName);
        if((*sgProduct.szAlternateArchiveSearchPath == '\0') || (!FileExists(szArchive)))
        {
          strcpy(szArchive, szSetupDir);
          AppendBackSlash(szArchive, sizeof(szArchive));
          strcat(szArchive, siCObject->szArchiveName);
          if(!FileExists(szArchive))
          {
            strcpy(szArchive, szTempDir);
            AppendBackSlash(szArchive, sizeof(szArchive));
            strcat(szArchive, siCObject->szArchiveName);
            if(!FileExists(szArchive))
            {
              char szEFileNotFound[MAX_BUF];

              if(GetPrivateProfileString("Messages", "ERROR_FILE_NOT_FOUND", "", szEFileNotFound, sizeof(szEFileNotFound), szFileIniInstall))
              {
                sprintf(szBuf, szEFileNotFound, szArchive);
                PrintError(szBuf, ERROR_CODE_HIDE);
              }
              return(1);
            }
          }
        }

        if(dwCurrentArchive == 0)
        {
          ++dwCurrentArchive;
          UpdateGaugeArchiveProgressBar((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));
        }

        sprintf(szBuf, szStrInstalling, siCObject->szDescriptionShort);
        WinSetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS0, szBuf);
        LogISXPInstallComponent(siCObject->szDescriptionShort);

        hrResult = pfnXpiInstall(szArchive, "", 0xFFFF);
        if(hrResult == E_REBOOT)
          bReboot = TRUE;
        else if((hrResult != WIZ_OK) &&
               !(siCObject->dwAttributes & SIC_IGNORE_XPINSTALL_ERROR))
        {
          LogMSXPInstallStatus(siCObject->szArchiveName, hrResult);
          LogISXPInstallComponentResult(hrResult);
          if(GetPrivateProfileString("Messages", "ERROR_XPI_INSTALL", "", szEXpiInstall, sizeof(szEXpiInstall), szFileIniInstall))
          {
            char szErrorString[MAX_BUF];

            GetErrorString(hrResult, szErrorString, sizeof(szErrorString));
            sprintf(szBuf, "%s - %s: %d %s", szEXpiInstall, siCObject->szDescriptionShort, hrResult, szErrorString);
            PrintError(szBuf, ERROR_CODE_HIDE);
          }

          
          break;
        }

        ++dwCurrentArchive;
        UpdateGaugeArchiveProgressBar((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));
        ProcessWindowsMessages();
        LogISXPInstallComponentResult(hrResult);

        if((hrResult != WIZ_OK) &&
          (siCObject->dwAttributes & SIC_IGNORE_XPINSTALL_ERROR))
          



          hrResult = WIZ_OK;
      }

      if(siCObject->dwAttributes & SIC_SELECTED)
        
         ProcessFileOps(T_POST_ARCHIVE, siCObject->szReferenceName);

      ++dwIndex0;
      siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
    } 

    LogMSXPInstallStatus(NULL, hrResult);
    pfnXpiExit();
    if(sgProduct.ulMode != SILENT)
      WinDestroyWindow(dlgInfo.hWndDlg);
  }
  else
  {
    ShowMessage(szMsgSmartUpdateStart, FALSE);
  }

  DeInitializeXPIStub();
  LogISXPInstall(W_END);

  return(hrResult);
}

void cbXPIStart(const char *URL, const char *UIName)
{
}

void cbXPIProgress(const char* msg, PRInt32 val, PRInt32 max)
{
  char szBuf[MAX_BUF];

  if(sgProduct.ulMode != SILENT)
  {
    TruncateString(WinWindowFromID(dlgInfo.hWndDlg, IDC_STATUS3), msg, szBuf, sizeof(szBuf));
    WinSetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS3, szBuf);
  }

  ProcessWindowsMessages();
}

void cbXPIFinal(const char *URL, PRInt32 finalStatus)
{
}








static void
CenterWindow(HWND hWndDlg)
{
  SWP swpDlg;

  WinQueryWindowPos(hWndDlg, &swpDlg);
  WinSetWindowPos(hWndDlg,
                  0,
                  (WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)/2)-(swpDlg.cx/2),
                  (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)/2)-(swpDlg.cy/2),
                  0,
                  0,
                  SWP_MOVE);
}


MRESULT APIENTRY
ProgressDlgProc(HWND hWndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hWndDlg);
      WinSetPresParam(hWndDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSendMsg(WinWindowFromID(hWndDlg, IDC_GAUGE_ARCHIVE), SLM_SETSLIDERINFO,
                               MPFROM2SHORT(SMA_SHAFTDIMENSIONS, 0),
                               (MPARAM)20);

      CenterWindow(hWndDlg);
      break;
   case WM_CLOSE:
   case WM_COMMAND:
      return (MRESULT)TRUE;
  }
  return WinDefDlgProc(hWndDlg, msg, mp1, mp2);
}



static void
UpdateGaugeArchiveProgressBar(unsigned value)
{
  if(sgProduct.ulMode != SILENT) {
    WinSendMsg(WinWindowFromID(dlgInfo.hWndDlg, IDC_GAUGE_ARCHIVE), SLM_SETSLIDERINFO,
                               MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
                               (MPARAM)(value-1));
  }
}

void InitProgressDlg()
{
  if(sgProduct.ulMode != SILENT)
  {
    dlgInfo.hWndDlg = WinLoadDlg(HWND_DESKTOP, hWndMain, ProgressDlgProc, hSetupRscInst, DLG_EXTRACTING, NULL);
    WinShowWindow(dlgInfo.hWndDlg, TRUE);
  }
}

