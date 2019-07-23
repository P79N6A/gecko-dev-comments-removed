






































#include "testxpi.h"

typedef HRESULT (_cdecl *XpiInit)(const char *, pfnXPIProgress);
typedef HRESULT (_cdecl *XpiInstall)(const char *, const char *, long);
typedef void    (_cdecl *XpiExit)(void);

static XpiInit          pfnXpiInit;
static XpiInstall       pfnXpiInstall;
static XpiExit          pfnXpiExit;

extern HANDLE           hXPIStubInst;

HRESULT InitializeXPIStub()
{
  char szBuf[MAX_BUF];
  char szXPIStubFile[MAX_BUF];

  hXPIStubInst = NULL;

  
  if(GetFullPathName("xpistub.dll", sizeof(szXPIStubFile), szXPIStubFile, NULL) == FALSE)
    PrintError("File not found: xpistub.dll", ERROR_CODE_SHOW, 2);

  
  if((hXPIStubInst = LoadLibraryEx(szXPIStubFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)) == NULL)
  {
    wsprintf(szBuf, "Error loading library: %s\n", szXPIStubFile);
    PrintError(szBuf, ERROR_CODE_SHOW, 1);
  }
  if(((FARPROC)pfnXpiInit = GetProcAddress(hXPIStubInst, "XPI_Init")) == NULL)
  {
    wsprintf(szBuf, "GetProcAddress() failed: XPI_Init\n");
    PrintError(szBuf, ERROR_CODE_SHOW, 1);
  }
  if(((FARPROC)pfnXpiInstall = GetProcAddress(hXPIStubInst, "XPI_Install")) == NULL)
  {
    wsprintf(szBuf, "GetProcAddress() failed: XPI_Install\n");
    PrintError(szBuf, ERROR_CODE_SHOW, 1);
  }
  if(((FARPROC)pfnXpiExit = GetProcAddress(hXPIStubInst, "XPI_Exit")) == NULL)
  {
    wsprintf(szBuf, "GetProcAddress() failed: XPI_Exit\n");
    PrintError(szBuf, ERROR_CODE_SHOW, 1);
  }

  return(0);
}

HRESULT DeInitializeXPIStub()
{
  pfnXpiInit    = NULL;
  pfnXpiInstall = NULL;
  pfnXpiExit    = NULL;

  if(hXPIStubInst)
    FreeLibrary(hXPIStubInst);

  return(0);
}

int GetTotalArchivesToInstall(LPSTR listArchive[])
{
  int i = 0;

  while(TRUE)
  {
    if(*listArchive[i] != '\0')
      ++i;
    else
      return(i);
  }
}

HRESULT SmartUpdateJars(LPSTR szAppName, LPSTR szAppPath, LPSTR listArchive[])
{
  int       i;
  int       iTotalArchives;
  char      szBuf[MAX_BUF];
  HRESULT   hrResult;

  hXPIStubInst = NULL;

  if((hrResult = InitializeXPIStub()) == TEST_OK)
  {
    RemoveBackSlash(szAppPath);
    hrResult = pfnXpiInit(szAppPath, cbXPIProgress);
    if(hrResult != 0)
    {
      wsprintf(szBuf, "XpiInit() failed: %d", hrResult);
      PrintError(szBuf, ERROR_CODE_HIDE, hrResult);
    }

    iTotalArchives = GetTotalArchivesToInstall(listArchive);
    for(i = 0; i < iTotalArchives; i++)
    {
      if(FileExists(listArchive[i]) == FALSE)
      {
        printf("File not found: %s\n", listArchive[i]);

        
        continue;
      }

      hrResult = pfnXpiInstall(listArchive[i], "", 0xFFFF);
      if((hrResult != TEST_OK) && (hrResult != 999))
      {
        wsprintf(szBuf, "XpiInstall() failed: %d", hrResult);
        PrintError(szBuf, ERROR_CODE_HIDE, hrResult);

        
        break;
      }
      printf("\n");
    }

    pfnXpiExit();
  }
  else
  {
    wsprintf(szBuf, "InitializeXPIStub() failed: %d", hrResult);
    PrintError(szBuf, ERROR_CODE_HIDE, hrResult);
  }

  DeInitializeXPIStub();
  return(hrResult);
}

void cbXPIStart(const char *URL, const char *UIName)
{
}

void cbXPIProgress(const char* msg, PRInt32 val, PRInt32 max)
{
  char szFilename[MAX_BUF];

  ParsePath((char *)msg, szFilename, sizeof(szFilename), PP_FILENAME_ONLY);
  if(max == 0)
    printf("Processing: %s\n", szFilename);
  else
    printf("Installing: %d/%d %s\n", val, max, szFilename);

  ProcessWindowsMessages();
}

void cbXPIFinal(const char *URL, PRInt32 finalStatus)
{
}

void ProcessWindowsMessages()
{
  MSG msg;

  while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
