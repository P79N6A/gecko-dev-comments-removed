










#include "vpx_ports/config.h"
#include "vpx_ports/x86.h"
#include "onyxd_int.h"


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
    int flags = x86_simd_caps();

    





#if CONFIG_RUNTIME_CPU_DETECT
    
#if HAVE_MMX

    if (flags & HAS_MMX)
    {
        pbi->dequant.block   = vp8_dequantize_b_mmx;
        pbi->dequant.idct    = vp8_dequant_idct_mmx;
        pbi->dequant.idct_dc = vp8_dequant_dc_idct_mmx;
    }

#endif
#endif
}
