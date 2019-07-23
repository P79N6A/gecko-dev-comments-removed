






































#ifdef MOZ_OS2_HIGH_MEMORY

#include <os2safe.h>
#endif

#include "extern.h"
#include "extra.h"
#include "parser.h"
#include "dialogs.h"
#include "ifuncns.h"

#define INDEX_STR_LEN       10
#define PN_PROCESS          TEXT("Process")
#define PN_THREAD           TEXT("Thread")

BOOL InitApplication(HMODULE hInstance)
{
  CLASSINFO classinfo;

  ulScreenX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
  ulScreenY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
  ulDlgFrameX = WinQuerySysValue(HWND_DESKTOP, SV_CXDLGFRAME);
  ulDlgFrameY = WinQuerySysValue(HWND_DESKTOP, SV_CYDLGFRAME);
  ulTitleBarY = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);

  WinQueryClassInfo((HAB)0, WC_FRAME, &classinfo);
  return (WinRegisterClass((HAB)0, szClassName, WinDefDlgProc,
                           CS_SAVEBITS, classinfo.cbWindowData));
}

void PrintError(PSZ szMsg, ULONG ulErrorCodeSH)
{
  ERRORID erridErrorCode;
  char  szErrorString[MAX_BUF];

  if(ulErrorCodeSH == ERROR_CODE_SHOW)
  {
    erridErrorCode = WinGetLastError((HAB)0);
    sprintf(szErrorString, "%d : %s", erridErrorCode, szMsg);
  }
  else
    sprintf(szErrorString, "%s", szMsg);

  if((ugUninstall.ulMode != SILENT) && (ugUninstall.ulMode != AUTO))
  {
    WinMessageBox(HWND_DESKTOP, hWndMain, szErrorString, NULL, 0, MB_ICONEXCLAMATION);
  }
  else if(ugUninstall.ulMode == AUTO)
  {
    ShowMessage(szErrorString, TRUE);
    DosSleep(5000);
    ShowMessage(szErrorString, FALSE);
  }
}

void *NS_GlobalAlloc(ULONG ulMaxBuf)
{
  PSZ szBuf = NULL;

  if((szBuf = calloc(1, ulMaxBuf)) == NULL)
  {     
    if((szEGlobalAlloc == NULL) || (*szEGlobalAlloc == '\0'))
      PrintError("Memory allocation error.", ERROR_CODE_HIDE);
    else
      PrintError(szEGlobalAlloc, ERROR_CODE_SHOW);

    return(NULL);
  }
  else
    return(szBuf);
}

void FreeMemory(void **vPointer)
{
  if(*vPointer != NULL)
    free(*vPointer);
  *vPointer = NULL;
}

