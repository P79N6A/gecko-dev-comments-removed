




































BOOL LaunchWinPostProcess(const WCHAR *installationDir,
                          const WCHAR *updateInfoDir, 
                          bool forceSync,
                          HANDLE userToken);
BOOL StartServiceUpdate(int argc, LPWSTR *argv);
BOOL GetUpdateDirectoryPath(LPWSTR path);
BOOL LaunchServiceSoftwareUpdateCommand(DWORD argc, LPCWSTR *argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL WriteStatusPending(LPCWSTR updateDirPath);
#define SERVICE_EVENT_NAME L"Global\\moz-5b780de9-065b-4341-a04f-ddd94b3723e5"
