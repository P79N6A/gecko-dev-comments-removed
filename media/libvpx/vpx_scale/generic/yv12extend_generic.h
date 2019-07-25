










#ifndef YV12_EXTEND_GENERIC_H
#define YV12_EXTEND_GENERIC_H

#include "vpx_scale/yv12config.h"

    void vp8_yv12_extend_frame_borders(YV12_BUFFER_CONFIG *ybf);

    
    void vp8_yv12_copy_frame(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

    
    void vp8_yv12_copy_y_c(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

#endif 
