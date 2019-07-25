



#include <shlobj.h>
#include <shlwapi.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <shellapi.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "rpcrt4.lib")

#include "nsWindowsHelpers.h"
#include "nsAutoPtr.h"

#include "workmonitor.h"
#include "serviceinstall.h"
#include "servicebase.h"
#include "registrycertificates.h"
#include "uachelper.h"
#include "updatehelper.h"
#include "errors.h"
#include "prefetch.h"




static const int TIME_TO_WAIT_ON_UPDATER = 15 * 60 * 1000;
PRUnichar* MakeCommandLine(int argc, PRUnichar **argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL PathGetSiblingFilePath(LPWSTR destinationBuffer,  LPCWSTR siblingFilePath, 
                            LPCWSTR newFileName);










static BOOL
IsStatusApplying(LPCWSTR updateDirPath, BOOL &isApplying)
{
  isApplying = FALSE;
  WCHAR updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    LOG(("Warning: Could not append path for update.status file\n"));
    return FALSE;
  }

  nsAutoHandle statusFile(CreateFileW(updateStatusFilePath, GENERIC_READ,
                                      FILE_SHARE_READ | 
                                      FILE_SHARE_WRITE | 
                                      FILE_SHARE_DELETE,
                                      NULL, OPEN_EXISTING, 0, NULL));

  if (INVALID_HANDLE_VALUE == statusFile) {
    LOG(("Warning: Could not open update.status file\n"));
    return FALSE;
  }

  char buf[32] = { 0 };
  DWORD read;
  if (!ReadFile(statusFile, buf, sizeof(buf), &read, NULL)) {
    LOG(("Warning: Could not read from update.status file\n"));
    return FALSE;
  }

  LOG(("updater.exe returned status: %s\n", buf));

  const char kApplying[] = "applying";
  isApplying = strncmp(buf, kApplying, 
                       sizeof(kApplying) - 1) == 0;
  return TRUE;
}









static BOOL
GetInstallationDir(int argcTmp, LPWSTR *argvTmp, WCHAR aResultDir[MAX_PATH])
{
  if (argcTmp < 2) {
    return FALSE;
  }
  wcscpy(aResultDir, argvTmp[2]);
  WCHAR* backSlash = wcsrchr(aResultDir, L'\\');
  
  if (backSlash && (backSlash[1] == L'\0')) {
    *backSlash = L'\0';
  }
  
  bool backgroundUpdate = (argcTmp == 4 && !wcscmp(argvTmp[3], L"-1"));
  bool replaceRequest = (argcTmp >= 4 && wcsstr(argvTmp[3], L"/replace"));
  if (backgroundUpdate || replaceRequest) {
    return PathRemoveFileSpecW(aResultDir);
  }
  return TRUE;
}










