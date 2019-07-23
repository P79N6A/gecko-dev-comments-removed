






































#include <windows.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <Winreg.h>

#include "CondMgr.h"
#include "HSAPI.h"
#include "resource.h"

#define MOZ_PALMSYNC_PROXY      ".\\PalmSyncProxy.dll"
#define CREATOR                  "addr"
#define CONDUIT_FILENAME         "mozABConduit.dll"
#define REMOTE_DB                "AddressDB"
#define CONDUIT_NAME             "address"
#define CONDUIT_PRIORITY         2

#define CONDMGR_FILENAME         "CondMgr.dll"
#define HSAPI_FILENAME           "HsApi.dll"
#define DIRECTORY_SEPARATOR      '\\'
#define DIRECTORY_SEPARATOR_STR  "\\"
#define EXECUTABLE_EXTENSION     ".exe"
#define HOTSYNC_MAX_WAIT         30         // wait for HotSync to start/stop in seconds

#define MAX_LOADSTRING 256


typedef int (WINAPI *CmGetCorePathPtr)(TCHAR *pPath, int *piSize);
typedef int (WINAPI *CmGetHotSyncExecPathPtr)(char *szPath, int *iSize);

typedef int (WINAPI *CmInstallCreatorPtr)(const char *pCreator, int iType);
typedef int (WINAPI *CmRemoveConduitByCreatorIDPtr)(const char *pCreator);
typedef int (WINAPI *CmRestoreHotSyncSettingsPtr)(BOOL bToDefaults);
typedef int (WINAPI *CmSetCreatorRemotePtr)(const char *pCreator, const TCHAR *pRemote);
typedef int (WINAPI *CmSetCreatorNamePtr)(const char *pCreator, const TCHAR *pConduitName);
typedef int (WINAPI *CmSetCreatorTitlePtr)(const char *pCreator, const TCHAR *pConduitTitle);
typedef int (WINAPI *CmSetCreatorFilePtr)(const char *pCreator, const TCHAR *pConduitFile);
typedef int (WINAPI *CmSetCreatorDirectoryPtr)(const char *pCreator, const TCHAR *pConduitDirectory);

typedef int (WINAPI *CmSetCreatorPriorityPtr)(const char *pCreator, DWORD dwPriority);
typedef int (WINAPI *CmSetCreatorIntegratePtr)(const char *pCreator, DWORD dwIntegrate);
typedef int (WINAPI *CmSetCreatorValueDwordPtr)(const char *pCreator, TCHAR *pValue, DWORD dwValue);
typedef int (WINAPI *CmSetCreatorValueStringPtr)(const char *pCreator, TCHAR *pValueName, TCHAR *value);
typedef int (WINAPI *CmSetCorePathPtr) (const char *pPath);
typedef int (WINAPI *CmSetHotSyncExePathPtr) (const char *pPath);
typedef int (WINAPI *CmSetCreatorModulePtr) (const char *pCreatorID, const TCHAR *pModule);

typedef int (WINAPI *CmGetCreatorNamePtr)(const char *pCreator, TCHAR *pConduitName, int *pSize);
typedef int (WINAPI *CmGetCreatorTitlePtr)(const char *pCreator, TCHAR *pConduitTitle, int *pSize);
typedef int (WINAPI *CmGetCreatorPriorityPtr)(const char *pCreator, DWORD *dwPriority);
typedef int (WINAPI *CmGetCreatorTypePtr)(const char *pCreator);
typedef int (WINAPI *CmGetCreatorIntegratePtr)(const char *pCreator, DWORD *dwIntegrate);
typedef int (WINAPI *CmGetCreatorValueDwordPtr)(const char *pCreator, TCHAR *pValueName, DWORD dwValue, DWORD dwDefault);
typedef int (WINAPI *CmGetCreatorValueStringPtr)(const char *pCreator, TCHAR *pValueName, TCHAR *pValue, int *pSize, TCHAR *pDefault);
typedef int (WINAPI *CmGetCreatorFilePtr) (const TCHAR *pCreatorID, TCHAR *pFile, int *piSize);
typedef int (WINAPI *CmGetCreatorDirectoryPtr) (const TCHAR *pCreatorID, TCHAR *pFile, int *piSize);
typedef int (WINAPI *CmGetCreatorModulePtr) (const char *pCreatorID, TCHAR *pModule, int *piSize);
typedef int (WINAPI *CmGetCreatorRemotePtr)(const char *pCreator, const TCHAR *pRemote, int*pSize);


