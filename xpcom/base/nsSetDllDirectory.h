





































#ifndef nsSetDllDirectory_h
#define nsSetDllDirectory_h

#ifndef XP_WIN
#error This file only makes sense on Windows.
#endif

#include <windows.h>
#include <nscore.h>
#include <stdlib.h>

namespace mozilla {

static void SanitizeEnvironmentVariables()
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





static inline void NS_SetDllDirectory(const WCHAR *aDllDirectory)
{
  typedef BOOL
  (WINAPI *pfnSetDllDirectory) (LPCWSTR);
  pfnSetDllDirectory setDllDirectory = nsnull;
  setDllDirectory = reinterpret_cast<pfnSetDllDirectory>
      (GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetDllDirectoryW"));
  if (setDllDirectory) {
    setDllDirectory(aDllDirectory);
  }
}

}

#endif
