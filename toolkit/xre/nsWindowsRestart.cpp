









































#ifdef nsWindowsRestart_cpp
#error "nsWindowsRestart.cpp is not a header file, and must only be included once."
#else
#define nsWindowsRestart_cpp
#endif

#include "nsUTF8Utils.h"
#include "nsWindowsHelpers.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>
#include <rpc.h>
#include <userenv.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "userenv.lib")

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740L
#endif

BOOL (WINAPI *pCreateProcessWithTokenW)(HANDLE,
                                        DWORD,
                                        LPCWSTR,
                                        LPWSTR,
                                        DWORD,
                                        LPVOID,
                                        LPCWSTR,
                                        LPSTARTUPINFOW,
                                        LPPROCESS_INFORMATION);

BOOL (WINAPI *pIsUserAnAdmin)(VOID);






static int ArgStrLen(const PRUnichar *s)
{
  int backslashes = 0;
  int i = wcslen(s);
  BOOL hasDoubleQuote = wcschr(s, L'"') != NULL;
  
  BOOL addDoubleQuotes = wcspbrk(s, L" \t") != NULL;

  if (addDoubleQuotes) {
    i += 2; 
  }

  if (hasDoubleQuote) {
    while (*s) {
      if (*s == '\\') {
        ++backslashes;
      } else {
        if (*s == '"') {
          
          i += backslashes + 1;
        }

        backslashes = 0;
      }

      ++s;
    }
  }

  return i;
}










static PRUnichar* ArgToString(PRUnichar *d, const PRUnichar *s)
{
  int backslashes = 0;
  BOOL hasDoubleQuote = wcschr(s, L'"') != NULL;
  
  BOOL addDoubleQuotes = wcspbrk(s, L" \t") != NULL;

  if (addDoubleQuotes) {
    *d = '"'; 
    ++d;
  }

  if (hasDoubleQuote) {
    int i;
    while (*s) {
      if (*s == '\\') {
        ++backslashes;
      } else {
        if (*s == '"') {
          
          for (i = 0; i <= backslashes; ++i) {
            *d = '\\';
            ++d;
          }
        }

        backslashes = 0;
      }

      *d = *s;
      ++d; ++s;
    }
  } else {
    wcscpy(d, s);
    d += wcslen(s);
  }

  if (addDoubleQuotes) {
    *d = '"'; 
    ++d;
  }

  return d;
}







PRUnichar*
MakeCommandLine(int argc, PRUnichar **argv)
{
  int i;
  int len = 0;

  
  for (i = 0; i < argc; ++i)
    len += ArgStrLen(argv[i]) + 1;

  
  if (len == 0)
    len = 1;

  PRUnichar *s = (PRUnichar*) malloc(len * sizeof(PRUnichar));
  if (!s)
    return NULL;

  PRUnichar *c = s;
  for (i = 0; i < argc; ++i) {
    c = ArgToString(c, argv[i]);
    if (i + 1 != argc) {
      *c = ' ';
      ++c;
    }
  }

  *c = '\0';

  return s;
}





static PRUnichar*
AllocConvertUTF8toUTF16(const char *arg)
{
  
  int len = strlen(arg);
  PRUnichar *s = new PRUnichar[(len + 1) * sizeof(PRUnichar)];
  if (!s)
    return NULL;

  ConvertUTF8toUTF16 convert(s);
  convert.write(arg, len);
  convert.write_terminator();
  return s;
}

static void
FreeAllocStrings(int argc, PRUnichar **argv)
{
  while (argc) {
    --argc;
    delete [] argv[argc];
  }

  delete [] argv;
}






