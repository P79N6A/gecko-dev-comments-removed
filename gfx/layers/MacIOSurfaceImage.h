




#ifndef GFX_MACIOSURFACEIMAGE_H
#define GFX_MACIOSURFACEIMAGE_H

#include "ImageContainer.h"
#include "mozilla/gfx/MacIOSurface.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/layers/TextureClient.h"

namespace mozilla {

namespace layers {

class MacIOSurfaceImage : public Image {
public:
  void SetSurface(MacIOSurface* aSurface) { mSurface = aSurface; }
  MacIOSurface* GetSurface() { return mSurface; }

  gfx::IntSize GetSize() MOZ_OVERRIDE {
    return gfx::IntSize(mSurface->GetDevicePixelWidth(), mSurface->GetDevicePixelHeight());
  }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE;

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) MOZ_OVERRIDE;

  MacIOSurfaceImage() : Image(nullptr, ImageFormat::MAC_IOSURFACE) {}

private:
  RefPtr<MacIOSurface> mSurface;
  RefPtr<TextureClient> mTextureClient;
};

} 
} 

#endif 
