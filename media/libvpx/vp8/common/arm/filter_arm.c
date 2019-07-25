










#include "vpx_ports/config.h"
#include <math.h>
#include "subpixel.h"
#include "vpx_ports/mem.h"

#define BLOCK_HEIGHT_WIDTH 4
#define VP8_FILTER_WEIGHT 128
#define VP8_FILTER_SHIFT  7

DECLARE_ALIGNED(16, static const short, sub_pel_filters[8][6]) =
{
    { 0,  0,  128,    0,   0,  0 },         
    { 0, -6,  123,   12,  -1,  0 },
    { 2, -11, 108,   36,  -8,  1 },         
    { 0, -9,   93,   50,  -6,  0 },
    { 3, -16,  77,   77, -16,  3 },         
    { 0, -6,   50,   93,  -9,  0 },
    { 1, -8,   36,  108, -11,  2 },         
    { 0, -1,   12,  123,  -6,  0 },
};


extern void vp8_filter_block2d_first_pass_armv6
(
    unsigned char *src_ptr,
    short         *output_ptr,
    unsigned int src_pixels_per_line,
    unsigned int output_width,
    unsigned int output_height,
    const short *vp8_filter
);

extern void vp8_filter_block2d_second_pass_armv6
(
    short         *src_ptr,
    unsigned char *output_ptr,
    unsigned int output_pitch,
    unsigned int cnt,
    const short *vp8_filter
);

extern void vp8_filter4_block2d_second_pass_armv6
(
    short         *src_ptr,
    unsigned char *output_ptr,
    unsigned int output_pitch,
    unsigned int cnt,
    const short *vp8_filter
);

extern void vp8_filter_block2d_first_pass_only_armv6
(
    unsigned char *src_ptr,
    unsigned char *output_ptr,
    unsigned int src_pixels_per_line,
    unsigned int cnt,
    unsigned int output_pitch,
    const short *vp8_filter
);


extern void vp8_filter_block2d_second_pass_only_armv6
(
    unsigned char *src_ptr,
    unsigned char *output_ptr,
    unsigned int src_pixels_per_line,
    unsigned int cnt,
    unsigned int output_pitch,
    const short *vp8_filter
);

#if HAVE_ARMV6
void vp8_sixtap_predict_armv6
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const short  *HFilter;
    const short  *VFilter;
    DECLARE_ALIGNED_ARRAY(4, short, FData, 12*4); 


    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];       

    
    if (xoffset && !yoffset)
    {
        


        vp8_filter_block2d_first_pass_only_armv6(src_ptr, dst_ptr, src_pixels_per_line, 4, dst_pitch, HFilter);
    }
    
    else if (!xoffset && yoffset)
    {
        vp8_filter_block2d_second_pass_only_armv6(src_ptr, dst_ptr, src_pixels_per_line, 4, dst_pitch, VFilter);
    }
    else
    {
        
        if (yoffset & 0x1)
        {
            vp8_filter_block2d_first_pass_armv6(src_ptr - src_pixels_per_line, FData + 1, src_pixels_per_line, 4, 7, HFilter);
            vp8_filter4_block2d_second_pass_armv6(FData + 2, dst_ptr, dst_pitch, 4, VFilter);
        }
        
        else
        {
            vp8_filter_block2d_first_pass_armv6(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 4, 9, HFilter);
            vp8_filter_block2d_second_pass_armv6(FData + 2, dst_ptr, dst_pitch, 4, VFilter);
        }
    }
}

#if 0
void vp8_sixtap_predict8x4_armv6
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const short  *HFilter;
    const short  *VFilter;
    DECLARE_ALIGNED_ARRAY(4, short, FData, 16*8); 

    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];       


    



    
    









        vp8_filter_block2d_first_pass_armv6 ( src_ptr-(2*src_pixels_per_line), FData, src_pixels_per_line, 8, 9, HFilter );

        vp8_filter_block2d_second_pass_armv6 ( FData+2, dst_ptr, dst_pitch, 4, 8, VFilter );
    
}
#endif

void vp8_sixtap_predict8x8_armv6
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const short  *HFilter;
    const short  *VFilter;
    DECLARE_ALIGNED_ARRAY(4, short, FData, 16*8); 

    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];       

    if (xoffset && !yoffset)
    {
        vp8_filter_block2d_first_pass_only_armv6(src_ptr, dst_ptr, src_pixels_per_line, 8, dst_pitch, HFilter);
    }
    
    else if (!xoffset && yoffset)
    {
        vp8_filter_block2d_second_pass_only_armv6(src_ptr, dst_ptr, src_pixels_per_line, 8, dst_pitch, VFilter);
    }
    else
    {
        if (yoffset & 0x1)
        {
            vp8_filter_block2d_first_pass_armv6(src_ptr - src_pixels_per_line, FData + 1, src_pixels_per_line, 8, 11, HFilter);
            vp8_filter4_block2d_second_pass_armv6(FData + 2, dst_ptr, dst_pitch, 8, VFilter);
        }
        else
        {
            vp8_filter_block2d_first_pass_armv6(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 8, 13, HFilter);
            vp8_filter_block2d_second_pass_armv6(FData + 2, dst_ptr, dst_pitch, 8, VFilter);
        }
    }
}


void vp8_sixtap_predict16x16_armv6
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const short  *HFilter;
    const short  *VFilter;
    DECLARE_ALIGNED_ARRAY(4, short, FData, 24*16);    

    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];       

    if (xoffset && !yoffset)
    {
        vp8_filter_block2d_first_pass_only_armv6(src_ptr, dst_ptr, src_pixels_per_line, 16, dst_pitch, HFilter);
    }
    
    else if (!xoffset && yoffset)
    {
        vp8_filter_block2d_second_pass_only_armv6(src_ptr, dst_ptr, src_pixels_per_line, 16, dst_pitch, VFilter);
    }
    else
    {
        if (yoffset & 0x1)
        {
            vp8_filter_block2d_first_pass_armv6(src_ptr - src_pixels_per_line, FData + 1, src_pixels_per_line, 16, 19, HFilter);
            vp8_filter4_block2d_second_pass_armv6(FData + 2, dst_ptr, dst_pitch, 16, VFilter);
        }
        else
        {
            vp8_filter_block2d_first_pass_armv6(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 16, 21, HFilter);
            vp8_filter_block2d_second_pass_armv6(FData + 2, dst_ptr, dst_pitch, 16, VFilter);
        }
    }

}
#endif
