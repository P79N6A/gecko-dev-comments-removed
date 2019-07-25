










#include "variance.h"
#include "vp8/common/filter.h"


unsigned int vp8_get_mb_ss_c
(
    const short *src_ptr
)
{
    unsigned int i = 0, sum = 0;

    do
    {
        sum += (src_ptr[i] * src_ptr[i]);
        i++;
    }
    while (i < 256);

    return sum;
}


static void variance(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    int  w,
    int  h,
    unsigned int *sse,
    int *sum)
{
    int i, j;
    int diff;

    *sum = 0;
    *sse = 0;

    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            diff = src_ptr[j] - ref_ptr[j];
            *sum += diff;
            *sse += diff * diff;
        }

        src_ptr += source_stride;
        ref_ptr += recon_stride;
    }
}


unsigned int vp8_variance16x16_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    unsigned int var;
    int avg;


    variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 16, &var, &avg);
    *sse = var;
    return (var - ((avg * avg) >> 8));
}

unsigned int vp8_variance8x16_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    unsigned int var;
    int avg;


    variance(src_ptr, source_stride, ref_ptr, recon_stride, 8, 16, &var, &avg);
    *sse = var;
    return (var - ((avg * avg) >> 7));
}

unsigned int vp8_variance16x8_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    unsigned int var;
    int avg;


    variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 8, &var, &avg);
    *sse = var;
    return (var - ((avg * avg) >> 7));
}


unsigned int vp8_variance8x8_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    unsigned int var;
    int avg;


    variance(src_ptr, source_stride, ref_ptr, recon_stride, 8, 8, &var, &avg);
    *sse = var;
    return (var - ((avg * avg) >> 6));
}

unsigned int vp8_variance4x4_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    unsigned int var;
    int avg;


    variance(src_ptr, source_stride, ref_ptr, recon_stride, 4, 4, &var, &avg);
    *sse = var;
    return (var - ((avg * avg) >> 4));
}


unsigned int vp8_mse16x16_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    unsigned int var;
    int avg;

    variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 16, &var, &avg);
    *sse = var;
    return var;
}






























static void var_filter_block2d_bil_first_pass
(
    const unsigned char *src_ptr,
    unsigned short *output_ptr,
    unsigned int src_pixels_per_line,
    int pixel_step,
    unsigned int output_height,
    unsigned int output_width,
    const short *vp8_filter
)
{
    unsigned int i, j;

    for (i = 0; i < output_height; i++)
    {
        for (j = 0; j < output_width; j++)
        {
            
            output_ptr[j] = (((int)src_ptr[0]          * vp8_filter[0]) +
                             ((int)src_ptr[pixel_step] * vp8_filter[1]) +
                             (VP8_FILTER_WEIGHT / 2)) >> VP8_FILTER_SHIFT;
            src_ptr++;
        }

        
        src_ptr    += src_pixels_per_line - output_width;
        output_ptr += output_width;
    }
}





























static void var_filter_block2d_bil_second_pass
(
    const unsigned short *src_ptr,
    unsigned char  *output_ptr,
    unsigned int  src_pixels_per_line,
    unsigned int  pixel_step,
    unsigned int  output_height,
    unsigned int  output_width,
    const short *vp8_filter
)
{
    unsigned int  i, j;
    int  Temp;

    for (i = 0; i < output_height; i++)
    {
        for (j = 0; j < output_width; j++)
        {
            
            Temp = ((int)src_ptr[0]         * vp8_filter[0]) +
                   ((int)src_ptr[pixel_step] * vp8_filter[1]) +
                   (VP8_FILTER_WEIGHT / 2);
            output_ptr[j] = (unsigned int)(Temp >> VP8_FILTER_SHIFT);
            src_ptr++;
        }

        
        src_ptr    += src_pixels_per_line - output_width;
        output_ptr += output_width;
    }
}


