










#include <stdlib.h>

#define BLOCK_HEIGHT_WIDTH 4
#define VP8_FILTER_WEIGHT 128
#define VP8_FILTER_SHIFT  7


static const int bilinear_filters[8][2] =
{
    { 128,   0 },
    { 112,  16 },
    {  96,  32 },
    {  80,  48 },
    {  64,  64 },
    {  48,  80 },
    {  32,  96 },
    {  16, 112 }
};


static const short sub_pel_filters[8][6] =
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

void vp8_filter_block2d_first_pass
(
    unsigned char *src_ptr,
    int *output_ptr,
    unsigned int src_pixels_per_line,
    unsigned int pixel_step,
    unsigned int output_height,
    unsigned int output_width,
    const short *vp8_filter
)
{
    unsigned int i, j;
    int  Temp;

    for (i = 0; i < output_height; i++)
    {
        for (j = 0; j < output_width; j++)
        {
            Temp = ((int)src_ptr[-2 * (int)pixel_step] * vp8_filter[0]) +
                   ((int)src_ptr[-1 * (int)pixel_step] * vp8_filter[1]) +
                   ((int)src_ptr[0]                 * vp8_filter[2]) +
                   ((int)src_ptr[pixel_step]         * vp8_filter[3]) +
                   ((int)src_ptr[2*pixel_step]       * vp8_filter[4]) +
                   ((int)src_ptr[3*pixel_step]       * vp8_filter[5]) +
                   (VP8_FILTER_WEIGHT >> 1);      

            
            Temp = Temp >> VP8_FILTER_SHIFT;

            if (Temp < 0)
                Temp = 0;
            else if (Temp > 255)
                Temp = 255;

            output_ptr[j] = Temp;
            src_ptr++;
        }

        
        src_ptr    += src_pixels_per_line - output_width;
        output_ptr += output_width;
    }
}

void vp8_filter_block2d_second_pass
(
    int *src_ptr,
    unsigned char *output_ptr,
    int output_pitch,
    unsigned int src_pixels_per_line,
    unsigned int pixel_step,
    unsigned int output_height,
    unsigned int output_width,
    const short *vp8_filter
)
{
    unsigned int i, j;
    int  Temp;

    for (i = 0; i < output_height; i++)
    {
        for (j = 0; j < output_width; j++)
        {
            
            Temp = ((int)src_ptr[-2 * (int)pixel_step] * vp8_filter[0]) +
                   ((int)src_ptr[-1 * (int)pixel_step] * vp8_filter[1]) +
                   ((int)src_ptr[0]                 * vp8_filter[2]) +
                   ((int)src_ptr[pixel_step]         * vp8_filter[3]) +
                   ((int)src_ptr[2*pixel_step]       * vp8_filter[4]) +
                   ((int)src_ptr[3*pixel_step]       * vp8_filter[5]) +
                   (VP8_FILTER_WEIGHT >> 1);   

            
            Temp = Temp >> VP8_FILTER_SHIFT;

            if (Temp < 0)
                Temp = 0;
            else if (Temp > 255)
                Temp = 255;

            output_ptr[j] = (unsigned char)Temp;
            src_ptr++;
        }

        
        src_ptr    += src_pixels_per_line - output_width;
        output_ptr += output_pitch;
    }
}


void vp8_filter_block2d
(
    unsigned char  *src_ptr,
    unsigned char  *output_ptr,
    unsigned int src_pixels_per_line,
    int output_pitch,
    const short  *HFilter,
    const short  *VFilter
)
{
    int FData[9*4]; 

    
    vp8_filter_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 9, 4, HFilter);

    
    vp8_filter_block2d_second_pass(FData + 8, output_ptr, output_pitch, 4, 4, 4, 4, VFilter);
}


void vp8_block_variation_c
(
    unsigned char  *src_ptr,
    int   src_pixels_per_line,
    int *HVar,
    int *VVar
)
{
    int i, j;
    unsigned char *Ptr = src_ptr;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            *HVar += abs((int)Ptr[j] - (int)Ptr[j+1]);
            *VVar += abs((int)Ptr[j] - (int)Ptr[j+src_pixels_per_line]);
        }

        Ptr += src_pixels_per_line;
    }
}




