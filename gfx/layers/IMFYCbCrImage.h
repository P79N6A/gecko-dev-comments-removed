




#ifndef GFX_IMFYCBCRIMAGE_H
#define GFX_IMFYCBCRIMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "Mfidl.h"

namespace mozilla {
namespace layers {

class IMFYCbCrImage : public PlanarYCbCrImage
{
public:
  IMFYCbCrImage(IMFMediaBuffer* aBuffer, IMF2DBuffer* a2DBuffer)
    : PlanarYCbCrImage(nullptr)
    , mBuffer(aBuffer)
    , m2DBuffer(a2DBuffer)
  {}

  virtual bool IsValid() { return true; }

protected:
  virtual uint8_t* AllocateBuffer(uint32_t aSize) MOZ_OVERRIDE {
    MOZ_CRASH("Can't do manual allocations with IMFYCbCrImage");
    return nullptr;
  }

  ~IMFYCbCrImage()
  {
    if (m2DBuffer) {
      m2DBuffer->Unlock2D();
    }
    else {
      mBuffer->Unlock();
    }
  }

  RefPtr<IMFMediaBuffer> mBuffer;
  RefPtr<IMF2DBuffer> m2DBuffer;
};

} 
} 

#endif 
