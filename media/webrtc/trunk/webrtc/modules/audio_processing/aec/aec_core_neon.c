















#include <arm_neon.h>
#include <math.h>
#include <string.h>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/aec/aec_common.h"
#include "webrtc/modules/audio_processing/aec/aec_core_internal.h"
#include "webrtc/modules/audio_processing/aec/aec_rdft.h"

enum { kShiftExponentIntoTopMantissa = 8 };
enum { kFloatExponentShift = 23 };

__inline static float MulRe(float aRe, float aIm, float bRe, float bIm) {
  return aRe * bRe - aIm * bIm;
}

__inline static float MulIm(float aRe, float aIm, float bRe, float bIm) {
  return aRe * bIm + aIm * bRe;
}

static void FilterFarNEON(AecCore* aec, float yf[2][PART_LEN1]) {
  int i;
  const int num_partitions = aec->num_partitions;
  for (i = 0; i < num_partitions; i++) {
    int j;
    int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
    int pos = i * PART_LEN1;
    
    if (i + aec->xfBufBlockPos >= num_partitions) {
      xPos -= num_partitions * PART_LEN1;
    }

    
    for (j = 0; j + 3 < PART_LEN1; j += 4) {
      const float32x4_t xfBuf_re = vld1q_f32(&aec->xfBuf[0][xPos + j]);
      const float32x4_t xfBuf_im = vld1q_f32(&aec->xfBuf[1][xPos + j]);
      const float32x4_t wfBuf_re = vld1q_f32(&aec->wfBuf[0][pos + j]);
      const float32x4_t wfBuf_im = vld1q_f32(&aec->wfBuf[1][pos + j]);
      const float32x4_t yf_re = vld1q_f32(&yf[0][j]);
      const float32x4_t yf_im = vld1q_f32(&yf[1][j]);
      const float32x4_t a = vmulq_f32(xfBuf_re, wfBuf_re);
      const float32x4_t e = vmlsq_f32(a, xfBuf_im, wfBuf_im);
      const float32x4_t c = vmulq_f32(xfBuf_re, wfBuf_im);
      const float32x4_t f = vmlaq_f32(c, xfBuf_im, wfBuf_re);
      const float32x4_t g = vaddq_f32(yf_re, e);
      const float32x4_t h = vaddq_f32(yf_im, f);
      vst1q_f32(&yf[0][j], g);
      vst1q_f32(&yf[1][j], h);
    }
    
    for (; j < PART_LEN1; j++) {
      yf[0][j] += MulRe(aec->xfBuf[0][xPos + j],
                        aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][pos + j],
                        aec->wfBuf[1][pos + j]);
      yf[1][j] += MulIm(aec->xfBuf[0][xPos + j],
                        aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][pos + j],
                        aec->wfBuf[1][pos + j]);
    }
  }
}

static float32x4_t vdivq_f32(float32x4_t a, float32x4_t b) {
  int i;
  float32x4_t x = vrecpeq_f32(b);
  
  
  
  
  
  
  for (i = 0; i < 2; i++) {
    x = vmulq_f32(vrecpsq_f32(b, x), x);
  }
  
  return vmulq_f32(a, x);
}

static float32x4_t vsqrtq_f32(float32x4_t s) {
  int i;
  float32x4_t x = vrsqrteq_f32(s);

  
  
  
  const uint32x4_t vec_p_inf = vdupq_n_u32(0x7F800000);
  
  const uint32x4_t div_by_zero = vceqq_u32(vec_p_inf, vreinterpretq_u32_f32(x));
  
  x = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(div_by_zero),
                                      vreinterpretq_u32_f32(x)));
  
  
  
  
  
  
  for (i = 0; i < 2; i++) {
    x = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, x), s), x);
  }
  
  return vmulq_f32(s, x);;
}

