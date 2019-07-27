







#ifndef XP_WIN
#error This file only makes sense on Windows.
#endif

#include "nsUTF8Utils.h"
#include <intrin.h>
#include <math.h>

#ifndef XRE_DONT_PROTECT_DLL_LOAD
#include "nsSetDllDirectory.h"
#endif

#if defined(__GNUC__)
#define XRE_DONT_SUPPORT_XPSP2
#endif

#ifndef XRE_DONT_SUPPORT_XPSP2
#include "WindowsCrtPatch.h"
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

#ifndef XRE_WANT_ENVIRON
int main(int argc, char **argv);
#else
int main(int argc, char **argv, char **envp);
#endif

static char*
AllocConvertUTF16toUTF8(char16ptr_t arg)
{
  
  int len = wcslen(arg);
  char *s = new char[len * 3 + 1];
  if (!s)
    return nullptr;

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

int wmain(int argc, WCHAR **argv)
{
#if !defined(XRE_DONT_SUPPORT_XPSP2)
  WindowsCrtPatch::Init();
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900 && defined(_M_X64)
  
  int cpuid0[4] = {0};
  int cpuid7[4] = {0};
  __cpuid(cpuid0, 0); 
  __cpuid(cpuid7, 7); 
  if (cpuid0[0] < 7 || !(cpuid7[1] & 0x20)) {
    _set_FMA3_enable(0);
  }
#endif

#ifndef XRE_DONT_PROTECT_DLL_LOAD
  mozilla::SanitizeEnvironmentVariables();
  SetDllDirectoryW(L"");
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
  argvConverted[argc] = nullptr;

  
  char **deleteUs = new char*[argc+1];
  if (!deleteUs) {
    FreeAllocStrings(argc, argvConverted);
    return 127;
  }
  for (int i = 0; i < argc; i++)
    deleteUs[i] = argvConverted[i];
#ifndef XRE_WANT_ENVIRON
  int result = main(argc, argvConverted);
#else
  
  getenv("PATH");
  int result = main(argc, argvConverted, _environ);
#endif

  delete[] argvConverted;
  FreeAllocStrings(argc, deleteUs);

  return result;
}