typedef int (WINAPI *HsCheckApiStatusPtr)(void);
typedef int (WINAPI *HsGetSyncStatusPtr)(DWORD *dwStatus);
typedef int (WINAPI *HsSetAppStatusPtr)(HsStatusType statusType, DWORD dwStartFlags);


typedef int (WINAPI *mozDllRegisterServerPtr)(void);
typedef int (WINAPI *mozDllUnregisterServerPtr)(void);


int  InstallConduit(HINSTANCE hInstance, TCHAR *installPath, TCHAR *appName);
int UninstallConduit();
void ConstructMessage(HINSTANCE hInstance, TCHAR *appName, DWORD dwMessageId, TCHAR *formattedMsg);


BOOL    gWasHotSyncRunning = FALSE;

void ConstructMessage(HINSTANCE hInstance, TCHAR *appName, DWORD dwMessageId, TCHAR *formattedMsg)
{
  
  TCHAR formatString[MAX_LOADSTRING];
  LoadString(hInstance, dwMessageId, formatString, MAX_LOADSTRING-1);

  
  if ((dwMessageId == IDS_SUCCESS_INSTALL) ||
      (dwMessageId == IDS_CONFIRM_INSTALL))
    _sntprintf(formattedMsg, MAX_LOADSTRING-1, formatString, appName, appName);
  else
    _sntprintf(formattedMsg, MAX_LOADSTRING-1, formatString, appName);

  formattedMsg[MAX_LOADSTRING-1]='\0';
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  TCHAR appTitle[MAX_LOADSTRING];
  TCHAR appName[MAX_LOADSTRING] = {0};
  TCHAR msgStr[MAX_LOADSTRING];

  enum eActionType
  {
    eInstall,
    eSilentInstall,
    eUninstall,
    eSilentUninstall
  };

  int res = -1;

  char* installDir = NULL;
  eActionType action = eInstall;

  if (__argc > 1)
  {
    char* arg;
    
    for (int i = 1; i < __argc; ++i)
    {
      
      arg = strstr(__argv[i], "/n");
      if (arg)
      {
        
        arg += 2;

        
        strncpy(appName, arg, MAX_LOADSTRING - 1);

        
        strcat(appName, " ");
      }
      else
      {
        
        
        arg = strstr(__argv[i], "/p");
        if (arg)
          
          installDir = arg + 2;
        else if (!strcmpi(__argv[i], "/u"))
          action = eUninstall;
        else if (!strcmpi(__argv[i], "/us"))
          action = eSilentUninstall;
        else if (!strcmpi(__argv[i], "/s"))
          action = eSilentInstall;
      }
    }
  }

  
  LocalFree(__argv);

  switch (action)
  {
  case eInstall:
    ConstructMessage(hInstance, appName, IDS_APP_TITLE_INSTALL, appTitle);
    ConstructMessage(hInstance, appName, IDS_CONFIRM_INSTALL, msgStr);

    if (MessageBox(NULL, msgStr, appTitle, MB_YESNO) == IDYES) 
    {
      res = InstallConduit(hInstance, installDir, appName);
      if (!res)
        res = IDS_SUCCESS_INSTALL;
    }
    break;

  case eSilentInstall:
    return InstallConduit(hInstance, installDir, appName);

  case eUninstall:
    ConstructMessage(hInstance, appName, IDS_APP_TITLE_UNINSTALL, appTitle);
    ConstructMessage(hInstance, appName, IDS_CONFIRM_UNINSTALL, msgStr);

    if (MessageBox(NULL, msgStr, appTitle, MB_YESNO) == IDYES) 
    {
      res = UninstallConduit();
      if (!res)
        res = IDS_SUCCESS_UNINSTALL;
      break;
    }
    return 0;

  case eSilentUninstall:
    return UninstallConduit();
  }

  if (res > IDS_ERR_MAX || res < IDS_ERR_GENERAL)
     res = IDS_ERR_GENERAL;

  ConstructMessage(hInstance, appName, res, msgStr);
  MessageBox(NULL, msgStr, appTitle, MB_OK);

  return 0;
}


