



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

  SkImageInfo info = SkImageInfo::Make(srcWidth,
                                       srcHeight,
                                       GfxFormatToSkiaColorType(format),
                                       alphaType);

  SkBitmap imgSrc;
  imgSrc.installPixels(info, srcData, srcStride);

  
  if (format != SurfaceFormat::B8G8R8A8) {
    imgSrc.copyTo(&imgSrc, kBGRA_8888_SkColorType);
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
