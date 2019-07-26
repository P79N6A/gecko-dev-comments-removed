




#ifndef GFX_MACIOSURFACEIMAGE_H
#define GFX_MACIOSURFACEIMAGE_H

#include "ImageContainer.h"
#include "mozilla/gfx/MacIOSurface.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/layers/TextureClient.h"
#include "gfxImageSurface.h"

namespace mozilla {

namespace layers {

class MacIOSurfaceImage : public Image,
                          public ISharedImage {
public:
  void SetSurface(MacIOSurface* aSurface) { mSurface = aSurface; }
  MacIOSurface* GetSurface() { return mSurface; }

  gfx::IntSize GetSize() {
    return gfx::IntSize(mSurface->GetDevicePixelWidth(), mSurface->GetDevicePixelHeight());
  }

  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }

  virtual already_AddRefed<gfxASurface> GetAsSurface() {
    mSurface->Lock();
    size_t bytesPerRow = mSurface->GetBytesPerRow();
    size_t ioWidth = mSurface->GetDevicePixelWidth();
    size_t ioHeight = mSurface->GetDevicePixelHeight();

    unsigned char* ioData = (unsigned char*)mSurface->GetBaseAddress();

    nsRefPtr<gfxImageSurface> imgSurface =
      new gfxImageSurface(gfxIntSize(ioWidth, ioHeight), gfxImageFormatARGB32);

    for (size_t i = 0; i < ioHeight; i++) {
      memcpy(imgSurface->Data() + i * imgSurface->Stride(),
             ioData + i * bytesPerRow, ioWidth * 4);
    }

    mSurface->Unlock();

    return imgSurface.forget();
  }

  virtual TextureClient* GetTextureClient() MOZ_OVERRIDE;
  virtual uint8_t* GetBuffer() MOZ_OVERRIDE { return nullptr; }

  MacIOSurfaceImage() : Image(nullptr, MAC_IOSURFACE) {}

private:
  RefPtr<MacIOSurface> mSurface;
  RefPtr<TextureClient> mTextureClient;
};

} 
} 

#endif 
