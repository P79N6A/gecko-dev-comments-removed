



#include "Scale.h"

#ifdef USE_SKIA
#include "HelpersSkia.h"
#include "skia/SkBitmap.h"
#include "image_operations.h"
#endif

namespace mozilla {
namespace gfx {

bool Scale(uint8_t* srcData, int32_t srcWidth, int32_t srcHeight, int32_t srcStride,
           uint8_t* dstData, int32_t dstWidth, int32_t dstHeight, int32_t dstStride,
           SurfaceFormat format)
{
#ifdef USE_SKIA
  SkAlphaType alphaType;
  if (format == SurfaceFormat::B8G8R8A8) {
    alphaType = kPremul_SkAlphaType;
  } else {
    alphaType = kOpaque_SkAlphaType;
  }

  SkBitmap::Config config = GfxFormatToSkiaConfig(format);

  SkBitmap imgSrc;
  imgSrc.setConfig(config, srcWidth, srcHeight, srcStride, alphaType);
  imgSrc.setPixels(srcData);

  
  if (config != SkBitmap::kARGB_8888_Config) {
    imgSrc.copyTo(&imgSrc, kRGBA_8888_SkColorType);
  }

  
  
  SkBitmap result = skia::ImageOperations::Resize(imgSrc,
                                                  skia::ImageOperations::RESIZE_BEST,
                                                  dstWidth, dstHeight,
                                                  dstData);

  return !result.isNull();
#else
  return false;
#endif
}

}
}
