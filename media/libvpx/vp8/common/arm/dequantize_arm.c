










#include "vpx_config.h"
#include "vp8/common/dequantize.h"
#include "vp8/common/idct.h"

#if HAVE_ARMV7
extern void vp8_dequantize_b_loop_neon(short *Q, short *DQC, short *DQ);
#endif

#if HAVE_ARMV6
extern void vp8_dequantize_b_loop_v6(short *Q, short *DQC, short *DQ);
#endif

#if HAVE_ARMV7

void vp8_dequantize_b_neon(BLOCKD *d, short *DQC)
{
    short *DQ  = d->dqcoeff;
    short *Q   = d->qcoeff;

    vp8_dequantize_b_loop_neon(Q, DQC, DQ);
}
#endif

#if HAVE_ARMV6
void vp8_dequantize_b_v6(BLOCKD *d, short *DQC)
{
    short *DQ  = d->dqcoeff;
    short *Q   = d->qcoeff;

    vp8_dequantize_b_loop_v6(Q, DQC, DQ);
}
#endif
