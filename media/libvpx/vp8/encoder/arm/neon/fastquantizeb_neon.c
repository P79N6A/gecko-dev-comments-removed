









#include <arm_neon.h>
#include "vp8/encoder/block.h"

static const uint16_t inv_zig_zag[16] = {
    1,  2,  6,   7,
    3,  5,  8,  13,
    4,  9,  12, 14,
    10, 11, 15, 16
};

void vp8_fast_quantize_b_neon(BLOCK *b, BLOCKD *d) {
    const int16x8_t one_q = vdupq_n_s16(-1),
                    z0 = vld1q_s16(b->coeff),
                    z1 = vld1q_s16(b->coeff + 8),
                    round0 = vld1q_s16(b->round),
                    round1 = vld1q_s16(b->round + 8),
                    quant0 = vld1q_s16(b->quant_fast),
                    quant1 = vld1q_s16(b->quant_fast + 8),
                    dequant0 = vld1q_s16(d->dequant),
                    dequant1 = vld1q_s16(d->dequant + 8);
    const uint16x8_t zig_zag0 = vld1q_u16(inv_zig_zag),
                     zig_zag1 = vld1q_u16(inv_zig_zag + 8);
    int16x8_t x0, x1, sz0, sz1, y0, y1;
    uint16x8_t eob0, eob1;
    uint16x4_t eob_d16;
    uint32x2_t eob_d32;
    uint32x4_t eob_q32;

    
    sz0 = vshrq_n_s16(z0, 15);
    sz1 = vshrq_n_s16(z1, 15);

    
    x0 = vabsq_s16(z0);
    x1 = vabsq_s16(z1);

    
    x0 = vaddq_s16(x0, round0);
    x1 = vaddq_s16(x1, round1);

    
    y0 = vqdmulhq_s16(x0, quant0);
    y1 = vqdmulhq_s16(x1, quant1);

    
    y0 = vshrq_n_s16(y0, 1);
    y1 = vshrq_n_s16(y1, 1);

    
    y0 = veorq_s16(y0, sz0);
    y1 = veorq_s16(y1, sz1);
    x0 = vsubq_s16(y0, sz0);
    x1 = vsubq_s16(y1, sz1);

    
    eob0 = vtstq_s16(x0, one_q);
    eob1 = vtstq_s16(x1, one_q);

    
    eob0 = vandq_u16(eob0, zig_zag0);
    eob1 = vandq_u16(eob1, zig_zag1);

    
    eob0 = vmaxq_u16(eob0, eob1);
    eob_d16 = vmax_u16(vget_low_u16(eob0), vget_high_u16(eob0));
    eob_q32 = vmovl_u16(eob_d16);
    eob_d32 = vmax_u32(vget_low_u32(eob_q32), vget_high_u32(eob_q32));
    eob_d32 = vpmax_u32(eob_d32, eob_d32);

    
    vst1q_s16(d->qcoeff, x0);
    vst1q_s16(d->qcoeff + 8, x1);

    
    vst1q_s16(d->dqcoeff, vmulq_s16(dequant0, x0));
    vst1q_s16(d->dqcoeff + 8, vmulq_s16(dequant1, x1));

    vst1_lane_s8((int8_t *)d->eob, vreinterpret_s8_u32(eob_d32), 0);
}
