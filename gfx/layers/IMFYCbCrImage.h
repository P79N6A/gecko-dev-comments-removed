




#ifndef GFX_IMFYCBCRIMAGE_H
#define GFX_IMFYCBCRIMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "mfidl.h"

namespace mozilla {
namespace layers {

class IMFYCbCrImage : public PlanarYCbCrImage
{
public:
  IMFYCbCrImage(IMFMediaBuffer* aBuffer, IMF2DBuffer* a2DBuffer);

  virtual bool IsValid() { return true; }

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

protected:
  virtual uint8_t* AllocateBuffer(uint32_t aSize) override {
    MOZ_CRASH("Can't do manual allocations with IMFYCbCrImage");
    return nullptr;
  }

  TextureClient* GetD3D9TextureClient(CompositableClient* aClient);

  ~IMFYCbCrImage();

  RefPtr<IMFMediaBuffer> mBuffer;
  RefPtr<IMF2DBuffer> m2DBuffer;
  RefPtr<TextureClient> mTextureClient;
};

} 
} 

#endif 
