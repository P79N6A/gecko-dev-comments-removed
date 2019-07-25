




































#include <windows.h>
#include <stdio.h>
#include "shlobj.h"
#include "updatehelper.h"


#include <shlwapi.h>

#include <tlhelp32.h>
#pragma comment(lib, "shlwapi.lib") 

WCHAR* MakeCommandLine(int argc, WCHAR **argv);
BOOL PathAppendSafe(LPWSTR base, LPCWSTR extra);









BOOL
PathGetSiblingFilePath(LPWSTR destinationBuffer, 
                       LPCWSTR siblingFilePath, 
                       LPCWSTR newFileName)
{
  if (wcslen(siblingFilePath) >= MAX_PATH) {
    return FALSE;
  }

  wcscpy(destinationBuffer, siblingFilePath);
  if (!PathRemoveFileSpecW(destinationBuffer)) {
    return FALSE;
  }

  if (wcslen(destinationBuffer) + wcslen(newFileName) >= MAX_PATH) {
    return FALSE;
  }

  return PathAppendSafe(destinationBuffer, newFileName);
}















BOOL
LaunchWinPostProcess(const WCHAR *installationDir,
                     const WCHAR *updateInfoDir,
                     bool forceSync,
                     HANDLE userToken)
{
  WCHAR workingDirectory[MAX_PATH + 1];
  wcscpy(workingDirectory, installationDir);

  
  
  WCHAR inifile[MAX_PATH + 1];
  wcscpy(inifile, installationDir);
  if (!PathAppendSafe(inifile, L"updater.ini")) {
    return FALSE;
  }

  WCHAR exefile[MAX_PATH + 1];
  WCHAR exearg[MAX_PATH + 1];
  WCHAR exeasync[10];
  bool async = true;
  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeRelPath", NULL, exefile,
                                MAX_PATH + 1, inifile)) {
    return FALSE;
  }

  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeArg", NULL, exearg,
                                MAX_PATH + 1, inifile)) {
    return FALSE;
  }

  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeAsync", L"TRUE", 
                                exeasync,
                                sizeof(exeasync)/sizeof(exeasync[0]), 
                                inifile)) {
    return FALSE;
  }

  WCHAR exefullpath[MAX_PATH + 1];
  wcscpy(exefullpath, installationDir);
  if (!PathAppendSafe(exefullpath, exefile)) {
    return false;
  }

  WCHAR dlogFile[MAX_PATH + 1];
  if (!PathGetSiblingFilePath(dlogFile, exefullpath, L"uninstall.update")) {
    return FALSE;
  }

  WCHAR slogFile[MAX_PATH + 1];
  wcscpy(slogFile, updateInfoDir);
  if (!PathAppendSafe(slogFile, L"update.log")) {
    return FALSE;
  }

  WCHAR dummyArg[14];
  wcscpy(dummyArg, L"argv0ignored ");

  size_t len = wcslen(exearg) + wcslen(dummyArg);
  WCHAR *cmdline = (WCHAR *) malloc((len + 1) * sizeof(WCHAR));
  if (!cmdline) {
    return FALSE;
  }

  wcscpy(cmdline, dummyArg);
  wcscat(cmdline, exearg);

  if (forceSync ||
      !_wcsnicmp(exeasync, L"false", 6) || 
      !_wcsnicmp(exeasync, L"0", 2)) {
    async = false;
  }
  
  
  
  
  CopyFileW(slogFile, dlogFile, false);

  STARTUPINFOW si = {sizeof(si), 0};
  si.lpDesktop = L"";
  PROCESS_INFORMATION pi = {0};

  bool ok;
  if (userToken) {
    ok = CreateProcessAsUserW(userToken,
                              exefullpath,
                              cmdline,
                              NULL,  
                              NULL,  
                              false, 
                              0,     
                              NULL,  
                              workingDirectory,
                              &si,
                              &pi);
  } else {
    ok = CreateProcessW(exefullpath,
                        cmdline,
                        NULL,  
                        NULL,  
                        false, 
                        0,     
                        NULL,  
                        workingDirectory,
                        &si,
                        &pi);
  }
  free(cmdline);
  if (ok) {
    if (!async)
      WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  return ok;
}









