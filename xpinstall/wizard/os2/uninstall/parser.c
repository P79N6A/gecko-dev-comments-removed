






































#include "extern.h"
#include "logkeys.h"
#include "extra.h"
#include "parser.h"
#include "ifuncns.h"
#include "dialogs.h"

#define KEY_SHARED_DLLS "Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"

BOOL DeleteOrDelayUntilReboot(PSZ szFile)
{
  FILE      *ofp;
  char      szWinDir[MAX_BUF];
  char      szWininitFile[MAX_BUF];
  BOOL      bDelayDelete = FALSE;
  BOOL      bWriteRenameSection;

  FileDelete(szFile);
  if(FileExists(szFile))
  {
    DosReplaceModule(szFile, NULL, NULL);
    FileDelete(szFile);
  }

  return TRUE;
}

void RemoveUninstaller(PSZ szUninstallFilename)
{
  char      szUninstallFile[MAX_BUF];

  strcpy(szUninstallFile, ugUninstall.szLogPath);
  strcat(szUninstallFile, "\\");
  strcat(szUninstallFile, szUninstallFilename);
  DeleteOrDelayUntilReboot(szUninstallFile);
  DirectoryRemove(ugUninstall.szLogPath, FALSE);
}

sil *InitSilNodes(char *szInFile)
{
  FILE      *ifp;
  char      szLineRead[MAX_BUF];
  sil       *silTemp;
  sil       *silHead;
  unsigned long long ullLineCount;

  if(FileExists(szInFile) == FALSE)
    return(NULL);

  ullLineCount = 0;
  silHead      = NULL;
  if((ifp = fopen(szInFile, "r")) == NULL)
    exit(1);

  while(fgets(szLineRead, MAX_BUF, ifp) != NULL)
  {
    silTemp = CreateSilNode();

    silTemp->ullLineNumber = ++ullLineCount;
    strcpy(silTemp->szLine, szLineRead);
    if(silHead == NULL)
    {
      silHead = silTemp;
    }
    else
    {
      SilNodeInsert(silHead, silTemp);
    }

    ProcessWindowsMessages();
  }
  fclose(ifp);
  return(silHead);
}

void DeInitSilNodes(sil **silHead)
{
  sil   *silTemp;
  
  if(*silHead == NULL)
  {
    return;
  }
  else if(((*silHead)->Prev == NULL) || ((*silHead)->Prev == *silHead))
  {
    SilNodeDelete(*silHead);
    return;
  }
  else
  {
    silTemp = (*silHead)->Prev;
  }

  while(silTemp != *silHead)
  {
    SilNodeDelete(silTemp);
    silTemp = (*silHead)->Prev;

    ProcessWindowsMessages();
  }
  SilNodeDelete(silTemp);
}

void ParseForFile(PSZ szString, PSZ szKeyStr, PSZ szFile, ULONG ulShortFilenameBufSize)
{
  int     iLen;
  PSZ     szFirstNonSpace;
  char    szBuf[MAX_BUF];

  if((szFirstNonSpace = GetFirstNonSpace(&(szString[strlen(szKeyStr)]))) != NULL)
  {
    iLen = strlen(szFirstNonSpace);
    if(szFirstNonSpace[iLen - 1] == '\n')
      szFirstNonSpace[iLen -1] = '\0';

    strcpy(szFile, szFirstNonSpace);
  }
}

void ParseForCopyFile(PSZ szString, PSZ szKeyStr, PSZ szFile, ULONG ulShortFilenameBufSize)
{
  int     iLen;
  PSZ     szFirstNonSpace;
  PSZ     szSubStr = NULL;
  char    szBuf[MAX_BUF];

  if((szSubStr = strstr(szString, " to ")) != NULL)
  {
    if((szFirstNonSpace = GetFirstNonSpace(&(szSubStr[strlen(" to ")]))) != NULL)
    {
      iLen = strlen(szFirstNonSpace);
      if(szFirstNonSpace[iLen - 1] == '\n')
        szFirstNonSpace[iLen -1] = '\0';

    strcpy(szFile, szFirstNonSpace);
    }
  }
}

