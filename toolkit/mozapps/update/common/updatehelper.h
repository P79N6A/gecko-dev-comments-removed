




































BOOL LaunchWinPostProcess(const WCHAR *installationDir,
                          const WCHAR *updateInfoDir, 
                          bool forceSync,
                          HANDLE userToken);
BOOL StartServiceUpdate(int argc, LPWSTR *argv);
BOOL GetUpdateDirectoryPath(LPWSTR path);
DWORD LaunchServiceSoftwareUpdateCommand(DWORD argc, LPCWSTR *argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL WriteStatusPending(LPCWSTR updateDirPath);
DWORD WaitForServiceStop(LPCWSTR serviceName, DWORD maxWaitSeconds);
DWORD WaitForProcessExit(LPCWSTR filename, DWORD maxSeconds);

#define SVC_NAME L"MozillaMaintenance"