static void ScaleErrorSignalNEON(AecCore* aec, float ef[2][PART_LEN1]) {
  const float mu = aec->extended_filter_enabled ? kExtendedMu : aec->normal_mu;
  const float error_threshold = aec->extended_filter_enabled ?
      kExtendedErrorThreshold : aec->normal_error_threshold;
  const float32x4_t k1e_10f = vdupq_n_f32(1e-10f);
  const float32x4_t kMu = vmovq_n_f32(mu);
  const float32x4_t kThresh = vmovq_n_f32(error_threshold);
  int i;
  
  for (i = 0; i + 3 < PART_LEN1; i += 4) {
    const float32x4_t xPow = vld1q_f32(&aec->xPow[i]);
    const float32x4_t ef_re_base = vld1q_f32(&ef[0][i]);
    const float32x4_t ef_im_base = vld1q_f32(&ef[1][i]);
    const float32x4_t xPowPlus = vaddq_f32(xPow, k1e_10f);
    float32x4_t ef_re = vdivq_f32(ef_re_base, xPowPlus);
    float32x4_t ef_im = vdivq_f32(ef_im_base, xPowPlus);
    const float32x4_t ef_re2 = vmulq_f32(ef_re, ef_re);
    const float32x4_t ef_sum2 = vmlaq_f32(ef_re2, ef_im, ef_im);
    const float32x4_t absEf = vsqrtq_f32(ef_sum2);
    const uint32x4_t bigger = vcgtq_f32(absEf, kThresh);
    const float32x4_t absEfPlus = vaddq_f32(absEf, k1e_10f);
    const float32x4_t absEfInv = vdivq_f32(kThresh, absEfPlus);
    uint32x4_t ef_re_if = vreinterpretq_u32_f32(vmulq_f32(ef_re, absEfInv));
    uint32x4_t ef_im_if = vreinterpretq_u32_f32(vmulq_f32(ef_im, absEfInv));
    uint32x4_t ef_re_u32 = vandq_u32(vmvnq_u32(bigger),
                                     vreinterpretq_u32_f32(ef_re));
    uint32x4_t ef_im_u32 = vandq_u32(vmvnq_u32(bigger),
                                     vreinterpretq_u32_f32(ef_im));
    ef_re_if = vandq_u32(bigger, ef_re_if);
    ef_im_if = vandq_u32(bigger, ef_im_if);
    ef_re_u32 = vorrq_u32(ef_re_u32, ef_re_if);
    ef_im_u32 = vorrq_u32(ef_im_u32, ef_im_if);
    ef_re = vmulq_f32(vreinterpretq_f32_u32(ef_re_u32), kMu);
    ef_im = vmulq_f32(vreinterpretq_f32_u32(ef_im_u32), kMu);
    vst1q_f32(&ef[0][i], ef_re);
    vst1q_f32(&ef[1][i], ef_im);
  }
  
  for (; i < PART_LEN1; i++) {
    float abs_ef;
    ef[0][i] /= (aec->xPow[i] + 1e-10f);
    ef[1][i] /= (aec->xPow[i] + 1e-10f);
    abs_ef = sqrtf(ef[0][i] * ef[0][i] + ef[1][i] * ef[1][i]);

    if (abs_ef > error_threshold) {
      abs_ef = error_threshold / (abs_ef + 1e-10f);
      ef[0][i] *= abs_ef;
      ef[1][i] *= abs_ef;
    }

    
    ef[0][i] *= mu;
    ef[1][i] *= mu;
  }
}

