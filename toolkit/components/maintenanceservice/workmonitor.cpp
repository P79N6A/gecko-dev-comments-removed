



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

#include "workmonitor.h"
#include "serviceinstall.h"
#include "servicebase.h"
#include "registrycertificates.h"
#include "uachelper.h"
#include "updatehelper.h"
#include "errors.h"




static const int TIME_TO_WAIT_ON_UPDATER = 15 * 60 * 1000;
wchar_t* MakeCommandLine(int argc, wchar_t** argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL PathGetSiblingFilePath(LPWSTR destinationBuffer,  LPCWSTR siblingFilePath, 
                            LPCWSTR newFileName);










static BOOL
IsStatusApplying(LPCWSTR updateDirPath, BOOL &isApplying)
{
  isApplying = FALSE;
  WCHAR updateStatusFilePath[MAX_PATH + 1] = {L'\0'};
  wcsncpy(updateStatusFilePath, updateDirPath, MAX_PATH);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    LOG_WARN(("Could not append path for update.status file"));
    return FALSE;
  }

  nsAutoHandle statusFile(CreateFileW(updateStatusFilePath, GENERIC_READ,
                                      FILE_SHARE_READ |
                                      FILE_SHARE_WRITE |
                                      FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, 0, nullptr));

  if (INVALID_HANDLE_VALUE == statusFile) {
    LOG_WARN(("Could not open update.status file"));
    return FALSE;
  }

  char buf[32] = { 0 };
  DWORD read;
  if (!ReadFile(statusFile, buf, sizeof(buf), &read, nullptr)) {
    LOG_WARN(("Could not read from update.status file"));
    return FALSE;
  }

  LOG(("updater.exe returned status: %s", buf));

  const char kApplying[] = "applying";
  isApplying = strncmp(buf, kApplying,
                       sizeof(kApplying) - 1) == 0;
  return TRUE;
}








static bool
IsUpdateBeingStaged(int argc, LPWSTR *argv)
{
  
  return argc == 4 && !wcscmp(argv[3], L"-1") ||
         argc == 5 && !wcscmp(argv[4], L"-1");
}







static bool
IsDigits(WCHAR *str)
{
  while (*str) {
    if (!iswdigit(*str++)) {
      return FALSE;
    }
  }
  return TRUE;
}











static bool
IsOldCommandline(int argc, LPWSTR *argv)
{
  return argc == 4 && !wcscmp(argv[3], L"-1") ||
         argc >= 4 && (wcsstr(argv[3], L"/replace") || IsDigits(argv[3]));
}








static BOOL
GetInstallationDir(int argcTmp, LPWSTR *argvTmp, WCHAR aResultDir[MAX_PATH + 1])
{
  int index = 3;
  if (IsOldCommandline(argcTmp, argvTmp)) {
    index = 2;
  }

  if (argcTmp < index) {
    return FALSE;
  }

  wcsncpy(aResultDir, argvTmp[2], MAX_PATH);
  WCHAR* backSlash = wcsrchr(aResultDir, L'\\');
  
  if (backSlash && (backSlash[1] == L'\0')) {
    *backSlash = L'\0';
  }

  
  if (index == 2) {
    bool backgroundUpdate = IsUpdateBeingStaged(argcTmp, argvTmp);
    bool replaceRequest = (argcTmp >= 4 && wcsstr(argvTmp[3], L"/replace"));
    if (backgroundUpdate || replaceRequest) {
      return PathRemoveFileSpecW(aResultDir);
    }
  }
  return TRUE;
}










