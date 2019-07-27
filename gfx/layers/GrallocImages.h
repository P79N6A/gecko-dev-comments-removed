




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
{
  typedef PlanarYCbCrData Data;
  static int32_t sColorIdMap[];
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

  enum {
    GRALLOC_SW_UAGE = android::GraphicBuffer::USAGE_SOFTWARE_MASK,
  };

  virtual already_AddRefed<gfx::SourceSurface> GetAsSourceSurface() override;

  android::sp<android::GraphicBuffer> GetGraphicBuffer() const;

  void* GetNativeBuffer();

  virtual bool IsValid() { return !!mTextureClient; }

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

  virtual GrallocImage* AsGrallocImage() override
  {
    return this;
  }

  virtual uint8_t* GetBuffer()
  {
    return static_cast<uint8_t*>(GetNativeBuffer());
  }

  int GetUsage()
  {
    return (static_cast<ANativeWindowBuffer*>(GetNativeBuffer()))->usage;
  }

  static int GetOmxFormat(int aFormat)
  {
    uint32_t omxFormat = 0;

    for (int i = 0; sColorIdMap[i]; i += 2) {
      if (sColorIdMap[i] == aFormat) {
        omxFormat = sColorIdMap[i + 1];
        break;
      }
    }

    return omxFormat;
  }

private:
  RefPtr<GrallocTextureClientOGL> mTextureClient;
};

} 
} 
#endif

#endif 