static void FilterAdaptationNEON(AecCore* aec,
                                 float* fft,
                                 float ef[2][PART_LEN1]) {
  int i;
  const int num_partitions = aec->num_partitions;
  for (i = 0; i < num_partitions; i++) {
    int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
    int pos = i * PART_LEN1;
    int j;
    
    if (i + aec->xfBufBlockPos >= num_partitions) {
      xPos -= num_partitions * PART_LEN1;
    }

    
    for (j = 0; j < PART_LEN; j += 4) {
      
      const float32x4_t xfBuf_re = vld1q_f32(&aec->xfBuf[0][xPos + j]);
      const float32x4_t xfBuf_im = vld1q_f32(&aec->xfBuf[1][xPos + j]);
      const float32x4_t ef_re = vld1q_f32(&ef[0][j]);
      const float32x4_t ef_im = vld1q_f32(&ef[1][j]);
      
      
      
      const float32x4_t a = vmulq_f32(xfBuf_re, ef_re);
      const float32x4_t e = vmlaq_f32(a, xfBuf_im, ef_im);
      const float32x4_t c = vmulq_f32(xfBuf_re, ef_im);
      const float32x4_t f = vmlsq_f32(c, xfBuf_im, ef_re);
      
      const float32x4x2_t g_n_h = vzipq_f32(e, f);
      
      vst1q_f32(&fft[2 * j + 0], g_n_h.val[0]);
      vst1q_f32(&fft[2 * j + 4], g_n_h.val[1]);
    }
    
    fft[1] = MulRe(aec->xfBuf[0][xPos + PART_LEN],
                   -aec->xfBuf[1][xPos + PART_LEN],
                   ef[0][PART_LEN],
                   ef[1][PART_LEN]);

    aec_rdft_inverse_128(fft);
    memset(fft + PART_LEN, 0, sizeof(float) * PART_LEN);

    
    {
      const float scale = 2.0f / PART_LEN2;
      const float32x4_t scale_ps = vmovq_n_f32(scale);
      for (j = 0; j < PART_LEN; j += 4) {
        const float32x4_t fft_ps = vld1q_f32(&fft[j]);
        const float32x4_t fft_scale = vmulq_f32(fft_ps, scale_ps);
        vst1q_f32(&fft[j], fft_scale);
      }
    }
    aec_rdft_forward_128(fft);

    {
      const float wt1 = aec->wfBuf[1][pos];
      aec->wfBuf[0][pos + PART_LEN] += fft[1];
      for (j = 0; j < PART_LEN; j += 4) {
        float32x4_t wtBuf_re = vld1q_f32(&aec->wfBuf[0][pos + j]);
        float32x4_t wtBuf_im = vld1q_f32(&aec->wfBuf[1][pos + j]);
        const float32x4_t fft0 = vld1q_f32(&fft[2 * j + 0]);
        const float32x4_t fft4 = vld1q_f32(&fft[2 * j + 4]);
        const float32x4x2_t fft_re_im = vuzpq_f32(fft0, fft4);
        wtBuf_re = vaddq_f32(wtBuf_re, fft_re_im.val[0]);
        wtBuf_im = vaddq_f32(wtBuf_im, fft_re_im.val[1]);

        vst1q_f32(&aec->wfBuf[0][pos + j], wtBuf_re);
        vst1q_f32(&aec->wfBuf[1][pos + j], wtBuf_im);
      }
      aec->wfBuf[1][pos] = wt1;
    }
  }
}

