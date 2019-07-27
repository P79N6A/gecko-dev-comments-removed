










#include <string.h>

#include "./vp8_rtcd.h"
#include "vpx/vpx_integer.h"


void vp8_copy32xn_c(const unsigned char *src_ptr, int src_stride,
                    unsigned char *dst_ptr, int dst_stride,
                    int height)
{
    int r;

    for (r = 0; r < height; r++)
    {
        memcpy(dst_ptr, src_ptr, 32);

        src_ptr += src_stride;
        dst_ptr += dst_stride;

    }
}
