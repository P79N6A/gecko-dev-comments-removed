





































#ifndef XP_WIN
#error This file only makes sense on Windows.
#endif

#include <windows.h>
#include "nsSetDllDirectory.h"

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
  }
  if (setDllDirectory) {
    setDllDirectory(aDllDirectory);
  }
}

}