BOOL
StartUpdateProcess(int argc,
                   LPWSTR *argv,
                   BOOL &processStarted)
{
  LOG(("Starting update process as the service in session 0.\n"));
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);
  si.lpDesktop = L"winsta0\\Default";
  PROCESS_INFORMATION pi = {0};

  
  
  LPWSTR cmdLine = MakeCommandLine(argc, argv);

  
  
  
  
  if (argc >= 2 ) {
    
    si.lpDesktop = L"";
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
  }

  
  
  
  
  
  
  WCHAR updaterINI[MAX_PATH + 1];
  WCHAR updaterINITemp[MAX_PATH + 1];
  BOOL selfHandlePostUpdate = FALSE;
  
  
  if (PathGetSiblingFilePath(updaterINI, argv[0], L"updater.ini") &&
      PathGetSiblingFilePath(updaterINITemp, argv[0], L"updater.tmp")) {
    selfHandlePostUpdate = MoveFileExW(updaterINI, updaterINITemp, 
                                       MOVEFILE_REPLACE_EXISTING);
  }

  
  
  
  putenv(const_cast<char*>("MOZ_USING_SERVICE=1"));
  LOG(("Starting service with cmdline: %ls\n", cmdLine));
  processStarted = CreateProcessW(argv[0], cmdLine, 
                                  NULL, NULL, FALSE, 
                                  CREATE_DEFAULT_ERROR_MODE, 
                                  NULL, 
                                  NULL, &si, &pi);
  
  putenv(const_cast<char*>("MOZ_USING_SERVICE="));
  
  BOOL updateWasSuccessful = FALSE;
  if (processStarted) {
    
    LOG(("Process was started... waiting on result.\n")); 
    DWORD waitRes = WaitForSingleObject(pi.hProcess, TIME_TO_WAIT_ON_UPDATER);
    if (WAIT_TIMEOUT == waitRes) {
      
      
      TerminateProcess(pi.hProcess, 1);
    } else {
      
      DWORD returnCode;
      if (GetExitCodeProcess(pi.hProcess, &returnCode)) {
        LOG(("Process finished with return code %d.\n", returnCode)); 
        
        updateWasSuccessful = (returnCode == 0);
      } else {
        LOG(("Process finished but could not obtain return code.\n")); 
      }
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    
    
    BOOL isApplying = FALSE;
    if (IsStatusApplying(argv[1], isApplying) && isApplying) {
      if (updateWasSuccessful) {
        LOG(("update.status is still applying even know update "
             " was successful.\n"));
        if (!WriteStatusFailure(argv[1], 
                                SERVICE_STILL_APPLYING_ON_SUCCESS)) {
          LOG(("Could not write update.status still applying on"
               " success error.\n"));
        }
        
        
        updateWasSuccessful = FALSE;
      } else {
        LOG(("update.status is still applying and update was not successful.\n"));
        if (!WriteStatusFailure(argv[1], 
                                SERVICE_STILL_APPLYING_ON_FAILURE)) {
          LOG(("Could not write update.status still applying on"
               " success error.\n"));
        }
      }
    }
  } else {
    DWORD lastError = GetLastError();
    LOG(("Could not create process as current user, "
         "updaterPath: %ls; cmdLine: %l.  (%d)\n", 
         argv[0], cmdLine, lastError));
  }

  
  
  
  if (selfHandlePostUpdate) {
    MoveFileExW(updaterINITemp, updaterINI, MOVEFILE_REPLACE_EXISTING);

    
    if (updateWasSuccessful && argc > 2) {
      LPCWSTR installationDir = argv[2];
      LPCWSTR updateInfoDir = argv[1];

      
      
      
      
      
      
      
      LOG(("Launching post update process as the service in session 0.\n"));
      if (!LaunchWinPostProcess(installationDir, updateInfoDir, true, NULL)) {
        LOG(("The post update process could not be launched.\n"));
      }
    }
  }

  free(cmdLine);
  return updateWasSuccessful;
}









BOOL
ProcessSoftwareUpdateCommand(DWORD argc, LPWSTR *argv)
{
  BOOL result = TRUE;
  if (argc < 3) {
    LOG(("Not enough command line parameters specified. "
         "Updating update.status.\n"));

    
    
    if (argc > 1 || 
        !WriteStatusFailure(argv[1], 
                            SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS)) {
      LOG(("Could not write update.status service update failure."
           "Last error: %d\n", GetLastError()));
    }
    return FALSE;
  }

  WCHAR installDir[MAX_PATH] = {L'\0'};
  if (!GetInstallationDir(argc, argv, installDir)) {
    LOG(("Could not get the installation directory"));
    if (!WriteStatusFailure(argv[1],
                            SERVICE_INSTALLDIR_ERROR)) {
      LOG(("Could not write update.status for GetInstallationDir failure.\n"));
    }
    return FALSE;
  }

  
  
  
  BOOL isLocal = FALSE;
  if (!IsLocalFile(argv[0], isLocal) || !isLocal) {
    LOG(("Filesystem in path %ls is not supported"
         "Last error: %d\n", argv[0], GetLastError()));
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_NOT_FIXED_DRIVE)) {
      LOG(("Could not write update.status service update failure."
           "Last error: %d\n", GetLastError()));
    }
    return FALSE;
  }

  nsAutoHandle noWriteLock(CreateFileW(argv[0], GENERIC_READ, FILE_SHARE_READ, 
                                       NULL, OPEN_EXISTING, 0, NULL));
  if (INVALID_HANDLE_VALUE == noWriteLock) {
      LOG(("Could not set no write sharing access on file."
           "Last error: %d\n", GetLastError()));
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_COULD_NOT_LOCK_UPDATER)) {
      LOG(("Could not write update.status service update failure."
           "Last error: %d\n", GetLastError()));
    }
    return FALSE;
  }

  
  
  
  WCHAR installDirUpdater[MAX_PATH + 1] = {L'\0'};
  wcsncpy(installDirUpdater, installDir, MAX_PATH);
  if (!PathAppendSafe(installDirUpdater, L"updater.exe")) {
    LOG(("Install directory updater could not be determined.\n"));
    result = FALSE;
  }

  BOOL updaterIsCorrect;
  if (result && !VerifySameFiles(argv[0], installDirUpdater, 
                                 updaterIsCorrect)) {
    LOG(("Error checking if the updaters are the same.\n"
         "Path 1: %ls\nPath 2: %ls\n", argv[0], installDirUpdater));
    result = FALSE;
  }

  if (result && !updaterIsCorrect) {
    LOG(("The updaters do not match, udpater will not run.\n")); 
    result = FALSE;
  }

  if (result) {
    LOG(("updater.exe was compared successfully to the installation directory"
         " updater.exe.\n"));
  } else {
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_COMPARE_ERROR)) {
      LOG(("Could not write update.status updater compare failure.\n"));
    }
    return FALSE;
  }

  
  
  
  HMODULE updaterModule = LoadLibraryEx(argv[0], NULL, 
                                        LOAD_LIBRARY_AS_DATAFILE);
  if (!updaterModule) {
    LOG(("updater.exe module could not be loaded. (%d)\n", GetLastError()));
    result = FALSE;
  } else {
    char updaterIdentity[64];
    if (!LoadStringA(updaterModule, IDS_UPDATER_IDENTITY, 
                     updaterIdentity, sizeof(updaterIdentity))) {
      LOG(("The updater.exe application does not contain the Mozilla"
           " updater identity.\n"));
      result = FALSE;
    }

    if (strcmp(updaterIdentity, UPDATER_IDENTITY_STRING)) {
      LOG(("The updater.exe identity string is not valid.\n"));
      result = FALSE;
    }
    FreeLibrary(updaterModule);
  }

  if (result) {
    LOG(("The updater.exe application contains the Mozilla"
          " updater identity.\n"));
  } else {
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_IDENTITY_ERROR)) {
      LOG(("Could not write update.status no updater identity.\n"));
    }
    return TRUE;
  }

  
  BOOL updaterSignProblem = FALSE;
