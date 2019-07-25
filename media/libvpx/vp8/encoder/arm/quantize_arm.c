










#include <math.h>
#include "vpx_mem/vpx_mem.h"

#include "vp8/encoder/quantize.h"
#include "vp8/common/entropy.h"


#if HAVE_ARMV7




void vp8_quantize_mby_neon(MACROBLOCK *x)
{
    int i;
    int has_2nd_order = (x->e_mbd.mode_info_context->mbmi.mode != B_PRED
        && x->e_mbd.mode_info_context->mbmi.mode != SPLITMV);

    for (i = 0; i < 16; i+=2)
        x->quantize_b_pair(&x->block[i], &x->block[i+1],
                           &x->e_mbd.block[i], &x->e_mbd.block[i+1]);

    if(has_2nd_order)
        x->quantize_b(&x->block[24], &x->e_mbd.block[24]);
}

void vp8_quantize_mb_neon(MACROBLOCK *x)
{
    int i;
    int has_2nd_order=(x->e_mbd.mode_info_context->mbmi.mode != B_PRED
        && x->e_mbd.mode_info_context->mbmi.mode != SPLITMV);

    for (i = 0; i < 24; i+=2)
        x->quantize_b_pair(&x->block[i], &x->block[i+1],
                           &x->e_mbd.block[i], &x->e_mbd.block[i+1]);

    if (has_2nd_order)
        x->quantize_b(&x->block[24], &x->e_mbd.block[24]);
}


void vp8_quantize_mbuv_neon(MACROBLOCK *x)
{
    int i;

    for (i = 16; i < 24; i+=2)
        x->quantize_b_pair(&x->block[i], &x->block[i+1],
                           &x->e_mbd.block[i], &x->e_mbd.block[i+1]);
}

#endif 
