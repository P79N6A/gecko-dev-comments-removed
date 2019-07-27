




#include "gfx2DGlue.h"

#include "YCbCrUtils.h"
#include "yuv_convert.h"
#include "ycbcr_to_rgb565.h"

namespace mozilla {
namespace gfx {

void
GetYCbCrToRGBDestFormatAndSize(const layers::PlanarYCbCrData& aData,
                               SurfaceFormat& aSuggestedFormat,
                               IntSize& aSuggestedSize)
{
  YUVType yuvtype =
    TypeFromSize(aData.mYSize.width,
                 aData.mYSize.height,
                 aData.mCbCrSize.width,
                 aData.mCbCrSize.height);

  
  
  bool prescale = aSuggestedSize.width > 0 && aSuggestedSize.height > 0 &&
                  aSuggestedSize != aData.mPicSize;

  if (aSuggestedFormat == SurfaceFormat::R5G6B5) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (prescale &&
        !IsScaleYCbCrToRGB565Fast(aData.mPicX,
                                  aData.mPicY,
                                  aData.mPicSize.width,
                                  aData.mPicSize.height,
                                  aSuggestedSize.width,
                                  aSuggestedSize.height,
                                  yuvtype,
                                  FILTER_BILINEAR) &&
        IsConvertYCbCrToRGB565Fast(aData.mPicX,
                                   aData.mPicY,
                                   aData.mPicSize.width,
                                   aData.mPicSize.height,
                                   yuvtype)) {
      prescale = false;
    }
#else
    
    aSuggestedFormat = SurfaceFormat::B8G8R8X8;
#endif
  }
  else if (aSuggestedFormat != SurfaceFormat::B8G8R8X8) {
    
    aSuggestedFormat = SurfaceFormat::B8G8R8X8;
  }
  if (aSuggestedFormat == SurfaceFormat::B8G8R8X8) {
    

    if (aData.mPicX != 0 || aData.mPicY != 0 || yuvtype == YV24)
      prescale = false;
  }
  if (!prescale) {
    aSuggestedSize = aData.mPicSize;
  }
}

void
ConvertYCbCrToRGB(const layers::PlanarYCbCrData& aData,
                  const SurfaceFormat& aDestFormat,
                  const IntSize& aDestSize,
                  unsigned char* aDestBuffer,
                  int32_t aStride)
{
  
  
  MOZ_ASSERT((aData.mCbCrSize.width == aData.mYSize.width ||
              aData.mCbCrSize.width == (aData.mYSize.width + 1) >> 1) &&
             (aData.mCbCrSize.height == aData.mYSize.height ||
              aData.mCbCrSize.height == (aData.mYSize.height + 1) >> 1));
  YUVType yuvtype =
    TypeFromSize(aData.mYSize.width,
                 aData.mYSize.height,
                 aData.mCbCrSize.width,
                 aData.mCbCrSize.height);

  
  if (aDestSize != aData.mPicSize) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (aDestFormat == SurfaceFormat::R5G6B5) {
      ScaleYCbCrToRGB565(aData.mYChannel,
                         aData.mCbChannel,
                         aData.mCrChannel,
                         aDestBuffer,
                         aData.mPicX,
                         aData.mPicY,
                         aData.mPicSize.width,
                         aData.mPicSize.height,
                         aDestSize.width,
                         aDestSize.height,
                         aData.mYStride,
                         aData.mCbCrStride,
                         aStride,
                         yuvtype,
                         FILTER_BILINEAR);
    } else
#endif
      ScaleYCbCrToRGB32(aData.mYChannel, 
                        aData.mCbChannel,
                        aData.mCrChannel,
                        aDestBuffer,
                        aData.mPicSize.width,
                        aData.mPicSize.height,
                        aDestSize.width,
                        aDestSize.height,
                        aData.mYStride,
                        aData.mCbCrStride,
                        aStride,
                        yuvtype,
                        ROTATE_0,
                        FILTER_BILINEAR);
  } else { 
#if defined(HAVE_YCBCR_TO_RGB565)
    if (aDestFormat == SurfaceFormat::R5G6B5) {
      ConvertYCbCrToRGB565(aData.mYChannel,
                           aData.mCbChannel,
                           aData.mCrChannel,
                           aDestBuffer,
                           aData.mPicX,
                           aData.mPicY,
                           aData.mPicSize.width,
                           aData.mPicSize.height,
                           aData.mYStride,
                           aData.mCbCrStride,
                           aStride,
                           yuvtype);
    } else 
#endif
      ConvertYCbCrToRGB32(aData.mYChannel, 
                          aData.mCbChannel,
                          aData.mCrChannel,
                          aDestBuffer,
                          aData.mPicX,
                          aData.mPicY,
                          aData.mPicSize.width,
                          aData.mPicSize.height,
                          aData.mYStride,
                          aData.mCbCrStride,
                          aStride,
                          yuvtype);
  }
}

} 
} 
