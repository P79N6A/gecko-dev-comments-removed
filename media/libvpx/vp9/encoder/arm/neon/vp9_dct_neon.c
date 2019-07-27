









#include <arm_neon.h>
#include "./vp9_rtcd.h"
#include "./vpx_config.h"

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_idct.h"

void vp9_fdct8x8_1_neon(const int16_t *input, int16_t *output, int stride) {
  int r;
  int16x8_t sum = vld1q_s16(&input[0]);
  for (r = 1; r < 8; ++r) {
    const int16x8_t input_00 = vld1q_s16(&input[r * stride]);
    sum = vaddq_s16(sum, input_00);
  }
  {
    const int32x4_t a = vpaddlq_s16(sum);
    const int64x2_t b = vpaddlq_s32(a);
    const int32x2_t c = vadd_s32(vreinterpret_s32_s64(vget_low_s64(b)),
                                 vreinterpret_s32_s64(vget_high_s64(b)));
    output[0] = vget_lane_s16(vreinterpret_s16_s32(c), 0);
    output[1] = 0;
  }
}

void vp9_fdct8x8_quant_neon(const int16_t *input, int stride,
                            int16_t* coeff_ptr, intptr_t n_coeffs,
                            int skip_block, const int16_t* zbin_ptr,
                            const int16_t* round_ptr, const int16_t* quant_ptr,
                            const int16_t* quant_shift_ptr,
                            int16_t* qcoeff_ptr, int16_t* dqcoeff_ptr,
                            const int16_t* dequant_ptr, uint16_t* eob_ptr,
                            const int16_t* scan_ptr,
                            const int16_t* iscan_ptr) {
  int16_t temp_buffer[64];
  (void)coeff_ptr;

  vp9_fdct8x8_neon(input, temp_buffer, stride);
  vp9_quantize_fp_neon(temp_buffer, n_coeffs, skip_block, zbin_ptr, round_ptr,
                       quant_ptr, quant_shift_ptr, qcoeff_ptr, dqcoeff_ptr,
                       dequant_ptr, eob_ptr, scan_ptr, iscan_ptr);
}