int GetPalmDesktopInstallDirectory(TCHAR *pPDInstallDirectory, unsigned long *pSize)
{
    HKEY   key;
    
    LONG rc = ::RegOpenKey(HKEY_CURRENT_USER, "Software\\U.S. Robotics\\Pilot Desktop\\Core", &key);
    if (rc == ERROR_SUCCESS) {
        
        rc = ::RegQueryValueEx(key, "Path", NULL, NULL, 
                               (LPBYTE)pPDInstallDirectory, pSize);
        if (rc == ERROR_SUCCESS) {
            *pSize = _tcslen(pPDInstallDirectory); 
            rc=0; 
        }
        
        ::RegCloseKey(key);
    }

    if(rc) {
        HKEY   key2;
        
        rc = ::RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\palm.exe", &key2);
        if (rc == ERROR_SUCCESS) {
            
            rc = ::RegQueryValueEx(key2, "", NULL, NULL, 
                                   (LPBYTE)pPDInstallDirectory, pSize);
            
            
            
            
            if (rc == ERROR_SUCCESS) {
              TCHAR *end = pPDInstallDirectory + _tcslen(pPDInstallDirectory);
              while ((*end != '\\') && (end != pPDInstallDirectory))
                end--;
              *end = '\0';
              rc=0; 
            }
            
            ::RegCloseKey(key2);
        }
    }
    
    return rc;
}


int LoadConduitManagerDll(HINSTANCE* hCondMgrDll, const TCHAR * szPalmDesktopDirectory)
{
    
    *hCondMgrDll=NULL;
    
    
    TCHAR   szPDCondMgrPath[_MAX_PATH];
    
    if((strlen(szPalmDesktopDirectory) + strlen(DIRECTORY_SEPARATOR_STR) +  strlen(CONDMGR_FILENAME)) >= _MAX_PATH)
        return IDS_ERR_LOADING_CONDMGR;
    strcpy(szPDCondMgrPath, szPalmDesktopDirectory);
    strcat(szPDCondMgrPath, DIRECTORY_SEPARATOR_STR);
    strcat(szPDCondMgrPath, CONDMGR_FILENAME);
    
    if( (*hCondMgrDll=LoadLibrary(szPDCondMgrPath)) != NULL )
        
    return 0;

    return IDS_ERR_LOADING_CONDMGR;
}


int LoadHsapiDll(HINSTANCE* hHsapiDLL, const TCHAR * szPalmDesktopDirectory)
{
    
    *hHsapiDLL=NULL;
    
    TCHAR   szHsapiPath[_MAX_PATH];
    
    if((strlen(szPalmDesktopDirectory) + strlen(DIRECTORY_SEPARATOR_STR) +  strlen(HSAPI_FILENAME)) >= _MAX_PATH)
        return IDS_ERR_LOADING_CONDMGR;
    strcpy(szHsapiPath, szPalmDesktopDirectory);
    strcat(szHsapiPath, DIRECTORY_SEPARATOR_STR);
    strcat(szHsapiPath, HSAPI_FILENAME);
    if( (*hHsapiDLL=LoadLibrary(szHsapiPath)) != NULL )
        
        return 0;

    
    return IDS_ERR_HSAPI_NOT_FOUND;
}


BOOL IsHotSyncRunning(HINSTANCE hHsapiDLL)
{
    BOOL    bRetVal = FALSE;
    
    if(!hHsapiDLL)
        return bRetVal;
    
    
    HsCheckApiStatusPtr lpfnHsCheckApiStatus;
    lpfnHsCheckApiStatus = (HsCheckApiStatusPtr) GetProcAddress(hHsapiDLL, "HsCheckApiStatus");
    
    if( lpfnHsCheckApiStatus )
    {
        if( (*lpfnHsCheckApiStatus)() == 0 )
            bRetVal = TRUE;
    }
    return bRetVal;
}


BOOL IsHotSyncInProgress(HINSTANCE hHsapiDLL)
{
    DWORD dwStatus;
    
    if(!hHsapiDLL)
        return FALSE;
    
    if(IsHotSyncRunning(hHsapiDLL))
    {
        
        HsGetSyncStatusPtr  lpfnHsGetSyncStatus;
        lpfnHsGetSyncStatus = (HsGetSyncStatusPtr) GetProcAddress(hHsapiDLL, "HsGetSyncStatus");
    
        if( lpfnHsGetSyncStatus )
        {
            if( (*lpfnHsGetSyncStatus)(&dwStatus) == 0 )
                if( dwStatus == HOTSYNC_STATUS_IDLE )
                    return FALSE;
        }
    }
    return TRUE;
}


