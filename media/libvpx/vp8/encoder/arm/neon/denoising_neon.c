









#include <arm_neon.h>

#include "vp8/encoder/denoising.h"
#include "vpx_mem/vpx_mem.h"
#include "./vp8_rtcd.h"
































int vp8_denoiser_filter_neon(unsigned char *mc_running_avg_y,
                             int mc_running_avg_y_stride,
                             unsigned char *running_avg_y,
                             int running_avg_y_stride,
                             unsigned char *sig, int sig_stride,
                             unsigned int motion_magnitude,
                             int increase_denoising) {
    



    int shift_inc  = (increase_denoising &&
        motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD) ? 1 : 0;
    const uint8x16_t v_level1_adjustment = vmovq_n_u8(
        (motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD) ? 4 + shift_inc : 3);
    const uint8x16_t v_delta_level_1_and_2 = vdupq_n_u8(1);
    const uint8x16_t v_delta_level_2_and_3 = vdupq_n_u8(2);
    const uint8x16_t v_level1_threshold = vmovq_n_u8(4 + shift_inc);
    const uint8x16_t v_level2_threshold = vdupq_n_u8(8);
    const uint8x16_t v_level3_threshold = vdupq_n_u8(16);
    int64x2_t v_sum_diff_total = vdupq_n_s64(0);

    
    int r;
    for (r = 0; r < 16; ++r) {
        
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

        uint8x16_t v_running_avg_y = vqaddq_u8(v_sig, v_pos_adjustment);
        v_running_avg_y = vqsubq_u8(v_running_avg_y, v_neg_adjustment);

        
        vst1q_u8(running_avg_y, v_running_avg_y);

        


        {
            const int8x16_t v_sum_diff =
                vqsubq_s8(vreinterpretq_s8_u8(v_pos_adjustment),
                          vreinterpretq_s8_u8(v_neg_adjustment));

            const int16x8_t fe_dc_ba_98_76_54_32_10 = vpaddlq_s8(v_sum_diff);

            const int32x4_t fedc_ba98_7654_3210 =
                vpaddlq_s16(fe_dc_ba_98_76_54_32_10);

            const int64x2_t fedcba98_76543210 =
                vpaddlq_s32(fedc_ba98_7654_3210);

            v_sum_diff_total = vqaddq_s64(v_sum_diff_total, fedcba98_76543210);
        }

        
        sig += sig_stride;
        mc_running_avg_y += mc_running_avg_y_stride;
        running_avg_y += running_avg_y_stride;
    }

    
    {
        int64x1_t x = vqadd_s64(vget_high_s64(v_sum_diff_total),
                                      vget_low_s64(v_sum_diff_total));
        int sum_diff = vget_lane_s32(vabs_s32(vreinterpret_s32_s64(x)), 0);
        int sum_diff_thresh = SUM_DIFF_THRESHOLD;

        if (increase_denoising) sum_diff_thresh = SUM_DIFF_THRESHOLD_HIGH;
        if (sum_diff > sum_diff_thresh) {
          
          
          
          
          
          
          

          
          
          int delta = ((sum_diff - sum_diff_thresh) >> 8) + 1;
          
          if (delta < 4) {
            const uint8x16_t k_delta = vmovq_n_u8(delta);
            sig -= sig_stride * 16;
            mc_running_avg_y -= mc_running_avg_y_stride * 16;
            running_avg_y -= running_avg_y_stride * 16;
            for (r = 0; r < 16; ++r) {
              uint8x16_t v_running_avg_y = vld1q_u8(running_avg_y);
              const uint8x16_t v_sig = vld1q_u8(sig);
              const uint8x16_t v_mc_running_avg_y = vld1q_u8(mc_running_avg_y);

              
              const uint8x16_t v_abs_diff      = vabdq_u8(v_sig,
                                                          v_mc_running_avg_y);
              const uint8x16_t v_diff_pos_mask = vcltq_u8(v_sig,
                                                          v_mc_running_avg_y);
              const uint8x16_t v_diff_neg_mask = vcgtq_u8(v_sig,
                                                          v_mc_running_avg_y);
              
              const uint8x16_t v_abs_adjustment =
                  vminq_u8(v_abs_diff, (k_delta));

              const uint8x16_t v_pos_adjustment = vandq_u8(v_diff_pos_mask,
                                                           v_abs_adjustment);
              const uint8x16_t v_neg_adjustment = vandq_u8(v_diff_neg_mask,
                                                           v_abs_adjustment);

              v_running_avg_y = vqsubq_u8(v_running_avg_y, v_pos_adjustment);
              v_running_avg_y = vqaddq_u8(v_running_avg_y, v_neg_adjustment);

              
              vst1q_u8(running_avg_y, v_running_avg_y);

              {
                  const int8x16_t v_sum_diff =
                      vqsubq_s8(vreinterpretq_s8_u8(v_neg_adjustment),
                                vreinterpretq_s8_u8(v_pos_adjustment));

                  const int16x8_t fe_dc_ba_98_76_54_32_10 =
                      vpaddlq_s8(v_sum_diff);
                  const int32x4_t fedc_ba98_7654_3210 =
                      vpaddlq_s16(fe_dc_ba_98_76_54_32_10);
                  const int64x2_t fedcba98_76543210 =
                      vpaddlq_s32(fedc_ba98_7654_3210);

                  v_sum_diff_total = vqaddq_s64(v_sum_diff_total,
                                                fedcba98_76543210);
              }
              
              sig += sig_stride;
              mc_running_avg_y += mc_running_avg_y_stride;
              running_avg_y += running_avg_y_stride;
            }
            {
              
              x = vqadd_s64(vget_high_s64(v_sum_diff_total),
                            vget_low_s64(v_sum_diff_total));
              sum_diff = vget_lane_s32(vabs_s32(vreinterpret_s32_s64(x)), 0);

              if (sum_diff > sum_diff_thresh) {
                return COPY_BLOCK;
              }
            }
          } else {
            return COPY_BLOCK;
          }
        }
    }

    
    running_avg_y -= running_avg_y_stride * 16;
    sig -= sig_stride * 16;

    vp8_copy_mem16x16(running_avg_y, running_avg_y_stride, sig, sig_stride);

    return FILTER_BLOCK;
}