BOOL
StartServiceUpdate(int argc, LPWSTR *argv)
{
  if (argc < 2) {
    return FALSE;
  }

  
  SC_HANDLE manager = OpenSCManager(NULL, NULL, 
                                    SC_MANAGER_ALL_ACCESS);
  if (!manager) {
    return FALSE;
  }

  
  SC_HANDLE svc = OpenServiceW(manager, SVC_NAME, 
                               SERVICE_ALL_ACCESS);
  if (!svc) {
    CloseServiceHandle(manager);
    return FALSE;
  }
  CloseServiceHandle(svc);
  CloseServiceHandle(manager);

  
  

  STARTUPINFOW si = {0};
  si.cb = sizeof(STARTUPINFOW);
  
  si.lpDesktop = L"";
  PROCESS_INFORMATION pi = {0};

  WCHAR maintserviceInstallerPath[MAX_PATH + 1];
  wcscpy(maintserviceInstallerPath, argv[2]);
  PathAppendSafe(maintserviceInstallerPath, 
                 L"maintenanceservice_installer.exe");
  WCHAR cmdLine[64];
  wcscpy(cmdLine, L"dummyparam.exe /Upgrade");
  BOOL svcUpdateProcessStarted = CreateProcessW(maintserviceInstallerPath, 
                                                cmdLine, 
                                                NULL, NULL, FALSE, 
                                                0, 
                                                NULL, argv[2], &si, &pi);
  if (svcUpdateProcessStarted) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  return svcUpdateProcessStarted;
}













DWORD
StartServiceCommand(int argc, LPCWSTR* argv)
{
  DWORD lastState = WaitForServiceStop(SVC_NAME, 5);
  if (lastState != SERVICE_STOPPED) {
    return 20000 + lastState;
  }

  
  SC_HANDLE serviceManager = OpenSCManager(NULL, NULL, 
                                           SC_MANAGER_CONNECT | 
                                           SC_MANAGER_ENUMERATE_SERVICE);
  if (!serviceManager)  {
    return 17001;
  }

  
  SC_HANDLE service = OpenServiceW(serviceManager, 
                                   SVC_NAME, 
                                   SERVICE_START);
  if (!service) {
    CloseServiceHandle(serviceManager);
    return 17002;
  }

  
  
  const DWORD maxWaitMS = 5000;
  DWORD currentWaitMS = 0;
  DWORD lastError = ERROR_SUCCESS;
  while (currentWaitMS < maxWaitMS) {
    BOOL result = StartServiceW(service, argc, argv);
    if (result) {
      lastError = ERROR_SUCCESS;
      break;
    } else {
      lastError = GetLastError();
    }
    Sleep(100);
    currentWaitMS += 100;
  }
  CloseServiceHandle(service);
  CloseServiceHandle(serviceManager);
  return lastError;
}











DWORD
LaunchServiceSoftwareUpdateCommand(DWORD argc, LPCWSTR* argv)
{
  
  
  
  LPCWSTR *updaterServiceArgv = new LPCWSTR[argc + 2];
  updaterServiceArgv[0] = L"MozillaMaintenance";
  updaterServiceArgv[1] = L"software-update";

  for (int i = 0; i < argc; ++i) {
    updaterServiceArgv[i + 2] = argv[i];
  }

  
  
  DWORD ret = StartServiceCommand(argc + 2, updaterServiceArgv);
  delete[] updaterServiceArgv;
  return ret;
}








BOOL
PathAppendSafe(LPWSTR base, LPCWSTR extra)
{
  if (wcslen(base) + wcslen(extra) >= MAX_PATH) {
    return FALSE;
  }

  return PathAppendW(base, extra);
}








BOOL
WriteStatusPending(LPCWSTR updateDirPath)
{
  WCHAR updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    return FALSE;
  }

  const char pending[] = "pending";
  HANDLE statusFile = CreateFileW(updateStatusFilePath, GENERIC_WRITE, 0, 
                                  NULL, CREATE_ALWAYS, 0, NULL);
  if (statusFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  DWORD wrote;
  BOOL ok = WriteFile(statusFile, pending, 
                      sizeof(pending) - 1, &wrote, NULL); 
  CloseHandle(statusFile);
  return ok && (wrote == sizeof(pending) - 1);
}







BOOL
WriteStatusFailure(LPCWSTR updateDirPath, int errorCode)
{
  WCHAR updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    return FALSE;
  }

  HANDLE statusFile = CreateFileW(updateStatusFilePath, GENERIC_WRITE, 0, 
                                  NULL, CREATE_ALWAYS, 0, NULL);
  if (statusFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  char failure[32];
  sprintf(failure, "failed: %d", errorCode);

  DWORD toWrite = strlen(failure);
  DWORD wrote;
  BOOL ok = WriteFile(statusFile, failure, 
                      toWrite, &wrote, NULL); 
  CloseHandle(statusFile);
  return ok && wrote == toWrite;
}

































