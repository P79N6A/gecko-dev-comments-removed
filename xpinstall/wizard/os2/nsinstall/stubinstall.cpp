





































#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\stat.h>


#include "resource.h"
#include "nsINIParser.h"

#define MAX_BUF       4096
#define WIZ_TEMP_DIR  "ns_temp"


#define NORMAL                          0
#define SILENT                          1
#define AUTO                            2


#define PP_FILENAME_ONLY                1
#define PP_PATH_ONLY                    2
#define PP_ROOT_ONLY                    3

#define CLASS_NAME_SETUP_DLG            "MozillaSetupDlg"




char      szTitle[MAX_BUF];
char      szCmdLineToSetup[MAX_BUF];
BOOL      gbUncompressOnly;
ULONG     ulMode;
static ULONG	nTotalBytes = 0;  

char szOSTempDir[CCHMAXPATH];




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
    if (strcmp((char*)(pfsqbuf2->szFSDName + pfsqbuf2->cbName), "FAT") != 0)
      return FALSE;
  }

  return TRUE;
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
      }
    }
  }
  WinEndEnumWindows(henum);
  return  hwnd;
}



static void AppendBackSlash(PSZ szInput, ULONG ulInputSize)
{
  if(szInput != NULL)
  {
    if(szInput[strlen(szInput) - 1] != '\\')
    {
      if(((ULONG)strlen(szInput) + 1) < ulInputSize)
      {
        strcat(szInput, "\\");
      }
    }
  }
}






static BOOL
GetFullTempPathName(PCSZ szFileName, ULONG ulBufferLength, PSZ szBuffer)
{
  strcpy(szBuffer, szOSTempDir);
  AppendBackSlash(szBuffer, MAX_BUF);
  strcat(szBuffer, WIZ_TEMP_DIR);

  if (szFileName) {
    AppendBackSlash(szBuffer, MAX_BUF);
    strcat(szBuffer, szFileName);
  }

  return TRUE;
}


static void CreateDirectoriesAll(char* szPath)
{
  int     i;
  int     iLen = strlen(szPath);
  char    szCreatePath[CCHMAXPATH];

  memset(szCreatePath, 0, CCHMAXPATH);
  memcpy(szCreatePath, szPath, iLen);
  for(i = 0; i < iLen; i++)
  {
    if((iLen > 1) &&
      ((i != 0) && ((szPath[i] == '\\') || (szPath[i] == '/'))) &&
      (!((szPath[0] == '\\') && (i == 1)) && !((szPath[1] == ':') && (i == 2))))
    {
      szCreatePath[i] = '\0';
      DosCreateDir(szCreatePath, NULL);  
      szCreatePath[i] = szPath[i];
    }
  }
}


