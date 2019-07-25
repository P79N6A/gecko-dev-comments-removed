










#ifndef BILINEARFILTER_ARM_H
#define BILINEARFILTER_ARM_H

extern void vp8_filter_block2d_bil_first_pass_armv6
(
    const unsigned char  *src_ptr,
    unsigned short       *dst_ptr,
    unsigned int          src_pitch,
    unsigned int          height,
    unsigned int          width,
    const short          *vp8_filter
);

extern void vp8_filter_block2d_bil_second_pass_armv6
(
    const unsigned short *src_ptr,
    unsigned char        *dst_ptr,
    int                   dst_pitch,
    unsigned int          height,
    unsigned int          width,
    const short         *vp8_filter
);

#endif 
