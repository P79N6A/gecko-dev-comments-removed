








































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
WinLaunchChild(const PRUnichar *exePath, int argc, PRUnichar **argv);

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, char **argv)
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

  BOOL ok = WinLaunchChild(exePath, argc, argvConverted);
  FreeAllocStrings(argc, argvConverted);
  return ok;
}

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, PRUnichar **argv)
{
  PRUnichar *cl;
  BOOL ok;

  cl = MakeCommandLine(argc, argv);
  if (!cl)
    return FALSE;

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
		  NULL
		  );
    wprintf(L"Error restarting: %s\n", lpMsgBuf ? lpMsgBuf : L"(null)");
    if (lpMsgBuf)
      LocalFree(lpMsgBuf);
  }

  free(cl);

  return ok;
}