static float32x4_t vpowq_f32(float32x4_t a, float32x4_t b) {
  
  
  float32x4_t log2_a, b_log2_a, a_exp_b;

  
  {
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    const uint32x4_t vec_float_exponent_mask = vdupq_n_u32(0x7F800000);
    const uint32x4_t vec_eight_biased_exponent = vdupq_n_u32(0x43800000);
    const uint32x4_t vec_implicit_leading_one = vdupq_n_u32(0x43BF8000);
    const uint32x4_t two_n = vandq_u32(vreinterpretq_u32_f32(a),
                                       vec_float_exponent_mask);
    const uint32x4_t n_1 = vshrq_n_u32(two_n, kShiftExponentIntoTopMantissa);
    const uint32x4_t n_0 = vorrq_u32(n_1, vec_eight_biased_exponent);
    const float32x4_t n =
        vsubq_f32(vreinterpretq_f32_u32(n_0),
                  vreinterpretq_f32_u32(vec_implicit_leading_one));
    
    const uint32x4_t vec_mantissa_mask = vdupq_n_u32(0x007FFFFF);
    const uint32x4_t vec_zero_biased_exponent_is_one = vdupq_n_u32(0x3F800000);
    const uint32x4_t mantissa = vandq_u32(vreinterpretq_u32_f32(a),
                                          vec_mantissa_mask);
    const float32x4_t y =
        vreinterpretq_f32_u32(vorrq_u32(mantissa,
                                        vec_zero_biased_exponent_is_one));
    
    
    const float32x4_t C5 = vdupq_n_f32(-3.4436006e-2f);
    const float32x4_t C4 = vdupq_n_f32(3.1821337e-1f);
    const float32x4_t C3 = vdupq_n_f32(-1.2315303f);
    const float32x4_t C2 = vdupq_n_f32(2.5988452f);
    const float32x4_t C1 = vdupq_n_f32(-3.3241990f);
    const float32x4_t C0 = vdupq_n_f32(3.1157899f);
    float32x4_t pol5_y = C5;
    pol5_y = vmlaq_f32(C4, y, pol5_y);
    pol5_y = vmlaq_f32(C3, y, pol5_y);
    pol5_y = vmlaq_f32(C2, y, pol5_y);
    pol5_y = vmlaq_f32(C1, y, pol5_y);
    pol5_y = vmlaq_f32(C0, y, pol5_y);
    const float32x4_t y_minus_one =
        vsubq_f32(y, vreinterpretq_f32_u32(vec_zero_biased_exponent_is_one));
    const float32x4_t log2_y = vmulq_f32(y_minus_one, pol5_y);

    
    log2_a = vaddq_f32(n, log2_y);
  }

  
  b_log2_a = vmulq_f32(b, log2_a);

  
  {
    
    
    
    
    
    
    
    
    
    
    
    
    const float32x4_t max_input = vdupq_n_f32(129.f);
    const float32x4_t min_input = vdupq_n_f32(-126.99999f);
    const float32x4_t x_min = vminq_f32(b_log2_a, max_input);
    const float32x4_t x_max = vmaxq_f32(x_min, min_input);
    
    const float32x4_t half = vdupq_n_f32(0.5f);
    const float32x4_t x_minus_half = vsubq_f32(x_max, half);
    const int32x4_t x_minus_half_floor = vcvtq_s32_f32(x_minus_half);

    
    const int32x4_t float_exponent_bias = vdupq_n_s32(127);
    const int32x4_t two_n_exponent =
        vaddq_s32(x_minus_half_floor, float_exponent_bias);
    const float32x4_t two_n =
        vreinterpretq_f32_s32(vshlq_n_s32(two_n_exponent, kFloatExponentShift));
    
    const float32x4_t y = vsubq_f32(x_max, vcvtq_f32_s32(x_minus_half_floor));

    
    const float32x4_t C2 = vdupq_n_f32(3.3718944e-1f);
    const float32x4_t C1 = vdupq_n_f32(6.5763628e-1f);
    const float32x4_t C0 = vdupq_n_f32(1.0017247f);
    float32x4_t exp2_y = C2;
    exp2_y = vmlaq_f32(C1, y, exp2_y);
    exp2_y = vmlaq_f32(C0, y, exp2_y);

    
    a_exp_b = vmulq_f32(exp2_y, two_n);
  }

  return a_exp_b;
}

static void OverdriveAndSuppressNEON(AecCore* aec,
                                     float hNl[PART_LEN1],
                                     const float hNlFb,
                                     float efw[2][PART_LEN1]) {
  int i;
  const float32x4_t vec_hNlFb = vmovq_n_f32(hNlFb);
  const float32x4_t vec_one = vdupq_n_f32(1.0f);
  const float32x4_t vec_minus_one = vdupq_n_f32(-1.0f);
  const float32x4_t vec_overDriveSm = vmovq_n_f32(aec->overDriveSm);

  
  for (i = 0; i + 3 < PART_LEN1; i += 4) {
    
    float32x4_t vec_hNl = vld1q_f32(&hNl[i]);
    const float32x4_t vec_weightCurve = vld1q_f32(&WebRtcAec_weightCurve[i]);
    const uint32x4_t bigger = vcgtq_f32(vec_hNl, vec_hNlFb);
    const float32x4_t vec_weightCurve_hNlFb = vmulq_f32(vec_weightCurve,
                                                        vec_hNlFb);
    const float32x4_t vec_one_weightCurve = vsubq_f32(vec_one, vec_weightCurve);
    const float32x4_t vec_one_weightCurve_hNl = vmulq_f32(vec_one_weightCurve,
                                                          vec_hNl);
    const uint32x4_t vec_if0 = vandq_u32(vmvnq_u32(bigger),
                                         vreinterpretq_u32_f32(vec_hNl));
    const float32x4_t vec_one_weightCurve_add =
        vaddq_f32(vec_weightCurve_hNlFb, vec_one_weightCurve_hNl);
    const uint32x4_t vec_if1 =
        vandq_u32(bigger, vreinterpretq_u32_f32(vec_one_weightCurve_add));

    vec_hNl = vreinterpretq_f32_u32(vorrq_u32(vec_if0, vec_if1));

    {
      const float32x4_t vec_overDriveCurve =
          vld1q_f32(&WebRtcAec_overDriveCurve[i]);
      const float32x4_t vec_overDriveSm_overDriveCurve =
          vmulq_f32(vec_overDriveSm, vec_overDriveCurve);
      vec_hNl = vpowq_f32(vec_hNl, vec_overDriveSm_overDriveCurve);
      vst1q_f32(&hNl[i], vec_hNl);
    }

    
    {
      float32x4_t vec_efw_re = vld1q_f32(&efw[0][i]);
      float32x4_t vec_efw_im = vld1q_f32(&efw[1][i]);
      vec_efw_re = vmulq_f32(vec_efw_re, vec_hNl);
      vec_efw_im = vmulq_f32(vec_efw_im, vec_hNl);

      
      
      vec_efw_im = vmulq_f32(vec_efw_im, vec_minus_one);
      vst1q_f32(&efw[0][i], vec_efw_re);
      vst1q_f32(&efw[1][i], vec_efw_im);
    }
  }

  
  for (; i < PART_LEN1; i++) {
    
    if (hNl[i] > hNlFb) {
      hNl[i] = WebRtcAec_weightCurve[i] * hNlFb +
               (1 - WebRtcAec_weightCurve[i]) * hNl[i];
    }

    hNl[i] = powf(hNl[i], aec->overDriveSm * WebRtcAec_overDriveCurve[i]);

    
    efw[0][i] *= hNl[i];
    efw[1][i] *= hNl[i];

    
    
    efw[1][i] *= -1;
  }
}

