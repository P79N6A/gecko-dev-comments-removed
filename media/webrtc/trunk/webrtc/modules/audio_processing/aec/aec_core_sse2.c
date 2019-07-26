













#include "aec_core.h"

#include <emmintrin.h>
#include <math.h>
#include <string.h>  

#include "aec_rdft.h"

__inline static float MulRe(float aRe, float aIm, float bRe, float bIm)
{
  return aRe * bRe - aIm * bIm;
}

__inline static float MulIm(float aRe, float aIm, float bRe, float bIm)
{
  return aRe * bIm + aIm * bRe;
}

static void FilterFarSSE2(aec_t *aec, float yf[2][PART_LEN1])
{
  int i;
  for (i = 0; i < NR_PART; i++) {
    int j;
    int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
    int pos = i * PART_LEN1;
    
    if (i + aec->xfBufBlockPos >= NR_PART) {
      xPos -= NR_PART*(PART_LEN1);
    }

    
    for (j = 0; j + 3 < PART_LEN1; j += 4) {
      const __m128 xfBuf_re = _mm_loadu_ps(&aec->xfBuf[0][xPos + j]);
      const __m128 xfBuf_im = _mm_loadu_ps(&aec->xfBuf[1][xPos + j]);
      const __m128 wfBuf_re = _mm_loadu_ps(&aec->wfBuf[0][pos + j]);
      const __m128 wfBuf_im = _mm_loadu_ps(&aec->wfBuf[1][pos + j]);
      const __m128 yf_re = _mm_loadu_ps(&yf[0][j]);
      const __m128 yf_im = _mm_loadu_ps(&yf[1][j]);
      const __m128 a = _mm_mul_ps(xfBuf_re, wfBuf_re);
      const __m128 b = _mm_mul_ps(xfBuf_im, wfBuf_im);
      const __m128 c = _mm_mul_ps(xfBuf_re, wfBuf_im);
      const __m128 d = _mm_mul_ps(xfBuf_im, wfBuf_re);
      const __m128 e = _mm_sub_ps(a, b);
      const __m128 f = _mm_add_ps(c, d);
      const __m128 g = _mm_add_ps(yf_re, e);
      const __m128 h = _mm_add_ps(yf_im, f);
      _mm_storeu_ps(&yf[0][j], g);
      _mm_storeu_ps(&yf[1][j], h);
    }
    
    for (; j < PART_LEN1; j++) {
      yf[0][j] += MulRe(aec->xfBuf[0][xPos + j], aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][ pos + j], aec->wfBuf[1][ pos + j]);
      yf[1][j] += MulIm(aec->xfBuf[0][xPos + j], aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][ pos + j], aec->wfBuf[1][ pos + j]);
    }
  }
}

static void ScaleErrorSignalSSE2(aec_t *aec, float ef[2][PART_LEN1])
{
  const __m128 k1e_10f = _mm_set1_ps(1e-10f);
  const __m128 kThresh = _mm_set1_ps(aec->errThresh);
  const __m128 kMu = _mm_set1_ps(aec->mu);

  int i;
  
  for (i = 0; i + 3 < PART_LEN1; i += 4) {
    const __m128 xPow = _mm_loadu_ps(&aec->xPow[i]);
    const __m128 ef_re_base = _mm_loadu_ps(&ef[0][i]);
    const __m128 ef_im_base = _mm_loadu_ps(&ef[1][i]);

    const __m128 xPowPlus = _mm_add_ps(xPow, k1e_10f);
    __m128 ef_re = _mm_div_ps(ef_re_base, xPowPlus);
    __m128 ef_im = _mm_div_ps(ef_im_base, xPowPlus);
    const __m128 ef_re2 = _mm_mul_ps(ef_re, ef_re);
    const __m128 ef_im2 = _mm_mul_ps(ef_im, ef_im);
    const __m128 ef_sum2 = _mm_add_ps(ef_re2, ef_im2);
    const __m128 absEf = _mm_sqrt_ps(ef_sum2);
    const __m128 bigger = _mm_cmpgt_ps(absEf, kThresh);
    __m128 absEfPlus = _mm_add_ps(absEf, k1e_10f);
    const __m128 absEfInv = _mm_div_ps(kThresh, absEfPlus);
    __m128 ef_re_if = _mm_mul_ps(ef_re, absEfInv);
    __m128 ef_im_if = _mm_mul_ps(ef_im, absEfInv);
    ef_re_if = _mm_and_ps(bigger, ef_re_if);
    ef_im_if = _mm_and_ps(bigger, ef_im_if);
    ef_re = _mm_andnot_ps(bigger, ef_re);
    ef_im = _mm_andnot_ps(bigger, ef_im);
    ef_re = _mm_or_ps(ef_re, ef_re_if);
    ef_im = _mm_or_ps(ef_im, ef_im_if);
    ef_re = _mm_mul_ps(ef_re, kMu);
    ef_im = _mm_mul_ps(ef_im, kMu);

    _mm_storeu_ps(&ef[0][i], ef_re);
    _mm_storeu_ps(&ef[1][i], ef_im);
  }
  
  for (; i < (PART_LEN1); i++) {
    float absEf;
    ef[0][i] /= (aec->xPow[i] + 1e-10f);
    ef[1][i] /= (aec->xPow[i] + 1e-10f);
    absEf = sqrtf(ef[0][i] * ef[0][i] + ef[1][i] * ef[1][i]);

    if (absEf > aec->errThresh) {
      absEf = aec->errThresh / (absEf + 1e-10f);
      ef[0][i] *= absEf;
      ef[1][i] *= absEf;
    }

    
    ef[0][i] *= aec->mu;
    ef[1][i] *= aec->mu;
  }
}