int vp8_denoiser_filter_uv_neon(unsigned char *mc_running_avg,
                             int mc_running_avg_stride,
                             unsigned char *running_avg,
                             int running_avg_stride,
                             unsigned char *sig, int sig_stride,
                             unsigned int motion_magnitude,
                             int increase_denoising) {
    



    int shift_inc  = (increase_denoising &&
        motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD_UV) ? 1 : 0;
    const uint8x16_t v_level1_adjustment = vmovq_n_u8(
        (motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD_UV) ? 4 + shift_inc : 3);

    const uint8x16_t v_delta_level_1_and_2 = vdupq_n_u8(1);
    const uint8x16_t v_delta_level_2_and_3 = vdupq_n_u8(2);
    const uint8x16_t v_level1_threshold = vmovq_n_u8(4 + shift_inc);
    const uint8x16_t v_level2_threshold = vdupq_n_u8(8);
    const uint8x16_t v_level3_threshold = vdupq_n_u8(16);
    int64x2_t v_sum_diff_total = vdupq_n_s64(0);
    int r;

    {
      uint16x4_t v_sum_block = vdup_n_u16(0);

      
      for (r = 0; r < 8; ++r) {
        const uint8x8_t v_sig = vld1_u8(sig);
        const uint16x4_t _76_54_32_10 = vpaddl_u8(v_sig);
        v_sum_block = vqadd_u16(v_sum_block, _76_54_32_10);
        sig += sig_stride;
      }
      sig -= sig_stride * 8;
      {
        const uint32x2_t _7654_3210 = vpaddl_u16(v_sum_block);
        const uint64x1_t _76543210 = vpaddl_u32(_7654_3210);
        const int sum_block =
            vget_lane_s32(vreinterpret_s32_u64(_76543210), 0);
        if (abs(sum_block - (128 * 8 * 8)) < SUM_DIFF_FROM_AVG_THRESH_UV) {
          return COPY_BLOCK;
        }
      }
    }

    
    for (r = 0; r < 4; ++r) {
        
        const uint8x8_t v_sig_lo = vld1_u8(sig);
        const uint8x8_t v_sig_hi = vld1_u8(&sig[sig_stride]);
        const uint8x16_t v_sig = vcombine_u8(v_sig_lo, v_sig_hi);
        const uint8x8_t v_mc_running_avg_lo = vld1_u8(mc_running_avg);
        const uint8x8_t v_mc_running_avg_hi =
            vld1_u8(&mc_running_avg[mc_running_avg_stride]);
        const uint8x16_t v_mc_running_avg =
            vcombine_u8(v_mc_running_avg_lo, v_mc_running_avg_hi);
        
        const uint8x16_t v_abs_diff      = vabdq_u8(v_sig, v_mc_running_avg);
        const uint8x16_t v_diff_pos_mask = vcltq_u8(v_sig, v_mc_running_avg);
        const uint8x16_t v_diff_neg_mask = vcgtq_u8(v_sig, v_mc_running_avg);

        
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

        uint8x16_t v_running_avg = vqaddq_u8(v_sig, v_pos_adjustment);
        v_running_avg = vqsubq_u8(v_running_avg, v_neg_adjustment);

        
        vst1_u8(running_avg, vget_low_u8(v_running_avg));
        vst1_u8(&running_avg[running_avg_stride], vget_high_u8(v_running_avg));

        


        {
            const int8x16_t v_sum_diff =
                vqsubq_s8(vreinterpretq_s8_u8(v_pos_adjustment),
                          vreinterpretq_s8_u8(v_neg_adjustment));

            const int16x8_t fe_dc_ba_98_76_54_32_10 = vpaddlq_s8(v_sum_diff);

            const int32x4_t fedc_ba98_7654_3210 =
                vpaddlq_s16(fe_dc_ba_98_76_54_32_10);

            const int64x2_t fedcba98_76543210 =
                vpaddlq_s32(fedc_ba98_7654_3210);

            v_sum_diff_total = vqaddq_s64(v_sum_diff_total, fedcba98_76543210);
        }

        
        sig += sig_stride * 2;
        mc_running_avg += mc_running_avg_stride * 2;
        running_avg += running_avg_stride * 2;
    }


    
    {
        int64x1_t x = vqadd_s64(vget_high_s64(v_sum_diff_total),
                                      vget_low_s64(v_sum_diff_total));
        int sum_diff = vget_lane_s32(vabs_s32(vreinterpret_s32_s64(x)), 0);
        int sum_diff_thresh = SUM_DIFF_THRESHOLD_UV;
        if (increase_denoising) sum_diff_thresh = SUM_DIFF_THRESHOLD_HIGH_UV;
        if (sum_diff > sum_diff_thresh) {
          
          
          
          
          
          
          

          
          
          int delta = ((sum_diff - sum_diff_thresh) >> 8) + 1;
          
          if (delta < 4) {
            const uint8x16_t k_delta = vmovq_n_u8(delta);
            sig -= sig_stride * 8;
            mc_running_avg -= mc_running_avg_stride * 8;
            running_avg -= running_avg_stride * 8;
            for (r = 0; r < 4; ++r) {
              const uint8x8_t v_sig_lo = vld1_u8(sig);
              const uint8x8_t v_sig_hi = vld1_u8(&sig[sig_stride]);
              const uint8x16_t v_sig = vcombine_u8(v_sig_lo, v_sig_hi);
              const uint8x8_t v_mc_running_avg_lo = vld1_u8(mc_running_avg);
              const uint8x8_t v_mc_running_avg_hi =
                  vld1_u8(&mc_running_avg[mc_running_avg_stride]);
              const uint8x16_t v_mc_running_avg =
                  vcombine_u8(v_mc_running_avg_lo, v_mc_running_avg_hi);
              
              const uint8x16_t v_abs_diff      = vabdq_u8(v_sig,
                                                          v_mc_running_avg);
              const uint8x16_t v_diff_pos_mask = vcltq_u8(v_sig,
                                                          v_mc_running_avg);
              const uint8x16_t v_diff_neg_mask = vcgtq_u8(v_sig,
                                                          v_mc_running_avg);
              
              const uint8x16_t v_abs_adjustment =
                  vminq_u8(v_abs_diff, (k_delta));

              const uint8x16_t v_pos_adjustment = vandq_u8(v_diff_pos_mask,
                                                           v_abs_adjustment);
              const uint8x16_t v_neg_adjustment = vandq_u8(v_diff_neg_mask,
                                                           v_abs_adjustment);
              const uint8x8_t v_running_avg_lo = vld1_u8(running_avg);
              const uint8x8_t v_running_avg_hi =
                  vld1_u8(&running_avg[running_avg_stride]);
              uint8x16_t v_running_avg =
                  vcombine_u8(v_running_avg_lo, v_running_avg_hi);

              v_running_avg = vqsubq_u8(v_running_avg, v_pos_adjustment);
              v_running_avg = vqaddq_u8(v_running_avg, v_neg_adjustment);

              
              vst1_u8(running_avg, vget_low_u8(v_running_avg));
              vst1_u8(&running_avg[running_avg_stride],
                      vget_high_u8(v_running_avg));

              {
                  const int8x16_t v_sum_diff =
                      vqsubq_s8(vreinterpretq_s8_u8(v_neg_adjustment),
                                vreinterpretq_s8_u8(v_pos_adjustment));

                  const int16x8_t fe_dc_ba_98_76_54_32_10 =
                      vpaddlq_s8(v_sum_diff);
                  const int32x4_t fedc_ba98_7654_3210 =
                      vpaddlq_s16(fe_dc_ba_98_76_54_32_10);
                  const int64x2_t fedcba98_76543210 =
                      vpaddlq_s32(fedc_ba98_7654_3210);

                  v_sum_diff_total = vqaddq_s64(v_sum_diff_total,
                                                fedcba98_76543210);
              }
              
              sig += sig_stride * 2;
              mc_running_avg += mc_running_avg_stride * 2;
              running_avg += running_avg_stride * 2;
            }
            {
              
              x = vqadd_s64(vget_high_s64(v_sum_diff_total),
                            vget_low_s64(v_sum_diff_total));
              sum_diff = vget_lane_s32(vabs_s32(vreinterpret_s32_s64(x)), 0);

              if (sum_diff > sum_diff_thresh) {
                return COPY_BLOCK;
              }
            }
          } else {
            return COPY_BLOCK;
          }
        }
    }

    
    running_avg -= running_avg_stride * 8;
    sig -= sig_stride * 8;

    vp8_copy_mem8x8(running_avg, running_avg_stride, sig, sig_stride);

    return FILTER_BLOCK;
}
