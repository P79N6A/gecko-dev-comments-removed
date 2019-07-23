




































#include <string.h>
#include <time.h>
#include <sys\stat.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include "xpnetHook.h"

#ifdef __cplusplus
}
#endif 

#include "nsFTPConn.h"
#include "nsHTTPConn.h"
#include "nsSocket.h"

#define UPDATE_INTERVAL_STATUS          1
#define UPDATE_INTERVAL_PROGRESS_BAR    1


#define CS_NONE         0x00000000
#define CS_CANCEL       0x00000001
#define CS_PAUSE        0x00000002
#define CS_RESUME       0x00000003

const int  kProxySrvrLen = 1024;
const char kHTTP[8]      = "http://";
const char kFTP[7]       = "ftp://";
const char kLoclFile[7]  = "zzzFTP";
const int  kModTimeOutValue = 3;

static nsHTTPConn       *connHTTP = NULL;
static nsFTPConn        *connFTP = NULL;
static long             glLastBytesSoFar;
static long             glAbsoluteBytesSoFar;
static long             glBytesResumedFrom;
static long             glTotalKb;
char                    gszStrCopyingFile[MAX_BUF_MEDIUM];
char                    gszCurrentDownloadPath[MAX_BUF];
char                    gszCurrentDownloadFilename[MAX_BUF_TINY];
char                    gszCurrentDownloadFileDescription[MAX_BUF_TINY];
char                    gszUrl[MAX_BUF];
char                    gszTo[MAX_BUF];
char                    gszFileInfo[MAX_BUF];
char                    *gszConfigIniFile;
BOOL                    gbDlgDownloadMinimized;
BOOL                    gbDlgDownloadJustMinimized;
BOOL                    gbUrlChanged;
BOOL                    gbShowDownloadRetryMsg;
DWORD                   gdwDownloadDialogStatus;
int                     giIndex;
int                     giTotalArchivesToDownload;
DWORD                   gdwTickStart;
BOOL                    gbStartTickCounter;

double GetPercentSoFar(void);

static void UpdateGaugeFileProgressBar(double value);
       int  ProgressCB(int aBytesSoFar, int aTotalFinalSize);
       void InitDownloadDlg(void);


siC *GetObjectFromArchiveName(char *szArchiveName);

struct DownloadFileInfo
{
  char szUrl[MAX_BUF];
  char szFile[MAX_BUF_TINY];
} dlFileInfo;

struct ExtractFilesDlgInfo
{
	HWND	hWndDlg;
	int		nMaxFileBars;	    
	int		nMaxArchiveBars;	
	int   nFileBars;	      
	int		nArchiveBars;		  
} dlgInfo;

struct TickInfo
{
  DWORD dwTickBegin;
  DWORD dwTickEnd;
  DWORD dwTickDif;
  BOOL  bTickStarted;
  BOOL  bTickDownloadResumed;
} gtiPaused;

BOOL CheckInterval(long *lModLastValue, int iInterval)
{
  BOOL bRv = FALSE;
  long lModCurrentValue;

  if(iInterval == 1)
  {
    lModCurrentValue = (long)time(NULL);
    if(lModCurrentValue != *lModLastValue)
      bRv = TRUE;
  }
  else
  {
    lModCurrentValue = (long)time(NULL) % iInterval;
    if((lModCurrentValue == 0) && (*lModLastValue != 0))
      bRv = TRUE;
  }

  *lModLastValue = lModCurrentValue;
  return(bRv);
}

char *GetTimeLeft(DWORD dwTimeLeft,
                  char *szTimeString,
                  DWORD dwTimeStringBufSize)
{
  DWORD      dwTimeLeftPP;
  ULONG hour, minute, second;

  dwTimeLeftPP         = dwTimeLeft + 1;
  hour         = (unsigned)(dwTimeLeftPP / 60 / 60);
  minute       = (unsigned)((dwTimeLeftPP / 60) % 60);
  second       = (unsigned)(dwTimeLeftPP % 60);

  memset(szTimeString, 0, dwTimeStringBufSize);
  sprintf(szTimeString, "%d:%d:%d", hour, minute, second);
  return(szTimeString);
}

DWORD AddToTick(DWORD dwTick, DWORD dwTickMoreToAdd)
{
  DWORD dwTickLeftTillWrap = 0;

  


  dwTickLeftTillWrap = 0xFFFFFFFF - dwTick;
  if(dwTickMoreToAdd > dwTickLeftTillWrap)
    dwTick = dwTickMoreToAdd - dwTickLeftTillWrap;
  else
    dwTick = dwTick + dwTickMoreToAdd;

  return(dwTick);
}

DWORD GetTickDif(DWORD dwTickEnd, DWORD dwTickStart)
{
  DWORD dwTickDif;

  




  if(dwTickEnd < dwTickStart)
    dwTickDif = 0xFFFFFFFF - dwTickStart + dwTickEnd;
  else
    dwTickDif = dwTickEnd - dwTickStart;

  return(dwTickDif);
}

void InitTickInfo(void)
{
  gtiPaused.dwTickBegin          = 0;
  gtiPaused.dwTickEnd            = 0;
  gtiPaused.dwTickDif            = 0;
  gtiPaused.bTickStarted         = FALSE;
  gtiPaused.bTickDownloadResumed = FALSE;
}