static int PartitionDelay(const AecCore* aec) {
  
  
  
  
  float wfEnMax = 0;
  int i;
  int delay = 0;

  for (i = 0; i < aec->num_partitions; i++) {
    int j;
    int pos = i * PART_LEN1;
    float wfEn = 0;
    float32x4_t vec_wfEn = vdupq_n_f32(0.0f);
    
    for (j = 0; j + 3 < PART_LEN1; j += 4) {
      const float32x4_t vec_wfBuf0 = vld1q_f32(&aec->wfBuf[0][pos + j]);
      const float32x4_t vec_wfBuf1 = vld1q_f32(&aec->wfBuf[1][pos + j]);
      vec_wfEn = vmlaq_f32(vec_wfEn, vec_wfBuf0, vec_wfBuf0);
      vec_wfEn = vmlaq_f32(vec_wfEn, vec_wfBuf1, vec_wfBuf1);
    }
    {
      float32x2_t vec_total;
      
      vec_total = vpadd_f32(vget_low_f32(vec_wfEn), vget_high_f32(vec_wfEn));
      
      vec_total = vpadd_f32(vec_total, vec_total);
      
      wfEn = vget_lane_f32(vec_total, 0);
    }

    
    for (; j < PART_LEN1; j++) {
      wfEn += aec->wfBuf[0][pos + j] * aec->wfBuf[0][pos + j] +
              aec->wfBuf[1][pos + j] * aec->wfBuf[1][pos + j];
    }

    if (wfEn > wfEnMax) {
      wfEnMax = wfEn;
      delay = i;
    }
  }
  return delay;
}