DWORD
WaitForServiceStop(LPCWSTR serviceName, DWORD maxWaitSeconds)
{
  
  DWORD lastServiceState = 0x000000CF;

  
  SC_HANDLE serviceManager = OpenSCManager(NULL, NULL, 
                                           SC_MANAGER_CONNECT | 
                                           SC_MANAGER_ENUMERATE_SERVICE);
  if (!serviceManager)  {
    DWORD lastError = GetLastError();
    switch(lastError) {
    case ERROR_ACCESS_DENIED:
      return 0x000000FD;
    case ERROR_DATABASE_DOES_NOT_EXIST:
      return 0x000000FE;
    default:
      return 0x000000FF;
    }
  }

  
  SC_HANDLE service = OpenServiceW(serviceManager, 
                                   serviceName, 
                                   SERVICE_QUERY_STATUS);
  if (!service) {
    DWORD lastError = GetLastError();
    CloseServiceHandle(serviceManager);
    switch(lastError) {
    case ERROR_ACCESS_DENIED:
      return 0x000000EB;
    case ERROR_INVALID_HANDLE:
      return 0x000000EC;
    case ERROR_INVALID_NAME:
      return 0x000000ED;
    case ERROR_SERVICE_DOES_NOT_EXIST:
      return 0x000000EE;
    default:
      return 0x000000EF;
    } 
  }

  DWORD currentWaitMS = 0;
  SERVICE_STATUS_PROCESS ssp;
  ssp.dwCurrentState = lastServiceState;
  while (currentWaitMS < maxWaitSeconds * 1000) {
    DWORD bytesNeeded;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                              sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
      DWORD lastError = GetLastError();
      switch (lastError) {
      case ERROR_INVALID_HANDLE:
        ssp.dwCurrentState = 0x000000D9;
        break;
      case ERROR_ACCESS_DENIED:
        ssp.dwCurrentState = 0x000000DA;
        break;
      case ERROR_INSUFFICIENT_BUFFER:
        ssp.dwCurrentState = 0x000000DB;
        break;
      case ERROR_INVALID_PARAMETER:
        ssp.dwCurrentState = 0x000000DC;
        break;
      case ERROR_INVALID_LEVEL:
        ssp.dwCurrentState = 0x000000DD;
        break;
      case ERROR_SHUTDOWN_IN_PROGRESS:
        ssp.dwCurrentState = 0x000000DE;
        break;
      
      
      case ERROR_INVALID_SERVICE_CONTROL:
      case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
      case ERROR_SERVICE_NOT_ACTIVE:
        currentWaitMS += 50;
        Sleep(50);
        continue;
      default:
        ssp.dwCurrentState = 0x000000DF;
      }

      
      break;
    }

    
    if (ssp.dwCurrentState == SERVICE_STOPPED) {
      break;
    }
    currentWaitMS += 50;
    Sleep(50);
  }

  lastServiceState = ssp.dwCurrentState;
  CloseServiceHandle(service);
  CloseServiceHandle(serviceManager);
  return lastServiceState;
}










DWORD
IsProcessRunning(LPCWSTR filename)
{
  
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (INVALID_HANDLE_VALUE == snapshot) {
    return GetLastError();
  }
  
  PROCESSENTRY32W processEntry;
  processEntry.dwSize = sizeof(PROCESSENTRY32W);
  if (!Process32FirstW(snapshot, &processEntry)) {
    DWORD lastError = GetLastError();
    CloseHandle(snapshot);
    return lastError;
  }

  do {
    if (wcsicmp(filename, processEntry.szExeFile) == 0) {
      CloseHandle(snapshot);
      return ERROR_SUCCESS;
    }
  } while (Process32NextW(snapshot, &processEntry));
  CloseHandle(snapshot);
  return ERROR_NOT_FOUND;
}











DWORD
WaitForProcessExit(LPCWSTR filename, DWORD maxSeconds)
{
  DWORD applicationRunningError = WAIT_TIMEOUT;
  for(DWORD i = 0; i < maxSeconds; i++) {
    DWORD applicationRunningError = IsProcessRunning(filename);
    if (ERROR_NOT_FOUND == applicationRunningError) {
      return ERROR_SUCCESS;
    }
    Sleep(1000);
  }

  if (ERROR_SUCCESS == applicationRunningError) {
    return WAIT_TIMEOUT;
  }

  return applicationRunningError;
}
