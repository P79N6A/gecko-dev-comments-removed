







































#include <math.h>

#include "nsIMemoryReporter.h"
#include "mozilla/ipc/SharedMemory.h"

namespace mozilla {
namespace ipc {

static PRInt64 gShmemAllocated;
static PRInt64 gShmemMapped;
static PRInt64 GetShmemAllocated() { return gShmemAllocated; }
static PRInt64 GetShmemMapped() { return gShmemMapped; }

NS_MEMORY_REPORTER_IMPLEMENT(ShmemAllocated,
    "shmem-allocated",
    KIND_OTHER,
    UNITS_BYTES,
    GetShmemAllocated,
    "Memory shared with other processes that is accessible (but not "
    "necessarily mapped).")
NS_MEMORY_REPORTER_IMPLEMENT(ShmemMapped,
    "shmem-mapped",
    KIND_OTHER,
    UNITS_BYTES,
    GetShmemMapped,
    "Memory shared with other processes that is mapped into the address space.")

SharedMemory::SharedMemory()
  : mAllocSize(0)
  , mMappedSize(0)
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
  mAllocSize = aNBytes;
  gShmemAllocated += mAllocSize;
}

void
SharedMemory::Mapped(size_t aNBytes)
{
  mMappedSize = aNBytes;
  gShmemMapped += mMappedSize;
}

void
SharedMemory::Unmapped()
{
  NS_ABORT_IF_FALSE(gShmemMapped >= PRInt64(mMappedSize),
                    "Can't unmap more than mapped");
  gShmemMapped -= mMappedSize;
  mMappedSize = 0;
}

 void
SharedMemory::Destroyed()
{
  NS_ABORT_IF_FALSE(gShmemAllocated >= PRInt64(mAllocSize),
                    "Can't destroy more than allocated");
  gShmemAllocated -= mAllocSize;
  mAllocSize = 0;
}

} 
} 
