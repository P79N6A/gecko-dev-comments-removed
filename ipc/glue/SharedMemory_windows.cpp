






































#include <windows.h>

#include "mozilla/ipc/SharedMemory.h"

namespace mozilla {
namespace ipc {

void
SharedMemory::SystemProtect(char* aAddr, size_t aSize, int aRights)
{
  DWORD flags;
  if ((aRights & RightsRead) && (aRights & RightsWrite))
    flags = PAGE_READWRITE;
  else if (aRights & RightsRead)
    flags = PAGE_READONLY;
  else
    flags = PAGE_NOACCESS;

  DWORD oldflags;
  if (!VirtualProtect(aAddr, aSize, flags, &oldflags))
    NS_RUNTIMEABORT("can't VirtualProtect()");
}

size_t
SharedMemory::SystemPageSize()
{
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
}

} 
} 
