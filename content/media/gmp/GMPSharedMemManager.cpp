




#include "GMPSharedMemManager.h"
#include "GMPMessageUtils.h"
#include "mozilla/ipc/SharedMemory.h"

namespace mozilla {
namespace gmp {








static nsTArray<ipc::Shmem> sGmpFreelist[GMPSharedMemManager::kGMPNumTypes];
static uint32_t sGmpAllocated[GMPSharedMemManager::kGMPNumTypes]; 

bool
GMPSharedMemManager::MgrAllocShmem(GMPMemoryClasses aClass, size_t aSize,
                                   ipc::Shmem::SharedMemory::SharedMemoryType aType,
                                   ipc::Shmem* aMem)
{
  CheckThread();

  
  for (uint32_t i = 0; i < sGmpFreelist[aClass].Length(); i++) {
    MOZ_ASSERT(sGmpFreelist[aClass][i].IsWritable());
    if (aSize <= sGmpFreelist[aClass][i].Size<uint8_t>()) {
      *aMem = sGmpFreelist[aClass][i];
      sGmpFreelist[aClass].RemoveElementAt(i);
      return true;
    }
  }

  
  size_t pagesize = ipc::SharedMemory::SystemPageSize();
  aSize = (aSize + (pagesize-1)) & ~(pagesize-1); 
  bool retval = Alloc(aSize, aType, aMem);
  if (retval) {
    sGmpAllocated[aClass]++;
  }
  return retval;
}

bool
GMPSharedMemManager::MgrDeallocShmem(GMPMemoryClasses aClass, ipc::Shmem& aMem)
{
  CheckThread();

  size_t size = aMem.Size<uint8_t>();
  size_t total = 0;
  
  
  if (sGmpFreelist[aClass].Length() > 10) {
    Dealloc(sGmpFreelist[aClass][0]);
    sGmpFreelist[aClass].RemoveElementAt(0);
    
    sGmpAllocated[aClass]--;
  }
  for (uint32_t i = 0; i < sGmpFreelist[aClass].Length(); i++) {
    MOZ_ASSERT(sGmpFreelist[aClass][i].IsWritable());
    total += sGmpFreelist[aClass][i].Size<uint8_t>();
    if (size < sGmpFreelist[aClass][i].Size<uint8_t>()) {
      sGmpFreelist[aClass].InsertElementAt(i, aMem);
      return true;
    }
  }
  sGmpFreelist[aClass].AppendElement(aMem);

  return true;
}

uint32_t
GMPSharedMemManager::NumInUse(GMPMemoryClasses aClass)
{
  return sGmpAllocated[aClass] - sGmpFreelist[aClass].Length();
}

}
}
