





#include <math.h>

#include "nsString.h"
#include "nsIMemoryReporter.h"
#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/Atomics.h"

namespace mozilla {
namespace ipc {

static Atomic<size_t> gShmemAllocated;
static Atomic<size_t> gShmemMapped;

class ShmemReporter MOZ_FINAL : public nsIMemoryReporter
{
  ~ShmemReporter() {}

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  NS_IMETHOD
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize) MOZ_OVERRIDE
  {
    nsresult rv;
    rv = MOZ_COLLECT_REPORT(
      "shmem-allocated", KIND_OTHER, UNITS_BYTES, gShmemAllocated,
      "Memory shared with other processes that is accessible (but not "
      "necessarily mapped).");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = MOZ_COLLECT_REPORT(
      "shmem-mapped", KIND_OTHER, UNITS_BYTES, gShmemMapped,
      "Memory shared with other processes that is mapped into the address "
      "space.");
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS(ShmemReporter, nsIMemoryReporter)

SharedMemory::SharedMemory()
  : mAllocSize(0)
  , mMappedSize(0)
{
  static Atomic<bool> registered;
  if (registered.compareExchange(false, true)) {
    RegisterStrongMemoryReporter(new ShmemReporter());
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
  MOZ_ASSERT(gShmemMapped >= mMappedSize,
             "Can't unmap more than mapped");
  gShmemMapped -= mMappedSize;
  mMappedSize = 0;
}

 void
SharedMemory::Destroyed()
{
  MOZ_ASSERT(gShmemAllocated >= mAllocSize,
             "Can't destroy more than allocated");
  gShmemAllocated -= mAllocSize;
  mAllocSize = 0;
}

} 
} 
