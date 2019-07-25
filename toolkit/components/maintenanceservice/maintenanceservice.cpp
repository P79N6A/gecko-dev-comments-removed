




































#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <wchar.h>
#include <shlobj.h>

#include "serviceinstall.h"
#include "maintenanceservice.h"
#include "servicebase.h"
#include "workmonitor.h"
#include "uachelper.h"
#include "updatehelper.h"

SERVICE_STATUS gSvcStatus = { 0 }; 
SERVICE_STATUS_HANDLE gSvcStatusHandle = NULL; 
HANDLE gWorkDoneEvent = NULL;
HANDLE gThread = NULL;
bool gServiceControlStopping = false;


#define LOGS_TO_KEEP 10

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
    { SVC_NAME, (LPSERVICE_MAIN_FUNCTIONW) SvcMain }, 
    { NULL, NULL } 
  }; 

  
  
  if (!StartServiceCtrlDispatcherW(DispatchTable)) {
    LOG(("StartServiceCtrlDispatcher failed (%d)\n", GetLastError()));
  }

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

    if (!MoveFileExW(oldPath, newPath, MOVEFILE_REPLACE_EXISTING)) {
      continue;
    }
  }
}















DWORD WINAPI
EnsureProcessTerminatedThread(LPVOID)
{
  Sleep(5000);
  exit(0);
  return 0;
}

void
StartTerminationThread()
{
  
  
  HANDLE thread = CreateThread(NULL, 0, EnsureProcessTerminatedThread, 
                               NULL, 0, NULL);
  if (thread) {
    CloseHandle(thread);
  }
}




void WINAPI
SvcMain(DWORD argc, LPWSTR *argv)
{
  
  WCHAR updatePath[MAX_PATH + 1];
  if (GetLogDirectoryPath(updatePath)) {
    BackupOldLogs(updatePath, LOGS_TO_KEEP);
    LogInit(updatePath, L"maintenanceservice.log");
  }

  
  
  UACHelper::DisablePrivileges(NULL);

  
  gSvcStatusHandle = RegisterServiceCtrlHandlerW(SVC_NAME, SvcCtrlHandler);
  if (!gSvcStatusHandle) {
    LOG(("RegisterServiceCtrlHandler failed (%d)\n", GetLastError()));
    ExecuteServiceCommand(argc, argv);  
    LogFinish();
    exit(1);
  } 

  
  gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  gSvcStatus.dwServiceSpecificExitCode = 0;

  
  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  
  
  gWorkDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (!gWorkDoneEvent) {
    ReportSvcStatus(SERVICE_STOPPED, 1, 0);
    StartTerminationThread();
    return;
  }

  
  
  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  
  
  
  ExecuteServiceCommand(argc, argv);  
  LogFinish();

  SetEvent(gWorkDoneEvent);

  
  
  
  if (!gServiceControlStopping) {
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    StartTerminationThread();
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

  if (SERVICE_START_PENDING == currentState || 
      SERVICE_STOP_PENDING == currentState) {
    gSvcStatus.dwControlsAccepted = 0;
  } else {
    gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
                                    SERVICE_ACCEPT_SHUTDOWN;
  }

  if ((SERVICE_RUNNING == currentState) ||
      (SERVICE_STOPPED == currentState)) {
    gSvcStatus.dwCheckPoint = 0;
  } else {
    gSvcStatus.dwCheckPoint = dwCheckPoint++;
  }

  
  SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}





DWORD WINAPI
StopServiceAndWaitForCommandThread(LPVOID)
{
  do {
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);
  } while(WaitForSingleObject(gWorkDoneEvent, 100) == WAIT_TIMEOUT);
  CloseHandle(gWorkDoneEvent);
  gWorkDoneEvent = NULL;
  ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
  StartTerminationThread();
  return 0;
}





void WINAPI
SvcCtrlHandler(DWORD dwCtrl)
{
  
  
  if (gServiceControlStopping) {
    return;
  }

  
  switch(dwCtrl) {
  case SERVICE_CONTROL_SHUTDOWN:
  case SERVICE_CONTROL_STOP: {
      gServiceControlStopping = true;
      ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);

      
      
      HANDLE thread = CreateThread(NULL, 0, StopServiceAndWaitForCommandThread, 
                                   NULL, 0, NULL);
      if (thread) {
        CloseHandle(thread);
      } else {
        
        
        
        StopServiceAndWaitForCommandThread(NULL);
      }
    }
    break;
  default:
    break;
  }
}
