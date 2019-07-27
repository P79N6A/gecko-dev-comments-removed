



BOOL LaunchWinPostProcess(const WCHAR *installationDir,
                          const WCHAR *updateInfoDir,
                          bool forceSync,
                          HANDLE userToken);
BOOL StartServiceUpdate(LPCWSTR installDir);
BOOL GetUpdateDirectoryPath(LPWSTR path);
DWORD LaunchServiceSoftwareUpdateCommand(int argc, LPCWSTR *argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL WriteStatusPending(LPCWSTR updateDirPath);
DWORD WaitForServiceStop(LPCWSTR serviceName, DWORD maxWaitSeconds);
DWORD WaitForProcessExit(LPCWSTR filename, DWORD maxSeconds);
BOOL DoesFallbackKeyExist();
BOOL IsLocalFile(LPCWSTR file, BOOL &isLocal);
DWORD StartServiceCommand(int argc, LPCWSTR* argv);
BOOL IsUnpromptedElevation(BOOL &isUnpromptedElevation);

#define SVC_NAME L"MozillaMaintenance"

#define BASE_SERVICE_REG_KEY \
  L"SOFTWARE\\Mozilla\\MaintenanceService"







#define TEST_ONLY_FALLBACK_KEY_PATH \
  BASE_SERVICE_REG_KEY L"\\3932ecacee736d366d6436db0f55bce4"

