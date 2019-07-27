




#ifndef GMPSharedMemManager_h_
#define GMPSharedMemManager_h_

#include "mozilla/ipc/Shmem.h"
#include "nsTArray.h"

namespace mozilla {
namespace gmp {

class GMPSharedMemManager
{
public:
  typedef enum {
    kGMPFrameData = 0,
    kGMPEncodedData,
    kGMPNumTypes
  } GMPMemoryClasses;

  
  
  
  
  
  static const uint32_t kGMPBufLimit = 20;

  GMPSharedMemManager() {}

  virtual ~GMPSharedMemManager() {
    
  }

  virtual bool MgrAllocShmem(GMPMemoryClasses aClass, size_t aSize,
                             ipc::Shmem::SharedMemory::SharedMemoryType aType,
                             ipc::Shmem* aMem);
  virtual bool MgrDeallocShmem(GMPMemoryClasses aClass, ipc::Shmem& aMem);

  
  virtual uint32_t NumInUse(GMPMemoryClasses aClass);

  
  virtual void CheckThread() = 0;

  
  
  virtual bool Alloc(size_t aSize, ipc::Shmem::SharedMemory::SharedMemoryType aType, ipc::Shmem* aMem) = 0;
  virtual void Dealloc(ipc::Shmem& aMem) = 0;
};

} 
} 

#endif 
