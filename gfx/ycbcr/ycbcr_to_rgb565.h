


#ifndef MEDIA_BASE_YCBCR_TO_RGB565_H_
#define MEDIA_BASE_YCBCR_TO_RGB565_H_
#include "yuv_convert.h"
#include "mozilla/arm.h"


#ifdef MOZILLA_MAY_SUPPORT_NEON
#define HAVE_YCBCR_TO_RGB565 1
#endif

namespace mozilla {

namespace gfx {

#ifdef HAVE_YCBCR_TO_RGB565

NS_GFX_(void) ConvertYCbCrToRGB565(const uint8* yplane,
                                   const uint8* uplane,
                                   const uint8* vplane,
                                   uint8* rgbframe,
                                   int pic_x,
                                   int pic_y,
                                   int pic_width,
                                   int pic_height,
                                   int ystride,
                                   int uvstride,
                                   int rgbstride,
                                   YUVType yuv_type);


NS_GFX_(bool) IsConvertYCbCrToRGB565Fast(int pic_x,
                                         int pic_y,
                                         int pic_width,
                                         int pic_height,
                                         YUVType yuv_type);


NS_GFX_(void) ScaleYCbCrToRGB565(const PRUint8 *yplane,
                                 const PRUint8 *uplane,
                                 const PRUint8 *vplane,
                                 PRUint8 *rgbframe,
                                 int source_x0,
                                 int source_y0,
                                 int source_width,
                                 int source_height,
                                 int width,
                                 int height,
                                 int ystride,
                                 int uvstride,
                                 int rgbstride,
                                 YUVType yuv_type,
                                 ScaleFilter filter);


NS_GFX_(bool) IsScaleYCbCrToRGB565Fast(int source_x0,
                                       int source_y0,
                                       int source_width,
                                       int source_height,
                                       int width,
                                       int height,
                                       YUVType yuv_type,
                                       ScaleFilter filter);
#endif 

}  

}  

#endif 