DWORD RoundDouble(double dValue)
{
  if(0.5 <= (dValue - (DWORD)dValue))
    return((DWORD)dValue + 1);
  else
    return((DWORD)dValue);
}

void SetStatusStatus(void)
{
  char        szStatusStatusLine[MAX_BUF_MEDIUM];
  char        szCurrentStatusInfo[MAX_BUF_MEDIUM];
  char        szPercentString[MAX_BUF_MEDIUM];
  char        szPercentageCompleted[MAX_BUF_MEDIUM];
  static long lModLastValue = 0;
  double        dRate;
  static double dRateCounter;
  DWORD         dwTickNow;
  DWORD         dwTickDif;
  DWORD         dwKBytesSoFar;
  DWORD         dwRoundedRate;
  char          szTimeLeft[MAX_BUF_TINY];

  



  if(gtiPaused.bTickDownloadResumed)
  {
    gdwTickStart = AddToTick(gdwTickStart, gtiPaused.dwTickDif);
    InitTickInfo();
  }

  


  dwTickNow = WinGetCurrentTime(NULLHANDLE);
  if((gdwTickStart == 0) && gbStartTickCounter)
    dwTickNow = gdwTickStart = WinGetCurrentTime(NULLHANDLE);

  dwTickDif = GetTickDif(dwTickNow, gdwTickStart);

  

  if(!CheckInterval(&lModLastValue, UPDATE_INTERVAL_STATUS))
    return;

  if(glAbsoluteBytesSoFar == 0)
    dRateCounter = 0.0;
  else
    dRateCounter = dwTickDif / 1000;

  if(dRateCounter == 0.0)
    dRate = 0.0;
  else
    dRate = (glAbsoluteBytesSoFar - glBytesResumedFrom) / dRateCounter / 1024;

  dwKBytesSoFar = glAbsoluteBytesSoFar / 1024;

  



  dwRoundedRate = RoundDouble(dRate);
  if(dwRoundedRate > 0)
    GetTimeLeft((glTotalKb - dwKBytesSoFar) / dwRoundedRate,
                 szTimeLeft,
                 sizeof(szTimeLeft));
  else
    strcpy(szTimeLeft, "00:00:00");

  if(!gbShowDownloadRetryMsg)
  {
    GetPrivateProfileString("Strings",
                            "Status Download",
                            "",
                            szStatusStatusLine,
                            sizeof(szStatusStatusLine),
                            szFileIniConfig);
    if(*szStatusStatusLine != '\0')
      sprintf(szCurrentStatusInfo,
              szStatusStatusLine,
              szTimeLeft,
              dRate,
              dwKBytesSoFar,
              glTotalKb);
    else
      sprintf(szCurrentStatusInfo,
              "%s at %.2fKB/sec (%uKB of %uKB downloaded)",
              szTimeLeft,
              dRate,
              dwKBytesSoFar,
              glTotalKb);
  }
  else
  {
    GetPrivateProfileString("Strings",
                            "Status Retry",
                            "",
                            szStatusStatusLine,
                            sizeof(szStatusStatusLine),
                            szFileIniConfig);
    if(*szStatusStatusLine != '\0')
      sprintf(szCurrentStatusInfo,
              szStatusStatusLine,
              szTimeLeft,
              dRate,
              dwKBytesSoFar,
              glTotalKb);
    else
      sprintf(szCurrentStatusInfo,
              "%s at %.2KB/sec (%uKB of %uKB downloaded)",
              szTimeLeft,
              dRate,
              dwKBytesSoFar,
              glTotalKb);
  }

  GetPrivateProfileString("Strings",
                          "Status Percentage Completed",
                          "",
                          szPercentageCompleted,
                          sizeof(szPercentageCompleted),
                          szFileIniConfig);
  sprintf(szPercentString, szPercentageCompleted, (int)GetPercentSoFar());

  
  WinSetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS_STATUS, szCurrentStatusInfo);
  WinSetDlgItemText(dlgInfo.hWndDlg, IDC_PERCENTAGE, szPercentString);
}

void SetStatusFile(void)
{
  char szString[MAX_BUF];

  
  sprintf(szString, gszFileInfo, gszCurrentDownloadFileDescription);
  WinSetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS_FILE, szString);
  SetStatusStatus();
}

void SetStatusUrl(void)
{
  
  if(gbUrlChanged)
  {
    char szUrlPathBuf[MAX_BUF];
    char szToPathBuf[MAX_BUF];
    HWND hStatusUrl = NULL;
    HWND hStatusTo  = NULL;

    hStatusUrl = WinWindowFromID(dlgInfo.hWndDlg, IDC_STATUS_URL);
    if(hStatusUrl)
      TruncateString(hStatusUrl, gszUrl, szUrlPathBuf, sizeof(szUrlPathBuf));
    else
      strcpy(szUrlPathBuf, gszUrl);

    hStatusTo = WinWindowFromID(dlgInfo.hWndDlg, IDC_STATUS_TO);
    if(hStatusTo)
      TruncateString(hStatusTo, gszTo, szToPathBuf, sizeof(szToPathBuf));
    else
      strcpy(szToPathBuf, gszTo);

    WinSetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS_URL, szUrlPathBuf);
    WinSetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS_TO,  szToPathBuf);
    SetStatusFile();
    gbUrlChanged = FALSE;
  }
}

double GetPercentSoFar(void)
{
  return((double)(((double)(glAbsoluteBytesSoFar / 1024) / (double)glTotalKb) * (double)100));
}