static void SmoothedPSD(AecCore* aec,
                        float efw[2][PART_LEN1],
                        float dfw[2][PART_LEN1],
                        float xfw[2][PART_LEN1]) {
  
  const float* ptrGCoh = aec->extended_filter_enabled
      ? WebRtcAec_kExtendedSmoothingCoefficients[aec->mult - 1]
      : WebRtcAec_kNormalSmoothingCoefficients[aec->mult - 1];
  int i;
  float sdSum = 0, seSum = 0;
  const float32x4_t vec_15 =  vdupq_n_f32(WebRtcAec_kMinFarendPSD);
  float32x4_t vec_sdSum = vdupq_n_f32(0.0f);
  float32x4_t vec_seSum = vdupq_n_f32(0.0f);

  for (i = 0; i + 3 < PART_LEN1; i += 4) {
    const float32x4_t vec_dfw0 = vld1q_f32(&dfw[0][i]);
    const float32x4_t vec_dfw1 = vld1q_f32(&dfw[1][i]);
    const float32x4_t vec_efw0 = vld1q_f32(&efw[0][i]);
    const float32x4_t vec_efw1 = vld1q_f32(&efw[1][i]);
    const float32x4_t vec_xfw0 = vld1q_f32(&xfw[0][i]);
    const float32x4_t vec_xfw1 = vld1q_f32(&xfw[1][i]);
    float32x4_t vec_sd = vmulq_n_f32(vld1q_f32(&aec->sd[i]), ptrGCoh[0]);
    float32x4_t vec_se = vmulq_n_f32(vld1q_f32(&aec->se[i]), ptrGCoh[0]);
    float32x4_t vec_sx = vmulq_n_f32(vld1q_f32(&aec->sx[i]), ptrGCoh[0]);
    float32x4_t vec_dfw_sumsq = vmulq_f32(vec_dfw0, vec_dfw0);
    float32x4_t vec_efw_sumsq = vmulq_f32(vec_efw0, vec_efw0);
    float32x4_t vec_xfw_sumsq = vmulq_f32(vec_xfw0, vec_xfw0);

    vec_dfw_sumsq = vmlaq_f32(vec_dfw_sumsq, vec_dfw1, vec_dfw1);
    vec_efw_sumsq = vmlaq_f32(vec_efw_sumsq, vec_efw1, vec_efw1);
    vec_xfw_sumsq = vmlaq_f32(vec_xfw_sumsq, vec_xfw1, vec_xfw1);
    vec_xfw_sumsq = vmaxq_f32(vec_xfw_sumsq, vec_15);
    vec_sd = vmlaq_n_f32(vec_sd, vec_dfw_sumsq, ptrGCoh[1]);
    vec_se = vmlaq_n_f32(vec_se, vec_efw_sumsq, ptrGCoh[1]);
    vec_sx = vmlaq_n_f32(vec_sx, vec_xfw_sumsq, ptrGCoh[1]);

    vst1q_f32(&aec->sd[i], vec_sd);
    vst1q_f32(&aec->se[i], vec_se);
    vst1q_f32(&aec->sx[i], vec_sx);

    {
      float32x4x2_t vec_sde = vld2q_f32(&aec->sde[i][0]);
      float32x4_t vec_dfwefw0011 = vmulq_f32(vec_dfw0, vec_efw0);
      float32x4_t vec_dfwefw0110 = vmulq_f32(vec_dfw0, vec_efw1);
      vec_sde.val[0] = vmulq_n_f32(vec_sde.val[0], ptrGCoh[0]);
      vec_sde.val[1] = vmulq_n_f32(vec_sde.val[1], ptrGCoh[0]);
      vec_dfwefw0011 = vmlaq_f32(vec_dfwefw0011, vec_dfw1, vec_efw1);
      vec_dfwefw0110 = vmlsq_f32(vec_dfwefw0110, vec_dfw1, vec_efw0);
      vec_sde.val[0] = vmlaq_n_f32(vec_sde.val[0], vec_dfwefw0011, ptrGCoh[1]);
      vec_sde.val[1] = vmlaq_n_f32(vec_sde.val[1], vec_dfwefw0110, ptrGCoh[1]);
      vst2q_f32(&aec->sde[i][0], vec_sde);
    }

    {
      float32x4x2_t vec_sxd = vld2q_f32(&aec->sxd[i][0]);
      float32x4_t vec_dfwxfw0011 = vmulq_f32(vec_dfw0, vec_xfw0);
      float32x4_t vec_dfwxfw0110 = vmulq_f32(vec_dfw0, vec_xfw1);
      vec_sxd.val[0] = vmulq_n_f32(vec_sxd.val[0], ptrGCoh[0]);
      vec_sxd.val[1] = vmulq_n_f32(vec_sxd.val[1], ptrGCoh[0]);
      vec_dfwxfw0011 = vmlaq_f32(vec_dfwxfw0011, vec_dfw1, vec_xfw1);
      vec_dfwxfw0110 = vmlsq_f32(vec_dfwxfw0110, vec_dfw1, vec_xfw0);
      vec_sxd.val[0] = vmlaq_n_f32(vec_sxd.val[0], vec_dfwxfw0011, ptrGCoh[1]);
      vec_sxd.val[1] = vmlaq_n_f32(vec_sxd.val[1], vec_dfwxfw0110, ptrGCoh[1]);
      vst2q_f32(&aec->sxd[i][0], vec_sxd);
    }

    vec_sdSum = vaddq_f32(vec_sdSum, vec_sd);
    vec_seSum = vaddq_f32(vec_seSum, vec_se);
  }
  {
    float32x2_t vec_sdSum_total;
    float32x2_t vec_seSum_total;
    
    vec_sdSum_total = vpadd_f32(vget_low_f32(vec_sdSum),
                                vget_high_f32(vec_sdSum));
    vec_seSum_total = vpadd_f32(vget_low_f32(vec_seSum),
                                vget_high_f32(vec_seSum));
    
    vec_sdSum_total = vpadd_f32(vec_sdSum_total, vec_sdSum_total);
    vec_seSum_total = vpadd_f32(vec_seSum_total, vec_seSum_total);
    
    sdSum = vget_lane_f32(vec_sdSum_total, 0);
    seSum = vget_lane_f32(vec_seSum_total, 0);
  }

  
  for (; i < PART_LEN1; i++) {
    aec->sd[i] = ptrGCoh[0] * aec->sd[i] +
                 ptrGCoh[1] * (dfw[0][i] * dfw[0][i] + dfw[1][i] * dfw[1][i]);
    aec->se[i] = ptrGCoh[0] * aec->se[i] +
                 ptrGCoh[1] * (efw[0][i] * efw[0][i] + efw[1][i] * efw[1][i]);
    
    
    
    
    aec->sx[i] =
        ptrGCoh[0] * aec->sx[i] +
        ptrGCoh[1] * WEBRTC_SPL_MAX(
            xfw[0][i] * xfw[0][i] + xfw[1][i] * xfw[1][i],
            WebRtcAec_kMinFarendPSD);

    aec->sde[i][0] =
        ptrGCoh[0] * aec->sde[i][0] +
        ptrGCoh[1] * (dfw[0][i] * efw[0][i] + dfw[1][i] * efw[1][i]);
    aec->sde[i][1] =
        ptrGCoh[0] * aec->sde[i][1] +
        ptrGCoh[1] * (dfw[0][i] * efw[1][i] - dfw[1][i] * efw[0][i]);

    aec->sxd[i][0] =
        ptrGCoh[0] * aec->sxd[i][0] +
        ptrGCoh[1] * (dfw[0][i] * xfw[0][i] + dfw[1][i] * xfw[1][i]);
    aec->sxd[i][1] =
        ptrGCoh[0] * aec->sxd[i][1] +
        ptrGCoh[1] * (dfw[0][i] * xfw[1][i] - dfw[1][i] * xfw[0][i]);

    sdSum += aec->sd[i];
    seSum += aec->se[i];
  }

  
  aec->divergeState = (aec->divergeState ? 1.05f : 1.0f) * seSum > sdSum;

  if (aec->divergeState)
    memcpy(efw, dfw, sizeof(efw[0][0]) * 2 * PART_LEN1);

  
  if (!aec->extended_filter_enabled && seSum > (19.95f * sdSum))
    memset(aec->wfBuf, 0, sizeof(aec->wfBuf));
}


