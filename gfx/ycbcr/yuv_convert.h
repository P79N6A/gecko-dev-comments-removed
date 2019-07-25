



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



enum Rotate {
  ROTATE_0,           
  ROTATE_90,          
  ROTATE_180,         
  ROTATE_270,         
  MIRROR_ROTATE_0,    
  MIRROR_ROTATE_90,   
  MIRROR_ROTATE_180,  
  MIRROR_ROTATE_270   
};


enum ScaleFilter {
  FILTER_NONE = 0,        
  FILTER_BILINEAR_H = 1,  
  FILTER_BILINEAR_V = 2,  
  FILTER_BILINEAR = 3     
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



NS_GFX_(void) ScaleYCbCrToRGB32(const uint8* yplane,
                                const uint8* uplane,
                                const uint8* vplane,
                                uint8* rgbframe,
                                int source_width,
                                int source_height,
                                int width,
                                int height,
                                int ystride,
                                int uvstride,
                                int rgbstride,
                                YUVType yuv_type,
                                Rotate view_rotate,
                                ScaleFilter filter);

}  
}  
 
#endif  