void SetMinimizedDownloadTitle(DWORD dwPercentSoFar)
{
  static DWORD dwLastPercentSoFar = 0;
  char szDownloadTitle[MAX_BUF_MEDIUM];
  char gszCurrentDownloadInfo[MAX_BUF_MEDIUM];

  GetPrivateProfileString("Strings", "Dialog Download Title Minimized", "", szDownloadTitle, sizeof(szDownloadTitle), szFileIniConfig);

  if(*szDownloadTitle != '\0')
    sprintf(gszCurrentDownloadInfo, szDownloadTitle, dwPercentSoFar, gszCurrentDownloadFilename);
  else
    sprintf(gszCurrentDownloadInfo, "%d%% for all files", dwPercentSoFar);

  
  if((dwLastPercentSoFar != dwPercentSoFar) || gbDlgDownloadJustMinimized)
  {
    




    gbDlgDownloadJustMinimized = FALSE;

    
    WinSetWindowText(dlgInfo.hWndDlg, gszCurrentDownloadInfo);
    dwLastPercentSoFar = dwPercentSoFar;
  }
}

void SetRestoredDownloadTitle(void)
{
  
  WinSetWindowText(dlgInfo.hWndDlg, diDownload.szTitle);
}

void GetTotalArchivesToDownload(int *iTotalArchivesToDownload, DWORD *dwTotalEstDownloadSize)
{
  int  iIndex = 0;
  char szUrl[MAX_BUF];
  char szSection[MAX_INI_SK];
  char szDownloadSize[MAX_ITOA];

  *dwTotalEstDownloadSize = 0;
  iIndex = 0;
  sprintf(szSection, "File%d", iIndex);
  GetPrivateProfileString(szSection,
                          "url0",
                          "",
                          szUrl,
                          sizeof(szUrl),
                          gszConfigIniFile);
  while(*szUrl != '\0')
  {
    GetPrivateProfileString(szSection, "size", "", szDownloadSize, sizeof(szDownloadSize), gszConfigIniFile);
    if((strlen(szDownloadSize) < 32) && (*szDownloadSize != '\0'))
      
      *dwTotalEstDownloadSize += atoi(szDownloadSize);

    ++iIndex;
    sprintf(szSection, "File%d", iIndex);
    GetPrivateProfileString(szSection,
                            "url0",
                            "",
                            szUrl,
                            sizeof(szUrl),
                            gszConfigIniFile);
  }
  *iTotalArchivesToDownload = iIndex;
}














int
ProcessWndMsgCB()
{
  int iRv = nsFTPConn::OK;

	ProcessWindowsMessages();
  if((gdwDownloadDialogStatus == CS_CANCEL) ||
     (gdwDownloadDialogStatus == CS_PAUSE))
    iRv = nsFTPConn::E_USER_CANCEL;

	return(iRv);
}


int WGet(char *szUrl,
         char *szFile,
         char *szProxyServer,
         char *szProxyPort,
         char *szProxyUser,
         char *szProxyPasswd)
{
  int        rv;
  char       proxyURL[MAX_BUF];
  nsHTTPConn *conn = NULL;

  if((szProxyServer != NULL) && (szProxyPort != NULL) &&
     (*szProxyServer != '\0') && (*szProxyPort != '\0'))
  {
    
    memset(proxyURL, 0, sizeof(proxyURL));
    sprintf(proxyURL, "http://%s:%s", szProxyServer, szProxyPort);

    conn = new nsHTTPConn(proxyURL);
    if(conn == NULL)
      return(WIZ_OUT_OF_MEMORY);

    if((szProxyUser != NULL) && (*szProxyUser != '\0') &&
       (szProxyPasswd != NULL) && (*szProxyPasswd != '\0'))
      
      conn->SetProxyInfo(szUrl, szProxyUser, szProxyPasswd);
    else
      conn->SetProxyInfo(szUrl, NULL, NULL);
  }
  else
  {
    
    conn = new nsHTTPConn(szUrl, ProcessWndMsgCB);
    if(conn == NULL)
      return(WIZ_OUT_OF_MEMORY);
  }
  
  rv = conn->Open();
  if(rv == WIZ_OK)
  {
    rv = conn->Get(NULL, szFile);
    conn->Close();
  }

  if(conn)
    delete(conn);

  return(rv);
}

int DownloadViaProxyOpen(char *szUrl, char *szProxyServer, char *szProxyPort, char *szProxyUser, char *szProxyPasswd)
{
  int  rv;
  char proxyURL[kProxySrvrLen];

  if((!szUrl) || (*szUrl == '\0'))
    return nsHTTPConn::E_PARAM;

  rv = nsHTTPConn::OK;
  memset(proxyURL, 0, kProxySrvrLen);
  sprintf(proxyURL, "http://%s:%s", szProxyServer, szProxyPort);

  connHTTP = new nsHTTPConn(proxyURL, ProcessWndMsgCB);
  if(connHTTP == NULL)
  {
    char szBuf[MAX_BUF_TINY];

    GetPrivateProfileString("Strings", "Error Out Of Memory", "", szBuf, sizeof(szBuf), szFileIniConfig);
    PrintError(szBuf, ERROR_CODE_HIDE);

    return(WIZ_OUT_OF_MEMORY);
  }

  if((szProxyUser != NULL) && (*szProxyUser != '\0') &&
     (szProxyPasswd != NULL) && (*szProxyPasswd != '\0'))
    connHTTP->SetProxyInfo(szUrl, szProxyUser, szProxyPasswd);
  else
    connHTTP->SetProxyInfo(szUrl, NULL, NULL);

  rv = connHTTP->Open();
  return(rv);
}

