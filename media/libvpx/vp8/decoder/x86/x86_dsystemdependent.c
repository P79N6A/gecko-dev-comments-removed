










#include "vpx_ports/config.h"
#include "vpx_ports/x86.h"
#include "vp8/decoder/onyxd_int.h"


#if HAVE_MMX
void vp8_dequantize_b_impl_mmx(short *sq, short *dq, short *q);

void vp8_dequantize_b_mmx(BLOCKD *d)
{
    short *sq = (short *) d->qcoeff;
    short *dq = (short *) d->dqcoeff;
    short *q = (short *) d->dequant;
    vp8_dequantize_b_impl_mmx(sq, dq, q);
}
#endif

void vp8_arch_x86_decode_init(VP8D_COMP *pbi)
{
#if CONFIG_RUNTIME_CPU_DETECT
    int flags = x86_simd_caps();

    





    
#if HAVE_MMX
    if (flags & HAS_MMX)
    {
        pbi->dequant.block               = vp8_dequantize_b_mmx;
        pbi->dequant.idct_add            = vp8_dequant_idct_add_mmx;
        pbi->dequant.dc_idct_add         = vp8_dequant_dc_idct_add_mmx;
        pbi->dequant.dc_idct_add_y_block = vp8_dequant_dc_idct_add_y_block_mmx;
        pbi->dequant.idct_add_y_block    = vp8_dequant_idct_add_y_block_mmx;
        pbi->dequant.idct_add_uv_block   = vp8_dequant_idct_add_uv_block_mmx;
    }
#endif
#if HAVE_SSE2
    if (flags & HAS_SSE2)
    {
        pbi->dequant.dc_idct_add_y_block = vp8_dequant_dc_idct_add_y_block_sse2;
        pbi->dequant.idct_add_y_block    = vp8_dequant_idct_add_y_block_sse2;
        pbi->dequant.idct_add_uv_block   = vp8_dequant_idct_add_uv_block_sse2;
    }
#endif

#endif
}