void ParseForOS2INIInfo(PSZ szString, PSZ szKeyStr, PSZ szApp, ULONG ulAppBufSize, PSZ szKey, ULONG ulKeyBufSize)
{
  int     i;
  int     iLen;
  int     iBrackets;
  char    szStrCopy[MAX_BUF];
  PSZ     szFirstNonSpace;
  PSZ     szFirstBackSlash;
  BOOL    bFoundOpenBracket;
  BOOL    bFoundName;

  strcpy(szStrCopy, szString);
  if((szFirstNonSpace = GetFirstNonSpace(&(szStrCopy[strlen(szKeyStr)]))) != NULL)
  {
    iLen = strlen(szFirstNonSpace);
    if(szFirstNonSpace[iLen - 1] == '\n')
    {
      szFirstNonSpace[--iLen] = '\0';
    }

    iLen = strlen(szFirstNonSpace);

    iBrackets         = 0;
    bFoundName        = FALSE;
    bFoundOpenBracket = FALSE;
    for(i = iLen - 1; i >= 0; i--)
    {
      if(bFoundName == FALSE)
      {
        




        if(szFirstNonSpace[i] == ']')
        {
          if(iBrackets == 0)
            szFirstNonSpace[i] = '\0';

          ++iBrackets;
        }
        else if(szFirstNonSpace[i] == '[')
        {
          bFoundOpenBracket = TRUE;
          --iBrackets;
        }

        if((bFoundOpenBracket) && (iBrackets == 0))
        {
          strcpy(szKey, &(szFirstNonSpace[i + 1]));
          bFoundName = TRUE;
        }
      }
      else
      {
        
        if(!isspace(szFirstNonSpace[i]))
        {
          szFirstNonSpace[i + 1] = '\0';
          strcpy(szApp, szFirstNonSpace);
          break;
        }
      }
    }
  }
}

ULONG Uninstall(sil* silInstallLogHead)
{
  sil   *silInstallLogTemp;
  PSZ   szSubStr;
  char  szLCLine[MAX_BUF];
  char  szApp[MAX_BUF];
  char  szKey[MAX_BUF];
  char  szFile[MAX_BUF];

  if(silInstallLogHead != NULL)
  {
    silInstallLogTemp = silInstallLogHead;
    do
    {
      silInstallLogTemp = silInstallLogTemp->Prev;
      strcpy(szLCLine, silInstallLogTemp->szLine);
      strlwr(szLCLine);

      if(((szSubStr = strstr(szLCLine, KEY_INSTALLING)) != NULL) &&
               (strstr(szLCLine, KEY_DO_NOT_UNINSTALL) == NULL))
      {
        
        ParseForFile(szSubStr, KEY_INSTALLING, szFile, sizeof(szFile));
        DeleteOrDelayUntilReboot(szFile);
      }
      else if(((szSubStr = strstr(szLCLine, KEY_REPLACING)) != NULL) &&
               (strstr(szLCLine, KEY_DO_NOT_UNINSTALL) == NULL))
      {
        
        ParseForFile(szSubStr, KEY_REPLACING, szFile, sizeof(szFile));
        DeleteOrDelayUntilReboot(szFile);
      }
      else if(((szSubStr = strstr(silInstallLogTemp->szLine, KEY_STORE_INI_ENTRY)) != NULL) &&
               (strstr(szLCLine, KEY_DO_NOT_UNINSTALL) == NULL))
      {
        
        ParseForOS2INIInfo(szSubStr, KEY_STORE_INI_ENTRY, szApp, sizeof(szKey), szKey, sizeof(szKey));
        PrfWriteProfileString(HINI_USER, szApp, szKey, NULL);
      }
      else if(((szSubStr = strstr(silInstallLogTemp->szLine, KEY_OS2_OBJECT)) != NULL) &&
               (strstr(szLCLine, KEY_DO_NOT_UNINSTALL) == NULL))
      {
        
        HOBJECT hObj = NULLHANDLE;
        ParseForFile(szSubStr, KEY_OS2_OBJECT, szFile, sizeof(szFile));
        hObj = WinQueryObject(szFile);
        if (hObj) {
           WinDestroyObject(hObj);
        }
      }
      else if(((szSubStr = strstr(szLCLine, KEY_CREATE_FOLDER)) != NULL) &&
               (strstr(szLCLine, KEY_DO_NOT_UNINSTALL) == NULL))
      {
        ParseForFile(szSubStr, KEY_CREATE_FOLDER, szFile, sizeof(szFile));
        DirectoryRemove(szFile, FALSE);
      }
      else if(((szSubStr = strstr(szLCLine, KEY_COPY_FILE)) != NULL) &&
               (strstr(szLCLine, KEY_DO_NOT_UNINSTALL) == NULL))
      {
        
        ParseForCopyFile(szSubStr, KEY_COPY_FILE, szFile, sizeof(szFile));
        DeleteOrDelayUntilReboot(szFile);
      }

      ProcessWindowsMessages();
    } while(silInstallLogTemp != silInstallLogHead);
  }

  return(0);
}