void DownloadViaProxyClose(void)
{
  gbStartTickCounter = FALSE;
  if(connHTTP)
  {
    connHTTP->Close();
    delete(connHTTP);
    connHTTP = NULL;
  }
}

int DownloadViaProxy(char *szUrl, char *szProxyServer, char *szProxyPort, char *szProxyUser, char *szProxyPasswd)
{
  int  rv;
  char *file = NULL;

  rv = nsHTTPConn::OK;
  if((!szUrl) || (*szUrl == '\0'))
    return nsHTTPConn::E_PARAM;

  if(connHTTP == NULL)
  {
    rv = DownloadViaProxyOpen(szUrl,
                              szProxyServer,
                              szProxyPort,
                              szProxyUser,
                              szProxyPasswd);

    if(rv != nsHTTPConn::OK)
    {
      DownloadViaProxyClose();
      return(rv);
    }
  }

  if(connHTTP == NULL)
  {
    char szBuf[MAX_BUF_TINY];

    GetPrivateProfileString("Strings", "Error Out Of Memory", "", szBuf, sizeof(szBuf), szFileIniConfig);
    PrintError(szBuf, ERROR_CODE_HIDE);

    return(WIZ_OUT_OF_MEMORY);
  }

  if(strrchr(szUrl, '/') != (szUrl + strlen(szUrl)))
    file = strrchr(szUrl, '/') + 1; 

  gbStartTickCounter = TRUE;
  rv = connHTTP->Get(ProgressCB, file); 
  DownloadViaProxyClose();
  return(rv);
}

int DownloadViaHTTPOpen(char *szUrl)
{
  int  rv;

  if((!szUrl) || (*szUrl == '\0'))
    return nsHTTPConn::E_PARAM;

  rv = nsHTTPConn::OK;
  connHTTP = new nsHTTPConn(szUrl, ProcessWndMsgCB);
  if(connHTTP == NULL)
  {
    char szBuf[MAX_BUF_TINY];

    GetPrivateProfileString("Strings", "Error Out Of Memory", "", szBuf, sizeof(szBuf), gszConfigIniFile);
    PrintError(szBuf, ERROR_CODE_HIDE);

    return(WIZ_OUT_OF_MEMORY);
  }
  
  rv = connHTTP->Open();
  return(rv);
}

void DownloadViaHTTPClose(void)
{
  gbStartTickCounter = FALSE;
  if(connHTTP)
  {
    connHTTP->Close();
    delete(connHTTP);
    connHTTP = NULL;
  }
}

int DownloadViaHTTP(char *szUrl)
{
  int  rv;
  char *file = NULL;

  if((!szUrl) || (*szUrl == '\0'))
    return nsHTTPConn::E_PARAM;

  if(connHTTP == NULL)
  {
    rv = DownloadViaHTTPOpen(szUrl);
    if(rv != nsHTTPConn::OK)
    {
      DownloadViaHTTPClose();
      return(rv);
    }
  }

  if(connHTTP == NULL)
  {
    char szBuf[MAX_BUF_TINY];

    GetPrivateProfileString("Strings", "Error Out Of Memory", "", szBuf, sizeof(szBuf), gszConfigIniFile);
    PrintError(szBuf, ERROR_CODE_HIDE);

    return(WIZ_OUT_OF_MEMORY);
  }
  
  rv = nsHTTPConn::OK;
  if(strrchr(szUrl, '/') != (szUrl + strlen(szUrl)))
    file = strrchr(szUrl, '/') + 1; 

  gbStartTickCounter = TRUE;
  rv = connHTTP->Get(ProgressCB, file);
  DownloadViaHTTPClose();
  return(rv);
}

int DownloadViaFTPOpen(char *szUrl)
{
  char *host = 0, *path = 0, *file = (char*) kLoclFile;
  int port = 21;
  int rv;

  if((!szUrl) || (*szUrl == '\0'))
    return nsFTPConn::E_PARAM;

  rv = nsHTTPConn::ParseURL(kFTP, szUrl, &host, &port, &path);

  connFTP = new nsFTPConn(host, ProcessWndMsgCB);
  if(connFTP == NULL)
  {
    char szBuf[MAX_BUF_TINY];

    GetPrivateProfileString("Strings", "Error Out Of Memory", "", szBuf, sizeof(szBuf), szFileIniConfig);
    PrintError(szBuf, ERROR_CODE_HIDE);

    return(WIZ_OUT_OF_MEMORY);
  }

  rv = connFTP->Open();
  if(host)
    free(host);
  if(path)
    free(path);

  return(rv);
}

void DownloadViaFTPClose(void)
{
  gbStartTickCounter = FALSE;
  if(connFTP)
  {
    connFTP->Close();
    delete(connFTP);
    connFTP = NULL;
  }
}

