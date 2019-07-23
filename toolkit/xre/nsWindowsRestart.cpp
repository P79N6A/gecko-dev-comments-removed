








































#ifdef nsWindowsRestart_cpp
#error "nsWindowsRestart.cpp is not a header file, and must only be included once."
#else
#define nsWindowsRestart_cpp
#endif

#include "nsUTF8Utils.h"

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







static PRUnichar*
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




static BOOL
LaunchAsNormalUser(const PRUnichar *exePath, PRUnichar *cl)
{
#ifdef WINCE
  return PR_FALSE;
#else
  if (!pCreateProcessWithTokenW) {
    
    *(FARPROC *)&pIsUserAnAdmin =
        GetProcAddress(GetModuleHandleA("shell32.dll"), "IsUserAnAdmin");

    
    *(FARPROC *)&pCreateProcessWithTokenW =
        GetProcAddress(GetModuleHandleA("advapi32.dll"),
                       "CreateProcessWithTokenW");

    if (!pCreateProcessWithTokenW)
      return FALSE;
  }

  
  if (!pIsUserAnAdmin || pIsUserAnAdmin && !pIsUserAnAdmin())
    return FALSE;

  
  HWND hwndShell = FindWindowA("Progman", NULL);
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

  
  
  
  WCHAR* myenv = GetEnvironmentStringsW();

  ok = pCreateProcessWithTokenW(hNewToken,
                                0,    
                                exePath,
                                cl,
                                CREATE_UNICODE_ENVIRONMENT,
                                myenv, 
                                NULL, 
                                &si,
                                &pi);

  if (myenv)
    FreeEnvironmentStringsW(myenv);

  CloseHandle(hNewToken);
  if (!ok)
    return FALSE;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return TRUE;
#endif
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
WinLaunchChild(const PRUnichar *exePath, int argc, PRUnichar **argv, int needElevation);

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, char **argv, int needElevation)
{
  PRUnichar** argvConverted = new PRUnichar*[argc];
  if (!argvConverted)
    return FALSE;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF8toUTF16(argv[i]);
    if (!argvConverted[i]) {
      return FALSE;
    }
  }

  BOOL ok = WinLaunchChild(exePath, argc, argvConverted, needElevation);
  FreeAllocStrings(argc, argvConverted);
  return ok;
}

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, PRUnichar **argv, int needElevation)
{
  PRUnichar *cl;
  BOOL ok;
  if (needElevation > 0) {
    cl = MakeCommandLine(argc - 1, argv + 1);
    if (!cl)
      return FALSE;
    ok = ShellExecuteW(NULL, 
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
    STARTUPINFOW si = {sizeof(si), 0};
    PROCESS_INFORMATION pi = {0};

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

    if (ok) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
  }

  free(cl);

  return ok;
}