__inline static void WindowData(float* x_windowed, const float* x) {
  int i;
  for (i = 0; i < PART_LEN; i += 4) {
    const float32x4_t vec_Buf1 = vld1q_f32(&x[i]);
    const float32x4_t vec_Buf2 = vld1q_f32(&x[PART_LEN + i]);
    const float32x4_t vec_sqrtHanning = vld1q_f32(&WebRtcAec_sqrtHanning[i]);
    
    float32x4_t vec_sqrtHanning_rev =
        vld1q_f32(&WebRtcAec_sqrtHanning[PART_LEN - i - 3]);
    
    vec_sqrtHanning_rev = vrev64q_f32(vec_sqrtHanning_rev);
    
    vec_sqrtHanning_rev = vcombine_f32(vget_high_f32(vec_sqrtHanning_rev),
                                       vget_low_f32(vec_sqrtHanning_rev));
    vst1q_f32(&x_windowed[i], vmulq_f32(vec_Buf1, vec_sqrtHanning));
    vst1q_f32(&x_windowed[PART_LEN + i],
            vmulq_f32(vec_Buf2, vec_sqrtHanning_rev));
  }
}


__inline static void StoreAsComplex(const float* data,
                                    float data_complex[2][PART_LEN1]) {
  int i;
  for (i = 0; i < PART_LEN; i += 4) {
    const float32x4x2_t vec_data = vld2q_f32(&data[2 * i]);
    vst1q_f32(&data_complex[0][i], vec_data.val[0]);
    vst1q_f32(&data_complex[1][i], vec_data.val[1]);
  }
  
  data_complex[1][0] = 0;
  data_complex[1][PART_LEN] = 0;
  data_complex[0][0] = data[0];
  data_complex[0][PART_LEN] = data[1];
}

