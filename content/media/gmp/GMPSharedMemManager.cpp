




#include "GMPSharedMemManager.h"
#include "GMPMessageUtils.h"
#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"

namespace mozilla {
namespace gmp {








bool
GMPSharedMemManager::MgrAllocShmem(GMPSharedMem::GMPMemoryClasses aClass, size_t aSize,
                                   ipc::Shmem::SharedMemory::SharedMemoryType aType,
                                   ipc::Shmem* aMem)
{
  mData->CheckThread();

  
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
  
  MOZ_ASSERT(aMem->Size<uint8_t>() >= aSize);
  if (retval) {
    mData->mGmpAllocated[aClass]++;
  }
  return retval;
}

bool
GMPSharedMemManager::MgrDeallocShmem(GMPSharedMem::GMPMemoryClasses aClass, ipc::Shmem& aMem)
{
  mData->CheckThread();

  size_t size = aMem.Size<uint8_t>();
  size_t total = 0;

  
  
  for (uint32_t i = 0; i < GetGmpFreelist(aClass).Length(); i++) {
    if (NS_WARN_IF(aMem == GetGmpFreelist(aClass)[i])) {
      
      
      MOZ_CRASH("Deallocating Shmem we already have in our cache!");
      
    }
  }

  
  
  if (GetGmpFreelist(aClass).Length() > 10) {
    Dealloc(GetGmpFreelist(aClass)[0]);
    GetGmpFreelist(aClass).RemoveElementAt(0);
    
    mData->mGmpAllocated[aClass]--;
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
GMPSharedMemManager::NumInUse(GMPSharedMem::GMPMemoryClasses aClass)
{
  return mData->mGmpAllocated[aClass] - GetGmpFreelist(aClass).Length();
}

}
}
