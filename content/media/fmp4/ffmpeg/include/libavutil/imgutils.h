

















#ifndef AVUTIL_IMGUTILS_H
#define AVUTIL_IMGUTILS_H









#include "avutil.h"
#include "pixdesc.h"

















void av_image_fill_max_pixsteps(int max_pixsteps[4], int max_pixstep_comps[4],
                                const AVPixFmtDescriptor *pixdesc);







int av_image_get_linesize(enum PixelFormat pix_fmt, int width, int plane);








int av_image_fill_linesizes(int linesizes[4], enum PixelFormat pix_fmt, int width);












int av_image_fill_pointers(uint8_t *data[4], enum PixelFormat pix_fmt, int height,
                           uint8_t *ptr, const int linesizes[4]);











int av_image_alloc(uint8_t *pointers[4], int linesizes[4],
                   int w, int h, enum PixelFormat pix_fmt, int align);










void av_image_copy_plane(uint8_t       *dst, int dst_linesize,
                         const uint8_t *src, int src_linesize,
                         int bytewidth, int height);







void av_image_copy(uint8_t *dst_data[4], int dst_linesizes[4],
                   const uint8_t *src_data[4], const int src_linesizes[4],
                   enum PixelFormat pix_fmt, int width, int height);











int av_image_check_size(unsigned int w, unsigned int h, int log_offset, void *log_ctx);

int ff_set_systematic_pal2(uint32_t pal[256], enum PixelFormat pix_fmt);






#endif 
