



#ifndef XP_WIN
#error This file only makes sense on Windows.
#endif

#include "nsUTF8Utils.h"

#ifndef XRE_DONT_PROTECT_DLL_LOAD
#include "nsSetDllDirectory.h"
#endif

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)) && defined(XRE_WANT_DLL_BLOCKLIST)
#include "nsWindowsDllBlocklist.cpp"
#else
#undef XRE_WANT_DLL_BLOCKLIST
#endif


#ifdef __MINGW32__





#include <shellapi.h>

int wmain(int argc, WCHAR **argv);

int main(int argc, char **argv)
{
  LPWSTR commandLine = GetCommandLineW();
  int argcw = 0;
  LPWSTR *argvw = CommandLineToArgvW(commandLine, &argcw);
  if (!argvw)
    return 127;

  int result = wmain(argcw, argvw);
  LocalFree(argvw);
  return result;
}
#endif 

#define main NS_internal_main

int main(int argc, char **argv);

static char*
AllocConvertUTF16toUTF8(const WCHAR *arg)
{
  
  int len = wcslen(arg);
  char *s = new char[len * 3 + 1];
  if (!s)
    return NULL;

  ConvertUTF16toUTF8 convert(s);
  convert.write(arg, len);
  convert.write_terminator();
  return s;
}

static void
FreeAllocStrings(int argc, char **argv)
{
  while (argc) {
    --argc;
    delete [] argv[argc];
  }

  delete [] argv;
}

#ifdef XRE_PRELOAD_XUL
static void preload(LPCWSTR dll)
{
  HANDLE fd = CreateFileW(dll, GENERIC_READ, FILE_SHARE_READ,
                          NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  char buf[64 * 1024];

  if (fd == INVALID_HANDLE_VALUE)
    return;
  
  DWORD dwBytesRead;
  
  
  
  while (ReadFile(fd, buf, sizeof(buf), &dwBytesRead, NULL) && dwBytesRead == sizeof(buf))
    ;
  
  CloseHandle(fd);
}


static void preload_libs(LPCWSTR appPath) 
{
  wchar_t buf[MAX_PATH];
  size_t pathlen = wcslen(appPath);

  

 
  

    pathlen++;

  if (pathlen + 10 > MAX_PATH) {
    return;
  }

  wcsncpy(buf, appPath, pathlen);
  size_t remaining = sizeof(buf)/sizeof(buf[0]) - pathlen - 1;
  
  
  LPCWSTR files[] = {L"xul.dll", 
#ifndef MOZ_STATIC_JS
                     L"mozjs.dll",
#endif
                     0};
  for(int i = 0; files[i]; i++) {
    wcsncpy(buf + pathlen, files[i], remaining);
    preload(buf);
  }
  return;
}
#endif

int wmain(int argc, WCHAR **argv)
{
#ifdef XRE_PRELOAD_XUL 
  
  
  
  
  
  IO_COUNTERS ioCounters;
  if (GetProcessIoCounters(GetCurrentProcess(), &ioCounters)
      && !ioCounters.ReadOperationCount)
    {
      preload_libs(argv[0]);
    }
#endif

#ifndef XRE_DONT_PROTECT_DLL_LOAD
  mozilla::NS_SetDllDirectory(L"");
#endif

#ifdef XRE_WANT_DLL_BLOCKLIST
  SetupDllBlocklist();
#endif
  
  char **argvConverted = new char*[argc + 1];
  if (!argvConverted)
    return 127;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF16toUTF8(argv[i]);
    if (!argvConverted[i]) {
      return 127;
    }
  }
  argvConverted[argc] = NULL;

  
  char **deleteUs = new char*[argc+1];
  if (!deleteUs) {
    FreeAllocStrings(argc, argvConverted);
    return 127;
  }
  for (int i = 0; i < argc; i++)
    deleteUs[i] = argvConverted[i];
  int result = main(argc, argvConverted);

  delete[] argvConverted;
  FreeAllocStrings(argc, deleteUs);

  return result;
}
