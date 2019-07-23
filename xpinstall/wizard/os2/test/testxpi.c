







































#include "testxpi.h"
#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS

#include <os2.h>
LHANDLE hXPIStubInst;


void ShowUsage(UCHAR *name)
{
    UCHAR szBuf[MAX_BUF];

    sprintf(szBuf, "Usage: %s <output sea name> <files [file1] [file2]...>\n", name);
    strcat(szBuf, "\n");
    strcat(szBuf, "    output sea name: name to use for the self-extracting executable\n");
    strcat(szBuf, "    files: one or more files to add to the self-extracing executable\n");
}


void PrintError(PSZ szMsg, ULONG dwErrorCodeSH, int iExitCode)
{
  ULONG dwErr;
  char  szErrorString[MAX_BUF];

  if(dwErrorCodeSH == ERROR_CODE_SHOW)
  {
    dwErr = WinGetLastError();
    sprintf(szErrorString, "%d : %s", dwErr, szMsg);
  }
  else
    sprintf(szErrorString, "%s", szMsg);

  printf("%s\n", szErrorString);
  printf("exit code: %d\n", iExitCode);
  if(iExitCode == 0)
    printf("\n** Success! **\n");

  DeInitializeXPIStub();
  exit(iExitCode);
}


void RemoveQuotes(PSZ lpszSrc, PSZ lpszDest, int iDestSize)
{
  char *lpszBegin;

  if(strlen(lpszSrc) > iDestSize)
    return;

  if(*lpszSrc == '\"')
    lpszBegin = &lpszSrc[1];
  else
    lpszBegin = lpszSrc;

  strcpy(lpszDest, lpszBegin);

  if(lpszDest[strlen(lpszDest) - 1] == '\"')
    lpszDest[strlen(lpszDest) - 1] = '\0';
}


void RemoveBackSlash(PSZ szInput)
{
  int   iCounter;
  ULONG dwInputLen;

  if(szInput != NULL)
  {
    dwInputLen = strlen(szInput);

    for(iCounter = dwInputLen -1; iCounter >= 0 ; iCounter--)
    {
      if(szInput[iCounter] == '\\')
        szInput[iCounter] = '\0';
      else
        break;
    }
  }
}


void AppendBackSlash(PSZ szInput, ULONG dwInputSize)
{
  if(szInput != NULL)
  {
    if(szInput[strlen(szInput) - 1] != '\\')
    {
      if(((ULONG)strlen(szInput) + 1) < dwInputSize)
      {
        strcat(szInput, "\\");
      }
    }
  }
}






void ParsePath(PSZ szInput, PSZ szOutput, ULONG dwOutputSize, ULONG dwType)
{
  int   iCounter;
  ULONG dwCounter;
  ULONG dwInputLen;
  BOOL  bFound;

  if((szInput != NULL) && (szOutput != NULL))
  {
    bFound        = FALSE;
    dwInputLen    = strlen(szInput);
    memset(szOutput, 0, dwOutputSize);

    if(dwInputLen < dwOutputSize)
    {
      switch(dwType)
      {
        case PP_FILENAME_ONLY:
          for(iCounter = dwInputLen - 1; iCounter >= 0; iCounter--)
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
          for(iCounter = dwInputLen - 1; iCounter >= 0; iCounter--)
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
            AppendBackSlash(szOutput, dwOutputSize);
          }
          else if(szInput[1] == '\\')
          {
            int iFoundBackSlash = 0;
            for(dwCounter = 0; dwCounter < dwInputLen; dwCounter++)
            {
              if(szInput[dwCounter] == '\\')
              {
                szOutput[dwCounter] = szInput[dwCounter];
                ++iFoundBackSlash;
              }

              if(iFoundBackSlash == 3)
                break;
            }

            if(iFoundBackSlash != 0)
              AppendBackSlash(szOutput, dwOutputSize);
          }
          break;
      }
    }
  }
}




long FileExists(PSZ szFile)
{
  ULONG rv;
  FILEFINDBUF ffbFindFileBuffer;
  APIRET rc;
  
  
  if((rc = DosQueryPathInfo(szFile, FIL_STANDARD, &ffbFindFileBuffer, sizeof(ffbFindFileBuffer))) != 0)
  {
    return(FALSE);
  }
  else
  {
    return(rc);
  }
}




PSZ GetFirstNonSpace(PSZ lpszString)
{
  int   i;
  int   iStrLength;

  iStrLength = strlen(lpszString);

  for(i = 0; i < iStrLength; i++)
  {
    if(!isspace(lpszString[i]))
      return(&lpszString[i]);
  }

  return(NULL);
}




