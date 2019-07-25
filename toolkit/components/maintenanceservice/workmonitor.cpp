




































#include <shlobj.h>
#include <shlwapi.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <shellapi.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shlwapi.lib")

#include "nsWindowsHelpers.h"
#include "nsAutoPtr.h"

#include "workmonitor.h"
#include "serviceinstall.h"
#include "servicebase.h"
#include "registrycertificates.h"
#include "uachelper.h"
#include "updatehelper.h"

extern BOOL gServiceStopping;




static const int TIME_TO_WAIT_ON_UPDATER = 15 * 60 * 1000;
PRUnichar* MakeCommandLine(int argc, PRUnichar **argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL PathGetSiblingFilePath(LPWSTR destinationBuffer,  LPCWSTR siblingFilePath, 
                            LPCWSTR newFileName);



const int SERVICE_UPDATER_COULD_NOT_BE_STARTED = 16000;
const int SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS = 16001;
const int SERVICE_UPDATER_SIGN_ERROR = 16002;
const int SERVICE_UPDATER_COMPARE_ERROR = 16003;
const int SERVICE_UPDATER_IDENTITY_ERROR = 16004;










BOOL
StartUpdateProcess(LPCWSTR updaterPath, 
                   LPCWSTR workingDir, 
                   int argcTmp,
                   LPWSTR *argvTmp,
                   BOOL &processStarted)
{
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);
  si.lpDesktop = L"winsta0\\Default";
  PROCESS_INFORMATION pi = {0};

  LOG(("Starting update process as the service in session 0."));

  
  
  LPWSTR cmdLine = MakeCommandLine(argcTmp, argvTmp);

  
  
  
  
  if (argcTmp >= 2 ) {
    
    si.lpDesktop = L"";
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
  }

  
  
  
  
  
  
  WCHAR updaterINI[MAX_PATH + 1];
  WCHAR updaterINITemp[MAX_PATH + 1];
  BOOL selfHandlePostUpdate = FALSE;
  
  
  if (PathGetSiblingFilePath(updaterINI, argvTmp[0], L"updater.ini") &&
      PathGetSiblingFilePath(updaterINITemp, argvTmp[0], L"updater.tmp")) {
    selfHandlePostUpdate = MoveFileEx(updaterINI, updaterINITemp, 
                                      MOVEFILE_REPLACE_EXISTING);
  }

  
  
  
  
  WCHAR envVarString[32];
  wsprintf(envVarString, L"MOZ_USING_SERVICE=1"); 
  _wputenv(envVarString);
  LPVOID environmentBlock = NULL;
  if (!CreateEnvironmentBlock(&environmentBlock, NULL, TRUE)) {
    LOG(("Could not create an environment block, setting it to NULL.\n"));
    environmentBlock = NULL;
  }
  
  _wputenv(L"MOZ_USING_SERVICE=");
  processStarted = CreateProcessW(updaterPath, cmdLine, 
                                  NULL, NULL, FALSE, 
                                  CREATE_DEFAULT_ERROR_MODE | 
                                  CREATE_UNICODE_ENVIRONMENT, 
                                  environmentBlock, 
                                  workingDir, &si, &pi);
  if (environmentBlock) {
    DestroyEnvironmentBlock(environmentBlock);
  }
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
  } else {
    DWORD lastError = GetLastError();
    LOG(("Could not create process as current user, "
         "updaterPath: %ls; cmdLine: %l.  (%d)\n", 
         updaterPath, cmdLine, lastError));
  }

  
  
  
  if (selfHandlePostUpdate) {
    MoveFileEx(updaterINITemp, updaterINI, MOVEFILE_REPLACE_EXISTING);

    
    if (updateWasSuccessful && argcTmp > 2) {
      LPCWSTR installationDir = argvTmp[2];
      LPCWSTR updateInfoDir = argvTmp[1];

      
      
      
      
      
      
      
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
ProcessWorkItem(LPCWSTR monitoringBasePath, 
                FILE_NOTIFY_INFORMATION &notifyInfo)
{
  int filenameLength = notifyInfo.FileNameLength / 
                       sizeof(notifyInfo.FileName[0]); 
  notifyInfo.FileName[filenameLength] = L'\0';

  
  
  BOOL haveWorkItem = notifyInfo.Action == FILE_ACTION_RENAMED_NEW_NAME && 
                      notifyInfo.FileNameLength > 3 && 
                      notifyInfo.FileName[filenameLength - 3] == L'.' &&
                      notifyInfo.FileName[filenameLength - 2] == L'm' &&
                      notifyInfo.FileName[filenameLength - 1] == L'z';
  if (!haveWorkItem) {
    
    return FALSE;
  }

  
  
  
  
  nsAutoHandle serviceRunningEvent(CreateEventW(NULL, TRUE, 
                                                FALSE, SERVICE_EVENT_NAME));

  LOG(("Processing new command meta file: %ls\n", notifyInfo.FileName));
  WCHAR fullMetaUpdateFilePath[MAX_PATH + 1];
  wcscpy(fullMetaUpdateFilePath, monitoringBasePath);

  
  
  if (!PathAppendSafe(fullMetaUpdateFilePath, notifyInfo.FileName)) {
    SetEvent(serviceRunningEvent);
    return TRUE;
  }

  nsAutoHandle metaUpdateFile(CreateFile(fullMetaUpdateFilePath, 
                                         GENERIC_READ, 
                                         FILE_SHARE_READ | 
                                         FILE_SHARE_WRITE | 
                                         FILE_SHARE_DELETE, 
                                         NULL, 
                                         OPEN_EXISTING,
                                         0, NULL));
  if (metaUpdateFile == INVALID_HANDLE_VALUE) {
    LOG(("Could not open command meta file: %ls\n", notifyInfo.FileName));
    SetEvent(serviceRunningEvent);
    return TRUE;
  }

  DWORD fileSize = GetFileSize(metaUpdateFile, NULL);
  DWORD commandID = 0;
  
  
  const int kSanityCheckFileSize = 1024 * 64;
  if (fileSize == INVALID_FILE_SIZE || 
      (fileSize %2) != 0 ||
      fileSize > kSanityCheckFileSize ||
      fileSize < sizeof(DWORD)) {
    LOG(("Could not obtain file size or an improper file size was encountered "
         "for command meta file: %ls\n", 
        notifyInfo.FileName));
    SetEvent(serviceRunningEvent);
    return TRUE;
  }

  
  
  DWORD commandIDCount;
  BOOL result = ReadFile(metaUpdateFile, &commandID, 
                         sizeof(DWORD), &commandIDCount, NULL);
  fileSize -= sizeof(DWORD);

  
  WCHAR updaterPath[MAX_PATH + 1];
  ZeroMemory(updaterPath, sizeof(updaterPath));
  DWORD updaterPathCount;
  result |= ReadFile(metaUpdateFile, updaterPath, 
                     MAX_PATH * sizeof(WCHAR), &updaterPathCount, NULL);
  fileSize -= updaterPathCount;

  
  WCHAR workingDirectory[MAX_PATH + 1];
  ZeroMemory(workingDirectory, sizeof(workingDirectory));
  DWORD workingDirectoryCount;
  result |= ReadFile(metaUpdateFile, workingDirectory, 
                     MAX_PATH * sizeof(WCHAR), &workingDirectoryCount, NULL);
  fileSize -= workingDirectoryCount;

  
  nsAutoArrayPtr<char> cmdlineBuffer = new char[fileSize + 2];
  DWORD cmdLineBufferRead;
  result |= ReadFile(metaUpdateFile, cmdlineBuffer, 
                     fileSize, &cmdLineBufferRead, NULL);
  fileSize -= cmdLineBufferRead;

  
  metaUpdateFile.reset();
  if (!DeleteFileW(fullMetaUpdateFilePath)) {
    LOG(("Could not delete work item file: %ls. (%d)\n", 
         fullMetaUpdateFilePath, GetLastError()));
  }

  if (!result ||
      commandID != 1 ||
      commandIDCount != sizeof(DWORD) ||
      updaterPathCount != MAX_PATH * sizeof(WCHAR) ||
      workingDirectoryCount != MAX_PATH * sizeof(WCHAR) ||
      fileSize != 0) {
    LOG(("Could not read command data for command meta file: %ls\n", 
         notifyInfo.FileName));
    SetEvent(serviceRunningEvent);
    return TRUE;
  }
  cmdlineBuffer[cmdLineBufferRead] = '\0';
  cmdlineBuffer[cmdLineBufferRead + 1] = '\0';
  WCHAR *cmdlineBufferWide = reinterpret_cast<WCHAR*>(cmdlineBuffer.get());
  LOG(("An update command was detected and is being processed for command meta "
       "file: %ls\n", notifyInfo.FileName));

  int argcTmp = 0;
  LPWSTR* argvTmp = CommandLineToArgvW(cmdlineBufferWide, &argcTmp);

  
  
  
  WCHAR installDirUpdater[MAX_PATH + 1];
  wcsncpy(installDirUpdater, argvTmp[2], MAX_PATH);
  if (!PathAppendSafe(installDirUpdater, L"updater.exe")) {
    LOG(("Install directory updater could not be determined.\n"));
    result = FALSE;
  }

  BOOL updaterIsCorrect;
  if (result && !VerifySameFiles(updaterPath, installDirUpdater, 
                                 updaterIsCorrect)) {
    LOG(("Error checking if the updaters are the same.\n")); 
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
    SetEvent(serviceRunningEvent);
    if (argcTmp < 2 || 
        !WriteStatusFailure(argvTmp[1], 
                            SERVICE_UPDATER_COMPARE_ERROR)) {
      LOG(("Could not write update.status updater compare failure.\n"));
    }
    return TRUE;
  }

  
  
  
  HMODULE updaterModule = LoadLibrary(updaterPath);
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
    SetEvent(serviceRunningEvent);
    if (argcTmp < 2 || 
        !WriteStatusFailure(argvTmp[1], 
                            SERVICE_UPDATER_IDENTITY_ERROR)) {
      LOG(("Could not write update.status no updater identity.\n"));
    }
    return TRUE;
  }

  
  BOOL updaterSignProblem = FALSE;
#ifndef DISABLE_UPDATER_AUTHENTICODE_CHECK
  if (argcTmp > 2) {
    updaterSignProblem = !DoesBinaryMatchAllowedCertificates(argvTmp[2], 
                                                             updaterPath);
  }
#endif

  
  
  if (argcTmp > 2 && !updaterSignProblem) {
    BOOL updateProcessWasStarted = FALSE;
    if (StartUpdateProcess(updaterPath, workingDirectory, 
                           argcTmp, argvTmp,
                           updateProcessWasStarted)) {
      LOG(("updater.exe was launched and run successfully!\n"));
      StartServiceUpdate(argcTmp, argvTmp);
    } else {
      LOG(("Error running update process. Updating update.status"
           " Last error: %d\n", GetLastError()));

      
      
      
      
      
      if (!updateProcessWasStarted) {
        if (!WriteStatusFailure(argvTmp[1], 
                                SERVICE_UPDATER_COULD_NOT_BE_STARTED)) {
          LOG(("Could not write update.status service update failure."
               "Last error: %d\n", GetLastError()));
        }
      }
    }
  } else if (argcTmp <= 2) {
    LOG(("Not enough command line parameters specified. "
         "Updating update.status.\n"));

    
    
    if (argcTmp != 2 || 
        !WriteStatusFailure(argvTmp[1], 
                            SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS)) {
      LOG(("Could not write update.status service update failure."
           "Last error: %d\n", GetLastError()));
    }
  } else {
    LOG(("Could not start process due to certificate check error on "
         "updater.exe. Updating update.status.  Last error: %d\n", GetLastError()));

    
    
    if (!WriteStatusFailure(argvTmp[1], 
                            SERVICE_UPDATER_SIGN_ERROR)) {
      LOG(("Could not write pending state to update.status.  (%d)\n", 
           GetLastError()));
    }
  }
  LocalFree(argvTmp);
  SetEvent(serviceRunningEvent);

  
  return TRUE;
}






BOOL
StartDirectoryChangeMonitor() 
{
  LOG(("Starting directory change monitor...\n"));

  
  
  
  WCHAR updateData[MAX_PATH + 1];
  if (!GetUpdateDirectoryPath(updateData)) {
    LOG(("Could not obtain update directory path\n"));
    return FALSE;
  }

  
  nsAutoHandle directoryHandle(CreateFile(updateData, 
                                          FILE_LIST_DIRECTORY, 
                                          FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                          NULL, 
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS, 
                                          NULL));
  if (directoryHandle == INVALID_HANDLE_VALUE) {
    LOG(("Could not obtain directory handle to monitor\n"));
    return FALSE;
  }

  
  
  
  char fileNotifyInfoBuffer[(sizeof(FILE_NOTIFY_INFORMATION) + 
                            MAX_PATH) * 128];
  ZeroMemory(&fileNotifyInfoBuffer, sizeof(fileNotifyInfoBuffer));
  
  DWORD bytesReturned;
  
  while (ReadDirectoryChangesW(directoryHandle, 
                               fileNotifyInfoBuffer, 
                               sizeof(fileNotifyInfoBuffer), 
                               TRUE, FILE_NOTIFY_CHANGE_FILE_NAME, 
                               &bytesReturned, NULL, NULL)) {
    char *fileNotifyInfoBufferLocation = fileNotifyInfoBuffer;

    
    while (bytesReturned/sizeof(FILE_NOTIFY_INFORMATION) > 0) {
      
      FILE_NOTIFY_INFORMATION &notifyInfo = 
        *((FILE_NOTIFY_INFORMATION*)fileNotifyInfoBufferLocation);
      fileNotifyInfoBufferLocation += notifyInfo .NextEntryOffset;
      bytesReturned -= notifyInfo .NextEntryOffset;
      if (!wcscmp(notifyInfo.FileName, L"stop") && gServiceStopping) {
        LOG(("A stop command was issued.\n"));
        return TRUE;
      }

      BOOL processedWorkItem = ProcessWorkItem(updateData, notifyInfo);
      if (processedWorkItem) {
        
        
        StopService();
        break;
      }

      
      
      if (0 == notifyInfo.NextEntryOffset) {
        break;
      }
    }
  }

  return TRUE;
}
