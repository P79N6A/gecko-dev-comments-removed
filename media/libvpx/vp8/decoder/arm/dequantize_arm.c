










#include "vpx_ports/config.h"
#include "vp8/decoder/dequantize.h"
#include "vp8/common/idct.h"
#include "vpx_mem/vpx_mem.h"

#if HAVE_ARMV7
extern void vp8_dequantize_b_loop_neon(short *Q, short *DQC, short *DQ);
#endif

#if HAVE_ARMV6
extern void vp8_dequantize_b_loop_v6(short *Q, short *DQC, short *DQ);
#endif

#if HAVE_ARMV7

void vp8_dequantize_b_neon(BLOCKD *d)
{
    short *DQ  = d->dqcoeff;
    short *Q   = d->qcoeff;
    short *DQC = d->dequant;

    vp8_dequantize_b_loop_neon(Q, DQC, DQ);
}
#endif

#if HAVE_ARMV6
void vp8_dequantize_b_v6(BLOCKD *d)
{
    short *DQ  = d->dqcoeff;
    short *Q   = d->qcoeff;
    short *DQC = d->dequant;

    vp8_dequantize_b_loop_v6(Q, DQC, DQ);
}
#endif