int DownloadViaFTP(char *szUrl)
{
  char *host = 0, *path = 0, *file = (char*) kLoclFile;
  int port = 21;
  int rv;

  if((!szUrl) || (*szUrl == '\0'))
    return nsFTPConn::E_PARAM;

  if(connFTP == NULL)
  {
    rv = DownloadViaFTPOpen(szUrl);
    if(rv != nsFTPConn::OK)
    {
      DownloadViaFTPClose();
      return(rv);
    }
  }

  if(connFTP == NULL)
  {
    char szBuf[MAX_BUF_TINY];

    GetPrivateProfileString("Strings", "Error Out Of Memory", "", szBuf, sizeof(szBuf), szFileIniConfig);
    PrintError(szBuf, ERROR_CODE_HIDE);

    return(WIZ_OUT_OF_MEMORY);
  }

  rv = nsHTTPConn::ParseURL(kFTP, szUrl, &host, &port, &path);

  if(strrchr(path, '/') != (path + strlen(path)))
    file = strrchr(path, '/') + 1; 

  gbStartTickCounter = TRUE;
  rv = connFTP->Get(path, file, nsFTPConn::BINARY, TRUE, ProgressCB);

  if(host)
    free(host);
  if(path)
    free(path);

  return(rv);
}

void PauseTheDownload(int rv, int *iFileDownloadRetries)
{
  if(rv != nsFTPConn::E_USER_CANCEL)
  {
    WinSendMsg(dlgInfo.hWndDlg, WM_COMMAND, (MPARAM)IDPAUSE, 0);
    --*iFileDownloadRetries;
  }

  while(gdwDownloadDialogStatus == CS_PAUSE)
  {
    DosSleep(0);
    ProcessWindowsMessages();
  }
}

void CloseSocket(char *szProxyServer, char *szProxyPort)
{
  
  if((szProxyServer != NULL) && (szProxyPort != NULL) &&
     (*szProxyServer != '\0') && (*szProxyPort != '\0'))
      DownloadViaProxyClose();
  else
  {
    
    if(strncmp(gszUrl, kHTTP, strlen(kHTTP)) == 0)
      DownloadViaHTTPClose();
    
    else if(strncmp(gszUrl, kFTP, strlen(kFTP)) == 0)
      DownloadViaFTPClose();
  }
}

siC *GetObjectFromArchiveName(char *szArchiveName)
{
  DWORD dwIndex;
  siC   *siCObject = NULL;
  siC   *siCNode   = NULL;

  dwIndex = 0;
  siCObject = SiCNodeGetObject(dwIndex, TRUE, AC_ALL);
  while(siCObject)
  {
    if(stricmp(szArchiveName, siCObject->szArchiveName) == 0)
    {
      siCNode = siCObject;
      break;
    }

    ++dwIndex;
    siCObject = SiCNodeGetObject(dwIndex, TRUE, AC_ALL);
  }

  return(siCNode);
}