unsigned int vp8_sub_pixel_variance4x4_c
(
    const unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    const unsigned char *dst_ptr,
    int dst_pixels_per_line,
    unsigned int *sse
)
{
    unsigned char  temp2[20*16];
    const short *HFilter, *VFilter;
    unsigned short FData3[5*4]; 

    HFilter = vp8_bilinear_filters[xoffset];
    VFilter = vp8_bilinear_filters[yoffset];

    
    var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 5, 4, HFilter);

    
    var_filter_block2d_bil_second_pass(FData3, temp2, 4,  4,  4,  4, VFilter);

    return vp8_variance4x4_c(temp2, 4, dst_ptr, dst_pixels_per_line, sse);
}


unsigned int vp8_sub_pixel_variance8x8_c
(
    const unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    const unsigned char *dst_ptr,
    int dst_pixels_per_line,
    unsigned int *sse
)
{
    unsigned short FData3[9*8]; 
    unsigned char  temp2[20*16];
    const short *HFilter, *VFilter;

    HFilter = vp8_bilinear_filters[xoffset];
    VFilter = vp8_bilinear_filters[yoffset];

    var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 9, 8, HFilter);
    var_filter_block2d_bil_second_pass(FData3, temp2, 8, 8, 8, 8, VFilter);

    return vp8_variance8x8_c(temp2, 8, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp8_sub_pixel_variance16x16_c
(
    const unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    const unsigned char *dst_ptr,
    int dst_pixels_per_line,
    unsigned int *sse
)
{
    unsigned short FData3[17*16];   
    unsigned char  temp2[20*16];
    const short *HFilter, *VFilter;

    HFilter = vp8_bilinear_filters[xoffset];
    VFilter = vp8_bilinear_filters[yoffset];

    var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 17, 16, HFilter);
    var_filter_block2d_bil_second_pass(FData3, temp2, 16, 16, 16, 16, VFilter);

    return vp8_variance16x16_c(temp2, 16, dst_ptr, dst_pixels_per_line, sse);
}


unsigned int vp8_variance_halfpixvar16x16_h_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    return vp8_sub_pixel_variance16x16_c(src_ptr, source_stride, 4, 0,
                                         ref_ptr, recon_stride, sse);
}


unsigned int vp8_variance_halfpixvar16x16_v_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    return vp8_sub_pixel_variance16x16_c(src_ptr, source_stride, 0, 4,
                                         ref_ptr, recon_stride, sse);
}


unsigned int vp8_variance_halfpixvar16x16_hv_c(
    const unsigned char *src_ptr,
    int  source_stride,
    const unsigned char *ref_ptr,
    int  recon_stride,
    unsigned int *sse)
{
    return vp8_sub_pixel_variance16x16_c(src_ptr, source_stride, 4, 4,
                                         ref_ptr, recon_stride, sse);
}


unsigned int vp8_sub_pixel_mse16x16_c
(
    const unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    const unsigned char *dst_ptr,
    int dst_pixels_per_line,
    unsigned int *sse
)
{
    vp8_sub_pixel_variance16x16_c(src_ptr, src_pixels_per_line, xoffset, yoffset, dst_ptr, dst_pixels_per_line, sse);
    return *sse;
}

unsigned int vp8_sub_pixel_variance16x8_c
(
    const unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    const unsigned char *dst_ptr,
    int dst_pixels_per_line,
    unsigned int *sse
)
{
    unsigned short FData3[16*9];    
    unsigned char  temp2[20*16];
    const short *HFilter, *VFilter;

    HFilter = vp8_bilinear_filters[xoffset];
    VFilter = vp8_bilinear_filters[yoffset];

    var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 9, 16, HFilter);
    var_filter_block2d_bil_second_pass(FData3, temp2, 16, 16, 8, 16, VFilter);

    return vp8_variance16x8_c(temp2, 16, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp8_sub_pixel_variance8x16_c
(
    const unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    const unsigned char *dst_ptr,
    int dst_pixels_per_line,
    unsigned int *sse
)
{
    unsigned short FData3[9*16];    
    unsigned char  temp2[20*16];
    const short *HFilter, *VFilter;


    HFilter = vp8_bilinear_filters[xoffset];
    VFilter = vp8_bilinear_filters[yoffset];


    var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 17, 8, HFilter);
    var_filter_block2d_bil_second_pass(FData3, temp2, 8, 8, 16, 8, VFilter);

    return vp8_variance8x16_c(temp2, 8, dst_ptr, dst_pixels_per_line, sse);
}