void vp8_sixtap_predict_c
(
    unsigned char  *src_ptr,
    int   src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int dst_pitch
)
{
    const short  *HFilter;
    const short  *VFilter;

    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];   

    vp8_filter_block2d(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter);
}
void vp8_sixtap_predict8x8_c
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
    int FData[13*16];   

    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];   

    
    vp8_filter_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 13, 8, HFilter);


    
    vp8_filter_block2d_second_pass(FData + 16, dst_ptr, dst_pitch, 8, 8, 8, 8, VFilter);

}

void vp8_sixtap_predict8x4_c
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
    int FData[13*16];   

    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];   

    
    vp8_filter_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 9, 8, HFilter);


    
    vp8_filter_block2d_second_pass(FData + 16, dst_ptr, dst_pitch, 8, 8, 4, 8, VFilter);

}

void vp8_sixtap_predict16x16_c
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
    int FData[21*24];   


    HFilter = sub_pel_filters[xoffset];   
    VFilter = sub_pel_filters[yoffset];   

    
    vp8_filter_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 21, 16, HFilter);

    
    vp8_filter_block2d_second_pass(FData + 32, dst_ptr, dst_pitch, 16, 16, 16, 16, VFilter);

}






























void vp8_filter_block2d_bil_first_pass
(
    unsigned char *src_ptr,
    unsigned short *output_ptr,
    unsigned int src_pixels_per_line,
    int pixel_step,
    unsigned int output_height,
    unsigned int output_width,
    const int *vp8_filter
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





























void vp8_filter_block2d_bil_second_pass
(
    unsigned short *src_ptr,
    unsigned char  *output_ptr,
    int output_pitch,
    unsigned int  src_pixels_per_line,
    unsigned int  pixel_step,
    unsigned int  output_height,
    unsigned int  output_width,
    const int *vp8_filter
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
        output_ptr += output_pitch;
    }
}






















void vp8_filter_block2d_bil
(
    unsigned char *src_ptr,
    unsigned char *output_ptr,
    unsigned int   src_pixels_per_line,
    unsigned int   dst_pitch,
    const int      *HFilter,
    const int      *VFilter,
    int            Width,
    int            Height
)
{

    unsigned short FData[17*16];    

    
    vp8_filter_block2d_bil_first_pass(src_ptr, FData, src_pixels_per_line, 1, Height + 1, Width, HFilter);

    
    vp8_filter_block2d_bil_second_pass(FData, output_ptr, dst_pitch, Width, Width, Height, Width, VFilter);
}


void vp8_bilinear_predict4x4_c
(
    unsigned char  *src_ptr,
    int   src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int dst_pitch
)
{
    const int  *HFilter;
    const int  *VFilter;

    HFilter = bilinear_filters[xoffset];
    VFilter = bilinear_filters[yoffset];
#if 0
    {
        int i;
        unsigned char temp1[16];
        unsigned char temp2[16];

        bilinear_predict4x4_mmx(src_ptr, src_pixels_per_line, xoffset, yoffset, temp1, 4);
        vp8_filter_block2d_bil(src_ptr, temp2, src_pixels_per_line, 4, HFilter, VFilter, 4, 4);

        for (i = 0; i < 16; i++)
        {
            if (temp1[i] != temp2[i])
            {
                bilinear_predict4x4_mmx(src_ptr, src_pixels_per_line, xoffset, yoffset, temp1, 4);
                vp8_filter_block2d_bil(src_ptr, temp2, src_pixels_per_line, 4, HFilter, VFilter, 4, 4);
            }
        }
    }
#endif
    vp8_filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 4, 4);

}

void vp8_bilinear_predict8x8_c
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const int  *HFilter;
    const int  *VFilter;

    HFilter = bilinear_filters[xoffset];
    VFilter = bilinear_filters[yoffset];

    vp8_filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 8, 8);

}

void vp8_bilinear_predict8x4_c
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const int  *HFilter;
    const int  *VFilter;

    HFilter = bilinear_filters[xoffset];
    VFilter = bilinear_filters[yoffset];

    vp8_filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 8, 4);

}

void vp8_bilinear_predict16x16_c
(
    unsigned char  *src_ptr,
    int  src_pixels_per_line,
    int  xoffset,
    int  yoffset,
    unsigned char *dst_ptr,
    int  dst_pitch
)
{
    const int  *HFilter;
    const int  *VFilter;

    HFilter = bilinear_filters[xoffset];
    VFilter = bilinear_filters[yoffset];

    vp8_filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 16, 16);
}