int DownloadFiles(char *szInputIniFile,
                  char *szDownloadDir,
                  char *szProxyServer,
                  char *szProxyPort,
                  char *szProxyUser,
                  char *szProxyPasswd,
                  BOOL bShowRetryMsg,
                  BOOL bIgnoreAllNetworkErrors,
                  char *szFailedFile,
                  DWORD dwFailedFileSize)
{
  char      szBuf[MAX_BUF];
  char      szCurrentFile[MAX_BUF];
  char      szSection[MAX_INI_SK];
  char      szKey[MAX_INI_SK];
  char      szSavedCwd[MAX_BUF_MEDIUM];
  int       iCounter;
  int       rv;
  int       iFileDownloadRetries;
  int       iIgnoreFileNetworkError;
  int       iLocalTimeOutCounter;
  DWORD     dwTotalEstDownloadSize;
  char      szPartiallyDownloadedFilename[MAX_BUF];
  BOOL      bDownloadInitiated;
  char      szTempURL[MAX_BUF];
  char      szWorkingURLPathOnly[MAX_BUF];
  siC       *siCCurrentFileObj = NULL;
  ULONG     ulBuf, ulDiskNum, ulDriveMap;

  memset(szTempURL, 0, sizeof(szTempURL));
  memset(szWorkingURLPathOnly, 0, sizeof(szWorkingURLPathOnly));
  if(szInputIniFile == NULL)
    return(WIZ_ERROR_UNDEFINED);

  if(szFailedFile)
    memset(szFailedFile, 0, dwFailedFileSize);

  InitTickInfo();
  ulBuf = sizeof(szSavedCwd-3);
  DosQueryCurrentDir(0, &szSavedCwd[3], &ulBuf);
  
  DosQueryCurrentDisk(&ulDiskNum, &ulDriveMap);

  
  if (isupper(szSavedCwd[3]))
     szSavedCwd[0] = (char)('A' - 1 + ulDiskNum);
  else
     szSavedCwd[0] = (char)('a' - 1 + ulDiskNum);
  szSavedCwd[1] = ':';
  szSavedCwd[2] = '\\';

  if (toupper(szSavedCwd[0]) != toupper(szDownloadDir[0]))
     DosSetDefaultDisk(toupper(szDownloadDir[0]) - 'A' + 1);
  DosSetCurrentDir(szDownloadDir);

  rv                        = WIZ_OK;
  dwTotalEstDownloadSize    = 0;
  giTotalArchivesToDownload = 0;
  glLastBytesSoFar          = 0;
  glAbsoluteBytesSoFar      = 0;
  glBytesResumedFrom        = 0;
  gdwTickStart              = 0; 

  gbStartTickCounter        = FALSE; 


  gbUrlChanged              = TRUE;
  gbDlgDownloadMinimized    = FALSE;
  gbDlgDownloadJustMinimized = FALSE;
  gdwDownloadDialogStatus   = CS_NONE;
  gbShowDownloadRetryMsg    = bShowRetryMsg;
  gszConfigIniFile          = szInputIniFile;
  bDownloadInitiated        = FALSE;

  GetTotalArchivesToDownload(&giTotalArchivesToDownload,
                             &dwTotalEstDownloadSize);
  glTotalKb                 = dwTotalEstDownloadSize;
  GetSetupCurrentDownloadFile(szPartiallyDownloadedFilename,
                              sizeof(szPartiallyDownloadedFilename));

  InitDownloadDlg();

  for(giIndex = 0; giIndex < giTotalArchivesToDownload; giIndex++)
  {
    

    iCounter     = 0;
    gbUrlChanged = TRUE; 
    sprintf(szSection, "File%d", giIndex);
    sprintf(szKey,     "url%d",  iCounter);
    GetPrivateProfileString(szSection,
                            szKey,
                            "",
                            szTempURL,
                            sizeof(szTempURL),
                            gszConfigIniFile);

    if(*szTempURL == '\0')
      continue;

    if(!bDownloadInitiated)
    {
      ParsePath(szTempURL,
                szWorkingURLPathOnly,
                sizeof(szWorkingURLPathOnly),
                TRUE, 
                PP_PATH_ONLY);
    }

    GetPrivateProfileString(szSection,
                            "desc",
                            "",
                            gszCurrentDownloadFileDescription,
                            sizeof(gszCurrentDownloadFileDescription),
                            gszConfigIniFile);
#ifdef OLDCODE
    iIgnoreFileNetworkError = GetPrivateProfileInt(szSection,
                            "Ignore File Network Error",
                            0,
                            gszConfigIniFile);
#endif
    iIgnoreFileNetworkError = 0;

    
    ParsePath(szTempURL,
              szCurrentFile,
              sizeof(szCurrentFile),
              TRUE, 
              PP_FILENAME_ONLY);

    RemoveSlash(szWorkingURLPathOnly);
    sprintf(gszUrl, "%s/%s", szWorkingURLPathOnly, szCurrentFile);

    
    siCCurrentFileObj = GetObjectFromArchiveName(szCurrentFile);

    if((*szPartiallyDownloadedFilename != 0) &&
       (stricmp(szPartiallyDownloadedFilename, szCurrentFile) == 0))
    {
      struct stat statBuf;

      if(stat(szPartiallyDownloadedFilename, &statBuf) != -1)
      {
        glAbsoluteBytesSoFar += statBuf.st_size;
        glBytesResumedFrom    = statBuf.st_size;
      }
    }

    strcpy(gszTo, szDownloadDir);
    AppendBackSlash(gszTo, sizeof(gszTo));
    strcat(gszTo, szCurrentFile);

    if(gbDlgDownloadMinimized)
      SetMinimizedDownloadTitle((int)GetPercentSoFar());
    else
    {
      SetStatusUrl();
      SetRestoredDownloadTitle();
    }

    SetSetupCurrentDownloadFile(szCurrentFile);
    iFileDownloadRetries = 0;
    iLocalTimeOutCounter = 0;
    do
    {
      ProcessWindowsMessages();
      
      if((szProxyServer != NULL) && (szProxyPort != NULL) &&
         (*szProxyServer != '\0') && (*szProxyPort != '\0'))
        
        rv = DownloadViaProxy(gszUrl,
                              szProxyServer,
                              szProxyPort,
                              szProxyUser,
                              szProxyPasswd);
      else
      {
        
        if(strncmp(gszUrl, kHTTP, strlen(kHTTP)) == 0)
          rv = DownloadViaHTTP(gszUrl);
        
        else if(strncmp(gszUrl, kFTP, strlen(kFTP)) == 0)
          rv = DownloadViaFTP(gszUrl);
      }

      bDownloadInitiated = TRUE;
      if((rv == nsFTPConn::E_USER_CANCEL) ||
         (gdwDownloadDialogStatus == CS_PAUSE))
      {
        if(gdwDownloadDialogStatus == CS_PAUSE)
        {
          CloseSocket(szProxyServer, szProxyPort);

          

          rv = nsFTPConn::E_CMD_UNEXPECTED;

          PauseTheDownload(rv, &iFileDownloadRetries);
          bDownloadInitiated = FALSE; 

        }
        else
        {
          
          break;
        }
      }
      else if((rv != nsFTPConn::OK) &&
              (rv != nsFTPConn::E_CMD_FAIL) &&
              (rv != nsSocket::E_BIND) &&
              (rv != nsHTTPConn::E_HTTP_RESPONSE) &&
              (gdwDownloadDialogStatus != CS_CANCEL))
      {
        


        char szTitle[MAX_BUF_SMALL];
        char szMsgDownloadPaused[MAX_BUF];

        
        if(rv == nsSocket::E_TIMEOUT)
        {
          ++siCCurrentFileObj->iNetTimeOuts;
          ++iLocalTimeOutCounter;
        }

        CloseSocket(szProxyServer, szProxyPort);

        



        if((rv != nsSocket::E_TIMEOUT) ||
           (rv == nsSocket::E_TIMEOUT) && ((iLocalTimeOutCounter % kModTimeOutValue) == 0))
        {
          

          if(!gtiPaused.bTickStarted)
          {
            gtiPaused.dwTickBegin          = WinGetCurrentTime(NULLHANDLE);
            gtiPaused.bTickStarted         = TRUE;
            gtiPaused.bTickDownloadResumed = FALSE;
          }

          


          GetPrivateProfileString("Messages",
                                  "MB_WARNING_STR",
                                  "",
                                  szTitle,
                                  sizeof(szTitle),
                                  szFileIniInstall);
          GetPrivateProfileString("Strings",
                                  "Message Download Paused",
                                  "",
                                  szMsgDownloadPaused,
                                  sizeof(szMsgDownloadPaused),
                                  szFileIniConfig);
          WinMessageBox(HWND_DESKTOP, dlgInfo.hWndDlg,
                        szMsgDownloadPaused,
                        szTitle, 0,
                        MB_ICONEXCLAMATION);

          
          gdwDownloadDialogStatus = CS_PAUSE;
          PauseTheDownload(rv, &iFileDownloadRetries);
        }
        else
          
          gdwDownloadDialogStatus = CS_NONE;
      }

      

      if(rv != nsSocket::E_TIMEOUT)
        ++iFileDownloadRetries;

      if((iFileDownloadRetries > MAX_FILE_DOWNLOAD_RETRIES) &&
         (rv != nsFTPConn::E_USER_CANCEL) &&
         (gdwDownloadDialogStatus != CS_CANCEL))
      {
        

        ++iCounter;
        sprintf(szKey, "url%d",  iCounter);
        GetPrivateProfileString(szSection,
                                szKey,
                                "",
                                szTempURL,
                                sizeof(szTempURL),
                                gszConfigIniFile);
        if(*szTempURL != '\0')
        {
          


          gbUrlChanged = TRUE;
          iFileDownloadRetries = 0;
          bDownloadInitiated = FALSE; 
          CloseSocket(szProxyServer, szProxyPort);
          ParsePath(szTempURL,
                    szWorkingURLPathOnly,
                    sizeof(szWorkingURLPathOnly),
                    TRUE, 
                    PP_PATH_ONLY);
          RemoveSlash(szWorkingURLPathOnly);
          sprintf(gszUrl, "%s/%s", szWorkingURLPathOnly, szCurrentFile);
          SetStatusUrl();
        }
      }
    } while((rv != nsFTPConn::E_USER_CANCEL) &&
            (rv != nsFTPConn::OK) &&
            (gdwDownloadDialogStatus != CS_CANCEL) &&
            (iFileDownloadRetries <= MAX_FILE_DOWNLOAD_RETRIES));

    
    siCCurrentFileObj->iNetRetries = iFileDownloadRetries < 1 ? 0:iFileDownloadRetries - 1;

    if((rv == nsFTPConn::E_USER_CANCEL) ||
       (gdwDownloadDialogStatus == CS_CANCEL))
    {
      

      rv = nsFTPConn::E_USER_CANCEL;

      if(szFailedFile && ((DWORD)strlen(szCurrentFile) <= dwFailedFileSize))
        strcpy(szFailedFile, gszCurrentDownloadFileDescription);

      
      break;
    }

    if((rv != nsFTPConn::OK) &&
       (iFileDownloadRetries > MAX_FILE_DOWNLOAD_RETRIES) &&
       !bIgnoreAllNetworkErrors &&
       !iIgnoreFileNetworkError)
    {
      
      char szMsg[MAX_BUF];

      if(szFailedFile && ((DWORD)strlen(szCurrentFile) <= dwFailedFileSize))
        strcpy(szFailedFile, gszCurrentDownloadFileDescription);

      GetPrivateProfileString("Strings",
                              "Error Too Many Network Errors",
                              "",
                              szMsg,
                              sizeof(szMsg),
                              szFileIniConfig);
      if(*szMsg != '\0')
      {
        sprintf(szBuf, szMsg, szCurrentFile);
        PrintError(szBuf, ERROR_CODE_HIDE);
      }

      


      rv = WIZ_TOO_MANY_NETWORK_ERRORS;
      break;
    }
    else if(bIgnoreAllNetworkErrors || iIgnoreFileNetworkError)
      rv = nsFTPConn::OK;

    UnsetSetupCurrentDownloadFile();
  }

  CloseSocket(szProxyServer, szProxyPort);
  if(sgProduct.ulMode != SILENT) 
    WinDestroyWindow(dlgInfo.hWndDlg);
  if (toupper(szSavedCwd[0]) != toupper(szDownloadDir[0]))
     DosSetDefaultDisk(toupper(szSavedCwd[0]) -1 + 'A');
  DosSetCurrentDir(szSavedCwd);

  return(rv);
}

