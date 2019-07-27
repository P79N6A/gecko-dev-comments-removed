




#ifndef GMPSharedMemManager_h_
#define GMPSharedMemManager_h_

#include "mozilla/ipc/Shmem.h"
#include "nsTArray.h"

namespace mozilla {
namespace gmp {

class GMPSharedMemManager;

class GMPSharedMem
{
public:
  typedef enum {
    kGMPFrameData = 0,
    kGMPEncodedData,
    kGMPNumTypes
  } GMPMemoryClasses;

  
  
  
  
  
  static const uint32_t kGMPBufLimit = 20;

  GMPSharedMem()
  {
    for (size_t i = 0; i < sizeof(mGmpAllocated)/sizeof(mGmpAllocated[0]); i++) {
      mGmpAllocated[i] = 0;
    }
  }
  virtual ~GMPSharedMem() {}

  
  virtual void CheckThread() = 0;

protected:
  friend class GMPSharedMemManager;

  nsTArray<ipc::Shmem> mGmpFreelist[GMPSharedMem::kGMPNumTypes];
  uint32_t mGmpAllocated[GMPSharedMem::kGMPNumTypes];
};

class GMPSharedMemManager
{
public:
  explicit GMPSharedMemManager(GMPSharedMem *aData) : mData(aData) {}
  virtual ~GMPSharedMemManager() {}

  virtual bool MgrAllocShmem(GMPSharedMem::GMPMemoryClasses aClass, size_t aSize,
                             ipc::Shmem::SharedMemory::SharedMemoryType aType,
                             ipc::Shmem* aMem);
  virtual bool MgrDeallocShmem(GMPSharedMem::GMPMemoryClasses aClass, ipc::Shmem& aMem);

  
  virtual uint32_t NumInUse(GMPSharedMem::GMPMemoryClasses aClass);

  
  
  virtual bool Alloc(size_t aSize, ipc::Shmem::SharedMemory::SharedMemoryType aType, ipc::Shmem* aMem) = 0;
  virtual void Dealloc(ipc::Shmem& aMem) = 0;

private:
  nsTArray<ipc::Shmem>& GetGmpFreelist(GMPSharedMem::GMPMemoryClasses aTypes)
  {
    return mData->mGmpFreelist[aTypes];
  }

  GMPSharedMem *mData;
};

} 
} 

#endif 