static void SubbandCoherenceNEON(AecCore* aec,
                                 float efw[2][PART_LEN1],
                                 float xfw[2][PART_LEN1],
                                 float* fft,
                                 float* cohde,
                                 float* cohxd) {
  float dfw[2][PART_LEN1];
  int i;

  if (aec->delayEstCtr == 0)
    aec->delayIdx = PartitionDelay(aec);

  
  memcpy(xfw,
         aec->xfwBuf + aec->delayIdx * PART_LEN1,
         sizeof(xfw[0][0]) * 2 * PART_LEN1);

  
  WindowData(fft, aec->dBuf);
  aec_rdft_forward_128(fft);
  StoreAsComplex(fft, dfw);

  
  WindowData(fft, aec->eBuf);
  aec_rdft_forward_128(fft);
  StoreAsComplex(fft, efw);

  SmoothedPSD(aec, efw, dfw, xfw);

  {
    const float32x4_t vec_1eminus10 =  vdupq_n_f32(1e-10f);

    
    for (i = 0; i + 3 < PART_LEN1; i += 4) {
      const float32x4_t vec_sd = vld1q_f32(&aec->sd[i]);
      const float32x4_t vec_se = vld1q_f32(&aec->se[i]);
      const float32x4_t vec_sx = vld1q_f32(&aec->sx[i]);
      const float32x4_t vec_sdse = vmlaq_f32(vec_1eminus10, vec_sd, vec_se);
      const float32x4_t vec_sdsx = vmlaq_f32(vec_1eminus10, vec_sd, vec_sx);
      float32x4x2_t vec_sde = vld2q_f32(&aec->sde[i][0]);
      float32x4x2_t vec_sxd = vld2q_f32(&aec->sxd[i][0]);
      float32x4_t vec_cohde = vmulq_f32(vec_sde.val[0], vec_sde.val[0]);
      float32x4_t vec_cohxd = vmulq_f32(vec_sxd.val[0], vec_sxd.val[0]);
      vec_cohde = vmlaq_f32(vec_cohde, vec_sde.val[1], vec_sde.val[1]);
      vec_cohde = vdivq_f32(vec_cohde, vec_sdse);
      vec_cohxd = vmlaq_f32(vec_cohxd, vec_sxd.val[1], vec_sxd.val[1]);
      vec_cohxd = vdivq_f32(vec_cohxd, vec_sdsx);

      vst1q_f32(&cohde[i], vec_cohde);
      vst1q_f32(&cohxd[i], vec_cohxd);
    }
  }
  
  for (; i < PART_LEN1; i++) {
    cohde[i] =
        (aec->sde[i][0] * aec->sde[i][0] + aec->sde[i][1] * aec->sde[i][1]) /
        (aec->sd[i] * aec->se[i] + 1e-10f);
    cohxd[i] =
        (aec->sxd[i][0] * aec->sxd[i][0] + aec->sxd[i][1] * aec->sxd[i][1]) /
        (aec->sx[i] * aec->sd[i] + 1e-10f);
  }
}

void WebRtcAec_InitAec_neon(void) {
  WebRtcAec_FilterFar = FilterFarNEON;
  WebRtcAec_ScaleErrorSignal = ScaleErrorSignalNEON;
  WebRtcAec_FilterAdaptation = FilterAdaptationNEON;
  WebRtcAec_OverdriveAndSuppress = OverdriveAndSuppressNEON;
  WebRtcAec_SubbandCoherence = SubbandCoherenceNEON;
}