int ProgressCB(int aBytesSoFar, int aTotalFinalSize)
{
  long   lBytesDiffSoFar;
  double dPercentSoFar;
  int    iRv = nsFTPConn::OK;

  if(sgProduct.ulMode != SILENT)
  {
    SetStatusUrl();

    if(glTotalKb == 0)
      glTotalKb = aTotalFinalSize;

    


    lBytesDiffSoFar = ((aBytesSoFar - glLastBytesSoFar) < 1) ? aBytesSoFar : (aBytesSoFar - glLastBytesSoFar);

    
    glLastBytesSoFar = aBytesSoFar;
    glAbsoluteBytesSoFar += lBytesDiffSoFar;

    dPercentSoFar = GetPercentSoFar();
    if(gbDlgDownloadMinimized)
      SetMinimizedDownloadTitle((int)dPercentSoFar);

    UpdateGaugeFileProgressBar(dPercentSoFar);
    SetStatusStatus();

    if((gdwDownloadDialogStatus == CS_CANCEL) ||
       (gdwDownloadDialogStatus == CS_PAUSE))
      iRv = nsFTPConn::E_USER_CANCEL;
  }

  ProcessWindowsMessages();
  return(iRv);
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


MRESULT EXPENTRY
DownloadDlgProc(HWND hWndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  PSWP pswp;

  switch (msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hWndDlg);
      WinSetPresParam(hWndDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSendMsg(WinWindowFromID(hWndDlg, IDC_GAUGE_FILE), SLM_SETSLIDERINFO,
                                 MPFROM2SHORT(SMA_SHAFTDIMENSIONS, 0),
                                 (MPARAM)20);
      GetPrivateProfileString("Strings",
                              "Status File Info",
                              "",
                              gszFileInfo,
                              sizeof(gszFileInfo),
                              szFileIniConfig);

      CenterWindow(hWndDlg);
      if(gbShowDownloadRetryMsg)
        WinSetDlgItemText(hWndDlg, IDC_MESSAGE0, diDownload.szMessageRetry0);
      else
        WinSetDlgItemText(hWndDlg, IDC_MESSAGE0, diDownload.szMessageDownload0);

      WinEnableWindow(WinWindowFromID(hWndDlg, IDRESUME), FALSE);
      WinSetDlgItemText(hWndDlg, IDC_STATIC1, sgInstallGui.szFile);
      WinSetDlgItemText(hWndDlg, IDC_STATIC2, sgInstallGui.szUrl);
      WinSetDlgItemText(hWndDlg, IDC_STATIC3, sgInstallGui.szTo);
      WinSetDlgItemText(hWndDlg, IDC_STATIC4, sgInstallGui.szStatus);
      WinSetDlgItemText(hWndDlg, IDCANCEL, sgInstallGui.szCancel_);
      WinSetDlgItemText(hWndDlg, IDPAUSE, sgInstallGui.szPause_);
      WinSetDlgItemText(hWndDlg, IDRESUME, sgInstallGui.szResume_);
      break;

    case WM_ADJUSTWINDOWPOS:
      pswp = (PSWP)mp1;
      if (pswp->fl & SWP_MINIMIZE) {
        SetMinimizedDownloadTitle((int)GetPercentSoFar());
        gbDlgDownloadMinimized = TRUE;
        gbDlgDownloadJustMinimized = TRUE;
      }
      if (pswp->fl & SWP_RESTORE) {
        SetStatusUrl();
        SetRestoredDownloadTitle();
        gbDlgDownloadMinimized = FALSE;
      }
      break;

   case WM_CLOSE:
      if(AskCancelDlg(hWndDlg))
        gdwDownloadDialogStatus = CS_CANCEL;
      return (MRESULT)TRUE;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDCANCEL:
          if(AskCancelDlg(hWndDlg))
            gdwDownloadDialogStatus = CS_CANCEL;
          break;

        case IDPAUSE:
          if(!gtiPaused.bTickStarted)
          {
            gtiPaused.dwTickBegin          = WinGetCurrentTime(NULLHANDLE);
            gtiPaused.bTickStarted         = TRUE;
            gtiPaused.bTickDownloadResumed = FALSE;
          }

          WinEnableWindow(WinWindowFromID(hWndDlg, IDPAUSE),  FALSE);
          WinEnableWindow(WinWindowFromID(hWndDlg, IDRESUME), TRUE);
          gdwDownloadDialogStatus = CS_PAUSE;
          break;

        case IDRESUME:
          gtiPaused.dwTickEnd = WinGetCurrentTime(NULLHANDLE);
          gtiPaused.dwTickDif = GetTickDif(gtiPaused.dwTickEnd,
                                           gtiPaused.dwTickBegin);
          gtiPaused.bTickDownloadResumed = TRUE;

          WinEnableWindow(WinWindowFromID(hWndDlg, IDRESUME), FALSE);
          WinEnableWindow(WinWindowFromID(hWndDlg, IDPAUSE),  TRUE);
          gdwDownloadDialogStatus = CS_NONE;
          break;

        default:
          break;
      }
      return (MRESULT)TRUE;
  }

  return WinDefDlgProc(hWndDlg, msg, mp1, mp2);
}



static void
UpdateGaugeFileProgressBar(double value)
{
  if(sgProduct.ulMode != SILENT)
  {
    ULONG ulPercentage = 100*value/100;
    WinSendMsg(WinWindowFromID(dlgInfo.hWndDlg, IDC_GAUGE_FILE), SLM_SETSLIDERINFO,
                               MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
                               (MPARAM)(ulPercentage-1));
  }
}

void InitDownloadDlg(void)
{
  if(sgProduct.ulMode != SILENT)
  {
    dlgInfo.hWndDlg = WinLoadDlg(HWND_DESKTOP, hWndMain, DownloadDlgProc, hSetupRscInst, DLG_DOWNLOADING, NULL);
    WinShowWindow(dlgInfo.hWndDlg, TRUE);
  }
}