void DirectoryRemove(PSZ szDestination, BOOL bRemoveSubdirs)
{
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  ULONG           ulAttrs;
  char            szDestTemp[MAX_BUF];
  BOOL            bFound;

  if(bRemoveSubdirs == TRUE)
  {
    strcpy(szDestTemp, szDestination);
    AppendBackSlash(szDestTemp, sizeof(szDestTemp));
    strcat(szDestTemp, "*");

    ulFindCount = 1;
    hFile = HDIR_CREATE;
    ulAttrs = FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY | FILE_ARCHIVED;
    if((DosFindFirst(szDestTemp, &hFile, ulAttrs, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
      bFound = FALSE;
    else
      bFound = TRUE;
    while(bFound == TRUE)
    {
      if((stricmp(fdFile.achName, ".") != 0) && (stricmp(fdFile.achName, "..") != 0))
      {
        
        strcpy(szDestTemp, szDestination);
        AppendBackSlash(szDestTemp, sizeof(szDestTemp));
        strcat(szDestTemp, fdFile.achName);

        if(fdFile.attrFile & FILE_DIRECTORY)
        {
          DirectoryRemove(szDestTemp, bRemoveSubdirs);
        }
        else
        {
          DosDelete(szDestTemp);
        }
      }

      ulFindCount = 1;
      if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) != NO_ERROR)
         bFound = FALSE;
      else
         bFound = TRUE;
    }

    DosFindClose(hFile);
  }
  
  DosDeleteDir(szDestination);
}

void RemoveBackSlash(PSZ szInput)
{
  int   iCounter;
  ULONG ulInputLen;

  if(szInput != NULL)
  {
    ulInputLen = strlen(szInput);

    for(iCounter = ulInputLen -1; iCounter >= 0 ; iCounter--)
    {
      if(szInput[iCounter] == '\\')
        szInput[iCounter] = '\0';
      else
        break;
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

void ParseCommandLine(int argc, char *argv[])
{
  char  szArgVBuf[MAX_BUF];
  int   i = 1;

  memset(szCmdLineToSetup, 0, MAX_BUF);
  ulMode = NORMAL;
  gbUncompressOnly = FALSE;
  strcpy(szCmdLineToSetup, " ");
  while(i < argc)
  {
    if((stricmp(argv[i], "-ms") == 0) || (stricmp(argv[i], "/ms") == 0))
    {
      ulMode = SILENT;
    }
    else if((stricmp(argv[i], "-u") == 0) || (stricmp(argv[i], "/u") == 0))
    {
      gbUncompressOnly = TRUE;
    }
    strcat(szCmdLineToSetup, argv[i]); 
    strcat(szCmdLineToSetup, " ");
    ++i;
  }
}





MRESULT EXPENTRY DialogProc(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg) {
    case WM_INITDLG:
      {
        SWP swpDlg;
        WinQueryWindowPos(hwndDlg, &swpDlg);
        WinSetWindowPos(hwndDlg,
                        HWND_TOP,
                        (WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)/2)-(swpDlg.cx/2),
                        (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)/2)-(swpDlg.cy/2),
                        0,
                        0,
                        SWP_MOVE);
        WinSendMsg(WinWindowFromID(hwndDlg, IDC_GAUGE),SLM_SETSLIDERINFO,
                   MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
                   0);
      }
  }
  return WinDefDlgProc(hwndDlg, msg, mp1, mp2);
}

void DeleteTempFiles(ULONG ulNumFiles)
{
  ULONG ulSize;
  void* pFileData;
  FILE* fhFile;
  char  szTmpFile[CCHMAXPATH] = "";
  char  szFileName[CCHMAXPATH] = "";

  for (int i=1;i<=ulNumFiles;i++) {
    WinLoadString(0, NULLHANDLE, ID_FILE_BASE+i, CCHMAXPATH, szFileName);
    GetFullTempPathName(szFileName, sizeof(szTmpFile), szTmpFile);
    DosDelete(szTmpFile);
  }
}


BOOL
ExtractFiles(ULONG ulNumFiles, HWND hwndDlg)
{
  ULONG ulSize;
  void* pFileData;
  FILE* fhFile;
  char  szTmpFile[CCHMAXPATH] = "";
  char  szFileName[CCHMAXPATH] = "";
  char  szArcLstFile[CCHMAXPATH] = "";
  char  szStatus[256];
  char  szText[256];
  ULONG nBytesWritten = 0;

  WinLoadString(0, NULLHANDLE, IDS_STATUS_EXTRACTING, sizeof(szText), szText);

  if(gbUncompressOnly == FALSE) {
    
    GetFullTempPathName("tempfile", sizeof(szTmpFile), szTmpFile);
    CreateDirectoriesAll(szTmpFile);
    GetFullTempPathName("Archive.lst", sizeof(szArcLstFile), szArcLstFile);
  } else {
    strcpy(szArcLstFile, "Archive.lst");
  }

  for (int i=1;i<=ulNumFiles;i++) {
    WinLoadString(0, NULLHANDLE, ID_FILE_BASE+i, CCHMAXPATH, szFileName);
    
    sprintf(szStatus, szText, szFileName);
    WinSetWindowText(WinWindowFromID(hwndDlg, IDC_STATUS), szStatus);
    if(gbUncompressOnly == FALSE) {
      GetFullTempPathName(szFileName, sizeof(szTmpFile), szTmpFile);
    } else {
      strcpy(szTmpFile, szFileName);
    }

    DosQueryResourceSize(NULLHANDLE, RT_RCDATA, ID_FILE_BASE+i, &ulSize);
    DosGetResource(NULLHANDLE, RT_RCDATA, ID_FILE_BASE+i, &pFileData);
    FILE* fhFile = fopen(szTmpFile, "wb+");
    fwrite(pFileData, ulSize, 1, fhFile);
    fclose(fhFile);
    DosFreeResource(pFileData);
    nBytesWritten += ulSize;
    WinSendMsg(WinWindowFromID(hwndDlg, IDC_GAUGE), SLM_SETSLIDERINFO,
                               MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
                               MPARAM(nBytesWritten * 100 / nTotalBytes));
    WritePrivateProfileString("Archives", szFileName, "TRUE", szArcLstFile);
  }
}

BOOL FileExists(PSZ szFile)
{
  struct stat st;
  int statrv;

  statrv = stat(szFile, &st);
  if (statrv == 0)
     return(TRUE);
  else
     return (FALSE);
}

void RunInstaller(ULONG ulNumFiles, HWND hwndDlg)
{
  char                szCmdLine[MAX_BUF];
  char                szSetupFile[CCHMAXPATH];
  char                szUninstallFile[CCHMAXPATH];
  char                szArcLstFile[CCHMAXPATH];
  char                szText[256];
  char                szTempPath[CCHMAXPATH];
  char                szFilename[CCHMAXPATH];
  char                szBuf[MAX_BUF];
  ULONG               ulLen;

  
  WinSendMsg(WinWindowFromID(hwndDlg, IDC_GAUGE),SLM_SETSLIDERINFO,
             MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
             0);
  WinLoadString(0, NULLHANDLE, IDS_STATUS_LAUNCHING_SETUP, sizeof(szText), szText);
  WinSetWindowText(WinWindowFromID(hwndDlg, IDC_STATUS), szText);

  
  GetFullTempPathName("Archive.lst",   sizeof(szArcLstFile),    szArcLstFile);
  GetFullTempPathName("SETUP.EXE",     sizeof(szSetupFile),     szSetupFile);
  GetFullTempPathName("uninstall.exe", sizeof(szUninstallFile), szUninstallFile);

  GetPrivateProfileString("Archives", "uninstall.exe", "", szBuf, sizeof(szBuf), szArcLstFile);
  if(FileExists(szUninstallFile) && (*szBuf != '\0'))
  {
    strcpy(szCmdLine, szUninstallFile);
    ulLen = strlen(szUninstallFile);
  }
  else
  {
    strcpy(szCmdLine, szSetupFile);
    PPIB ppib;
    PTIB ptib;
    char buffer[CCHMAXPATH];
    DosGetInfoBlocks( &ptib, &ppib);
    DosQueryModuleName(ppib->pib_hmte, sizeof(szBuf), szBuf);
    ParsePath(szBuf, szFilename, sizeof(szFilename), PP_FILENAME_ONLY);

    strcat(szCmdLine, " -n ");
    strcat(szCmdLine, szFilename);
    ulLen = strlen(szSetupFile);
  }

  strcat(szCmdLine, szCmdLineToSetup);

  
  RESULTCODES rcChild;
  PID pidChild;
  CHAR szLoadError[CCHMAXPATH];

  szCmdLine[strlen(szCmdLine)] = '\0';
  szCmdLine[strlen(szCmdLine)+1] = '\0';
  szCmdLine[ulLen] = '\0';

  DosExecPgm(szLoadError,
             sizeof(szLoadError),
             EXEC_ASYNCRESULT,
             szCmdLine,
             NULL,
             &rcChild,
             szCmdLine);

  if(ulMode != SILENT)
  {
    WinDestroyWindow(hwndDlg);
  }

  DosWaitChild(DCWA_PROCESS,
               DCWW_WAIT,
               &rcChild,
               &pidChild,
               rcChild.codeTerminate);

  DeleteTempFiles(ulNumFiles);

  GetFullTempPathName("Archive.lst", sizeof(szTempPath), szTempPath);
  DosDelete(szTempPath);
  GetFullTempPathName("xpcom.ns", sizeof(szTempPath), szTempPath);
  DirectoryRemove(szTempPath, TRUE);
  GetFullTempPathName(NULL, sizeof(szTempPath), szTempPath);
  DirectoryRemove(szTempPath, TRUE);
}

int main(int argc, char *argv[], char *envp[])
{
  HAB hab;
  HMQ hmq;
  HWND hwndFW, hwndDlg;
  ATOM atom;

  hab = WinInitialize(0);
  hmq = WinCreateMsgQueue(hab,0);

  atom = WinAddAtom(WinQuerySystemAtomTable(), "NSExtracting");

  char *tempEnvVar = NULL;

  
  tempEnvVar = getenv("TMP");
  if ((tempEnvVar) && (!(isFAT(tempEnvVar)))) {
    strcpy(szOSTempDir, tempEnvVar);
  }
  else
  {
    tempEnvVar = getenv("TEMP");
    if (tempEnvVar)
      strcpy(szOSTempDir, tempEnvVar);
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
    strcpy(szOSTempDir, buffer);
    strcat(szOSTempDir, "TEMP");
  }

  WinLoadString(0, NULLHANDLE, IDS_TITLE, MAX_BUF, szTitle);

  


  if(FindWindow("NSExtracting") != NULL)
    return(1);

  

  if((hwndFW = FindWindow(CLASS_NAME_SETUP_DLG)) != NULL)
  {
    WinSetActiveWindow(HWND_DESKTOP, hwndFW);
    return(1);
  }

  
  ParseCommandLine(argc, argv);

  ULONG ulSize;
  ULONG ulNumFiles = 0;
  INT i = 1;
  APIRET rc = NO_ERROR;

  while(rc == NO_ERROR) {
    rc = DosQueryResourceSize(NULLHANDLE, RT_RCDATA, ID_FILE_BASE+i, &ulSize);
    if (rc == NO_ERROR) {
      nTotalBytes += ulSize;
      ulNumFiles++;
    }
    i++;
  }

  if(ulMode != SILENT)
  {
    hwndDlg = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, DialogProc, NULLHANDLE, IDD_EXTRACTING, NULL);
    WinSetWindowULong(hwndDlg, QWL_USER, atom);
    WinSetWindowPos(hwndDlg, 0, 0, 0, 0, 0, SWP_SHOW | SWP_ACTIVATE);
  }

  
  ExtractFiles(ulNumFiles, hwndDlg);

  if (gbUncompressOnly == FALSE) {
    
    RunInstaller(ulNumFiles, hwndDlg);
  }

  WinDeleteAtom(WinQuerySystemAtomTable(), atom);

  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}
