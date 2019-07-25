




































#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <wchar.h>

#include "serviceinstall.h"
#include "maintenanceservice.h"
#include "servicebase.h"
#include "workmonitor.h"
#include "shlobj.h"

SERVICE_STATUS gSvcStatus = { 0 }; 
SERVICE_STATUS_HANDLE gSvcStatusHandle = NULL; 
HANDLE ghSvcStopEvent = NULL;
BOOL gServiceStopping = FALSE;


#define LOGS_TO_KEEP 5

BOOL GetLogDirectoryPath(WCHAR *path);

int 
wmain(int argc, WCHAR **argv)
{
  
  
  
  
  
  
  
  
  bool forceInstall = !lstrcmpi(argv[1], L"forceinstall");
  if (!lstrcmpi(argv[1], L"install") || forceInstall) {
    WCHAR updatePath[MAX_PATH + 1];
    if (GetLogDirectoryPath(updatePath)) {
      LogInit(updatePath, L"maintenanceservice-install.log");
    }

    LOG(("Installing service"));
    SvcInstallAction action = InstallSvc;
    if (forceInstall) {
      action = ForceInstallSvc;
      LOG((" with force specified"));
    }
    LOG(("...\n"));

    if (!SvcInstall(action)) {
      LOG(("Could not install service (%d)\n", GetLastError()));
      LogFinish();
      return 1;
    }

    LOG(("The service was installed successfully\n"));
    LogFinish();
    return 0;
  } 

  if (!lstrcmpi(argv[1], L"upgrade")) {
    WCHAR updatePath[MAX_PATH + 1];
    if (GetLogDirectoryPath(updatePath)) {
      LogInit(updatePath, L"maintenanceservice-install.log");
    }
    LOG(("Upgrading service if installed...\n"));
    if (!SvcInstall(UpgradeSvc)) {
      LOG(("Could not upgrade service (%d)\n", GetLastError()));
      LogFinish();
      return 1;
    }

    LOG(("The service was upgraded successfully\n"));
    LogFinish();
    return 0;
  }

  if (!lstrcmpi(argv[1], L"uninstall")) {
    WCHAR updatePath[MAX_PATH + 1];
    if (GetLogDirectoryPath(updatePath)) {
      LogInit(updatePath, L"maintenanceservice-uninstall.log");
    }
    LOG(("Uninstalling service...\n"));
    if (!SvcUninstall()) {
      LOG(("Could not uninstall service (%d)\n", GetLastError()));
      LogFinish();
      return 1;
    }
    LOG(("The service was uninstalled successfully\n"));
    LogFinish();
    return 0;
  }

  SERVICE_TABLE_ENTRYW DispatchTable[] = { 
    { SVC_NAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
    { NULL, NULL } 
  }; 

  
  
  if (!StartServiceCtrlDispatcher(DispatchTable)) {
    LOG(("StartServiceCtrlDispatcher failed (%d)\n", GetLastError()));
  }

  return 0;
}






DWORD
WINAPI StartMonitoringThreadProc(LPVOID param) 
{
  StartDirectoryChangeMonitor();
  return 0;
}







BOOL
GetLogDirectoryPath(WCHAR *path) 
{
  HRESULT hr = SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 
    SHGFP_TYPE_CURRENT, path);
  if (FAILED(hr)) {
    return FALSE;
  }

  if (!PathAppendSafe(path, L"Mozilla")) {
    return FALSE;
  }
  
  
  CreateDirectoryW(path, NULL);

  if (!PathAppendSafe(path, L"logs")) {
    return FALSE;
  }
  CreateDirectoryW(path, NULL);
  return TRUE;
}









BOOL
GetBackupLogPath(LPWSTR path, LPCWSTR basePath, int logNumber)
{
  WCHAR logName[64];
  wcscpy(path, basePath);
  if (logNumber <= 0) {
    swprintf(logName, L"maintenanceservice.log");
  } else {
    swprintf(logName, L"maintenanceservice-%d.log", logNumber);
  }
  return PathAppendSafe(path, logName);
}