#ifndef DISABLE_UPDATER_AUTHENTICODE_CHECK
  updaterSignProblem = !DoesBinaryMatchAllowedCertificates(installDir,
                                                           argv[0]);
#endif

  
  if (!updaterSignProblem) {
    BOOL updateProcessWasStarted = FALSE;
    if (StartUpdateProcess(argc, argv,
                           updateProcessWasStarted)) {
      LOG(("updater.exe was launched and run successfully!\n"));
      LogFlush();

      
      
      StartServiceUpdate(installDir);
    } else {
      result = FALSE;
      LOG(("Error running update process. Updating update.status"
           " Last error: %d\n", GetLastError()));
      LogFlush();

      
      
      
      
      
      if (!updateProcessWasStarted) {
        if (!WriteStatusFailure(argv[1], 
                                SERVICE_UPDATER_COULD_NOT_BE_STARTED)) {
          LOG(("Could not write update.status service update failure."
               "Last error: %d\n", GetLastError()));
        }
      }
    }
  } else {
    result = FALSE;
    LOG(("Could not start process due to certificate check error on "
         "updater.exe. Updating update.status.  Last error: %d\n", GetLastError()));

    
    
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_SIGN_ERROR)) {
      LOG(("Could not write pending state to update.status.  (%d)\n", 
           GetLastError()));
    }
  }

  return result;
}











BOOL
ExecuteServiceCommand(int argc, LPWSTR *argv)
{
  if (argc < 3) {
    LOG(("Not enough command line arguments to execute a service command\n"));
    return FALSE;
  }

  
  
  RPC_WSTR guidString = RPC_WSTR(L"");
  GUID guid;
  HRESULT hr = CoCreateGuid(&guid);
  if (SUCCEEDED(hr)) {
    UuidToString(&guid, &guidString);
  }
  LOG(("Executing service command %ls, ID: %ls\n", 
       argv[2], reinterpret_cast<LPCWSTR>(guidString)));
  RpcStringFree(&guidString);

  BOOL result = FALSE;
  if (!lstrcmpi(argv[2], L"software-update")) {
    result = ProcessSoftwareUpdateCommand(argc - 3, argv + 3);
    
    
    
    LOG(("Service command %ls complete.\n", argv[2]));
  } else if (!lstrcmpi(argv[2], L"clear-prefetch")) {
    result = ClearKnownPrefetch();
  } else {
    LOG(("Service command not recognized: %ls.\n", argv[2]));
    
  }

  LOG(("service command %ls complete with result: %ls.\n", 
       argv[1], (result ? L"Success" : L"Failure")));
  return TRUE;
}