BOOL 
EnsureWindowsServiceRunning() {
  
  nsAutoServiceHandle serviceManager(OpenSCManager(NULL, NULL, 
                                                   SC_MANAGER_CONNECT | 
                                                   SC_MANAGER_ENUMERATE_SERVICE));
  if (!serviceManager)  {
    return FALSE;
  }

  
  nsAutoServiceHandle service(OpenServiceW(serviceManager, 
                                           L"MozillaMaintenance", 
                                           SERVICE_QUERY_STATUS | SERVICE_START));
  if (!service) { 
    return FALSE;
  }

  
  SERVICE_STATUS_PROCESS ssp;
  DWORD bytesNeeded;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                            sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
    return FALSE;
  }

  if (ssp.dwCurrentState == SERVICE_STOPPED) {
    if (!StartService(service, 0, NULL)) {
      return FALSE;
    }

    
    
    
    DWORD totalWaitTime = 0;
    static const int maxWaitTime = 1000 * 5; 
    while (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                                sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
      if (ssp.dwCurrentState == SERVICE_RUNNING) {
        break;
      }
      
      if (ssp.dwCurrentState == SERVICE_START_PENDING &&
          totalWaitTime > maxWaitTime) {
        
        break;
      }
      
      if (ssp.dwCurrentState != SERVICE_START_PENDING) {
        return FALSE;
      }

      Sleep(ssp.dwWaitHint);
      
      
      totalWaitTime += (ssp.dwWaitHint + 10);
    }
  }

  return ssp.dwCurrentState == SERVICE_RUNNING;
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
GetUpdateDirectoryPath(PRUnichar *path) 
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

  if (!PathAppendSafe(path, L"updates")) {
    return FALSE;
  }
  CreateDirectoryW(path, NULL);
  return TRUE;
}










BOOL
WinLaunchServiceCommand(const PRUnichar *exePath, int argc, PRUnichar **argv)
{
  
  
  if (!EnsureWindowsServiceRunning()) {
    return FALSE;
  }

  PRUnichar updateData[MAX_PATH + 1];
  if (!GetUpdateDirectoryPath(updateData)) {
    return FALSE;
  }

  
  PRUnichar tempFilePath[MAX_PATH + 1];
  const int USE_SYSTEM_TIME = 0;
  if (!GetTempFileNameW(updateData, L"moz", USE_SYSTEM_TIME, tempFilePath)) {
    return FALSE;
  }
  
  const int FILE_SHARE_NONE = 0;
  nsAutoHandle updateMetaFile(CreateFileW(tempFilePath, GENERIC_WRITE, 
                                          FILE_SHARE_NONE, NULL, CREATE_ALWAYS, 
                                          0, NULL));
  if (updateMetaFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  
  
  
  DWORD commandID = 1, commandIDWrote;
  BOOL result = WriteFile(updateMetaFile, &commandID, 
                          sizeof(DWORD), 
                          &commandIDWrote, NULL);

  
  PRUnichar *commandLineBuffer = MakeCommandLine(argc, argv);
  DWORD sessionID, sessionIDWrote;
  ProcessIdToSessionId(GetCurrentProcessId(), &sessionID);
  result |= WriteFile(updateMetaFile, &sessionID, 
                      sizeof(DWORD), 
                      &sessionIDWrote, NULL);

  PRUnichar appBuffer[MAX_PATH + 1];
  ZeroMemory(appBuffer, sizeof(appBuffer));
  wcscpy(appBuffer, exePath);
  DWORD appBufferWrote;
  result |= WriteFile(updateMetaFile, appBuffer, 
                      MAX_PATH * sizeof(PRUnichar), 
                      &appBufferWrote, NULL);

  PRUnichar workingDirectory[MAX_PATH + 1];
  ZeroMemory(workingDirectory, sizeof(appBuffer));
  GetCurrentDirectoryW(sizeof(workingDirectory) / sizeof(workingDirectory[0]), 
                       workingDirectory);
  DWORD workingDirectoryWrote;
  result |= WriteFile(updateMetaFile, workingDirectory, 
                      MAX_PATH * sizeof(PRUnichar), 
                      &workingDirectoryWrote, NULL);

  DWORD commandLineLength = wcslen(commandLineBuffer) * sizeof(PRUnichar);
  DWORD commandLineWrote;
  result |= WriteFile(updateMetaFile, commandLineBuffer, 
                      commandLineLength, 
                      &commandLineWrote, NULL);
  free(commandLineBuffer);
  if (!result ||
      sessionIDWrote != sizeof(DWORD) ||
      commandIDWrote != sizeof(DWORD) ||
      appBufferWrote != MAX_PATH * sizeof(PRUnichar) ||
      workingDirectoryWrote != MAX_PATH * sizeof(PRUnichar) ||
      commandLineWrote != commandLineLength) {
    updateMetaFile.reset();
    DeleteFileW(tempFilePath);
    return FALSE;
  }

  
  
  
  
  updateMetaFile.reset();
  PRUnichar completedMetaFilePath[MAX_PATH + 1];
  wcscpy(completedMetaFilePath, tempFilePath);

  
  LPWSTR extensionPart = 
    &(completedMetaFilePath[wcslen(completedMetaFilePath) - 3]);
  wcscpy(extensionPart, L"mz");
  return MoveFileExW(tempFilePath, completedMetaFilePath, 
                     MOVEFILE_REPLACE_EXISTING);
}








BOOL
WriteStatusPending(LPCWSTR updateDirPath)
{
  PRUnichar updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    return FALSE;
  }

  const char pending[] = "pending";
  nsAutoHandle statusFile(CreateFileW(updateStatusFilePath, GENERIC_WRITE, 0, 
                                      NULL, CREATE_ALWAYS, 0, NULL));
  if (statusFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  DWORD wrote;
  BOOL ok = WriteFile(statusFile, pending, 
                      sizeof(pending) - 1, &wrote, NULL); 
  return ok && (wrote == sizeof(pending) - 1);
}







BOOL
WriteStatusFailure(LPCWSTR updateDirPath, int errorCode) 
{
  PRUnichar updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    return FALSE;
  }

  nsAutoHandle statusFile(CreateFileW(updateStatusFilePath, GENERIC_WRITE, 0, 
                                      NULL, CREATE_ALWAYS, 0, NULL));
  if (statusFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  char failure[32];
  sprintf(failure, "failed: %d", errorCode);

  DWORD toWrite = strlen(failure);
  DWORD wrote;
  BOOL ok = WriteFile(statusFile, failure, 
                      toWrite, &wrote, NULL); 
  return ok && wrote == toWrite;
}










BOOL
WinLaunchServiceCommand(const PRUnichar *exePath, int argc, char **argv)
{
  PRUnichar** argvConverted = new PRUnichar*[argc];
  if (!argvConverted)
    return FALSE;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF8toUTF16(argv[i]);
    if (!argvConverted[i]) {
      FreeAllocStrings(i, argvConverted);
      return FALSE;
    }
  }

  BOOL ok = WinLaunchServiceCommand(exePath, argc, argvConverted);
  FreeAllocStrings(argc, argvConverted);
  return ok;
}









