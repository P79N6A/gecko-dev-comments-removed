





































#ifndef XP_WIN
#error This file only makes sense on Windows.
#endif

#include <windows.h>
#include <stdlib.h>
#include "nsSetDllDirectory.h"

void
SanitizeEnvironmentVariables()
{
  DWORD bufferSize = GetEnvironmentVariableW(L"PATH", NULL, 0);
  if (bufferSize) {
    wchar_t* originalPath = new wchar_t[bufferSize];
    if (bufferSize - 1 == GetEnvironmentVariableW(L"PATH", originalPath, bufferSize)) {
      bufferSize = ExpandEnvironmentStringsW(originalPath, NULL, 0);
      if (bufferSize) {
        wchar_t* newPath = new wchar_t[bufferSize];
        if (ExpandEnvironmentStringsW(originalPath,
                                      newPath,
                                      bufferSize)) {
          SetEnvironmentVariableW(L"PATH", newPath);
        }
        delete[] newPath;
      }
    }
    delete[] originalPath;
  }
}

namespace mozilla {

XPCOM_API(void)
NS_SetDllDirectory(const WCHAR *aDllDirectory)
{
  typedef BOOL
  (WINAPI *pfnSetDllDirectory) (LPCWSTR);
  static pfnSetDllDirectory setDllDirectory = nsnull;
  if (!setDllDirectory) {
    setDllDirectory = reinterpret_cast<pfnSetDllDirectory>
      (GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetDllDirectoryW"));

    
    
    SanitizeEnvironmentVariables();
  }
  if (setDllDirectory) {
    setDllDirectory(aDllDirectory);
  }
}

}

