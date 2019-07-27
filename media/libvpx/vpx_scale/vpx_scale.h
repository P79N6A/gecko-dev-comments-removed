










#ifndef VPX_SCALE_VPX_SCALE_H_
#define VPX_SCALE_VPX_SCALE_H_

#include "vpx_scale/yv12config.h"

extern void vpx_scale_frame(YV12_BUFFER_CONFIG *src,
                            YV12_BUFFER_CONFIG *dst,
                            unsigned char *temp_area,
                            unsigned char temp_height,
                            unsigned int hscale,
                            unsigned int hratio,
                            unsigned int vscale,
                            unsigned int vratio,
                            unsigned int interlaced);

#endif  
