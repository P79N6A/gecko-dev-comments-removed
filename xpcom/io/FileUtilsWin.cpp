





#include "FileUtilsWin.h"

#include <windows.h>
#include <psapi.h>

#include "nsWindowsHelpers.h"

namespace {


struct ScopedMappedViewTraits
{
  typedef void* type;
  static void* empty()
  {
    return nullptr;
  }
  static void release(void* aPtr)
  {
    UnmapViewOfFile(aPtr);
  }
};
typedef mozilla::Scoped<ScopedMappedViewTraits> ScopedMappedView;

} 

namespace mozilla {

bool
HandleToFilename(HANDLE aHandle, const LARGE_INTEGER& aOffset,
                 nsAString& aFilename)
{
  aFilename.Truncate();
  
  
  nsAutoHandle fileMapping(CreateFileMapping(aHandle, nullptr, PAGE_READONLY,
                                             0, 1, nullptr));
  if (!fileMapping) {
    return false;
  }
  ScopedMappedView view(MapViewOfFile(fileMapping, FILE_MAP_READ,
                                      aOffset.HighPart, aOffset.LowPart, 1));
  if (!view) {
    return false;
  }
  nsAutoString mappedFilename;
  DWORD len = 0;
  SetLastError(ERROR_SUCCESS);
  do {
    mappedFilename.SetLength(mappedFilename.Length() + MAX_PATH);
    len = GetMappedFileNameW(GetCurrentProcess(), view,
                             wwc(mappedFilename.BeginWriting()),
                             mappedFilename.Length());
  } while (!len && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
  if (!len) {
    return false;
  }
  mappedFilename.Truncate(len);
  return NtPathToDosPath(mappedFilename, aFilename);
}

} 

