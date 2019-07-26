




#ifndef MOZILLA_LAYERS_SHMEMYCBCRIMAGE_H
#define MOZILLA_LAYERS_SHMEMYCBCRIMAGE_H

#include "base/basictypes.h"
#include "Shmem.h"
#include "gfxPoint.h"

namespace mozilla {
namespace ipc {
  class Shmem;
}
namespace layers {













class ShmemYCbCrImage
{
public:
  typedef mozilla::ipc::Shmem Shmem;

  ShmemYCbCrImage() : mOffset(0) {}

  ShmemYCbCrImage(Shmem& shm, size_t offset = 0) {
    NS_ABORT_IF_FALSE(Open(shm,offset), "Invalid data in Shmem.");
  }

  




  static size_t ComputeMinBufferSize(const gfxIntSize& aYSize,
                                     const gfxIntSize& aCbCrSize);
  




  static void InitializeBufferInfo(uint8_t* aBuffer,
                                   const gfxIntSize& aYSize,
                                   const gfxIntSize& aCbCrSize);

  


  bool IsValid();

  


  uint8_t* GetYData();
  


  uint8_t* GetCbData();
  


  uint8_t* GetCrData();

  


  uint32_t GetYStride();
  


  uint32_t GetCbCrStride();

  


  gfxIntSize GetYSize();

  


  gfxIntSize GetCbCrSize();

private:
  bool Open(Shmem& aShmem, size_t aOffset = 0);

  ipc::Shmem mShmem;
  size_t mOffset;
};

} 
} 

#endif