static void FilterAdaptationSSE2(aec_t *aec, float *fft, float ef[2][PART_LEN1]) {
  int i, j;
  for (i = 0; i < NR_PART; i++) {
    int xPos = (i + aec->xfBufBlockPos)*(PART_LEN1);
    int pos = i * PART_LEN1;
    
    if (i + aec->xfBufBlockPos >= NR_PART) {
      xPos -= NR_PART * PART_LEN1;
    }

    
    for (j = 0; j < PART_LEN; j+= 4) {
      
      const __m128 xfBuf_re = _mm_loadu_ps(&aec->xfBuf[0][xPos + j]);
      const __m128 xfBuf_im = _mm_loadu_ps(&aec->xfBuf[1][xPos + j]);
      const __m128 ef_re = _mm_loadu_ps(&ef[0][j]);
      const __m128 ef_im = _mm_loadu_ps(&ef[1][j]);
      
      
      
      const __m128 a = _mm_mul_ps(xfBuf_re, ef_re);
      const __m128 b = _mm_mul_ps(xfBuf_im, ef_im);
      const __m128 c = _mm_mul_ps(xfBuf_re, ef_im);
      const __m128 d = _mm_mul_ps(xfBuf_im, ef_re);
      const __m128 e = _mm_add_ps(a, b);
      const __m128 f = _mm_sub_ps(c, d);
      
      const __m128 g = _mm_unpacklo_ps(e, f);
      const __m128 h = _mm_unpackhi_ps(e, f);
      
      _mm_storeu_ps(&fft[2*j + 0], g);
      _mm_storeu_ps(&fft[2*j + 4], h);
    }
    
    fft[1] = MulRe(aec->xfBuf[0][xPos + PART_LEN],
                   -aec->xfBuf[1][xPos + PART_LEN],
                   ef[0][PART_LEN], ef[1][PART_LEN]);

    aec_rdft_inverse_128(fft);
    memset(fft + PART_LEN, 0, sizeof(float)*PART_LEN);

    
    {
      float scale = 2.0f / PART_LEN2;
      const __m128 scale_ps = _mm_load_ps1(&scale);
      for (j = 0; j < PART_LEN; j+=4) {
        const __m128 fft_ps = _mm_loadu_ps(&fft[j]);
        const __m128 fft_scale = _mm_mul_ps(fft_ps, scale_ps);
        _mm_storeu_ps(&fft[j], fft_scale);
      }
    }
    aec_rdft_forward_128(fft);

    {
      float wt1 = aec->wfBuf[1][pos];
      aec->wfBuf[0][pos + PART_LEN] += fft[1];
      for (j = 0; j < PART_LEN; j+= 4) {
        __m128 wtBuf_re = _mm_loadu_ps(&aec->wfBuf[0][pos + j]);
        __m128 wtBuf_im = _mm_loadu_ps(&aec->wfBuf[1][pos + j]);
        const __m128 fft0 = _mm_loadu_ps(&fft[2 * j + 0]);
        const __m128 fft4 = _mm_loadu_ps(&fft[2 * j + 4]);
        const __m128 fft_re = _mm_shuffle_ps(fft0, fft4, _MM_SHUFFLE(2, 0, 2 ,0));
        const __m128 fft_im = _mm_shuffle_ps(fft0, fft4, _MM_SHUFFLE(3, 1, 3 ,1));
        wtBuf_re = _mm_add_ps(wtBuf_re, fft_re);
        wtBuf_im = _mm_add_ps(wtBuf_im, fft_im);
        _mm_storeu_ps(&aec->wfBuf[0][pos + j], wtBuf_re);
        _mm_storeu_ps(&aec->wfBuf[1][pos + j], wtBuf_im);
      }
      aec->wfBuf[1][pos] = wt1;
    }
  }
}

