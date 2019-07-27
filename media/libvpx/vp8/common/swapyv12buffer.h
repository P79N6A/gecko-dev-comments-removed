










#ifndef VP8_COMMON_SWAPYV12BUFFER_H_
#define VP8_COMMON_SWAPYV12BUFFER_H_

#include "vpx_scale/yv12config.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp8_swap_yv12_buffer(YV12_BUFFER_CONFIG *new_frame, YV12_BUFFER_CONFIG *last_frame);

#ifdef __cplusplus
}  
#endif

#endif
