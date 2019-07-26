










#ifndef VPXSCALE_H
#define VPXSCALE_H

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