HRESULT NS_LoadStringAlloc(HMODULE hInstance, ULONG ulID, PSZ *szStringBuf, ULONG ulStringBuf)
{
  char szBuf[MAX_BUF];

  if((*szStringBuf = NS_GlobalAlloc(MAX_BUF)) == NULL)
    exit(1);
  
  if(!WinLoadString((HAB)0, hInstance, ulID, ulStringBuf, *szStringBuf))
  {
    if((szEStringLoad == NULL) ||(*szEStringLoad == '\0'))
      sprintf(szBuf, "Could not load string resource ID %d", ulID);
    else
      sprintf(szBuf, szEStringLoad, ulID);

    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  return(0);
}

HRESULT NS_LoadString(HMODULE hInstance, ULONG ulID, PSZ szStringBuf, ULONG ulStringBuf)
{
  char szBuf[MAX_BUF];

  if(!WinLoadString((HAB)0, hInstance, ulID, ulStringBuf, szStringBuf))
  {
    if((szEStringLoad == NULL) ||(*szEStringLoad == '\0'))
      sprintf(szBuf, "Could not load string resource ID %d", ulID);
    else
      sprintf(szBuf, szEStringLoad, ulID);

    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  return(WIZ_OK);
}

HRESULT Initialize(HMODULE hInstance, PSZ szAppName)
{
  char szBuf[MAX_BUF];
  HWND hwndFW;

  char *tempEnvVar = NULL;

  hDlgMessage = NULL;
  gulWhatToDo = WTD_ASK;


  
  if(NS_LoadStringAlloc(0, IDS_ERROR_GLOBALALLOC, &szEGlobalAlloc, MAX_BUF))
    return(1);
  if(NS_LoadStringAlloc(0, IDS_ERROR_STRING_LOAD, &szEStringLoad,  MAX_BUF))
    return(1);
  if(NS_LoadStringAlloc(0, IDS_ERROR_DLL_LOAD,    &szEDllLoad,     MAX_BUF))
    return(1);
  if(NS_LoadStringAlloc(0, IDS_ERROR_STRING_NULL, &szEStringNull,  MAX_BUF))
    return(1);

  memset(szBuf, 0, sizeof(MAX_BUF));
  if((szClassName = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  strcpy(szClassName, CLASS_NAME);

#ifdef OLDCODE
  


  if((hwndFW = FindWindow(szClassName, szClassName)) != NULL)
  {
    ShowWindow(hwndFW, SW_RESTORE);
    SetForegroundWindow(hwndFW);
    return(1);
  }
#endif

  if((szUninstallDir = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  ParsePath(szAppName, szUninstallDir, MAX_BUF, PP_PATH_ONLY);

  if((szTempDir = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  if((szOSTempDir = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  if((szFileIniUninstall = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  strcpy(szFileIniUninstall, szUninstallDir);
  AppendBackSlash(szFileIniUninstall, MAX_BUF);
  strcat(szFileIniUninstall, FILE_INI_UNINSTALL);

  if((szFileIniDefaultsInfo = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  strcpy(szFileIniDefaultsInfo, szUninstallDir);
  AppendBackSlash(szFileIniDefaultsInfo, MAX_BUF);
  GetPrivateProfileString("General", "Defaults Info Filename", "", szBuf, MAX_BUF, szFileIniUninstall);
  strcat(szFileIniDefaultsInfo, szBuf);

  
  tempEnvVar = getenv("TMP");
  if ((tempEnvVar) && (!(isFAT(tempEnvVar)))) {
    strcpy(szTempDir, tempEnvVar);
  }
  else
  {
    tempEnvVar = getenv("TEMP");
    if (tempEnvVar)
      strcpy(szTempDir, tempEnvVar);
  }
  if ((!tempEnvVar) || (isFAT(tempEnvVar)))
  {
    ULONG ulBootDrive = 0;
    APIRET rc;
    char  buffer[] = " :\\OS2\\";
    DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                    &ulBootDrive, sizeof(ulBootDrive));
    buffer[0] = 'A' - 1 + ulBootDrive;
    if (isFAT(buffer)) {
       
       ULONG ulDriveNum;
       ULONG ulDriveMap;
       strcpy(buffer, " :\\");
       DosQueryCurrentDisk(&ulDriveNum, &ulDriveMap);
       buffer[0] = 'A' - 1 + ulDriveNum;
       if (isFAT(buffer)) {
         int i;
         for (i = 2; i < 26; i++) {
           if ((ulDriveMap<<(31-i)) >> 31) {
             buffer[0] = 'A' + i;
             if (!(isFAT(buffer))) {
                break;
             }
           }
         }
         if (i == 26) {
            char szBuf[MAX_BUF];
            WinLoadString(0, NULLHANDLE, IDS_ERROR_NO_LONG_FILENAMES, sizeof(szBuf), szBuf);
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, szBuf, NULL, 0, MB_ICONEXCLAMATION);
            return(1);
         }
       }
    }
    strcpy(szTempDir, buffer);
    strcat(szTempDir, "TEMP");
  }
  strcpy(szOSTempDir, szTempDir);
  AppendBackSlash(szTempDir, MAX_BUF);
  strcat(szTempDir, WIZ_TEMP_DIR);

  if(!FileExists(szTempDir))
  {
    AppendBackSlash(szTempDir, MAX_BUF);
    CreateDirectoriesAll(szTempDir);
    RemoveBackSlash(szTempDir);
    if(!FileExists(szTempDir))
    {
      char szECreateTempDir[MAX_BUF];

      if(GetPrivateProfileString("Messages", "ERROR_CREATE_TEMP_DIR", "",
                                 szECreateTempDir, sizeof(szECreateTempDir), 
                                 szFileIniUninstall))
      {
        sprintf(szBuf, szECreateTempDir, szTempDir);
        PrintError(szBuf, ERROR_CODE_HIDE);
      }
      return(1);
    }
  }

  ugUninstall.bVerbose = FALSE;
  ugUninstall.bUninstallFiles = TRUE;

  return(0);
}


void RemoveQuotes(PSZ szSrc, PSZ szDest, int iDestSize)
{
  char *szBegin;

  if(strlen(szSrc) > iDestSize)
    return;

  if(*szSrc == '\"')
    szBegin = &szSrc[1];
  else
    szBegin = szSrc;

  strcpy(szDest, szBegin);

  if(szDest[strlen(szDest) - 1] == '\"')
    szDest[strlen(szDest) - 1] = '\0';
}



PSZ GetFirstNonSpace(PSZ szString)
{
  int   i;
  int   iStrLength;

  iStrLength = strlen(szString);

  for(i = 0; i < iStrLength; i++)
  {
    if(!isspace(szString[i]))
      return(&szString[i]);
  }

  return(NULL);
}

void SetUninstallRunMode(PSZ szMode)
{
  if(stricmp(szMode, "NORMAL") == 0)
    ugUninstall.ulMode = NORMAL;
  if(stricmp(szMode, "AUTO") == 0)
    ugUninstall.ulMode = AUTO;
  if(stricmp(szMode, "SILENT") == 0)
    ugUninstall.ulMode = SILENT;
}

void RemoveBackSlash(PSZ szInput)
{
  char  *ptrChar = NULL;

  if(!szInput)
    return;

  ptrChar = WinPrevChar(0, 0, 0, szInput, szInput + strlen(szInput));
  if (*ptrChar == '\\') {
    *ptrChar = '\0';
  }
}

void AppendBackSlash(PSZ szInput, ULONG ulInputSize)
{
  if(szInput != NULL)
  {
    if(*szInput == '\0')
    {
      if(((ULONG)strlen(szInput) + 1) < ulInputSize)
      {
        strcat(szInput, "\\");
      }
    }
    else if(szInput[strlen(szInput) - 1] != '\\')
    {
      if(((ULONG)strlen(szInput) + 1) < ulInputSize)
      {
        strcat(szInput, "\\");
      }
    }
  }
}

void RemoveSlash(PSZ szInput)
{
  int   iCounter;
  ULONG ulInputLen;

  if(szInput != NULL)
  {
    ulInputLen = strlen(szInput);

    for(iCounter = ulInputLen -1; iCounter >= 0 ; iCounter--)
    {
      if(szInput[iCounter] == '/')
        szInput[iCounter] = '\0';
      else
        break;
    }
  }
}

void AppendSlash(PSZ szInput, ULONG ulInputSize)
{
  if(szInput != NULL)
  {
    if(*szInput == '\0')
    {
      if(((ULONG)strlen(szInput) + 1) < ulInputSize)
      {
        strcat(szInput, "/");
      }
    }
    else if(szInput[strlen(szInput) - 1] != '/')
    {
      if(((ULONG)strlen(szInput) + 1) < ulInputSize)
      {
        strcat(szInput, "/");
      }
    }
  }
}

void ParsePath(PSZ szInput, PSZ szOutput, ULONG ulOutputSize, ULONG ulType)
{
  int   iCounter;
  ULONG ulCounter;
  ULONG ulInputLen;
  BOOL  bFound;

  if((szInput != NULL) && (szOutput != NULL))
  {
    bFound        = TRUE;
    ulInputLen    = strlen(szInput);
    memset(szOutput, 0, ulOutputSize);

    if(ulInputLen < ulOutputSize)
    {
      switch(ulType)
      {
        case PP_FILENAME_ONLY:
          for(iCounter = ulInputLen - 1; iCounter >= 0; iCounter--)
          {
            if(szInput[iCounter] == '\\')
            {
              strcpy(szOutput, &szInput[iCounter + 1]);
              bFound = TRUE;
              break;
            }
          }
          if(bFound == FALSE)
            strcpy(szOutput, szInput);

          break;

        case PP_PATH_ONLY:
          for(iCounter = ulInputLen - 1; iCounter >= 0; iCounter--)
          {
            if(szInput[iCounter] == '\\')
            {
              strcpy(szOutput, szInput);
              szOutput[iCounter + 1] = '\0';
              bFound = TRUE;
              break;
            }
          }
          if(bFound == FALSE)
            strcpy(szOutput, szInput);

          break;

        case PP_ROOT_ONLY:
          if(szInput[1] == ':')
          {
            szOutput[0] = szInput[0];
            szOutput[1] = szInput[1];
            AppendBackSlash(szOutput, ulOutputSize);
          }
          else if(szInput[1] == '\\')
          {
            int iFoundBackSlash = 0;
            for(ulCounter = 0; ulCounter < ulInputLen; ulCounter++)
            {
              if(szInput[ulCounter] == '\\')
              {
                szOutput[ulCounter] = szInput[ulCounter];
                ++iFoundBackSlash;
              }

              if(iFoundBackSlash == 3)
                break;
            }

            if(iFoundBackSlash != 0)
              AppendBackSlash(szOutput, ulOutputSize);
          }
          break;
      }
    }
  }
}

HRESULT WinSpawn(PSZ szClientName, PSZ szParameters, PSZ szCurrentDir, BOOL bWait)
{
  STARTDATA startdata;
  PID       pid, endpid;
  ULONG     ulSessID;
  APIRET rc;
  RESULTCODES resultcodes;
  ULONG     ulFlags;
  
  rc = DosQueryAppType(szClientName, &ulFlags);
  if (rc == NO_ERROR) {
    memset(&startdata, 0, sizeof(STARTDATA));
    startdata.Length  = sizeof(STARTDATA);
    startdata.PgmName = szClientName;
    startdata.PgmInputs = szParameters;
    rc = DosStartSession(&startdata, &ulSessID, &pid);
    if (rc == NO_ERROR) {
      if (bWait) {
        DosWaitChild(DCWA_PROCESS, DCWW_NOWAIT, &resultcodes, &endpid, pid);
      }
      return (TRUE);
    }
  } else {
    CHAR szBuf[CCHMAXPATH];
    HOBJECT hobject;
    strcpy(szBuf, szCurrentDir);
    strcat(szBuf, szClientName);
    hobject = WinQueryObject(szBuf);
    WinSetFocus(HWND_DESKTOP, HWND_DESKTOP);
    WinOpenObject(hobject, 0, TRUE); 
  }

  return(FALSE);
}

HRESULT InitDlgUninstall(diU *diDialog)
{
  diDialog->bShowDialog = FALSE;
  if((diDialog->szTitle = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((diDialog->szMessage0 = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  return(0);
}

void DeInitDlgUninstall(diU *diDialog)
{
  FreeMemory(&(diDialog->szTitle));
  FreeMemory(&(diDialog->szMessage0));
}

HRESULT InitUninstallGeneral()
{
  ugUninstall.ulMode = NORMAL;

  if((ugUninstall.szAppPath                 = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szLogPath                 = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szLogFilename             = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szCompanyName             = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szProductName             = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szOIKey                   = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szUserAgent               = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szDefaultComponent        = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szOIMainApp               = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szDescription             = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szUninstallKeyDescription = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szUninstallFilename       = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);
  if((ugUninstall.szAppID                   = NS_GlobalAlloc(MAX_BUF)) == NULL)
    return(1);

  return(0);
}

void DeInitUninstallGeneral()
{
  FreeMemory(&(ugUninstall.szAppPath));
  FreeMemory(&(ugUninstall.szLogPath));
  FreeMemory(&(ugUninstall.szLogFilename));
  FreeMemory(&(ugUninstall.szDescription));
  FreeMemory(&(ugUninstall.szUninstallKeyDescription));
  FreeMemory(&(ugUninstall.szUninstallFilename));
  FreeMemory(&(ugUninstall.szUserAgent));
  FreeMemory(&(ugUninstall.szDefaultComponent));
  FreeMemory(&(ugUninstall.szOIKey));
  FreeMemory(&(ugUninstall.szCompanyName));
  FreeMemory(&(ugUninstall.szProductName));
  FreeMemory(&(ugUninstall.szOIMainApp));
  FreeMemory(&(ugUninstall.szAppID));
}

sil *CreateSilNode()
{
  sil *silNode;

  if((silNode = NS_GlobalAlloc(sizeof(struct sInfoLine))) == NULL)
    exit(1);

  if((silNode->szLine = NS_GlobalAlloc(MAX_BUF)) == NULL)
    exit(1);

  silNode->ullLineNumber = 0;
  silNode->Next          = silNode;
  silNode->Prev          = silNode;

  return(silNode);
}

void SilNodeInsert(sil *silHead, sil *silTemp)
{
  if(silHead == NULL)
  {
    silHead          = silTemp;
    silHead->Next    = silHead;
    silHead->Prev    = silHead;
  }
  else
  {
    silTemp->Next           = silHead;
    silTemp->Prev           = silHead->Prev;
    silHead->Prev->Next     = silTemp;
    silHead->Prev           = silTemp;
  }
}

void SilNodeDelete(sil *silTemp)
{
  if(silTemp != NULL)
  {
    silTemp->Next->Prev = silTemp->Prev;
    silTemp->Prev->Next = silTemp->Next;
    silTemp->Next       = NULL;
    silTemp->Prev       = NULL;

    FreeMemory(&(silTemp->szLine));
    FreeMemory(&silTemp);
  }
}

ULONG ParseCommandLine(int argc, char *argv[])
{
  int   i;

  i     = 0;
  while(i < argc)
  {
    if((stricmp(argv[i], "-ma") == 0) || (stricmp(argv[i], "/ma") == 0))
    {
      SetUninstallRunMode("AUTO");
    }
    else if((stricmp(argv[i], "-ms") == 0) || (stricmp(argv[i], "/ms") == 0))
    {
      SetUninstallRunMode("SILENT");
    }
    else if((stricmp(argv[i], "-ua") == 0) || (stricmp(argv[i], "/ua") == 0))
    {
      if((i + 1) < argc) {
        i++;
        strcpy(ugUninstall.szUserAgent, argv[i]);
      }
    }
    else if((stricmp(argv[i], "-app") == 0) || (stricmp(argv[i], "/app") == 0))
    
    {
      if((i + 1) < argc) {
        i++;
        strcpy(ugUninstall.szAppID, argv[i]);
      }
    }
    else if((stricmp(argv[i], "-reg_path") == 0) || (stricmp(argv[i], "/reg_path") == 0))
    
    {
      if((i + 1) < argc) {
        i++;
        strcpy(ugUninstall.szOIMainApp, argv[i]);
      }
    }
    else if((stricmp(argv[i], "-v") == 0) || (stricmp(argv[i], "/v") == 0))
    
    {
      ugUninstall.bVerbose = TRUE;
    }

    ++i;
  }
}

#define BUFMIN	8*1024
#define BUFMAX	256*1024
#define BUFDEFAULT 32*1024

BOOL CheckForProcess(PID pid, PSZ szProcessName, ULONG ulProcessName, PSZ szFQProcessName, ULONG ulFQProcessName)
{

#ifdef QS_PROCESS
  ULONG bufsize = BUFDEFAULT;
  QSPTRREC* pbh;
  APIRET rc = 0;
  CHAR szUpperAppName[CCHMAXPATH] = {0};

  
  if (pid && szProcessName) {
    return FALSE;
  }
  if (szProcessName) {
    strcpy(szUpperAppName, szProcessName);
    strupr(szUpperAppName);
  }
  do {
    pbh = (QSPTRREC*) malloc(bufsize);
    if(!pbh) {
      if(bufsize <= BUFMIN)
        rc = ERROR_NOT_ENOUGH_MEMORY;
      else if(rc != ERROR_BUFFER_OVERFLOW)
        bufsize /= 2;
    } else {
      rc = DosQuerySysState(QS_PROCESS | QS_MTE, 0, 0, 0, pbh, bufsize);
      if(rc == ERROR_BUFFER_OVERFLOW) {
        if(bufsize < BUFMAX) {
          free(pbh);
          bufsize *= 2;
        } else {
          rc = ERROR_TOO_MANY_NAMES;    
        }
      }
    }
  } while(rc == ERROR_BUFFER_OVERFLOW);

  if(rc == NO_ERROR) {
    QSPREC* ppiLocal = pbh->pProcRec;
    while(ppiLocal->RecType == QS_PROCESS) {
      QSLREC* pmi = pbh->pLibRec;
      while (pmi && pmi->hmte != ppiLocal->hMte)
        pmi = (QSLREC*)pmi->pNextRec;
      if(pmi) {
        if ((szUpperAppName[0] && strstr((char*)pmi->pName, szUpperAppName)) ||
           (ppiLocal->pid == pid)) {
            if (szFQProcessName)
              strcpy(szFQProcessName, (char*)pmi->pName);
            if (pbh)
              free(pbh);
            return TRUE;
        }
      }
      ppiLocal=(QSPREC*)(ppiLocal->pThrdRec+ppiLocal->cTCB);
    }
  }
  if(pbh)
    free(pbh);
#endif
  return FALSE;
}

int PreCheckInstance(char *szSection, char *szIniFile, char *szFQProcessName)
{
  char  szParameter[MAX_BUF];
  char  szPath[MAX_BUF];
  ULONG ulCounter = 0;
  BOOL  bContinue = TRUE;
  char  szExtraCmd[] = "Extra Cmd";
  char  szExtraCmdParameter[MAX_BUF];

  do
  {
    
    sprintf(szExtraCmdParameter, "%s%d Parameter", szExtraCmd, ulCounter);
    GetPrivateProfileString(szSection,
                            szExtraCmdParameter,
                            "",
                            szParameter,
                            sizeof(szParameter),
                            szIniFile);
    if(*szParameter == '\0')
    {
      bContinue = FALSE;
      continue;
    }

    ParsePath(szFQProcessName, szPath, sizeof(szPath), PP_PATH_ONLY);

    
    
    
    
    
    
    
    
    bContinue = FALSE;

    
    WinSpawn(szFQProcessName, szParameter, szPath, TRUE);

    


    DosSleep(2000);
    
    ++ulCounter;
  } while(bContinue);

  return(WIZ_OK);
}

HRESULT CheckInstances()
{
  char  szFQProcessName[CCHMAXPATH];
  char  szProcessName[CCHMAXPATH];
  char  szSection[MAX_BUF];
  char  szClassName[MAX_BUF];
  char  szWindowName[MAX_BUF];
  char  szMessage[MAX_BUF];
  char  szIndex[MAX_BUF];
  int   iIndex;
  BOOL  bContinue;
  HWND  hwndFW;
  PSZ   szWN;
  PSZ   szCN;
  ULONG ulRv0;
  ULONG ulRv1;

  bContinue = TRUE;
  iIndex    = -1;
  while(bContinue)
  {
    memset(szClassName,  0, sizeof(szClassName));
    memset(szWindowName, 0, sizeof(szWindowName));
    memset(szMessage,    0, sizeof(szMessage));

    ++iIndex;
    _itoa(iIndex, szIndex, 10);
    strcpy(szSection, "Check Instance");
    strcat(szSection, szIndex);

    GetPrivateProfileString(szSection, "Message", "", szMessage, MAX_BUF, szFileIniUninstall);

    if(GetPrivateProfileString(szSection, "Process Name", "", szProcessName, sizeof(szProcessName), szFileIniUninstall) != 0L)
    {
      if(*szProcessName != '\0')
      {
        
        if(CheckForProcess(0, szProcessName, sizeof(szProcessName), szFQProcessName, sizeof(szFQProcessName)) == TRUE)
          PreCheckInstance(szSection, szFileIniUninstall, szFQProcessName);

        if(CheckForProcess(0, szProcessName, sizeof(szProcessName), NULL, 0) == TRUE)
        {
          if(*szMessage != '\0')
          {
            switch(ugUninstall.ulMode)
            {
              case NORMAL:
                switch(WinMessageBox(HWND_DESKTOP, hWndMain, szMessage, NULL, 0, MB_ICONEXCLAMATION | MB_OKCANCEL))
                {
                  case MBID_CANCEL:
                    
                    return(TRUE);

                  case MBID_RETRY:
                  case MBID_OK:
                    
                    iIndex = -1;
                    break;
                }
                break;

              case AUTO:
                ShowMessage(szMessage, TRUE);
                DosSleep(5000);
                ShowMessage(szMessage, FALSE);

                
                return(TRUE);

              case SILENT:
                return(TRUE);
            }
          }
          else
          {
            
            return(TRUE);
          }
        }
      }

      
      continue;
    }

    
    ulRv0 = GetPrivateProfileString(szSection, "Class Name", "", szClassName, MAX_BUF, szFileIniUninstall);
    ulRv1 = GetPrivateProfileString(szSection, "Window Name", "", szWindowName, MAX_BUF, szFileIniUninstall);
    if((ulRv0 == 0L) &&
       (ulRv1 == 0L))
    {
      bContinue = FALSE;
    }
    else if((*szClassName != '\0') || (*szWindowName != '\0'))
    {
      if(*szClassName == '\0')
        szCN = NULL;
      else
        szCN = szClassName;

      if(*szWindowName == '\0')
        szWN = NULL;
      else
        szWN = szWindowName;

      
      if((hwndFW = FindWindow(szCN)) != NULL) {
        PID pid;
        TID tid;
        WinQueryWindowProcess(hwndFW, &pid, &tid);
        CheckForProcess(pid, NULL, 0, szFQProcessName, sizeof(szFQProcessName));
        PreCheckInstance(szSection, szFileIniUninstall, szFQProcessName);
      }

      if((hwndFW = FindWindow(szCN)) != NULL)
      {
        if(*szMessage != '\0')
        {
          switch(ugUninstall.ulMode)
          {
            case NORMAL:
              switch(WinMessageBox(HWND_DESKTOP, hWndMain, szMessage, NULL, 0, MB_ICONEXCLAMATION | MB_RETRYCANCEL))
              {
                case MBID_CANCEL:
                  
                  return(TRUE);

                case MBID_RETRY:
                  
                  iIndex = -1;
                  break;
              }
              break;

            case AUTO:
              ShowMessage(szMessage, TRUE);
              DosSleep(5000);
              ShowMessage(szMessage, FALSE);

              
              return(TRUE);

            case SILENT:
              return(TRUE);
          }
        }
        else
        {
          
          return(TRUE);
        }
      }
    }
  }

  return(FALSE);
}

HRESULT GetAppPath()
{
  char szTmpAppPath[MAX_BUF];
  char szApp[MAX_BUF];

  if(*ugUninstall.szUserAgent != '\0')
  {
    sprintf(szApp, "%s %s", ugUninstall.szOIMainApp, ugUninstall.szUserAgent);
  }
  else
  {
    strcpy(szApp, ugUninstall.szOIKey);
  }

  PrfQueryProfileString(HINI_USERPROFILE, szApp, "PathToExe", "", szTmpAppPath, sizeof(szTmpAppPath));

  if(FileExists(szTmpAppPath))
  {
    strcpy(ugUninstall.szAppPath, szTmpAppPath);
  }

  return(0);
}

HRESULT GetUninstallLogPath()
{
  char szBuf[MAX_BUF];
  char szLogFolder[MAX_BUF];
  char szApp[MAX_BUF];
  char szWindowsUninstallKey[MAX_BUF];
  char szErrorMsg[MAX_BUF];
  char szEUninstallLogFolder[MAX_BUF];
  BOOL bRestrictedAccess;

  if(*ugUninstall.szUserAgent != '\0')
  {
    sprintf(szApp, "%s %s", ugUninstall.szOIMainApp, ugUninstall.szUserAgent);
  }
  else
  {
    strcpy(szApp, ugUninstall.szOIKey);
  }

  PrfQueryProfileString(HINI_USERPROFILE, szApp, "Uninstall Log Folder", "", szLogFolder, sizeof(szLogFolder));

  strcpy(ugUninstall.szDescription, ugUninstall.szOIMainApp);

  if(FileExists(szLogFolder) == FALSE)
  {
    if(GetPrivateProfileString("Messages", "ERROR_UNINSTALL_LOG_FOLDER", "",
                               szEUninstallLogFolder, sizeof(szEUninstallLogFolder), 
                               szFileIniUninstall))
    {
      strcpy(szBuf, "\n\n    ");

      strcat(szBuf, szLogFolder);

      strcat(szBuf, "\n");
      sprintf(szErrorMsg, szEUninstallLogFolder, szBuf);
      PrintError(szErrorMsg, ERROR_CODE_SHOW);
    }

    return(1);
  }
  strcpy(ugUninstall.szLogPath, szLogFolder);

  return(0);
}

HRESULT ParseUninstallIni(int argc, char *argv[])
{
  char szBuf[MAX_BUF];
  char szAppCrypted[MAX_BUF];
  char szKeyCrypted[MAX_BUF];
  char szShowDialog[MAX_BUF];
  char fontName[MAX_BUF];
  char fontSize[MAX_BUF];
  char charSet[MAX_BUF];

  if(InitUninstallGeneral())
    return(1);

  strcpy(ugUninstall.szLogFilename, FILE_LOG_INSTALL);

  
  GetPrivateProfileString("General", "Run Mode", "", szBuf, MAX_BUF, szFileIniUninstall);
  SetUninstallRunMode(szBuf);
  ParseCommandLine(argc, argv);

  if(CheckInstances())
    return(1);
  if(InitDlgUninstall(&diUninstall))
    return(1);
 
  
  GetPrivateProfileString("General", "Company Name", "", ugUninstall.szCompanyName, MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("General", "Product Name", "", ugUninstall.szProductName, MAX_BUF, szFileIniUninstall);

  GetPrivateProfileString("General", "App",          "", szKeyCrypted, MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("General", "Decrypt App",  "", szBuf, MAX_BUF, szFileIniUninstall);
  if(stricmp(szBuf, "TRUE") == 0)
  {
    DecryptString(ugUninstall.szOIKey, szKeyCrypted);
  }
  else
    strcpy(ugUninstall.szOIKey, szKeyCrypted);

  GetPrivateProfileString("General", "Main App",         "", szAppCrypted, MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("General", "Decrypt Main App", "", szBuf, MAX_BUF, szFileIniUninstall);

  
  
  if(*ugUninstall.szOIMainApp == '\0') 
  {
    if(stricmp(szBuf, "TRUE") == 0)
    {
      DecryptString(ugUninstall.szOIMainApp, szAppCrypted);
    }
    else
      strcpy(ugUninstall.szOIMainApp, szAppCrypted);
  }

  GetPrivateProfileString("General", "Uninstall Filename", "", ugUninstall.szUninstallFilename, MAX_BUF, szFileIniUninstall);


  
  GetPrivateProfileString("Dialog Uninstall",       "Show Dialog",  "", szShowDialog,                 MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("Dialog Uninstall",       "Title",        "", diUninstall.szTitle,          MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("Dialog Uninstall",       "Message0",     "", diUninstall.szMessage0,       MAX_BUF, szFileIniUninstall);
  if(stricmp(szShowDialog, "TRUE") == 0)
    diUninstall.bShowDialog = TRUE;

  switch(ugUninstall.ulMode)
  {
    case AUTO:
    case SILENT:
      gulWhatToDo             = WTD_NO_TO_ALL;
      diUninstall.bShowDialog = FALSE;
      break;
  }

  
  GetPrivateProfileString("Dialog Uninstall", "FONTNAME", "", fontName, MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("Dialog Uninstall", "FONTSIZE", "", fontSize, MAX_BUF, szFileIniUninstall);
  GetPrivateProfileString("Dialog Uninstall", "CHARSET", "", charSet, MAX_BUF, szFileIniUninstall);
  strcpy(ugUninstall.szDefinedFont, fontSize);
  strcat(ugUninstall.szDefinedFont, ".");
  strcat(ugUninstall.szDefinedFont, fontName);

  GetAppPath();

  ugUninstall.bUninstallFiles = TRUE;

  return(GetUninstallLogPath());
}

HRESULT DecryptVariable(PSZ szVariable, ULONG ulVariableSize)
{
  char szBuf[MAX_BUF];
  char szKey[MAX_BUF];
  char szName[MAX_BUF];
  char szValue[MAX_BUF];

  
  memset(szBuf,           0, sizeof(szBuf));
  memset(szKey,           0, sizeof(szKey));
  memset(szName,          0, sizeof(szName));
  memset(szValue,         0, sizeof(szValue));

  if(stricmp(szVariable, "PROGRAMFILESDIR") == 0)
  {
    
  }
  else if(stricmp(szVariable, "STARTUP") == 0)
  {
    HOBJECT hobj;
    hobj = WinQueryObject("<WP_STARTUP>");
    WinQueryObjectPath(hobj, szVariable, ulVariableSize);
  }
  else if(stricmp(szVariable, "DESKTOP") == 0)
  {
    HOBJECT hobj;
    hobj = WinQueryObject("<WP_DESKTOP>");
    WinQueryObjectPath(hobj, szVariable, ulVariableSize);
  }
  else if(stricmp(szVariable, "WARPCENTER") == 0)
  {
    HOBJECT hobj;
    hobj = WinQueryObject("<WP_WARPCENTER????>");
    WinQueryObjectPath(hobj, szVariable, ulVariableSize);
  }
  else if(stricmp(szVariable, "WIZTEMP") == 0)
  {
    
    strcpy(szVariable, szTempDir);
    if(szVariable[strlen(szVariable) - 1] == '\\')
      szVariable[strlen(szVariable) - 1] = '\0';
  }
  else if(stricmp(szVariable, "TEMP") == 0)
  {
    
    strcpy(szVariable, szOSTempDir);
    if(szVariable[strlen(szVariable) - 1] == '\\')
      szVariable[strlen(szVariable) - 1] = '\0';
  }
  else if(stricmp(szVariable, "OS2DISK") == 0)
  {
    
    ULONG ulBootDrive = 0;
    memset(szVariable, '\0', MAX_BUF);
    DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                    &ulBootDrive, sizeof(ulBootDrive));
    szVariable[0] = 'A' - 1 + ulBootDrive;
    szVariable[1] = ':';
  }
  else if(stricmp(szVariable, "OS2DIR") == 0)
  {
    
    ULONG ulBootDrive = 0;
    APIRET rc;
    char  buffer[] = " :\\OS2\\";
    DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                    &ulBootDrive, sizeof(ulBootDrive));
    buffer[0] = 'A' - 1 + ulBootDrive;
    strcpy(szVariable, buffer);
  }
  else if(stricmp(szVariable, "JRE BIN PATH") == 0)
  {
  }
  else if(stricmp(szVariable, "JRE PATH") == 0)
  {
  }
  else if(stricmp(szVariable, "UNINSTALL STARTUP PATH") == 0)
  {
    strcpy(szVariable, szUninstallDir);
  }
  else if(stricmp(szVariable, "Product CurrentVersion") == 0)
  {
    char szApp[MAX_BUF];

    sprintf(szApp, "%s", ugUninstall.szProductName);

    
    PrfQueryProfileString(HINI_USERPROFILE, szApp, "CurrentVersion", "",
                          szBuf, sizeof(szBuf));

    if(*szBuf == '\0')
      return(FALSE);

    strcpy(szVariable, szBuf);
  }
  else if(stricmp(szVariable, "Product OS2INIApp") == 0)
  {
    sprintf(szVariable, "%s", ugUninstall.szProductName);
  }
  else
    return(FALSE);

  return(TRUE);
}

HRESULT DecryptString(PSZ szOutputStr, PSZ szInputStr)
{
  ULONG ulLenInputStr;
  ULONG ulCounter;
  ULONG ulVar;
  ULONG ulPrepend;
  char  szBuf[MAX_BUF];
  char  szVariable[MAX_BUF];
  char  szPrepend[MAX_BUF];
  char  szAppend[MAX_BUF];
  char  szResultStr[MAX_BUF];
  BOOL  bFoundVar;
  BOOL  bBeginParse;
  BOOL  bDecrypted;

  
  memset(szBuf,       '\0', MAX_BUF);
  memset(szVariable,  '\0', MAX_BUF);
  memset(szPrepend,   '\0', MAX_BUF);
  memset(szAppend,    '\0', MAX_BUF);
  memset(szResultStr, '\0', MAX_BUF);

  strcpy(szPrepend, szInputStr);
  ulLenInputStr = strlen(szInputStr);
  bBeginParse   = FALSE;
  bFoundVar     = FALSE;

  for(ulCounter = 0; ulCounter < ulLenInputStr; ulCounter++)
  {
    if((szInputStr[ulCounter] == ']') && bBeginParse)
      break;

    if(bBeginParse)
      szVariable[ulVar++] = szInputStr[ulCounter];

    if((szInputStr[ulCounter] == '[') && !bBeginParse)
    {
      ulVar        = 0;
      ulPrepend    = ulCounter;
      bBeginParse  = TRUE;
    }
  }

  if(ulCounter == ulLenInputStr)
    
    ulCounter = 0;
  else
  {
    bFoundVar = TRUE;
    ++ulCounter;
  }

  if(bFoundVar)
  {
    strcpy(szAppend, &szInputStr[ulCounter]);

    szPrepend[ulPrepend] = '\0';

    bDecrypted = DecryptVariable(szVariable, MAX_BUF);
    if(!bDecrypted)
    {
      
      
      
      strcpy(szBuf, "[");
      strcat(szBuf, szVariable);
      strcat(szBuf, "]");
      strcpy(szVariable, szBuf);
    }

    strcpy(szOutputStr, szPrepend);
    strcat(szOutputStr, szVariable);
    strcat(szOutputStr, szAppend);

    if(bDecrypted)
    {
      DecryptString(szResultStr, szOutputStr);
      strcpy(szOutputStr, szResultStr);
    }
  }
  else
    strcpy(szOutputStr, szInputStr);

  return(TRUE);
}

#define S_IFMT (S_IFDIR | S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

HRESULT FileExists(PSZ szFile)
{
  struct stat st;
  int statrv;

  statrv = stat(szFile, &st);
  if (statrv == 0)
     if (S_ISDIR(st.st_mode))
        return FILE_DIRECTORY;
     else
        return(TRUE);
  else if ((strlen(szFile) == 2) && (szFile[1] == ':'))
  {
     char temp[4] = {0};
     strcpy(temp, szFile);
     strcat(temp, "\\");
     statrv = stat(temp, &st);
     if (statrv == 0)
        return FILE_DIRECTORY;
  }
  return (FALSE);
}

BOOL isFAT(char* szPath)
{
  APIRET rc;
  ULONG ulSize;
  PFSQBUFFER2 pfsqbuf2;
  CHAR szDrive[3];

  ulSize = sizeof(FSQBUFFER2) + 3 * CCHMAXPATH;
  pfsqbuf2 = (PFSQBUFFER2)malloc(ulSize);
  strncpy(szDrive, szPath, 2);
  szDrive[2] = '\0';

  DosError(FERR_DISABLEHARDERR);
  rc = DosQueryFSAttach(szDrive, 0, FSAIL_QUERYNAME,
                        pfsqbuf2, &ulSize);
  DosError(FERR_ENABLEHARDERR);

  if (rc == NO_ERROR) {
    if (strcmp(pfsqbuf2->szFSDName + pfsqbuf2->cbName, "FAT") != 0)
      return FALSE;
  }

  return TRUE;
}

void DeInitialize()
{
  DeInitDlgUninstall(&diUninstall);
  DeInitUninstallGeneral();

  FreeMemory(&szTempDir);
  FreeMemory(&szOSTempDir);
  FreeMemory(&szUninstallDir);
  FreeMemory(&szEGlobalAlloc);
  FreeMemory(&szEDllLoad);
  FreeMemory(&szEStringLoad);
  FreeMemory(&szEStringNull);
  FreeMemory(&szFileIniUninstall);
  FreeMemory(&szFileIniDefaultsInfo);
}

HWND FindWindow(PCSZ pszAtomString)
{
  ATOM atom;
  HENUM henum;
  HWND hwndClient, hwnd = NULLHANDLE;


  atom = WinFindAtom(WinQuerySystemAtomTable(), pszAtomString);
  if (atom) {
    henum = WinBeginEnumWindows(HWND_DESKTOP);
    while ((hwnd = WinGetNextWindow(henum)) != NULLHANDLE)
    {
      ULONG ulWindowWord;
      ulWindowWord = WinQueryWindowULong(hwnd, QWL_USER);
      if (ulWindowWord == atom) {
        break;
      } else {
        
        HWND hwndClient;
        CHAR szClassName[MAX_BUF];
        hwndClient = WinWindowFromID(hwnd, FID_CLIENT);
        WinQueryClassName(hwndClient ? hwndClient : hwnd, MAX_BUF, szClassName);
        if (strcmp(szClassName, pszAtomString) == 0) {
           break;
        }
      }
    }
    WinEndEnumWindows(henum);
  }
  if (!hwnd) {
     
    henum = WinBeginEnumWindows(HWND_OBJECT);
    while ((hwnd = WinGetNextWindow(henum)) != NULLHANDLE)
    {
      
      HWND hwndClient;
      CHAR szClassName[MAX_BUF];
      hwndClient = WinWindowFromID(hwnd, FID_CLIENT);
      WinQueryClassName(hwndClient ? hwndClient : hwnd, MAX_BUF, szClassName);
      if (strcmp(szClassName, pszAtomString) == 0) {
         break;
      }
    }
  }
  return  hwnd;
}

