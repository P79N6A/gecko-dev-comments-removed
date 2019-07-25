










#ifndef YV12_EXTEND_ARM_H
#define YV12_EXTEND_ARM_H

#include "vpx_config.h"
#include "vpx_scale/yv12config.h"

#if HAVE_ARMV7
    void vp8_yv12_extend_frame_borders_neon(YV12_BUFFER_CONFIG *ybf);

    
    void vp8_yv12_copy_frame_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

    
    void vp8_yv12_copy_y_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
#endif

#endif 
