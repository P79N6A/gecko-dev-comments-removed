







































#ifdef nsWindowsRestart_cpp
#error "nsWindowsRestart.cpp is not a header file, and must only be included once."
#else
#define nsWindowsRestart_cpp
#endif

#include <shellapi.h>

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




static int QuotedStrLen(const char *s)
{
  int i = 2; 
  while (*s) {
    if (*s == '"') {
      ++i;
    }

    ++i; ++s;
  }
  return i;
}








static char* QuoteString(char *d, const char *s)
{
  *d = '"';
  ++d;

  while (*s) {
    *d = *s;
    if (*s == '"') {
      ++d;
      *d = '"';
    }

    ++d; ++s;
  }

  *d = '"';
  ++d;

  return d;
}





static char*
MakeCommandLine(int argc, char **argv)
{
  int i;
  int len = 1; 

  for (i = 0; i < argc; ++i)
    len += QuotedStrLen(argv[i]) + 1;

  char *s = (char*) malloc(len);
  if (!s)
    return NULL;

  char *c = s;
  for (i = 0; i < argc; ++i) {
    c = QuoteString(c, argv[i]);
    *c = ' ';
    ++c;
  }

  *c = '\0';

  return s;
}




static PRUnichar *
AllocConvertAToW(const char *buf)
{
  PRUint32 inputLen = strlen(buf) + 1;
  int n = MultiByteToWideChar(CP_ACP, 0, buf, inputLen, NULL, 0);
  if (n <= 0)
    return NULL;
  PRUnichar *result = (PRUnichar *)malloc(n * sizeof(PRUnichar));
  if (!result)
    return NULL;
  MultiByteToWideChar(CP_ACP, 0, buf, inputLen, result, n);
  return result;
}




static BOOL
LaunchAsNormalUser(const char *exePath, char *cl)
{
  if (!pCreateProcessWithTokenW) {
    
    *(FARPROC *)&pIsUserAnAdmin =
        GetProcAddress(GetModuleHandle("shell32.dll"), "IsUserAnAdmin");

    
    *(FARPROC *)&pCreateProcessWithTokenW =
        GetProcAddress(GetModuleHandle("advapi32.dll"),
                       "CreateProcessWithTokenW");

    if (!pCreateProcessWithTokenW)
      return FALSE;
  }

  
  if (!pIsUserAnAdmin || pIsUserAnAdmin && !pIsUserAnAdmin())
    return FALSE;

  
  HWND hwndShell = FindWindow("Progman", NULL);
  DWORD dwProcessId;
  GetWindowThreadProcessId(hwndShell, &dwProcessId);

  HANDLE hProcessShell = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcessId);
  if (!hProcessShell)
    return FALSE;

  HANDLE hTokenShell;
  BOOL ok = OpenProcessToken(hProcessShell, MAXIMUM_ALLOWED, &hTokenShell);
  CloseHandle(hProcessShell);
  if (!ok)
    return FALSE;

  HANDLE hNewToken;
  ok = DuplicateTokenEx(hTokenShell,
                        MAXIMUM_ALLOWED,
                        NULL,
                        SecurityDelegation,
                        TokenPrimary,
                        &hNewToken);
  CloseHandle(hTokenShell);
  if (!ok)
    return FALSE;

  STARTUPINFOW si = {sizeof(si), 0};
  PROCESS_INFORMATION pi = {0};

  PRUnichar *exePathW = AllocConvertAToW(exePath);
  PRUnichar *clW = AllocConvertAToW(cl);
  ok = exePathW && clW;
  if (ok) {
    ok = pCreateProcessWithTokenW(hNewToken,
                                  0,    
                                  exePathW,
                                  clW,
                                  0,    
                                  NULL, 
                                  NULL, 
                                  &si,
                                  &pi);
  }
  free(exePathW);
  free(clW);
  CloseHandle(hNewToken);
  if (!ok)
    return FALSE;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return TRUE;
}






BOOL
WinLaunchChild(const char *exePath, int argc, char **argv, int needElevation)
{
  char *cl;
  BOOL ok;
  if (needElevation > 0) {
    cl = MakeCommandLine(argc - 1, argv + 1);
    if (!cl)
      return FALSE;
    ok = ShellExecute(NULL, 
                      NULL, 
                      exePath,
                      cl,
                      NULL, 
                      SW_SHOWDEFAULT) > (HINSTANCE)32;
    free(cl);
    return ok;
  }

  cl = MakeCommandLine(argc, argv);
  if (!cl)
    return FALSE;

  if (needElevation < 0) {
    
    ok = LaunchAsNormalUser(exePath, cl);
    
    if (!ok)
      needElevation = 0;
  }
  if (needElevation == 0) {
    STARTUPINFO si = {sizeof(si), 0};
    PROCESS_INFORMATION pi = {0};

    ok = CreateProcess(exePath,
                       cl,
                       NULL,  
                       NULL,  
                       FALSE, 
                       0,     
                       NULL,  
                       NULL,  
                       &si,
                       &pi);

    if (ok) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
  }

  free(cl);

  return ok;
}
