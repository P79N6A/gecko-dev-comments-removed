




















#ifndef AVUTIL_PIXDESC_H
#define AVUTIL_PIXDESC_H

#include <inttypes.h>

#include "attributes.h"
#include "pixfmt.h"

typedef struct AVComponentDescriptor{
    uint16_t plane        :2;            

    



    uint16_t step_minus1  :3;

    



    uint16_t offset_plus1 :3;
    uint16_t shift        :3;            
    uint16_t depth_minus1 :4;            
}AVComponentDescriptor;










typedef struct AVPixFmtDescriptor{
    const char *name;
    uint8_t nb_components;      

    






    uint8_t log2_chroma_w;      

    






    uint8_t log2_chroma_h;
    uint8_t flags;

    




    AVComponentDescriptor comp[4];
}AVPixFmtDescriptor;




#define AV_PIX_FMT_FLAG_BE           (1 << 0)



#define AV_PIX_FMT_FLAG_PAL          (1 << 1)



#define AV_PIX_FMT_FLAG_BITSTREAM    (1 << 2)



#define AV_PIX_FMT_FLAG_HWACCEL      (1 << 3)



#define AV_PIX_FMT_FLAG_PLANAR       (1 << 4)



#define AV_PIX_FMT_FLAG_RGB          (1 << 5)





#define AV_PIX_FMT_FLAG_PSEUDOPAL    (1 << 6)



#define AV_PIX_FMT_FLAG_ALPHA        (1 << 7)

#if FF_API_PIX_FMT



#define PIX_FMT_BE        AV_PIX_FMT_FLAG_BE
#define PIX_FMT_PAL       AV_PIX_FMT_FLAG_PAL
#define PIX_FMT_BITSTREAM AV_PIX_FMT_FLAG_BITSTREAM
#define PIX_FMT_HWACCEL   AV_PIX_FMT_FLAG_HWACCEL
#define PIX_FMT_PLANAR    AV_PIX_FMT_FLAG_PLANAR
#define PIX_FMT_RGB       AV_PIX_FMT_FLAG_RGB
#define PIX_FMT_PSEUDOPAL AV_PIX_FMT_FLAG_PSEUDOPAL
#define PIX_FMT_ALPHA     AV_PIX_FMT_FLAG_ALPHA
#endif

#if FF_API_PIX_FMT_DESC



extern attribute_deprecated const AVPixFmtDescriptor av_pix_fmt_descriptors[];
#endif

















void av_read_image_line(uint16_t *dst, const uint8_t *data[4], const int linesize[4],
                        const AVPixFmtDescriptor *desc, int x, int y, int c, int w, int read_pal_component);















void av_write_image_line(const uint16_t *src, uint8_t *data[4], const int linesize[4],
                         const AVPixFmtDescriptor *desc, int x, int y, int c, int w);












enum AVPixelFormat av_get_pix_fmt(const char *name);







const char *av_get_pix_fmt_name(enum AVPixelFormat pix_fmt);











char *av_get_pix_fmt_string (char *buf, int buf_size, enum AVPixelFormat pix_fmt);










int av_get_bits_per_pixel(const AVPixFmtDescriptor *pixdesc);





const AVPixFmtDescriptor *av_pix_fmt_desc_get(enum AVPixelFormat pix_fmt);








const AVPixFmtDescriptor *av_pix_fmt_desc_next(const AVPixFmtDescriptor *prev);





enum AVPixelFormat av_pix_fmt_desc_get_id(const AVPixFmtDescriptor *desc);











int av_pix_fmt_get_chroma_sub_sample(enum AVPixelFormat pix_fmt,
                                     int *h_shift, int *v_shift);





int av_pix_fmt_count_planes(enum AVPixelFormat pix_fmt);










enum AVPixelFormat av_pix_fmt_swap_endianness(enum AVPixelFormat pix_fmt);


#endif 
