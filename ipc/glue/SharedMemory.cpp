





#include <math.h>

#include "nsString.h"
#include "nsIMemoryReporter.h"
#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/Atomics.h"

namespace mozilla {
namespace ipc {

static Atomic<size_t> gShmemAllocated;
static Atomic<size_t> gShmemMapped;

class ShmemAllocatedReporter MOZ_FINAL : public MemoryReporterBase
{
public:
  ShmemAllocatedReporter()
    : MemoryReporterBase("shmem-allocated", KIND_OTHER, UNITS_BYTES,
"Memory shared with other processes that is accessible (but not necessarily "
"mapped).")
  {}
private:
  int64_t Amount() MOZ_OVERRIDE { return gShmemAllocated; }
};

class ShmemMappedReporter MOZ_FINAL : public MemoryReporterBase
{
public:
  ShmemMappedReporter()
    : MemoryReporterBase("shmem-mapped", KIND_OTHER, UNITS_BYTES,
"Memory shared with other processes that is mapped into the address space.")
  {}
private:
  int64_t Amount() MOZ_OVERRIDE { return gShmemMapped; }
};

SharedMemory::SharedMemory()
  : mAllocSize(0)
  , mMappedSize(0)
{
  static Atomic<uint32_t> registered;
  if (registered.compareExchange(0, 1)) {
    NS_RegisterMemoryReporter(new ShmemAllocatedReporter());
    NS_RegisterMemoryReporter(new ShmemMappedReporter());
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
  NS_ABORT_IF_FALSE(gShmemMapped >= mMappedSize,
                    "Can't unmap more than mapped");
  gShmemMapped -= mMappedSize;
  mMappedSize = 0;
}

 void
SharedMemory::Destroyed()
{
  NS_ABORT_IF_FALSE(gShmemAllocated >= mAllocSize,
                    "Can't destroy more than allocated");
  gShmemAllocated -= mAllocSize;
  mAllocSize = 0;
}

} 
} 
