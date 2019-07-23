



#ifndef MEDIA_BASE_YUV_CONVERT_H_
#define MEDIA_BASE_YUV_CONVERT_H_

#include "chromium_types.h"
#include "gfxCore.h"

namespace mozilla {

namespace gfx {



enum YUVType {
  YV12 = 0,           
  YV16 = 1,           
  YV24 = 2            
};



NS_GFX_(void) ConvertYCbCrToRGB32(const uint8* yplane,
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

}  
}  

#endif  
