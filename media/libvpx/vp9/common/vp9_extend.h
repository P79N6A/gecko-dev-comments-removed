









#ifndef VP9_COMMON_VP9_EXTEND_H_
#define VP9_COMMON_VP9_EXTEND_H_

#include "vpx_scale/yv12config.h"
#include "vpx/vpx_integer.h"


void vp9_copy_and_extend_frame(const YV12_BUFFER_CONFIG *src,
                               YV12_BUFFER_CONFIG *dst);

void vp9_copy_and_extend_frame_with_rect(const YV12_BUFFER_CONFIG *src,
                                         YV12_BUFFER_CONFIG *dst,
                                         int srcy, int srcx,
                                         int srch, int srcw);
#endif  
