










#ifndef VP8_COMMON_EXTEND_H_
#define VP8_COMMON_EXTEND_H_

#include "vpx_scale/yv12config.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp8_extend_mb_row(YV12_BUFFER_CONFIG *ybf, unsigned char *YPtr, unsigned char *UPtr, unsigned char *VPtr);
void vp8_copy_and_extend_frame(YV12_BUFFER_CONFIG *src,
                               YV12_BUFFER_CONFIG *dst);
void vp8_copy_and_extend_frame_with_rect(YV12_BUFFER_CONFIG *src,
                                         YV12_BUFFER_CONFIG *dst,
                                         int srcy, int srcx,
                                         int srch, int srcw);

#ifdef __cplusplus
}  
#endif

#endif
