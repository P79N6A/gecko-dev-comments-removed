









#include <arm_neon.h>

void vp8_short_inv_walsh4x4_neon(
        int16_t *input,
        int16_t *mb_dqcoeff) {
    int16x8_t q0s16, q1s16, q2s16, q3s16;
    int16x4_t d4s16, d5s16, d6s16, d7s16;
    int16x4x2_t v2tmp0, v2tmp1;
    int32x2x2_t v2tmp2, v2tmp3;
    int16x8_t qAdd3;

    q0s16 = vld1q_s16(input);
    q1s16 = vld1q_s16(input + 8);

    
    d4s16 = vadd_s16(vget_low_s16(q0s16), vget_high_s16(q1s16));
    d6s16 = vadd_s16(vget_high_s16(q0s16), vget_low_s16(q1s16));
    d5s16 = vsub_s16(vget_low_s16(q0s16), vget_high_s16(q1s16));
    d7s16 = vsub_s16(vget_high_s16(q0s16), vget_low_s16(q1s16));

    q2s16 = vcombine_s16(d4s16, d5s16);
    q3s16 = vcombine_s16(d6s16, d7s16);

    q0s16 = vaddq_s16(q2s16, q3s16);
    q1s16 = vsubq_s16(q2s16, q3s16);

    v2tmp2 = vtrn_s32(vreinterpret_s32_s16(vget_low_s16(q0s16)),
                      vreinterpret_s32_s16(vget_low_s16(q1s16)));
    v2tmp3 = vtrn_s32(vreinterpret_s32_s16(vget_high_s16(q0s16)),
                      vreinterpret_s32_s16(vget_high_s16(q1s16)));
    v2tmp0 = vtrn_s16(vreinterpret_s16_s32(v2tmp2.val[0]),
                      vreinterpret_s16_s32(v2tmp3.val[0]));
    v2tmp1 = vtrn_s16(vreinterpret_s16_s32(v2tmp2.val[1]),
                      vreinterpret_s16_s32(v2tmp3.val[1]));

    
    d4s16 = vadd_s16(v2tmp0.val[0], v2tmp1.val[1]);
    d6s16 = vadd_s16(v2tmp0.val[1], v2tmp1.val[0]);
    d5s16 = vsub_s16(v2tmp0.val[0], v2tmp1.val[1]);
    d7s16 = vsub_s16(v2tmp0.val[1], v2tmp1.val[0]);
    q2s16 = vcombine_s16(d4s16, d5s16);
    q3s16 = vcombine_s16(d6s16, d7s16);

    qAdd3 = vdupq_n_s16(3);

    q0s16 = vaddq_s16(q2s16, q3s16);
    q1s16 = vsubq_s16(q2s16, q3s16);

    q0s16 = vaddq_s16(q0s16, qAdd3);
    q1s16 = vaddq_s16(q1s16, qAdd3);

    q0s16 = vshrq_n_s16(q0s16, 3);
    q1s16 = vshrq_n_s16(q1s16, 3);

    
    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q0s16),  0);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q0s16), 0);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q1s16),  0);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q1s16), 0);
    mb_dqcoeff += 16;

    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q0s16),  1);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q0s16), 1);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q1s16),  1);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q1s16), 1);
    mb_dqcoeff += 16;

    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q0s16),  2);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q0s16), 2);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q1s16),  2);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q1s16), 2);
    mb_dqcoeff += 16;

    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q0s16),  3);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q0s16), 3);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_low_s16(q1s16),  3);
    mb_dqcoeff += 16;
    vst1_lane_s16(mb_dqcoeff, vget_high_s16(q1s16), 3);
    mb_dqcoeff += 16;
    return;
}
