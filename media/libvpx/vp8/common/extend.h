










#ifndef __INC_EXTEND_H
#define __INC_EXTEND_H

#include "vpx_scale/yv12config.h"

void vp8_extend_mb_row(YV12_BUFFER_CONFIG *ybf, unsigned char *YPtr, unsigned char *UPtr, unsigned char *VPtr);
void vp8_copy_and_extend_frame(YV12_BUFFER_CONFIG *src,
                               YV12_BUFFER_CONFIG *dst);

#endif