static __m128 mm_pow_ps(__m128 a, __m128 b)
{
  
  
  __m128 log2_a, b_log2_a, a_exp_b;

  
  {
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    static const ALIGN16_BEG int float_exponent_mask[4] ALIGN16_END =
        {0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000};
    static const ALIGN16_BEG int eight_biased_exponent[4] ALIGN16_END =
        {0x43800000, 0x43800000, 0x43800000, 0x43800000};
    static const ALIGN16_BEG int implicit_leading_one[4] ALIGN16_END =
        {0x43BF8000, 0x43BF8000, 0x43BF8000, 0x43BF8000};
    static const int shift_exponent_into_top_mantissa = 8;
    const __m128 two_n = _mm_and_ps(a, *((__m128 *)float_exponent_mask));
    const __m128 n_1 = _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(two_n),
        shift_exponent_into_top_mantissa));
    const __m128 n_0 = _mm_or_ps(n_1, *((__m128 *)eight_biased_exponent));
    const __m128 n   = _mm_sub_ps(n_0,  *((__m128 *)implicit_leading_one));

    
    static const ALIGN16_BEG int mantissa_mask[4] ALIGN16_END =
        {0x007FFFFF, 0x007FFFFF, 0x007FFFFF, 0x007FFFFF};
    static const ALIGN16_BEG int zero_biased_exponent_is_one[4] ALIGN16_END =
        {0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000};
    const __m128 mantissa = _mm_and_ps(a, *((__m128 *)mantissa_mask));
    const __m128 y        = _mm_or_ps(
        mantissa,  *((__m128 *)zero_biased_exponent_is_one));

    
    
    static const ALIGN16_BEG float ALIGN16_END C5[4] =
        {-3.4436006e-2f, -3.4436006e-2f, -3.4436006e-2f, -3.4436006e-2f};
    static const ALIGN16_BEG float ALIGN16_END C4[4] =
        {3.1821337e-1f, 3.1821337e-1f, 3.1821337e-1f, 3.1821337e-1f};
    static const ALIGN16_BEG float ALIGN16_END C3[4] =
        {-1.2315303f, -1.2315303f, -1.2315303f, -1.2315303f};
    static const ALIGN16_BEG float ALIGN16_END C2[4] =
        {2.5988452f, 2.5988452f, 2.5988452f, 2.5988452f};
    static const ALIGN16_BEG float ALIGN16_END C1[4] =
        {-3.3241990f, -3.3241990f, -3.3241990f, -3.3241990f};
    static const ALIGN16_BEG float ALIGN16_END C0[4] =
        {3.1157899f, 3.1157899f, 3.1157899f, 3.1157899f};
    const __m128 pol5_y_0 = _mm_mul_ps(y,        *((__m128 *)C5));
    const __m128 pol5_y_1 = _mm_add_ps(pol5_y_0, *((__m128 *)C4));
    const __m128 pol5_y_2 = _mm_mul_ps(pol5_y_1, y);
    const __m128 pol5_y_3 = _mm_add_ps(pol5_y_2, *((__m128 *)C3));
    const __m128 pol5_y_4 = _mm_mul_ps(pol5_y_3, y);
    const __m128 pol5_y_5 = _mm_add_ps(pol5_y_4, *((__m128 *)C2));
    const __m128 pol5_y_6 = _mm_mul_ps(pol5_y_5, y);
    const __m128 pol5_y_7 = _mm_add_ps(pol5_y_6, *((__m128 *)C1));
    const __m128 pol5_y_8 = _mm_mul_ps(pol5_y_7, y);
    const __m128 pol5_y   = _mm_add_ps(pol5_y_8, *((__m128 *)C0));
    const __m128 y_minus_one = _mm_sub_ps(
        y, *((__m128 *)zero_biased_exponent_is_one));
    const __m128 log2_y = _mm_mul_ps(y_minus_one ,  pol5_y);

    
    log2_a = _mm_add_ps(n, log2_y);
  }

  
  b_log2_a = _mm_mul_ps(b, log2_a);

  
  {
    
    
    
    
    
    
    
    
    
    
    

    
    static const ALIGN16_BEG float max_input[4] ALIGN16_END =
        {129.f, 129.f, 129.f, 129.f};
    static const ALIGN16_BEG float min_input[4] ALIGN16_END =
        {-126.99999f, -126.99999f, -126.99999f, -126.99999f};
    const __m128 x_min = _mm_min_ps(b_log2_a, *((__m128 *)max_input));
    const __m128 x_max = _mm_max_ps(x_min,    *((__m128 *)min_input));
    
    static const ALIGN16_BEG float half[4] ALIGN16_END =
        {0.5f, 0.5f, 0.5f, 0.5f};
    const __m128  x_minus_half = _mm_sub_ps(x_max, *((__m128 *)half));
    const __m128i x_minus_half_floor = _mm_cvtps_epi32(x_minus_half);
    
    static const ALIGN16_BEG int float_exponent_bias[4] ALIGN16_END =
        {127, 127, 127, 127};
    static const int float_exponent_shift = 23;
    const __m128i two_n_exponent = _mm_add_epi32(
        x_minus_half_floor, *((__m128i *)float_exponent_bias));
    const __m128  two_n = _mm_castsi128_ps(_mm_slli_epi32(
        two_n_exponent, float_exponent_shift));
    
    const __m128 y = _mm_sub_ps(x_max, _mm_cvtepi32_ps(x_minus_half_floor));
    
    static const ALIGN16_BEG float C2[4] ALIGN16_END =
        {3.3718944e-1f, 3.3718944e-1f, 3.3718944e-1f, 3.3718944e-1f};
    static const ALIGN16_BEG float C1[4] ALIGN16_END =
        {6.5763628e-1f, 6.5763628e-1f, 6.5763628e-1f, 6.5763628e-1f};
    static const ALIGN16_BEG float C0[4] ALIGN16_END =
        {1.0017247f, 1.0017247f, 1.0017247f, 1.0017247f};
    const __m128 exp2_y_0 = _mm_mul_ps(y,        *((__m128 *)C2));
    const __m128 exp2_y_1 = _mm_add_ps(exp2_y_0, *((__m128 *)C1));
    const __m128 exp2_y_2 = _mm_mul_ps(exp2_y_1, y);
    const __m128 exp2_y   = _mm_add_ps(exp2_y_2, *((__m128 *)C0));

    
    a_exp_b = _mm_mul_ps(exp2_y, two_n);
  }
  return a_exp_b;
}

