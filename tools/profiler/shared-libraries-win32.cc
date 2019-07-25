





































#include <windows.h>
#include <tlhelp32.h>

#include "shared-libraries.h"
#include "nsWindowsHelpers.h"

SharedLibraryInfo SharedLibraryInfo::GetInfoForSelf()
{
  SharedLibraryInfo sharedLibraryInfo;

  nsAutoHandle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId()));

  MODULEENTRY32 module = {0};
  module.dwSize = sizeof(MODULEENTRY32);
  if (Module32First(snap, &module)) {
    do {
      SharedLibrary shlib((uintptr_t)module.modBaseAddr,
                          (uintptr_t)module.modBaseAddr+module.modBaseSize,
                          0, 
                          module.szModule);
      sharedLibraryInfo.AddSharedLibrary(shlib);
    } while (Module32Next(snap, &module));
  }

  return sharedLibraryInfo;
}

