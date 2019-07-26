









#include "vpx_config.h"
#include "vp8_rtcd.h"

#if HAVE_MEDIA

void vp8_short_fdct8x4_armv6(short *input, short *output, int pitch)
{
    vp8_short_fdct4x4_armv6(input,   output,    pitch);
    vp8_short_fdct4x4_armv6(input + 4, output + 16, pitch);
}

#endif 