BOOL
WinLaunchChild(const PRUnichar *exePath, 
               int argc, PRUnichar **argv, 
               HANDLE userToken = NULL);

BOOL
WinLaunchChild(const PRUnichar *exePath, 
               int argc, char **argv, 
               HANDLE userToken)
{
  PRUnichar** argvConverted = new PRUnichar*[argc];
  if (!argvConverted)
    return FALSE;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF8toUTF16(argv[i]);
    if (!argvConverted[i]) {
      FreeAllocStrings(i, argvConverted);
      return FALSE;
    }
  }

  BOOL ok = WinLaunchChild(exePath, argc, argvConverted, userToken);
  FreeAllocStrings(argc, argvConverted);
  return ok;
}

BOOL
WinLaunchChild(const PRUnichar *exePath, 
               int argc, 
               PRUnichar **argv, 
               HANDLE userToken)
{
  PRUnichar *cl;
  BOOL ok;

  cl = MakeCommandLine(argc, argv);
  if (!cl) {
    return FALSE;
  }

  STARTUPINFOW si = {0};
  si.cb = sizeof(STARTUPINFOW);
  si.lpDesktop = L"winsta0\\Default";
  PROCESS_INFORMATION pi = {0};

  if (userToken == NULL) {
    ok = CreateProcessW(exePath,
                        cl,
                        NULL,  
                        NULL,  
                        FALSE, 
                        0,     
                        NULL,  
                        NULL,  
                        &si,
                        &pi);
  } else {
    
    
    LPVOID environmentBlock = NULL;
    if (!CreateEnvironmentBlock(&environmentBlock, userToken, TRUE)) {
      environmentBlock = NULL;
    }

    ok = CreateProcessAsUserW(userToken, 
                              exePath,
                              cl,
                              NULL,  
                              NULL,  
                              FALSE, 
                              CREATE_DEFAULT_ERROR_MODE |
#ifdef DEBUG
                              CREATE_NEW_CONSOLE |
#endif
                              CREATE_UNICODE_ENVIRONMENT,                              
                              environmentBlock,
                              NULL,  
                              &si,
                              &pi);

    if (environmentBlock) {
      DestroyEnvironmentBlock(environmentBlock);
    }
  }

  if (ok) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  } else {
    LPVOID lpMsgBuf = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL);
    wprintf(L"Error restarting: %s\n", lpMsgBuf ? lpMsgBuf : L"(null)");
    if (lpMsgBuf)
      LocalFree(lpMsgBuf);
  }

  free(cl);

  return ok;
}

