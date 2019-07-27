



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
#include "registrycertificates.h"



#pragma comment(linker, "/SUBSYSTEM:windows")

SERVICE_STATUS gSvcStatus = { 0 }; 
SERVICE_STATUS_HANDLE gSvcStatusHandle = nullptr; 
HANDLE gWorkDoneEvent = nullptr;
HANDLE gThread = nullptr;
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

    SvcInstallAction action = InstallSvc;
    if (forceInstall) {
      action = ForceInstallSvc;
      LOG(("Installing service with force specified..."));
    } else {
      LOG(("Installing service..."));
    }

    bool ret = SvcInstall(action);
    if (!ret) {
      LOG_WARN(("Could not install service.  (%d)", GetLastError()));
      LogFinish();
      return 1;
    }

    LOG(("The service was installed successfully"));
    LogFinish();
    return 0;
  } 

  if (!lstrcmpi(argv[1], L"upgrade")) {
    WCHAR updatePath[MAX_PATH + 1];
    if (GetLogDirectoryPath(updatePath)) {
      LogInit(updatePath, L"maintenanceservice-install.log");
    }

    LOG(("Upgrading service if installed..."));
    if (!SvcInstall(UpgradeSvc)) {
      LOG_WARN(("Could not upgrade service.  (%d)", GetLastError()));
      LogFinish();
      return 1;
    }

    LOG(("The service was upgraded successfully"));
    LogFinish();
    return 0;
  }

  if (!lstrcmpi(argv[1], L"uninstall")) {
    WCHAR updatePath[MAX_PATH + 1];
    if (GetLogDirectoryPath(updatePath)) {
      LogInit(updatePath, L"maintenanceservice-uninstall.log");
    }
    LOG(("Uninstalling service..."));
    if (!SvcUninstall()) {
      LOG_WARN(("Could not uninstall service.  (%d)", GetLastError()));
      LogFinish();
      return 1;
    }
    LOG(("The service was uninstalled successfully"));
    LogFinish();
    return 0;
  }

  if (!lstrcmpi(argv[1], L"check-cert") && argc > 2) {
    return DoesBinaryMatchAllowedCertificates(argv[2], argv[3], FALSE) ? 0 : 1;
  }

  SERVICE_TABLE_ENTRYW DispatchTable[] = { 
    { SVC_NAME, (LPSERVICE_MAIN_FUNCTIONW) SvcMain }, 
    { nullptr, nullptr } 
  }; 

  
  
  if (!StartServiceCtrlDispatcherW(DispatchTable)) {
    LOG_WARN(("StartServiceCtrlDispatcher failed.  (%d)", GetLastError()));
  }

  return 0;
}







BOOL
GetLogDirectoryPath(WCHAR *path)
{
  HRESULT hr = SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, 
    SHGFP_TYPE_CURRENT, path);
  if (FAILED(hr)) {
    return FALSE;
  }

  if (!PathAppendSafe(path, L"Mozilla")) {
    return FALSE;
  }
  
  
  CreateDirectoryW(path, nullptr);

  if (!PathAppendSafe(path, L"logs")) {
    return FALSE;
  }
  CreateDirectoryW(path, nullptr);
  return TRUE;
}









BOOL
GetBackupLogPath(LPWSTR path, LPCWSTR basePath, int logNumber)
{
  WCHAR logName[64] = { L'\0' };
  wcsncpy(path, basePath, sizeof(logName) / sizeof(logName[0]) - 1);
  if (logNumber <= 0) {
    swprintf(logName, sizeof(logName) / sizeof(logName[0]),
             L"maintenanceservice.log");
  } else {
    swprintf(logName, sizeof(logName) / sizeof(logName[0]),
             L"maintenanceservice-%d.log", logNumber);
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
  
  
  HANDLE thread = CreateThread(nullptr, 0, EnsureProcessTerminatedThread,
                               nullptr, 0, nullptr);
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

  
  
  UACHelper::DisablePrivileges(nullptr);

  
  gSvcStatusHandle = RegisterServiceCtrlHandlerW(SVC_NAME, SvcCtrlHandler);
  if (!gSvcStatusHandle) {
    LOG_WARN(("RegisterServiceCtrlHandler failed.  (%d)", GetLastError()));
    ExecuteServiceCommand(argc, argv);  
    LogFinish();
    exit(1);
  } 

  
  gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  gSvcStatus.dwServiceSpecificExitCode = 0;

  
  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  
  
  gWorkDoneEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
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
  gWorkDoneEvent = nullptr;
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

      
      
      HANDLE thread = CreateThread(nullptr, 0,
                                   StopServiceAndWaitForCommandThread,
                                   nullptr, 0, nullptr);
      if (thread) {
        CloseHandle(thread);
      } else {
        
        
        
        StopServiceAndWaitForCommandThread(nullptr);
      }
    }
    break;
  default:
    break;
  }
}