BOOL
StartUpdateProcess(int argc,
                   LPWSTR *argv,
                   LPCWSTR installDir,
                   BOOL &processStarted)
{
  LOG(("Starting update process as the service in session 0."));
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);
  si.lpDesktop = L"winsta0\\Default";
  PROCESS_INFORMATION pi = {0};

  
  
  LPWSTR cmdLine = MakeCommandLine(argc, argv);

  int index = 3;
  if (IsOldCommandline(argc, argv)) {
    index = 2;
  }

  
  
  
  
  if (argc >= index) {
    
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
  LOG(("Starting service with cmdline: %ls", cmdLine));
  processStarted = CreateProcessW(argv[0], cmdLine,
                                  nullptr, nullptr, FALSE,
                                  CREATE_DEFAULT_ERROR_MODE,
                                  nullptr,
                                  nullptr, &si, &pi);

  BOOL updateWasSuccessful = FALSE;
  if (processStarted) {
    BOOL processTerminated = FALSE;
    BOOL noProcessExitCode = FALSE;
    
    LOG(("Process was started... waiting on result."));
    DWORD waitRes = WaitForSingleObject(pi.hProcess, TIME_TO_WAIT_ON_UPDATER);
    if (WAIT_TIMEOUT == waitRes) {
      
      
      TerminateProcess(pi.hProcess, 1);
      processTerminated = TRUE;
    } else {
      
      DWORD returnCode;
      if (GetExitCodeProcess(pi.hProcess, &returnCode)) {
        LOG(("Process finished with return code %d.", returnCode));
        
        updateWasSuccessful = (returnCode == 0);
      } else {
        LOG_WARN(("Process finished but could not obtain return code."));
        noProcessExitCode = TRUE;
      }
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    
    
    BOOL isApplying = FALSE;
    if (IsStatusApplying(argv[1], isApplying) && isApplying) {
      if (updateWasSuccessful) {
        LOG(("update.status is still applying even though update was "
             "successful."));
        if (!WriteStatusFailure(argv[1],
                                SERVICE_STILL_APPLYING_ON_SUCCESS)) {
          LOG_WARN(("Could not write update.status still applying on "
                    "success error."));
        }
        
        
        updateWasSuccessful = FALSE;
      } else {
        LOG_WARN(("update.status is still applying and update was not successful."));
        int failcode = SERVICE_STILL_APPLYING_ON_FAILURE;
        if (noProcessExitCode) {
          failcode = SERVICE_STILL_APPLYING_NO_EXIT_CODE;
        } else if (processTerminated) {
          failcode = SERVICE_STILL_APPLYING_TERMINATED;
        }
        if (!WriteStatusFailure(argv[1], failcode)) {
          LOG_WARN(("Could not write update.status still applying on "
                    "failure error."));
        }
      }
    }
  } else {
    DWORD lastError = GetLastError();
    LOG_WARN(("Could not create process as current user, "
              "updaterPath: %ls; cmdLine: %ls.  (%d)",
              argv[0], cmdLine, lastError));
  }

  
  
  
  if (selfHandlePostUpdate) {
    MoveFileExW(updaterINITemp, updaterINI, MOVEFILE_REPLACE_EXISTING);

    
    if (updateWasSuccessful && argc > index) {
      LPCWSTR updateInfoDir = argv[1];
      bool stagingUpdate = IsUpdateBeingStaged(argc, argv);

      
      
      
      
      
      
      
      
      
      
      if (!stagingUpdate) {
        LOG(("Launching post update process as the service in session 0."));
        if (!LaunchWinPostProcess(installDir, updateInfoDir, true, nullptr)) {
          LOG_WARN(("The post update process could not be launched."
                    " installDir: %ls, updateInfoDir: %ls",
                    installDir, updateInfoDir));
        }
      }
    }
  }
  
  putenv(const_cast<char*>("MOZ_USING_SERVICE="));

  free(cmdLine);
  return updateWasSuccessful;
}









BOOL
ProcessSoftwareUpdateCommand(DWORD argc, LPWSTR *argv)
{
  BOOL result = TRUE;
  if (argc < 3) {
    LOG_WARN(("Not enough command line parameters specified. "
              "Updating update.status."));

    
    
    if (argc < 2 ||
        !WriteStatusFailure(argv[1],
                            SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS)) {
      LOG_WARN(("Could not write update.status service update failure.  (%d)",
                GetLastError()));
    }
    return FALSE;
  }

  WCHAR installDir[MAX_PATH + 1] = {L'\0'};
  if (!GetInstallationDir(argc, argv, installDir)) {
    LOG_WARN(("Could not get the installation directory"));
    if (!WriteStatusFailure(argv[1],
                            SERVICE_INSTALLDIR_ERROR)) {
      LOG_WARN(("Could not write update.status for GetInstallationDir failure."));
    }
    return FALSE;
  }

  
  
  
  BOOL isLocal = FALSE;
  if (!IsLocalFile(argv[0], isLocal) || !isLocal) {
    LOG_WARN(("Filesystem in path %ls is not supported (%d)",
              argv[0], GetLastError()));
    if (!WriteStatusFailure(argv[1],
                            SERVICE_UPDATER_NOT_FIXED_DRIVE)) {
      LOG_WARN(("Could not write update.status service update failure.  (%d)",
                GetLastError()));
    }
    return FALSE;
  }

  nsAutoHandle noWriteLock(CreateFileW(argv[0], GENERIC_READ, FILE_SHARE_READ,
                                       nullptr, OPEN_EXISTING, 0, nullptr));
  if (INVALID_HANDLE_VALUE == noWriteLock) {
      LOG_WARN(("Could not set no write sharing access on file.  (%d)",
                GetLastError()));
    if (!WriteStatusFailure(argv[1],
                            SERVICE_COULD_NOT_LOCK_UPDATER)) {
      LOG_WARN(("Could not write update.status service update failure.  (%d)",
                GetLastError()));
    }
    return FALSE;
  }

  
  
  
  WCHAR installDirUpdater[MAX_PATH + 1] = { L'\0' };
  wcsncpy(installDirUpdater, installDir, MAX_PATH);
  if (!PathAppendSafe(installDirUpdater, L"updater.exe")) {
    LOG_WARN(("Install directory updater could not be determined."));
    result = FALSE;
  }

  BOOL updaterIsCorrect;
  if (result && !VerifySameFiles(argv[0], installDirUpdater,
                                 updaterIsCorrect)) {
    LOG_WARN(("Error checking if the updaters are the same.\n"
              "Path 1: %ls\nPath 2: %ls", argv[0], installDirUpdater));
    result = FALSE;
  }

  if (result && !updaterIsCorrect) {
    LOG_WARN(("The updaters do not match, updater will not run.\n"
              "Path 1: %ls\nPath 2: %ls", argv[0], installDirUpdater));
    result = FALSE;
  }

  if (result) {
    LOG(("updater.exe was compared successfully to the installation directory"
         " updater.exe."));
  } else {
    if (!WriteStatusFailure(argv[1],
                            SERVICE_UPDATER_COMPARE_ERROR)) {
      LOG_WARN(("Could not write update.status updater compare failure."));
    }
    return FALSE;
  }

  
  
  
  HMODULE updaterModule = LoadLibraryEx(argv[0], nullptr,
                                        LOAD_LIBRARY_AS_DATAFILE);
  if (!updaterModule) {
    LOG_WARN(("updater.exe module could not be loaded. (%d)", GetLastError()));
    result = FALSE;
  } else {
    char updaterIdentity[64];
    if (!LoadStringA(updaterModule, IDS_UPDATER_IDENTITY,
                     updaterIdentity, sizeof(updaterIdentity))) {
      LOG_WARN(("The updater.exe application does not contain the Mozilla"
                " updater identity."));
      result = FALSE;
    }

    if (strcmp(updaterIdentity, UPDATER_IDENTITY_STRING)) {
      LOG_WARN(("The updater.exe identity string is not valid."));
      result = FALSE;
    }
    FreeLibrary(updaterModule);
  }

  if (result) {
    LOG(("The updater.exe application contains the Mozilla"
          " updater identity."));
  } else {
    if (!WriteStatusFailure(argv[1],
                            SERVICE_UPDATER_IDENTITY_ERROR)) {
      LOG_WARN(("Could not write update.status no updater identity."));
    }
    return TRUE;
  }

  
  int rv = OK;
#ifndef DISABLE_UPDATER_AUTHENTICODE_CHECK
  rv = DoesBinaryMatchAllowedCertificates(installDir, argv[0]);
#endif

  
  if (rv == OK) {
    BOOL updateProcessWasStarted = FALSE;
    if (StartUpdateProcess(argc, argv, installDir,
                           updateProcessWasStarted)) {
      LOG(("updater.exe was launched and run successfully!"));
      LogFlush();

      
      if (!IsUpdateBeingStaged(argc, argv)) {
        
        
        StartServiceUpdate(installDir);
      }
    } else {
      result = FALSE;
      LOG_WARN(("Error running update process. Updating update.status  (%d)",
                GetLastError()));
      LogFlush();

      
      
      
      
      
      if (!updateProcessWasStarted) {
        if (!WriteStatusFailure(argv[1],
                                SERVICE_UPDATER_COULD_NOT_BE_STARTED)) {
          LOG_WARN(("Could not write update.status service update failure.  (%d)",
                    GetLastError()));
        }
      }
    }
  } else {
    result = FALSE;
    LOG_WARN(("Could not start process due to certificate check error on "
              "updater.exe. Updating update.status.  (%d)", GetLastError()));

    
    
    if (!WriteStatusFailure(argv[1], rv)) {
      LOG_WARN(("Could not write failed state to update.status.  (%d)",
                GetLastError()));
    }
  }

  return result;
}










BOOL
GetSecureUpdaterPath(WCHAR serviceUpdaterPath[MAX_PATH + 1])
{
  if (!GetModuleFileNameW(nullptr, serviceUpdaterPath, MAX_PATH)) {
    LOG_WARN(("Could not obtain module filename when attempting to "
              "use a secure updater path.  (%d)", GetLastError()));
    return FALSE;
  }

  if (!PathRemoveFileSpecW(serviceUpdaterPath)) {
    LOG_WARN(("Couldn't remove file spec when attempting to use a secure "
              "updater path.  (%d)", GetLastError()));
    return FALSE;
  }

  if (!PathAppendSafe(serviceUpdaterPath, L"update")) {
    LOG_WARN(("Couldn't append file spec when attempting to use a secure "
              "updater path.  (%d)", GetLastError()));
    return FALSE;
  }

  CreateDirectoryW(serviceUpdaterPath, nullptr);

  if (!PathAppendSafe(serviceUpdaterPath, L"updater.exe")) {
    LOG_WARN(("Couldn't append file spec when attempting to use a secure "
              "updater path.  (%d)", GetLastError()));
    return FALSE;
  }

  return TRUE;
}







BOOL
DeleteSecureUpdater(WCHAR serviceUpdaterPath[MAX_PATH + 1])
{
  BOOL result = FALSE;
  if (serviceUpdaterPath[0]) {
    result = DeleteFileW(serviceUpdaterPath);
    if (!result && GetLastError() != ERROR_PATH_NOT_FOUND &&
        GetLastError() != ERROR_FILE_NOT_FOUND) {
      LOG_WARN(("Could not delete service updater path: '%ls'.",
                serviceUpdaterPath));
    }

    WCHAR updaterINIPath[MAX_PATH + 1] = { L'\0' };
    if (PathGetSiblingFilePath(updaterINIPath, serviceUpdaterPath,
                               L"updater.ini")) {
      result = DeleteFileW(updaterINIPath);
      if (!result && GetLastError() != ERROR_PATH_NOT_FOUND &&
          GetLastError() != ERROR_FILE_NOT_FOUND) {
        LOG_WARN(("Could not delete service updater INI path: '%ls'.",
                  updaterINIPath));
      }
    }
  }
  return result;
}











BOOL
ExecuteServiceCommand(int argc, LPWSTR *argv)
{
  if (argc < 3) {
    LOG_WARN(("Not enough command line arguments to execute a service command"));
    return FALSE;
  }

  
  
  RPC_WSTR guidString = RPC_WSTR(L"");
  GUID guid;
  HRESULT hr = CoCreateGuid(&guid);
  if (SUCCEEDED(hr)) {
    UuidToString(&guid, &guidString);
  }
  LOG(("Executing service command %ls, ID: %ls",
       argv[2], reinterpret_cast<LPCWSTR>(guidString)));
  RpcStringFree(&guidString);

  BOOL result = FALSE;
  if (!lstrcmpi(argv[2], L"software-update")) {

    
    
    
    
    
    LPWSTR oldUpdaterPath = argv[3];
    WCHAR secureUpdaterPath[MAX_PATH + 1] = { L'\0' };
    result = GetSecureUpdaterPath(secureUpdaterPath); 
    if (result) {
      LOG(("Passed in path: '%ls'; Using this path for updating: '%ls'.",
           oldUpdaterPath, secureUpdaterPath));
      DeleteSecureUpdater(secureUpdaterPath);
      result = CopyFileW(oldUpdaterPath, secureUpdaterPath, FALSE);
    }

    if (!result) {
      LOG_WARN(("Could not copy path to secure location.  (%d)",
                GetLastError()));
      if (argc > 4 && !WriteStatusFailure(argv[4],
                                          SERVICE_COULD_NOT_COPY_UPDATER)) {
        LOG_WARN(("Could not write update.status could not copy updater error"));
      }
    } else {

      
      
      argv[3] = secureUpdaterPath;

      WCHAR oldUpdaterINIPath[MAX_PATH + 1] = { L'\0' };
      WCHAR secureUpdaterINIPath[MAX_PATH + 1] = { L'\0' };
      if (PathGetSiblingFilePath(secureUpdaterINIPath, secureUpdaterPath,
                                 L"updater.ini") &&
          PathGetSiblingFilePath(oldUpdaterINIPath, oldUpdaterPath,
                                 L"updater.ini")) {
        
        if (!CopyFileW(oldUpdaterINIPath, secureUpdaterINIPath, FALSE)) {
          LOG_WARN(("Could not copy updater.ini from: '%ls' to '%ls'.  (%d)",
                    oldUpdaterINIPath, secureUpdaterINIPath, GetLastError()));
        }
      }

      result = ProcessSoftwareUpdateCommand(argc - 3, argv + 3);
      DeleteSecureUpdater(secureUpdaterPath);
    }

    
    
    
    LOG(("Service command %ls complete.", argv[2]));
  } else {
    LOG_WARN(("Service command not recognized: %ls.", argv[2]));
    
  }

  LOG(("service command %ls complete with result: %ls.",
       argv[1], (result ? L"Success" : L"Failure")));
  return TRUE;
}
