










#ifndef YV12_EXTEND_H
#define YV12_EXTEND_H

#include "vpx_config.h"
#include "vpx_scale/yv12config.h"
#include "vpx_scale/generic/yv12extend_generic.h"

#if HAVE_ARMV7
#include "vpx_scale/arm/yv12extend_arm.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    extern void (*vp8_yv12_extend_frame_borders_ptr)(YV12_BUFFER_CONFIG *ybf);

    
    extern void (*vp8_yv12_copy_frame_ptr)(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

    
    extern void (*vp8_yv12_copy_y_ptr)(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

#ifdef __cplusplus
}
#endif

#endif