extern const float WebRtcAec_weightCurve[65];
extern const float WebRtcAec_overDriveCurve[65];

static void OverdriveAndSuppressSSE2(aec_t *aec, float hNl[PART_LEN1],
                                     const float hNlFb,
                                     float efw[2][PART_LEN1]) {
  int i;
  const __m128 vec_hNlFb = _mm_set1_ps(hNlFb);
  const __m128 vec_one = _mm_set1_ps(1.0f);
  const __m128 vec_minus_one = _mm_set1_ps(-1.0f);
  const __m128 vec_overDriveSm = _mm_set1_ps(aec->overDriveSm);
  
  for (i = 0; i + 3 < PART_LEN1; i+=4) {
    
    __m128 vec_hNl = _mm_loadu_ps(&hNl[i]);
    const __m128 vec_weightCurve = _mm_loadu_ps(&WebRtcAec_weightCurve[i]);
    const __m128 bigger = _mm_cmpgt_ps(vec_hNl, vec_hNlFb);
    const __m128 vec_weightCurve_hNlFb = _mm_mul_ps(
        vec_weightCurve, vec_hNlFb);
    const __m128 vec_one_weightCurve = _mm_sub_ps(vec_one, vec_weightCurve);
    const __m128 vec_one_weightCurve_hNl = _mm_mul_ps(
        vec_one_weightCurve, vec_hNl);
    const __m128 vec_if0 = _mm_andnot_ps(bigger, vec_hNl);
    const __m128 vec_if1 = _mm_and_ps(
        bigger, _mm_add_ps(vec_weightCurve_hNlFb, vec_one_weightCurve_hNl));
    vec_hNl = _mm_or_ps(vec_if0, vec_if1);

    {
      const __m128 vec_overDriveCurve = _mm_loadu_ps(
          &WebRtcAec_overDriveCurve[i]);
      const __m128 vec_overDriveSm_overDriveCurve = _mm_mul_ps(
          vec_overDriveSm, vec_overDriveCurve);
      vec_hNl = mm_pow_ps(vec_hNl, vec_overDriveSm_overDriveCurve);
      _mm_storeu_ps(&hNl[i], vec_hNl);
    }

    
    {
      __m128 vec_efw_re = _mm_loadu_ps(&efw[0][i]);
      __m128 vec_efw_im = _mm_loadu_ps(&efw[1][i]);
      vec_efw_re = _mm_mul_ps(vec_efw_re, vec_hNl);
      vec_efw_im = _mm_mul_ps(vec_efw_im, vec_hNl);

      
      
      vec_efw_im = _mm_mul_ps(vec_efw_im, vec_minus_one);
      _mm_storeu_ps(&efw[0][i], vec_efw_re);
      _mm_storeu_ps(&efw[1][i], vec_efw_im);
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

void WebRtcAec_InitAec_SSE2(void) {
  WebRtcAec_FilterFar = FilterFarSSE2;
  WebRtcAec_ScaleErrorSignal = ScaleErrorSignalSSE2;
  WebRtcAec_FilterAdaptation = FilterAdaptationSSE2;
  WebRtcAec_OverdriveAndSuppress = OverdriveAndSuppressSSE2;
}

