




































void WINAPI SvcMain(DWORD dwArgc, LPWSTR *lpszArgv);
void SvcInit(DWORD dwArgc, LPWSTR *lpszArgv);
void WINAPI SvcCtrlHandler(DWORD dwCtrl);
void ReportSvcStatus(DWORD dwCurrentState, 
                     DWORD dwWin32ExitCode, 
                     DWORD dwWaitHint);
