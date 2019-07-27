




















#ifndef AVUTIL_PIXDESC_H
#define AVUTIL_PIXDESC_H

#include <inttypes.h>
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

#define PIX_FMT_BE        1 ///< Pixel format is big-endian.
#define PIX_FMT_PAL       2 ///< Pixel format has a palette in data[1], values are indexes in this palette.
#define PIX_FMT_BITSTREAM 4 ///< All values of a component are bit-wise packed end to end.
#define PIX_FMT_HWACCEL   8 ///< Pixel format is an HW accelerated format.
#define PIX_FMT_PLANAR   16 ///< At least one pixel component is not in the first data plane
#define PIX_FMT_RGB      32 ///< The pixel format contains RGB-like data (as opposed to YUV/grayscale)




extern const AVPixFmtDescriptor av_pix_fmt_descriptors[];

















void av_read_image_line(uint16_t *dst, const uint8_t *data[4], const int linesize[4],
                        const AVPixFmtDescriptor *desc, int x, int y, int c, int w, int read_pal_component);















void av_write_image_line(const uint16_t *src, uint8_t *data[4], const int linesize[4],
                         const AVPixFmtDescriptor *desc, int x, int y, int c, int w);












enum PixelFormat av_get_pix_fmt(const char *name);







const char *av_get_pix_fmt_name(enum PixelFormat pix_fmt);











char *av_get_pix_fmt_string (char *buf, int buf_size, enum PixelFormat pix_fmt);









int av_get_bits_per_pixel(const AVPixFmtDescriptor *pixdesc);

#endif 
