







































#include <math.h>

#include "nsIMemoryReporter.h"
#include "mozilla/ipc/SharedMemory.h"

namespace mozilla {
namespace ipc {

static PRInt64 gShmemAllocated;
static PRInt64 gShmemMapped;
static PRInt64 GetShmemAllocated(void*) { return gShmemAllocated; }
static PRInt64 GetShmemMapped(void*) { return gShmemMapped; }

NS_MEMORY_REPORTER_IMPLEMENT(ShmemAllocated,
                             "shmem/allocated",
                             "Shmem bytes accessible (not necessarily mapped)",
                             GetShmemAllocated,
                             nsnull)
NS_MEMORY_REPORTER_IMPLEMENT(ShmemMapped,
                             "shmem/mapped",
                             "Shmem bytes mapped into address space",
                             GetShmemMapped,
                             nsnull)

SharedMemory::SharedMemory()
{
  
  
  static bool registered;
  if (!registered) {
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(ShmemAllocated));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(ShmemMapped));
    registered = true;
  }
}

 size_t
SharedMemory::PageAlignedSize(size_t aSize)
{
  size_t pageSize = SystemPageSize();
  size_t nPagesNeeded = size_t(ceil(double(aSize) / double(pageSize)));
  return pageSize * nPagesNeeded;
}

 void
SharedMemory::Created(size_t aNBytes)
{
  gShmemAllocated += aNBytes;
}

 void
SharedMemory::Mapped(size_t aNBytes)
{
  gShmemMapped += aNBytes;
}

 void
SharedMemory::Unmapped(size_t aNBytes)
{
  NS_ABORT_IF_FALSE(gShmemMapped >= PRInt64(aNBytes),
                    "Can't unmap more than mapped");
  gShmemMapped -= aNBytes;
}

 void
SharedMemory::Destroyed(size_t aNBytes)
{
  NS_ABORT_IF_FALSE(gShmemAllocated >= PRInt64(aNBytes),
                    "Can't destroy more than allocated");
  gShmemAllocated -= aNBytes;
}

} 
} 
