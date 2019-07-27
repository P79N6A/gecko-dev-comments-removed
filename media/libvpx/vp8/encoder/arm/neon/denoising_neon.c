









#include <arm_neon.h>

#include "vp8/encoder/denoising.h"
#include "vpx_mem/vpx_mem.h"
#include "./vp8_rtcd.h"
































int vp8_denoiser_filter_neon(YV12_BUFFER_CONFIG *mc_running_avg,
                             YV12_BUFFER_CONFIG *running_avg,
                             MACROBLOCK *signal, unsigned int motion_magnitude,
                             int y_offset, int uv_offset) {
    



    const uint8x16_t v_level1_adjustment = vdupq_n_u8(
        (motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD) ? 4 : 3);
    const uint8x16_t v_delta_level_1_and_2 = vdupq_n_u8(1);
    const uint8x16_t v_delta_level_2_and_3 = vdupq_n_u8(2);
    const uint8x16_t v_level1_threshold = vdupq_n_u8(4);
    const uint8x16_t v_level2_threshold = vdupq_n_u8(8);
    const uint8x16_t v_level3_threshold = vdupq_n_u8(16);

    
    unsigned char *sig = signal->thismb;
    int            sig_stride = 16;
    unsigned char *mc_running_avg_y = mc_running_avg->y_buffer + y_offset;
    int            mc_running_avg_y_stride = mc_running_avg->y_stride;
    unsigned char *running_avg_y = running_avg->y_buffer + y_offset;
    int            running_avg_y_stride = running_avg->y_stride;

    
    int i;
    int sum_diff = 0;
    for (i = 0; i < 16; ++i) {
        int8x16_t v_sum_diff = vdupq_n_s8(0);
        uint8x16_t v_running_avg_y;

        
        const uint8x16_t v_sig = vld1q_u8(sig);
        const uint8x16_t v_mc_running_avg_y = vld1q_u8(mc_running_avg_y);

        
        const uint8x16_t v_abs_diff      = vabdq_u8(v_sig, v_mc_running_avg_y);
        const uint8x16_t v_diff_pos_mask = vcltq_u8(v_sig, v_mc_running_avg_y);
        const uint8x16_t v_diff_neg_mask = vcgtq_u8(v_sig, v_mc_running_avg_y);

        
        const uint8x16_t v_level1_mask = vcleq_u8(v_level1_threshold,
                                                  v_abs_diff);
        const uint8x16_t v_level2_mask = vcleq_u8(v_level2_threshold,
                                                  v_abs_diff);
        const uint8x16_t v_level3_mask = vcleq_u8(v_level3_threshold,
                                                  v_abs_diff);

        
        const uint8x16_t v_level2_adjustment = vandq_u8(v_level2_mask,
                                                        v_delta_level_1_and_2);
        const uint8x16_t v_level3_adjustment = vandq_u8(v_level3_mask,
                                                        v_delta_level_2_and_3);
        const uint8x16_t v_level1and2_adjustment = vaddq_u8(v_level1_adjustment,
            v_level2_adjustment);
        const uint8x16_t v_level1and2and3_adjustment = vaddq_u8(
            v_level1and2_adjustment, v_level3_adjustment);

        


        const uint8x16_t v_abs_adjustment = vbslq_u8(v_level1_mask,
            v_level1and2and3_adjustment, v_abs_diff);

        



        const uint8x16_t v_pos_adjustment = vandq_u8(v_diff_pos_mask,
                                                     v_abs_adjustment);
        const uint8x16_t v_neg_adjustment = vandq_u8(v_diff_neg_mask,
                                                     v_abs_adjustment);
        v_running_avg_y = vqaddq_u8(v_sig, v_pos_adjustment);
        v_running_avg_y = vqsubq_u8(v_running_avg_y, v_neg_adjustment);
        v_sum_diff = vqaddq_s8(v_sum_diff,
                               vreinterpretq_s8_u8(v_pos_adjustment));
        v_sum_diff = vqsubq_s8(v_sum_diff,
                               vreinterpretq_s8_u8(v_neg_adjustment));

        
        vst1q_u8(running_avg_y, v_running_avg_y);

        


        {
            int s0 = vgetq_lane_s8(v_sum_diff,  0) +
                     vgetq_lane_s8(v_sum_diff,  1) +
                     vgetq_lane_s8(v_sum_diff,  2) +
                     vgetq_lane_s8(v_sum_diff,  3);
            int s1 = vgetq_lane_s8(v_sum_diff,  4) +
                     vgetq_lane_s8(v_sum_diff,  5) +
                     vgetq_lane_s8(v_sum_diff,  6) +
                     vgetq_lane_s8(v_sum_diff,  7);
            int s2 = vgetq_lane_s8(v_sum_diff,  8) +
                     vgetq_lane_s8(v_sum_diff,  9) +
                     vgetq_lane_s8(v_sum_diff, 10) +
                     vgetq_lane_s8(v_sum_diff, 11);
            int s3 = vgetq_lane_s8(v_sum_diff, 12) +
                     vgetq_lane_s8(v_sum_diff, 13) +
                     vgetq_lane_s8(v_sum_diff, 14) +
                     vgetq_lane_s8(v_sum_diff, 15);
            sum_diff += s0 + s1+ s2 + s3;
        }

        
        sig += sig_stride;
        mc_running_avg_y += mc_running_avg_y_stride;
        running_avg_y += running_avg_y_stride;
    }

    
    if (abs(sum_diff) > SUM_DIFF_THRESHOLD)
        return COPY_BLOCK;

    
    vp8_copy_mem16x16(running_avg->y_buffer + y_offset, running_avg_y_stride,
                      signal->thismb, sig_stride);
    return FILTER_BLOCK;
}