ULONG GetLogFile(PSZ szTargetPath, PSZ szInFilename, PSZ szOutBuf, ULONG dwOutBufSize)
{
  int             iFilenameOnlyLen;
  char            szSearchFilename[MAX_BUF];
  char            szSearchTargetFullFilename[MAX_BUF];
  char            szFilenameOnly[MAX_BUF];
  char            szFilenameExtensionOnly[MAX_BUF];
  char            szNumber[MAX_BUF];
  long            ulNumber;
  long            ulMaxNumber;
  PSZ             szDotPtr;
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  ULONG           ulAttrs;
  BOOL            bFound;

  if(FileExists(szTargetPath))
  {
    
    memset(szOutBuf,                0, dwOutBufSize);
    memset(szSearchFilename,        0, sizeof(szSearchFilename));
    memset(szFilenameOnly,          0, sizeof(szFilenameOnly));
    memset(szFilenameExtensionOnly, 0, sizeof(szFilenameExtensionOnly));

    
    if((szDotPtr = strstr(szInFilename, ".")) != NULL)
    {
      *szDotPtr = '\0';
      strcpy(szSearchFilename, szInFilename);
      strcpy(szFilenameOnly, szInFilename);
      strcpy(szFilenameExtensionOnly, &szDotPtr[1]);
      *szDotPtr = '.';
    }
    else
    {
      strcpy(szFilenameOnly, szInFilename);
      strcpy(szSearchFilename, szInFilename);
    }

    
    strcat(szSearchFilename, "*.*");
    strcpy(szSearchTargetFullFilename, szTargetPath);
    AppendBackSlash(szSearchTargetFullFilename, sizeof(szSearchTargetFullFilename));
    strcat(szSearchTargetFullFilename, szSearchFilename);

    iFilenameOnlyLen = strlen(szFilenameOnly);
    ulNumber         = 0;
    ulMaxNumber      = -1;

    
    ulFindCount = 1;
    hFile = HDIR_CREATE;
    ulAttrs = FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY | FILE_ARCHIVED;
    if((DosFindFirst(szSearchTargetFullFilename, &hFile, ulAttrs, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
      bFound = FALSE;
    else
      bFound = TRUE;

    while(bFound)
    {
       memset(szNumber, 0, sizeof(szNumber));
      if((stricmp(fdFile.achName, ".") != 0) && (stricmp(fdFile.achName, "..") != 0))
      {
        strcpy(szNumber, &fdFile.achName[iFilenameOnlyLen]);
        ulNumber = atoi(szNumber);
        if(ulNumber > ulMaxNumber)
          ulMaxNumber = ulNumber;
      }

      ulFindCount = 1;
      if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) == NO_ERROR) {
        bFound = TRUE;
      } else {
        bFound = FALSE;
      }
    }

    DosFindClose(hFile);

    strcpy(szOutBuf, szTargetPath);
    AppendBackSlash(szOutBuf, dwOutBufSize);
    strcat(szOutBuf, szFilenameOnly);
    _itoa(ulMaxNumber, szNumber, 10);
    strcat(szOutBuf, szNumber);

    if(*szFilenameExtensionOnly != '\0')
    {
      strcat(szOutBuf, ".");
      strcat(szOutBuf, szFilenameExtensionOnly);
    }
  }
  else
    return(0);
  return(FileExists(szOutBuf));
}

