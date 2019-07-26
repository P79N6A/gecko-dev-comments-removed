




#ifndef GRALLOCIMAGES_H
#define GRALLOCIMAGES_H

#ifdef MOZ_WIDGET_GONK

#include "ImageLayers.h"
#include "ImageContainer.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/layers/AtomicRefCountedWithFinalize.h"
#include "mozilla/layers/FenceUtils.h"
#include "mozilla/layers/LayersSurfaces.h"

#include <ui/GraphicBuffer.h>

namespace mozilla {
namespace layers {

class GrallocTextureClientOGL;






















class GrallocImage : public PlanarYCbCrImage
                   , public ISharedImage
{
  typedef PlanarYCbCrData Data;
  static uint32_t sColorIdMap[];
public:
  struct GrallocData {
    nsRefPtr<TextureClient> mGraphicBuffer;
    gfx::IntSize mPicSize;
  };

  GrallocImage();

  virtual ~GrallocImage();

  



  virtual void SetData(const Data& aData);

  



  virtual void SetData(const GrallocData& aData);

  
  enum {
    
    HAL_PIXEL_FORMAT_YCbCr_422_P            = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_P            = 0x103,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x10A,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x7FA30C03,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS     = 0x7FA30C04,
  };

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE;

  void* GetNativeBuffer();

  virtual bool IsValid() { return GetSurfaceDescriptor().type() != SurfaceDescriptor::T__None; }

  SurfaceDescriptor GetSurfaceDescriptor();

  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer()
  {
    return static_cast<uint8_t*>(GetNativeBuffer());
  }

private:
  RefPtr<GrallocTextureClientOGL> mTextureClient;
};

} 
} 
#endif

#endif 
