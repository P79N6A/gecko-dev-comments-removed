




#ifndef GFX_MACIOSURFACEIMAGE_H
#define GFX_MACIOSURFACEIMAGE_H

#include "mozilla/gfx/MacIOSurface.h"
#include "gfxImageSurface.h"

namespace mozilla {

namespace layers {

class MacIOSurfaceImage : public Image {
public:
  void SetSurface(MacIOSurface* aSurface) { mSurface = aSurface; }
  MacIOSurface* GetSurface() { return mSurface; }

  gfxIntSize GetSize() {
    return gfxIntSize(mSurface->GetDevicePixelWidth(), mSurface->GetDevicePixelHeight());
  }

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

  MacIOSurfaceImage() : Image(nullptr, MAC_IOSURFACE) {}

private:
  RefPtr<MacIOSurface> mSurface;
};

} 
} 

#endif 