int GetArgC(PSZ lpszCommandLine)
{
  int   i;
  int   iArgCount;
  int   iStrLength;
  PSZ lpszBeginStr;
  BOOL  bFoundQuote;
  BOOL  bFoundSpace;

  iArgCount    = 0;
  lpszBeginStr = GetFirstNonSpace(lpszCommandLine);

  if(lpszBeginStr == NULL)
    return(iArgCount);

  iStrLength   = strlen(lpszBeginStr);
  bFoundQuote  = FALSE;
  bFoundSpace  = TRUE;

  for(i = 0; i < iStrLength; i++)
  {
    if(lpszCommandLine[i] == '\"')
    {
      if(bFoundQuote == FALSE)
      {
        ++iArgCount;
        bFoundQuote = TRUE;
      }
      else
      {
        bFoundQuote = FALSE;
      }
    }
    else if(bFoundQuote == FALSE)
    {
      if(!isspace(lpszCommandLine[i]) && (bFoundSpace == TRUE))
      {
        ++iArgCount;
        bFoundSpace = FALSE;
      }
      else if(isspace(lpszCommandLine[i]))
      {
        bFoundSpace = TRUE;
      }
    }
  }

  return(iArgCount);
}




PSZ GetArgV(PSZ lpszCommandLine, int iIndex, PSZ lpszDest, int iDestSize)
{
  int   i;
  int   j;
  int   iArgCount;
  int   iStrLength;
  PSZ lpszBeginStr;
  PSZ lpszDestTemp;
  BOOL  bFoundQuote;
  BOOL  bFoundSpace;

  iArgCount    = 0;
  lpszBeginStr = GetFirstNonSpace(lpszCommandLine);

  if(lpszBeginStr == NULL)
    return(NULL);

  lpszDestTemp = (char *)calloc(iDestSize, sizeof(char));
  if(lpszDestTemp == NULL)
    PrintError("Out of memory", ERROR_CODE_HIDE, 1);

  memset(lpszDest, 0, iDestSize);
  iStrLength      = strlen(lpszBeginStr);
  bFoundQuote   = FALSE;
  bFoundSpace   = TRUE;
  j                    = 0;

  for(i = 0; i < iStrLength; i++)
  {
    if(lpszCommandLine[i] == '\"')
    {
      if(bFoundQuote == FALSE)
      {
        ++iArgCount;
        bFoundQuote = TRUE;
      }
      else
      {
        bFoundQuote = FALSE;
      }
    }
    else if(bFoundQuote == FALSE)
    {
      if(!isspace(lpszCommandLine[i]) && (bFoundSpace == TRUE))
      {
        ++iArgCount;
        bFoundSpace = FALSE;
      }
      else if(isspace(lpszCommandLine[i]))
      {
        bFoundSpace = TRUE;
      }
    }

    if((iIndex == (iArgCount - 1)) &&
      ((bFoundQuote == TRUE) || (bFoundSpace == FALSE) ||
      ((bFoundQuote == FALSE) && (lpszCommandLine[i] == '\"'))))
    {
      if(j < iDestSize)
      {
        lpszDestTemp[j] = lpszCommandLine[i];
        ++j;
      }
      else
      {
        lpszDestTemp[j] = '\0';
      }
    }
  }

  RemoveQuotes(lpszDestTemp, lpszDest, iDestSize);
  free(lpszDestTemp);
  return(lpszDest);
}

int main(void)
{
  APIRET           hrResult;
  UCHAR            szBuf[MAX_BUF];
  UCHAR            szAppName[MAX_BUF];
  UCHAR            szAppPath[MAX_BUF];
  UCHAR            *listArchive[] = {"e:\\cmonkey\\mozilla\\dist\\win32_d.obj\\bin\\test1.xpi",
                                              "e:\\cmonkey\\mozilla\\dist\\win32_d.obj\\bin\\test2.xpi",
                                              "\0"};
  
  if(DosQueryModuleName(NULL, sizeof(szBuf), szBuf) == 0L)
    PrintError("DosQueryModuleName() failed", ERROR_CODE_SHOW, 1);

  ParsePath(szBuf, szAppPath, sizeof(szAppPath), PP_PATH_ONLY);
  ParsePath(szBuf, szAppName, sizeof(szAppName), PP_FILENAME_ONLY);

  hrResult = SmartUpdateJars(szAppName, szAppPath, listArchive);

  if(hrResult == 999)
    sprintf(szBuf, "%s done\nReboot required\n", szAppName);
  else
    sprintf(szBuf, "%s done\n", szAppName);

  PrintError(szBuf, ERROR_CODE_SHOW, hrResult);
  return(0);
}

