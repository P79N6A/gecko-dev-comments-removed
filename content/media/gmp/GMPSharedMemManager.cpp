




#include "GMPSharedMemManager.h"
#include "GMPMessageUtils.h"
#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"

namespace mozilla {
namespace gmp {








static StaticAutoPtr<nsTArray<ipc::Shmem>> sGmpFreelist[GMPSharedMemManager::kGMPNumTypes];
static uint32_t sGMPShmemManagerCount = 0;

GMPSharedMemManager::GMPSharedMemManager()
{
  if (!sGMPShmemManagerCount) {
    for (uint32_t i = 0; i < GMPSharedMemManager::kGMPNumTypes; i++) {
      sGmpFreelist[i] = new nsTArray<ipc::Shmem>();
    }
  }
  sGMPShmemManagerCount++;
}

GMPSharedMemManager::~GMPSharedMemManager()
{
  MOZ_ASSERT(sGMPShmemManagerCount > 0);
  sGMPShmemManagerCount--;
  if (!sGMPShmemManagerCount) {
    for (uint32_t i = 0; i < GMPSharedMemManager::kGMPNumTypes; i++) {
      sGmpFreelist[i] = nullptr;
    }
  }
}

static nsTArray<ipc::Shmem>&
GetGmpFreelist(GMPSharedMemManager::GMPMemoryClasses aTypes)
{
  return *(sGmpFreelist[aTypes]);
}

static uint32_t sGmpAllocated[GMPSharedMemManager::kGMPNumTypes]; 

bool
GMPSharedMemManager::MgrAllocShmem(GMPMemoryClasses aClass, size_t aSize,
                                   ipc::Shmem::SharedMemory::SharedMemoryType aType,
                                   ipc::Shmem* aMem)
{
  CheckThread();

  
  for (uint32_t i = 0; i < GetGmpFreelist(aClass).Length(); i++) {
    MOZ_ASSERT(GetGmpFreelist(aClass)[i].IsWritable());
    if (aSize <= GetGmpFreelist(aClass)[i].Size<uint8_t>()) {
      *aMem = GetGmpFreelist(aClass)[i];
      GetGmpFreelist(aClass).RemoveElementAt(i);
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
  
  
  if (GetGmpFreelist(aClass).Length() > 10) {
    Dealloc(GetGmpFreelist(aClass)[0]);
    GetGmpFreelist(aClass).RemoveElementAt(0);
    
    sGmpAllocated[aClass]--;
  }
  for (uint32_t i = 0; i < GetGmpFreelist(aClass).Length(); i++) {
    MOZ_ASSERT(GetGmpFreelist(aClass)[i].IsWritable());
    total += GetGmpFreelist(aClass)[i].Size<uint8_t>();
    if (size < GetGmpFreelist(aClass)[i].Size<uint8_t>()) {
      GetGmpFreelist(aClass).InsertElementAt(i, aMem);
      return true;
    }
  }
  GetGmpFreelist(aClass).AppendElement(aMem);

  return true;
}

uint32_t
GMPSharedMemManager::NumInUse(GMPMemoryClasses aClass)
{
  return sGmpAllocated[aClass] - GetGmpFreelist(aClass).Length();
}

}
}
