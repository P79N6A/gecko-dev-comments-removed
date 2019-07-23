






































#include <sys/mman.h>         
#include <unistd.h>           

#include "mozilla/ipc/SharedMemory.h"

namespace mozilla {
namespace ipc {

void
SharedMemory::SystemProtect(char* aAddr, size_t aSize, int aRights)
{
  int flags = 0;
  if (aRights & RightsRead)
    flags |= PROT_READ;
  if (aRights & RightsWrite)
    flags |= PROT_WRITE;
  if (RightsNone == aRights)
    flags = PROT_NONE;

  if (0 < mprotect(aAddr, aSize, flags))
    NS_RUNTIMEABORT("can't mprotect()");
}

size_t
SharedMemory::SystemPageSize()
{
  return sysconf(_SC_PAGESIZE);
}

} 
} 
