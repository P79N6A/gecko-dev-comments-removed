




#ifndef GMPSharedMemManager_h_
#define GMPSharedMemManager_h_

#include "mozilla/ipc/Shmem.h"

namespace mozilla {
namespace gmp {

class GMPSharedMemManager
{
public:
  virtual bool MgrAllocShmem(size_t aSize,
                             ipc::Shmem::SharedMemory::SharedMemoryType aType,
                             ipc::Shmem* aMem) = 0;
  virtual bool MgrDeallocShmem(ipc::Shmem& aMem) = 0;
};

} 
} 

#endif 