void ShutdownHotSync(HINSTANCE hHsapiDLL)
{
    if(!hHsapiDLL)
        return;

    BOOL    bHotSyncRunning=IsHotSyncRunning(hHsapiDLL);
    
    if(bHotSyncRunning)
    {
        
        HsSetAppStatusPtr   lpfnHsSetAppStatus;
        lpfnHsSetAppStatus = (HsSetAppStatusPtr) GetProcAddress(hHsapiDLL, "HsSetAppStatus");
    
        if( lpfnHsSetAppStatus )
            (*lpfnHsSetAppStatus)(HsCloseApp, HSFLAG_NONE);
        
        
        for( int i=0; (i<HOTSYNC_MAX_WAIT*2) && bHotSyncRunning; i++ )
        {
            if( (bHotSyncRunning=IsHotSyncRunning(hHsapiDLL)) == TRUE )
                Sleep(500);
        }
    }
}


void StartHotSync(HINSTANCE hHsapiDLL)
{
    if(!hHsapiDLL)
        return;

    BOOL    bHotSyncRunning=IsHotSyncRunning(hHsapiDLL);
    
    if(!bHotSyncRunning)
    {
        
        HsSetAppStatusPtr   lpfnHsSetAppStatus;
        lpfnHsSetAppStatus = (HsSetAppStatusPtr) GetProcAddress(hHsapiDLL, "HsSetAppStatus");
    
        if( lpfnHsSetAppStatus )
            (*lpfnHsSetAppStatus)(HsStartApp, HSFLAG_NONE);
        
        
        for( int i=0; (i<HOTSYNC_MAX_WAIT*2) && !bHotSyncRunning; i++ )
        {
            if( (bHotSyncRunning=IsHotSyncRunning(hHsapiDLL)) == FALSE )
                Sleep(500);
        }
    }
}

int RegisterMozPalmSyncDll()
{
    HINSTANCE hMozPalmSyncProxyDll = NULL;
    if( (hMozPalmSyncProxyDll=LoadLibrary(MOZ_PALMSYNC_PROXY)) != NULL ) {
        mozDllRegisterServerPtr lpfnmozDllRegisterServer;
        lpfnmozDllRegisterServer = (mozDllRegisterServerPtr) GetProcAddress(hMozPalmSyncProxyDll, "DllRegisterServer");
        DWORD dwReturnCode = (*lpfnmozDllRegisterServer)();
        if(dwReturnCode == S_OK)
            
            return 0;
    }

    return IDS_ERR_REGISTERING_MOZ_DLL;
}

int UnregisterMozPalmSyncDll()
{
    HINSTANCE hMozPalmSyncProxyDll = NULL;
    if( (hMozPalmSyncProxyDll=LoadLibrary(MOZ_PALMSYNC_PROXY)) != NULL ) {
        mozDllUnregisterServerPtr lpfnmozDllUnregisterServer;
        lpfnmozDllUnregisterServer = (mozDllUnregisterServerPtr) GetProcAddress(hMozPalmSyncProxyDll, "DllUnregisterServer");
        DWORD dwReturnCode = (*lpfnmozDllUnregisterServer)();
        if(dwReturnCode == S_OK)
            
            return 0;
    }

    return IDS_ERR_UNREGISTERING_MOZ_DLL;
}


char *mystrsep(char **stringp, char delim)
{
  char *endStr = strchr(*stringp, delim);
  char *retStr;
  if (endStr)
  {
    bool foundDelim = (*endStr == delim);
    *endStr = '\0';
    retStr = *stringp;
    *stringp = endStr + !!foundDelim;
  }
  else
    return NULL;
  return retStr;
}

char oldSettingsStr[500];
static char             gSavedCwd[_MAX_PATH];