void vp9_fdct8x8_neon(const int16_t *input, int16_t *final_output, int stride) {
  int i;
  
  int16x8_t input_0 = vshlq_n_s16(vld1q_s16(&input[0 * stride]), 2);
  int16x8_t input_1 = vshlq_n_s16(vld1q_s16(&input[1 * stride]), 2);
  int16x8_t input_2 = vshlq_n_s16(vld1q_s16(&input[2 * stride]), 2);
  int16x8_t input_3 = vshlq_n_s16(vld1q_s16(&input[3 * stride]), 2);
  int16x8_t input_4 = vshlq_n_s16(vld1q_s16(&input[4 * stride]), 2);
  int16x8_t input_5 = vshlq_n_s16(vld1q_s16(&input[5 * stride]), 2);
  int16x8_t input_6 = vshlq_n_s16(vld1q_s16(&input[6 * stride]), 2);
  int16x8_t input_7 = vshlq_n_s16(vld1q_s16(&input[7 * stride]), 2);
  for (i = 0; i < 2; ++i) {
    int16x8_t out_0, out_1, out_2, out_3, out_4, out_5, out_6, out_7;
    const int16x8_t v_s0 = vaddq_s16(input_0, input_7);
    const int16x8_t v_s1 = vaddq_s16(input_1, input_6);
    const int16x8_t v_s2 = vaddq_s16(input_2, input_5);
    const int16x8_t v_s3 = vaddq_s16(input_3, input_4);
    const int16x8_t v_s4 = vsubq_s16(input_3, input_4);
    const int16x8_t v_s5 = vsubq_s16(input_2, input_5);
    const int16x8_t v_s6 = vsubq_s16(input_1, input_6);
    const int16x8_t v_s7 = vsubq_s16(input_0, input_7);
    
    int16x8_t v_x0 = vaddq_s16(v_s0, v_s3);
    int16x8_t v_x1 = vaddq_s16(v_s1, v_s2);
    int16x8_t v_x2 = vsubq_s16(v_s1, v_s2);
    int16x8_t v_x3 = vsubq_s16(v_s0, v_s3);
    
    int32x4_t v_t0_lo = vaddl_s16(vget_low_s16(v_x0), vget_low_s16(v_x1));
    int32x4_t v_t0_hi = vaddl_s16(vget_high_s16(v_x0), vget_high_s16(v_x1));
    int32x4_t v_t1_lo = vsubl_s16(vget_low_s16(v_x0), vget_low_s16(v_x1));
    int32x4_t v_t1_hi = vsubl_s16(vget_high_s16(v_x0), vget_high_s16(v_x1));
    int32x4_t v_t2_lo = vmull_n_s16(vget_low_s16(v_x2), (int16_t)cospi_24_64);
    int32x4_t v_t2_hi = vmull_n_s16(vget_high_s16(v_x2), (int16_t)cospi_24_64);
    int32x4_t v_t3_lo = vmull_n_s16(vget_low_s16(v_x3), (int16_t)cospi_24_64);
    int32x4_t v_t3_hi = vmull_n_s16(vget_high_s16(v_x3), (int16_t)cospi_24_64);
    v_t2_lo = vmlal_n_s16(v_t2_lo, vget_low_s16(v_x3), (int16_t)cospi_8_64);
    v_t2_hi = vmlal_n_s16(v_t2_hi, vget_high_s16(v_x3), (int16_t)cospi_8_64);
    v_t3_lo = vmlsl_n_s16(v_t3_lo, vget_low_s16(v_x2), (int16_t)cospi_8_64);
    v_t3_hi = vmlsl_n_s16(v_t3_hi, vget_high_s16(v_x2), (int16_t)cospi_8_64);
    v_t0_lo = vmulq_n_s32(v_t0_lo, cospi_16_64);
    v_t0_hi = vmulq_n_s32(v_t0_hi, cospi_16_64);
    v_t1_lo = vmulq_n_s32(v_t1_lo, cospi_16_64);
    v_t1_hi = vmulq_n_s32(v_t1_hi, cospi_16_64);
    {
      const int16x4_t a = vrshrn_n_s32(v_t0_lo, DCT_CONST_BITS);
      const int16x4_t b = vrshrn_n_s32(v_t0_hi, DCT_CONST_BITS);
      const int16x4_t c = vrshrn_n_s32(v_t1_lo, DCT_CONST_BITS);
      const int16x4_t d = vrshrn_n_s32(v_t1_hi, DCT_CONST_BITS);
      const int16x4_t e = vrshrn_n_s32(v_t2_lo, DCT_CONST_BITS);
      const int16x4_t f = vrshrn_n_s32(v_t2_hi, DCT_CONST_BITS);
      const int16x4_t g = vrshrn_n_s32(v_t3_lo, DCT_CONST_BITS);
      const int16x4_t h = vrshrn_n_s32(v_t3_hi, DCT_CONST_BITS);
      out_0 = vcombine_s16(a, c);  
      out_2 = vcombine_s16(e, g);  
      out_4 = vcombine_s16(b, d);  
      out_6 = vcombine_s16(f, h);  
    }
    
    v_x0 = vsubq_s16(v_s6, v_s5);
    v_x1 = vaddq_s16(v_s6, v_s5);
    v_t0_lo = vmull_n_s16(vget_low_s16(v_x0), (int16_t)cospi_16_64);
    v_t0_hi = vmull_n_s16(vget_high_s16(v_x0), (int16_t)cospi_16_64);
    v_t1_lo = vmull_n_s16(vget_low_s16(v_x1), (int16_t)cospi_16_64);
    v_t1_hi = vmull_n_s16(vget_high_s16(v_x1), (int16_t)cospi_16_64);
    {
      const int16x4_t a = vrshrn_n_s32(v_t0_lo, DCT_CONST_BITS);
      const int16x4_t b = vrshrn_n_s32(v_t0_hi, DCT_CONST_BITS);
      const int16x4_t c = vrshrn_n_s32(v_t1_lo, DCT_CONST_BITS);
      const int16x4_t d = vrshrn_n_s32(v_t1_hi, DCT_CONST_BITS);
      const int16x8_t ab = vcombine_s16(a, b);
      const int16x8_t cd = vcombine_s16(c, d);
      
      v_x0 = vaddq_s16(v_s4, ab);
      v_x1 = vsubq_s16(v_s4, ab);
      v_x2 = vsubq_s16(v_s7, cd);
      v_x3 = vaddq_s16(v_s7, cd);
    }
    
    v_t0_lo = vmull_n_s16(vget_low_s16(v_x3), (int16_t)cospi_4_64);
    v_t0_hi = vmull_n_s16(vget_high_s16(v_x3), (int16_t)cospi_4_64);
    v_t0_lo = vmlal_n_s16(v_t0_lo, vget_low_s16(v_x0), (int16_t)cospi_28_64);
    v_t0_hi = vmlal_n_s16(v_t0_hi, vget_high_s16(v_x0), (int16_t)cospi_28_64);
    v_t1_lo = vmull_n_s16(vget_low_s16(v_x1), (int16_t)cospi_12_64);
    v_t1_hi = vmull_n_s16(vget_high_s16(v_x1), (int16_t)cospi_12_64);
    v_t1_lo = vmlal_n_s16(v_t1_lo, vget_low_s16(v_x2), (int16_t)cospi_20_64);
    v_t1_hi = vmlal_n_s16(v_t1_hi, vget_high_s16(v_x2), (int16_t)cospi_20_64);
    v_t2_lo = vmull_n_s16(vget_low_s16(v_x2), (int16_t)cospi_12_64);
    v_t2_hi = vmull_n_s16(vget_high_s16(v_x2), (int16_t)cospi_12_64);
    v_t2_lo = vmlsl_n_s16(v_t2_lo, vget_low_s16(v_x1), (int16_t)cospi_20_64);
    v_t2_hi = vmlsl_n_s16(v_t2_hi, vget_high_s16(v_x1), (int16_t)cospi_20_64);
    v_t3_lo = vmull_n_s16(vget_low_s16(v_x3), (int16_t)cospi_28_64);
    v_t3_hi = vmull_n_s16(vget_high_s16(v_x3), (int16_t)cospi_28_64);
    v_t3_lo = vmlsl_n_s16(v_t3_lo, vget_low_s16(v_x0), (int16_t)cospi_4_64);
    v_t3_hi = vmlsl_n_s16(v_t3_hi, vget_high_s16(v_x0), (int16_t)cospi_4_64);
    {
      const int16x4_t a = vrshrn_n_s32(v_t0_lo, DCT_CONST_BITS);
      const int16x4_t b = vrshrn_n_s32(v_t0_hi, DCT_CONST_BITS);
      const int16x4_t c = vrshrn_n_s32(v_t1_lo, DCT_CONST_BITS);
      const int16x4_t d = vrshrn_n_s32(v_t1_hi, DCT_CONST_BITS);
      const int16x4_t e = vrshrn_n_s32(v_t2_lo, DCT_CONST_BITS);
      const int16x4_t f = vrshrn_n_s32(v_t2_hi, DCT_CONST_BITS);
      const int16x4_t g = vrshrn_n_s32(v_t3_lo, DCT_CONST_BITS);
      const int16x4_t h = vrshrn_n_s32(v_t3_hi, DCT_CONST_BITS);
      out_1 = vcombine_s16(a, c);  
      out_3 = vcombine_s16(e, g);  
      out_5 = vcombine_s16(b, d);  
      out_7 = vcombine_s16(f, h);  
    }
    
    {
      
      
      
      
      
      
      
      
      const int32x4x2_t r02_s32 = vtrnq_s32(vreinterpretq_s32_s16(out_0),
                                            vreinterpretq_s32_s16(out_2));
      const int32x4x2_t r13_s32 = vtrnq_s32(vreinterpretq_s32_s16(out_1),
                                            vreinterpretq_s32_s16(out_3));
      const int32x4x2_t r46_s32 = vtrnq_s32(vreinterpretq_s32_s16(out_4),
                                            vreinterpretq_s32_s16(out_6));
      const int32x4x2_t r57_s32 = vtrnq_s32(vreinterpretq_s32_s16(out_5),
                                            vreinterpretq_s32_s16(out_7));
      const int16x8x2_t r01_s16 =
          vtrnq_s16(vreinterpretq_s16_s32(r02_s32.val[0]),
                    vreinterpretq_s16_s32(r13_s32.val[0]));
      const int16x8x2_t r23_s16 =
          vtrnq_s16(vreinterpretq_s16_s32(r02_s32.val[1]),
                    vreinterpretq_s16_s32(r13_s32.val[1]));
      const int16x8x2_t r45_s16 =
          vtrnq_s16(vreinterpretq_s16_s32(r46_s32.val[0]),
                    vreinterpretq_s16_s32(r57_s32.val[0]));
      const int16x8x2_t r67_s16 =
          vtrnq_s16(vreinterpretq_s16_s32(r46_s32.val[1]),
                    vreinterpretq_s16_s32(r57_s32.val[1]));
      input_0 = r01_s16.val[0];
      input_1 = r01_s16.val[1];
      input_2 = r23_s16.val[0];
      input_3 = r23_s16.val[1];
      input_4 = r45_s16.val[0];
      input_5 = r45_s16.val[1];
      input_6 = r67_s16.val[0];
      input_7 = r67_s16.val[1];
      
      
      
      
      
      
      
      
    }
  }  
  {
    
    
    
    
    const int16x8_t sign_in0 = vshrq_n_s16(input_0, 15);
    const int16x8_t sign_in1 = vshrq_n_s16(input_1, 15);
    const int16x8_t sign_in2 = vshrq_n_s16(input_2, 15);
    const int16x8_t sign_in3 = vshrq_n_s16(input_3, 15);
    const int16x8_t sign_in4 = vshrq_n_s16(input_4, 15);
    const int16x8_t sign_in5 = vshrq_n_s16(input_5, 15);
    const int16x8_t sign_in6 = vshrq_n_s16(input_6, 15);
    const int16x8_t sign_in7 = vshrq_n_s16(input_7, 15);
    input_0 = vhsubq_s16(input_0, sign_in0);
    input_1 = vhsubq_s16(input_1, sign_in1);
    input_2 = vhsubq_s16(input_2, sign_in2);
    input_3 = vhsubq_s16(input_3, sign_in3);
    input_4 = vhsubq_s16(input_4, sign_in4);
    input_5 = vhsubq_s16(input_5, sign_in5);
    input_6 = vhsubq_s16(input_6, sign_in6);
    input_7 = vhsubq_s16(input_7, sign_in7);
    
    vst1q_s16(&final_output[0 * 8], input_0);
    vst1q_s16(&final_output[1 * 8], input_1);
    vst1q_s16(&final_output[2 * 8], input_2);
    vst1q_s16(&final_output[3 * 8], input_3);
    vst1q_s16(&final_output[4 * 8], input_4);
    vst1q_s16(&final_output[5 * 8], input_5);
    vst1q_s16(&final_output[6 * 8], input_6);
    vst1q_s16(&final_output[7 * 8], input_7);
  }
}

