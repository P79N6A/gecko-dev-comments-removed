









#include "vpx_config.h"
#include "vp8/encoder/dct.h"

#if HAVE_ARMV6

void vp8_fast_fdct8x4_armv6(short *input, short *output, int pitch)
{
    vp8_fast_fdct4x4_armv6(input,   output,    pitch);
    vp8_fast_fdct4x4_armv6(input + 4, output + 16, pitch);
}

#endif 


