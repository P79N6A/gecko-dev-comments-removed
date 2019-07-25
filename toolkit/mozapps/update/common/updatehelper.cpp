




































#include <windows.h>
#include <stdio.h>
#include "shlobj.h"
#include "updatehelper.h"


#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") 

WCHAR*
MakeCommandLine(int argc, WCHAR **argv);
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
                                                CREATE_DEFAULT_ERROR_MODE | 
                                                CREATE_UNICODE_ENVIRONMENT, 
                                                NULL, argv[2], &si, &pi);
  if (svcUpdateProcessStarted) {
    
    
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  return svcUpdateProcessStarted;
}









BOOL 
StartServiceCommand(int argc, LPCWSTR* argv) 
{
  
  SC_HANDLE serviceManager = OpenSCManager(NULL, NULL, 
                                           SC_MANAGER_CONNECT | 
                                           SC_MANAGER_ENUMERATE_SERVICE);
  if (!serviceManager)  {
    return FALSE;
  }

  
  SC_HANDLE service = OpenServiceW(serviceManager, 
                                   SVC_NAME, 
                                   SERVICE_QUERY_STATUS | SERVICE_START);
  if (!service) {
    CloseServiceHandle(serviceManager);
    return FALSE;
  }

  
  SERVICE_STATUS_PROCESS ssp;
  DWORD bytesNeeded;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                            sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
    CloseServiceHandle(service);
    CloseServiceHandle(serviceManager);
    return FALSE;
  }

  
  if (ssp.dwCurrentState != SERVICE_STOPPED) {
    CloseServiceHandle(service);
    CloseServiceHandle(serviceManager);
    return FALSE;
  }

  return StartServiceW(service, argc, argv);
}











BOOL
LaunchServiceSoftwareUpdateCommand(DWORD argc, LPCWSTR* argv)
{
  
  
  
  LPCWSTR *updaterServiceArgv = new LPCWSTR[argc + 2];
  updaterServiceArgv[0] = L"maintenanceservice.exe";
  updaterServiceArgv[1] = L"software-update";

  for (int i = 0; i < argc; ++i) {
    updaterServiceArgv[i + 2] = argv[i];
  }

  
  
  BOOL result = StartServiceCommand(argc + 2, updaterServiceArgv);
  delete[] updaterServiceArgv;
  return result;
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











BOOL
WaitForServiceStop(LPCWSTR serviceName, DWORD maxWaitSeconds) 
{
  
  SC_HANDLE serviceManager = OpenSCManager(NULL, NULL, 
                                           SC_MANAGER_CONNECT | 
                                           SC_MANAGER_ENUMERATE_SERVICE);
  if (!serviceManager)  {
    return FALSE;
  }

  
  SC_HANDLE service = OpenServiceW(serviceManager, 
                                   serviceName, 
                                   SERVICE_QUERY_STATUS);
  if (!service) {
    CloseServiceHandle(serviceManager);
    return FALSE;
  }

  BOOL gotStop = FALSE;
  DWORD currentWaitMS = 0;
  while (currentWaitMS < maxWaitSeconds * 1000) {
    
    SERVICE_STATUS_PROCESS ssp;
    DWORD bytesNeeded;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                              sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
      break;
    }

    
    if (ssp.dwCurrentState == SERVICE_STOPPED) {
      gotStop = TRUE;
      break;
    }
    currentWaitMS += 50;
    Sleep(50);
  }

  CloseServiceHandle(service);
  CloseServiceHandle(serviceManager);
  return gotStop;
}