int InstallConduit(HINSTANCE hInstance, TCHAR *installDir, TCHAR *appName)
{ 
    int dwReturnCode;
    BOOL    bHotSyncRunning = FALSE;

    
    
    

    TCHAR szConduitPath[_MAX_PATH];
    if (!installDir)
    {
      if(!GetModuleFileName(NULL, szConduitPath, _MAX_PATH))
          return IDS_ERR_CONDUIT_NOT_FOUND;

      
      int index = strlen(szConduitPath)-1;
      while((szConduitPath[index] != DIRECTORY_SEPARATOR) && index)
          index--;
      szConduitPath[index] = 0;
    }
    else
      strncpy(szConduitPath, installDir, sizeof(szConduitPath) - 1);

    
    if((strlen(szConduitPath) + strlen(DIRECTORY_SEPARATOR_STR) + strlen(CONDUIT_FILENAME)) > _MAX_PATH)
        return IDS_ERR_LOADING_CONDMGR;
    
    
    
    if (!strstr(szConduitPath, ".DLL") && !strstr(szConduitPath, CONDUIT_FILENAME))
    {
      if (szConduitPath[strlen(szConduitPath) - 1] != DIRECTORY_SEPARATOR)
        strcat(szConduitPath, DIRECTORY_SEPARATOR_STR);
      strcat(szConduitPath, CONDUIT_FILENAME);
    }

    TCHAR shortConduitPath[_MAX_PATH];

    
    if (!GetShortPathName(szConduitPath, shortConduitPath, _MAX_PATH))
      
      strncpy(shortConduitPath, szConduitPath, _MAX_PATH);

    
    struct _finddata_t dll_file;
    long hFile;
    if( (hFile = _findfirst( shortConduitPath, &dll_file )) == -1L )
        return IDS_ERR_CONDUIT_NOT_FOUND;

    
    if( (dwReturnCode = RegisterMozPalmSyncDll()) != 0)
        return dwReturnCode;
    
    
    TCHAR   szPalmDesktopDir[_MAX_PATH];
    unsigned long desktopSize=_MAX_PATH;
    
    
    TCHAR szOldCreatorName[_MAX_PATH];
    TCHAR szOldRemote[_MAX_PATH];
    TCHAR szOldCreatorTitle[_MAX_PATH];
    TCHAR szOldCreatorFile[_MAX_PATH];
    TCHAR szOldCreatorDirectory[_MAX_PATH];
    DWORD oldPriority;
    DWORD oldIntegrate;
    int   oldType;

    
    HINSTANCE hConduitManagerDLL;
    if( (dwReturnCode=GetPalmDesktopInstallDirectory(szPalmDesktopDir, &desktopSize)) == 0 ) 
    {
        
        

        GetCurrentDirectory(sizeof(gSavedCwd), gSavedCwd);
        SetCurrentDirectory(szPalmDesktopDir);

        if( (dwReturnCode = LoadConduitManagerDll(&hConduitManagerDLL, szPalmDesktopDir)) != 0 )
            return dwReturnCode;
    }
    else 
        return IDS_ERR_CONDUIT_NOT_FOUND;
    
    
    CmInstallCreatorPtr lpfnCmInstallCreator;
    lpfnCmInstallCreator = (CmInstallCreatorPtr) GetProcAddress(hConduitManagerDLL, "CmInstallCreator");
    CmSetCreatorRemotePtr   lpfnCmSetCreatorRemote;
    lpfnCmSetCreatorRemote = (CmSetCreatorRemotePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorRemote");
    CmSetCreatorNamePtr lpfnCmSetCreatorName;
    lpfnCmSetCreatorName = (CmSetCreatorNamePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorName");
    CmSetCreatorTitlePtr lpfnCmSetCreatorTitle;
    lpfnCmSetCreatorTitle = (CmSetCreatorTitlePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorTitle");
    CmSetCreatorPriorityPtr lpfnCmSetCreatorPriority;
    lpfnCmSetCreatorPriority = (CmSetCreatorPriorityPtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorPriority");
    CmSetCreatorIntegratePtr    lpfnCmSetCreatorIntegrate;
    lpfnCmSetCreatorIntegrate = (CmSetCreatorIntegratePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorIntegrate");
    CmRemoveConduitByCreatorIDPtr   lpfnCmRemoveConduitByCreatorID;
    lpfnCmRemoveConduitByCreatorID = (CmRemoveConduitByCreatorIDPtr) GetProcAddress(hConduitManagerDLL, "CmRemoveConduitByCreatorID");
    CmSetCreatorValueStringPtr lpfnCmSetCreatorValueString = (CmSetCreatorValueStringPtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorValueString");

    CmGetCreatorRemotePtr   lpfnCmGetCreatorRemote;
    lpfnCmGetCreatorRemote = (CmGetCreatorRemotePtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorRemote");
    CmGetCreatorNamePtr lpfnCmGetCreatorName;
    lpfnCmGetCreatorName = (CmGetCreatorNamePtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorName");
    CmGetCreatorTitlePtr lpfnCmGetCreatorTitle;
    lpfnCmGetCreatorTitle = (CmGetCreatorTitlePtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorTitle");
    CmGetCreatorPriorityPtr lpfnCmGetCreatorPriority;
    lpfnCmGetCreatorPriority = (CmGetCreatorPriorityPtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorPriority");
    CmGetCreatorIntegratePtr    lpfnCmGetCreatorIntegrate;
    lpfnCmGetCreatorIntegrate = (CmGetCreatorIntegratePtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorIntegrate");
    CmGetCreatorTypePtr lpfnCmGetCreatorType = (CmGetCreatorTypePtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorType");
    CmGetCreatorFilePtr lpfnCmGetCreatorFile = (CmGetCreatorFilePtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorFile");
    CmGetCreatorDirectoryPtr lpfnCmGetCreatorDirectory = (CmGetCreatorDirectoryPtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorDirectory");
    if( (lpfnCmInstallCreator == NULL) 
        || (lpfnCmSetCreatorRemote == NULL)
        || (lpfnCmSetCreatorName == NULL)
        || (lpfnCmSetCreatorTitle == NULL)
        || (lpfnCmSetCreatorPriority == NULL)
        || (lpfnCmSetCreatorIntegrate == NULL)
        )
    {
        
        return(IDS_ERR_LOADING_CONDMGR);
    }
 
    szOldCreatorTitle[0] = '\0';
    szOldCreatorName[0] = '\0';
    szOldRemote[0] = '\0';
    szOldCreatorTitle[0] = '\0';
    szOldCreatorFile[0] = '\0';
    szOldCreatorDirectory[0] = '\0';
    
    int remoteBufSize = sizeof(szOldRemote);
    (*lpfnCmGetCreatorRemote) (CREATOR, szOldRemote, &remoteBufSize);
    int creatorBufSize = sizeof(szOldCreatorName);
    (*lpfnCmGetCreatorName)(CREATOR, szOldCreatorName, &creatorBufSize);
    int creatorTitleBufSize = sizeof(szOldCreatorTitle);
    (*lpfnCmGetCreatorTitle)(CREATOR, szOldCreatorTitle, &creatorTitleBufSize);
    int creatorFileBufSize = sizeof(szOldCreatorFile);
    int creatorDirectoryBufSize = sizeof(szOldCreatorDirectory);
    (*lpfnCmGetCreatorFile)(CREATOR, szOldCreatorFile, &creatorFileBufSize);
    (*lpfnCmGetCreatorDirectory)(CREATOR, szOldCreatorDirectory, &creatorDirectoryBufSize);
    (*lpfnCmGetCreatorPriority)(CREATOR, &oldPriority);
    (*lpfnCmGetCreatorIntegrate)(CREATOR, &oldIntegrate);
    oldType = (*lpfnCmGetCreatorType) (CREATOR);

    
    if (!oldSettingsStr[0])
      _snprintf(oldSettingsStr, sizeof(oldSettingsStr), "%s,%s,%s,%s,%s,%d,%d,%d", szOldRemote, szOldCreatorName, 
        szOldCreatorTitle, szOldCreatorFile, szOldCreatorDirectory, oldType, oldPriority, oldIntegrate);

    
    HINSTANCE hHsapiDLL;
    if( (dwReturnCode = LoadHsapiDll(&hHsapiDLL, szPalmDesktopDir)) != 0 )
        return dwReturnCode;
        
    
    if( (bHotSyncRunning=IsHotSyncRunning(hHsapiDLL)) )
    {
        
        if( IsHotSyncInProgress(hHsapiDLL) )
            return IDS_ERR_HOTSYNC_IN_PROGRESS;
            
        ShutdownHotSync(hHsapiDLL);
        
        gWasHotSyncRunning = TRUE;
    }
    
    
    dwReturnCode = (*lpfnCmInstallCreator)(CREATOR, CONDUIT_APPLICATION);
    if(dwReturnCode == ERR_CREATORID_ALREADY_IN_USE) {
        dwReturnCode = (*lpfnCmRemoveConduitByCreatorID)(CREATOR);
        if(dwReturnCode >= 0 ) {
            
            FreeLibrary(hConduitManagerDLL);
            FreeLibrary(hHsapiDLL);
            return InstallConduit(hInstance, shortConduitPath, appName);
        }
    }
    if( dwReturnCode == 0 )
    {
        (*lpfnCmSetCreatorValueString) (CREATOR, "oldConduitSettings", oldSettingsStr);
        dwReturnCode = (*lpfnCmSetCreatorName)(CREATOR, shortConduitPath);
        if( dwReturnCode != 0 ) return dwReturnCode;
        TCHAR title[MAX_LOADSTRING];
        
        ConstructMessage(hInstance, appName, IDS_CONDUIT_TITLE, title);
        dwReturnCode = (*lpfnCmSetCreatorTitle)(CREATOR, title);
        if( dwReturnCode != 0 ) return dwReturnCode;
        dwReturnCode = (*lpfnCmSetCreatorRemote)(CREATOR, REMOTE_DB);
        if( dwReturnCode != 0 ) return dwReturnCode;
        dwReturnCode = (*lpfnCmSetCreatorPriority)(CREATOR, CONDUIT_PRIORITY);
        if( dwReturnCode != 0 ) return dwReturnCode;
        
        dwReturnCode = (*lpfnCmSetCreatorIntegrate)(CREATOR, (DWORD)0);
        (*lpfnCmSetCreatorValueString) (CREATOR, "oldConduitSettings", oldSettingsStr);
    }
    
    
    if( gWasHotSyncRunning )
        StartHotSync(hHsapiDLL);

    
    if (gSavedCwd[0])
      SetCurrentDirectory(gSavedCwd);

    return(dwReturnCode);
}


int UninstallConduit()
{ 
    int dwReturnCode;
    BOOL    bHotSyncRunning = FALSE;

    
    TCHAR   szPalmDesktopDir[_MAX_PATH];
    unsigned long desktopSize=_MAX_PATH;
    
    HINSTANCE hConduitManagerDLL;
    if( (dwReturnCode=GetPalmDesktopInstallDirectory(szPalmDesktopDir, &desktopSize)) == 0 )
    {
        if( (dwReturnCode = LoadConduitManagerDll(&hConduitManagerDLL, szPalmDesktopDir)) != 0 )
                return(dwReturnCode);
    }
    
    else 
          return(dwReturnCode);
    
    
    

    GetCurrentDirectory(sizeof(gSavedCwd), gSavedCwd);
    SetCurrentDirectory(szPalmDesktopDir);

    
    CmRemoveConduitByCreatorIDPtr   lpfnCmRemoveConduitByCreatorID;
    lpfnCmRemoveConduitByCreatorID = (CmRemoveConduitByCreatorIDPtr) GetProcAddress(hConduitManagerDLL, "CmRemoveConduitByCreatorID");
    if( (lpfnCmRemoveConduitByCreatorID == NULL) )
        return(IDS_ERR_LOADING_CONDMGR);
        CmSetCorePathPtr lpfnCmSetCorePath = (CmSetCorePathPtr) GetProcAddress(hConduitManagerDLL, "CmSetCorePath");
    CmSetHotSyncExePathPtr lpfnCmSetHotSyncExePath = (CmSetHotSyncExePathPtr) GetProcAddress(hConduitManagerDLL, "CmSetHotSyncExecPath");
    CmRestoreHotSyncSettingsPtr lpfnCmRestoreHotSyncSettings;
    lpfnCmRestoreHotSyncSettings = (CmRestoreHotSyncSettingsPtr) GetProcAddress(hConduitManagerDLL, "CmRestoreHotSyncSettings");
    CmGetCreatorValueStringPtr lpfnCmGetCreatorValueString = (CmGetCreatorValueStringPtr) GetProcAddress(hConduitManagerDLL, "CmGetCreatorValueString");
    CmInstallCreatorPtr lpfnCmInstallCreator;
    lpfnCmInstallCreator = (CmInstallCreatorPtr) GetProcAddress(hConduitManagerDLL, "CmInstallCreator");
    CmSetCreatorRemotePtr   lpfnCmSetCreatorRemote;
    lpfnCmSetCreatorRemote = (CmSetCreatorRemotePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorRemote");
    CmSetCreatorNamePtr lpfnCmSetCreatorName;
    lpfnCmSetCreatorName = (CmSetCreatorNamePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorName");
    CmSetCreatorTitlePtr lpfnCmSetCreatorTitle;
    lpfnCmSetCreatorTitle = (CmSetCreatorTitlePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorTitle");
    CmSetCreatorPriorityPtr lpfnCmSetCreatorPriority;
    lpfnCmSetCreatorPriority = (CmSetCreatorPriorityPtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorPriority");
    CmSetCreatorIntegratePtr    lpfnCmSetCreatorIntegrate;
    lpfnCmSetCreatorIntegrate = (CmSetCreatorIntegratePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorIntegrate");
    CmSetCreatorFilePtr lpfnCmSetCreatorFile = (CmSetCreatorFilePtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorFile");
    CmSetCreatorDirectoryPtr lpfnCmSetCreatorDirectory = (CmSetCreatorDirectoryPtr) GetProcAddress(hConduitManagerDLL, "CmSetCreatorDirectory");
    if( (lpfnCmRestoreHotSyncSettings == NULL) )
        return(IDS_ERR_LOADING_CONDMGR);
    
    
    HINSTANCE hHsapiDLL;
    if( (dwReturnCode = LoadHsapiDll(&hHsapiDLL, szPalmDesktopDir)) != 0 )
          return(dwReturnCode);
        
    
    if( (bHotSyncRunning=IsHotSyncRunning(hHsapiDLL)) )
    {
        
        if( IsHotSyncInProgress(hHsapiDLL) )
            return IDS_ERR_HOTSYNC_IN_PROGRESS;
            
        ShutdownHotSync(hHsapiDLL);
    }
    
    TCHAR oldConduitSettings[500];
    int strSize = sizeof(oldConduitSettings);
    dwReturnCode = (*lpfnCmGetCreatorValueString)(CREATOR, "oldConduitSettings", oldConduitSettings, &strSize, "");

    
    dwReturnCode = (*lpfnCmRemoveConduitByCreatorID)(CREATOR);

    if(dwReturnCode >= 0) 
    {
        
        dwReturnCode = UnregisterMozPalmSyncDll();

    }

    if (dwReturnCode >= 0)
    {
      char * szOldCreatorName;
      char *szOldRemote;
      char *szOldCreatorTitle;
      char *szOldCreatorFile;
      char *szOldCreatorDirectory;
      char *oldIntStr;
      DWORD oldPriority;
      DWORD oldIntegrate;
      int   oldType;

      char *strPtr = oldConduitSettings;
      szOldRemote = mystrsep(&strPtr, ',');
      szOldCreatorName = mystrsep(&strPtr, ',');
      szOldCreatorTitle = mystrsep(&strPtr, ',');
      szOldCreatorFile = mystrsep(&strPtr, ',');
      szOldCreatorDirectory = mystrsep(&strPtr, ',');
      oldIntStr = mystrsep(&strPtr, ',');
      oldType = (oldIntStr) ? atoi(oldIntStr) : 0;
      oldIntStr = mystrsep(&strPtr, ',');
      oldPriority = (oldIntStr) ? atoi(oldIntStr) : 0;
      oldIntStr = mystrsep(&strPtr, ',');
      oldIntegrate = (oldIntStr) ? atoi(oldIntStr) : 0;

      dwReturnCode = (*lpfnCmInstallCreator)(CREATOR, oldType);
      if( dwReturnCode == 0 )
      {
          dwReturnCode = (*lpfnCmSetCreatorName)(CREATOR, szOldCreatorName);
          if( dwReturnCode != 0 ) return dwReturnCode;
          
          dwReturnCode = (*lpfnCmSetCreatorTitle)(CREATOR, szOldCreatorTitle);
          if( dwReturnCode != 0 ) return dwReturnCode;
          dwReturnCode = (*lpfnCmSetCreatorRemote)(CREATOR, szOldRemote);
          if( dwReturnCode != 0 ) return dwReturnCode;
          dwReturnCode = (*lpfnCmSetCreatorFile)(CREATOR, szOldCreatorFile);
          if( dwReturnCode != 0 ) return dwReturnCode;
          dwReturnCode = (*lpfnCmSetCreatorDirectory)(CREATOR, szOldCreatorDirectory);
          if( dwReturnCode != 0 ) return dwReturnCode;
          dwReturnCode = (*lpfnCmSetCreatorPriority)(CREATOR, oldPriority);
          if( dwReturnCode != 0 ) return dwReturnCode;
          
          dwReturnCode = (*lpfnCmSetCreatorIntegrate)(CREATOR, oldIntegrate);
      }
    }





    
    
    
#if 0
    HKEY key;
    LONG rc = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\U.S. Robotics\\Pilot Desktop\\Core",
                                0, KEY_ALL_ACCESS, &key);
    if(rc == ERROR_SUCCESS)
        ::RegSetValueEx(key, "Path", 0, REG_SZ, (const BYTE *) szPalmDesktopDir, desktopSize);
 
    if(rc == ERROR_SUCCESS)
        ::RegSetValueEx(key, "HotSyncPath", 0, REG_SZ, (const BYTE *) szPalmHotSyncInstallDir, installSize);
#endif
    
    if( bHotSyncRunning )
        StartHotSync(hHsapiDLL);
        
    
    if (gSavedCwd[0])
      SetCurrentDirectory(gSavedCwd);

    if( dwReturnCode < 0 ) 
        return dwReturnCode;
    else 
        return(0);
}