void
BackupOldLogs(LPCWSTR basePath, int numLogsToKeep) 
{
  WCHAR oldPath[MAX_PATH + 1];
  WCHAR newPath[MAX_PATH + 1];
  for (int i = numLogsToKeep; i >= 1; i--) {
    if (!GetBackupLogPath(oldPath, basePath, i -1)) {
      continue;
    }

    if (!GetBackupLogPath(newPath, basePath, i)) {
      continue;
    }

    if (!MoveFileEx(oldPath, newPath, MOVEFILE_REPLACE_EXISTING)) {
      continue;
    }
  }
}




void WINAPI 
SvcMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
  
  WCHAR updatePath[MAX_PATH + 1];
  if (GetLogDirectoryPath(updatePath)) {
    BackupOldLogs(updatePath, LOGS_TO_KEEP);
    LogInit(updatePath, L"maintenanceservice.log");
  }

  
  gSvcStatusHandle = RegisterServiceCtrlHandlerW(SVC_NAME, SvcCtrlHandler);
  if (!gSvcStatusHandle) {
    LOG(("RegisterServiceCtrlHandler failed (%d)\n", GetLastError()));
    return; 
  } 

  
  gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  gSvcStatus.dwServiceSpecificExitCode = 0;

  
  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  
  SvcInit(dwArgc, lpszArgv);
}




void
SvcInit(DWORD dwArgc, LPWSTR *lpszArgv)
{
  
  
  ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (NULL == ghSvcStopEvent) {
    ReportSvcStatus(SERVICE_STOPPED, 1, 0);
    return;
  }

  DWORD threadID;
  HANDLE thread = CreateThread(NULL, 0, StartMonitoringThreadProc, 0, 
                               0, &threadID);

  
  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  
  for(;;) {
    
    WaitForSingleObject(ghSvcStopEvent, INFINITE);

    WCHAR stopFilePath[MAX_PATH +1];
    if (!GetUpdateDirectoryPath(stopFilePath)) {
      LOG(("Could not obtain update directory path, terminating thread "
           "forcefully.\n"));
      TerminateThread(thread, 1);
    }

    
    
    
    gServiceStopping = TRUE;
    if (!PathAppendSafe(stopFilePath, L"stop")) {
      TerminateThread(thread, 2);
    }
    HANDLE stopFile = CreateFile(stopFilePath, GENERIC_READ, 0, 
                                 NULL, CREATE_ALWAYS, 0, NULL);
    if (stopFile == INVALID_HANDLE_VALUE) {
      LOG(("Could not create stop file, terminating thread forcefully.\n"));
      TerminateThread(thread, 3);
    } else {
      CloseHandle(stopFile);
      DeleteFile(stopFilePath);
    }

    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    return;
  }
}








void
ReportSvcStatus(DWORD currentState, 
                DWORD exitCode, 
                DWORD waitHint)
{
  static DWORD dwCheckPoint = 1;

  
  gSvcStatus.dwCurrentState = currentState;
  gSvcStatus.dwWin32ExitCode = exitCode;
  gSvcStatus.dwWaitHint = waitHint;

  if (SERVICE_START_PENDING == currentState) {
    gSvcStatus.dwControlsAccepted = 0;
  } else {
    gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  }

  if ((SERVICE_RUNNING == currentState) ||
      (SERVICE_STOPPED == currentState)) {
    gSvcStatus.dwCheckPoint = 0;
  } else {
    gSvcStatus.dwCheckPoint = dwCheckPoint++;
  }

  
  SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}





void WINAPI
SvcCtrlHandler(DWORD dwCtrl)
{
  
  switch(dwCtrl) {
  case SERVICE_CONTROL_STOP: 
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
    
    SetEvent(ghSvcStopEvent);
    ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
    LogFinish();
    break;
  case SERVICE_CONTROL_INTERROGATE: 
    break; 
  default: 
    break;
  }
}
