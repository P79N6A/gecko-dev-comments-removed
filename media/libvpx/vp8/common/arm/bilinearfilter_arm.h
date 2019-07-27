










#ifndef VP8_COMMON_ARM_BILINEARFILTER_ARM_H_
#define VP8_COMMON_ARM_BILINEARFILTER_ARM_H_

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}  
#endif

#endif
