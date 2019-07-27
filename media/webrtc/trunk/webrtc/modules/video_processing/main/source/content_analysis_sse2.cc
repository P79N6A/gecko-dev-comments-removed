









#include "webrtc/modules/video_processing/main/source/content_analysis.h"

#include <emmintrin.h>
#include <math.h>

namespace webrtc {

int32_t VPMContentAnalysis::TemporalDiffMetric_SSE2() {
  uint32_t num_pixels = 0;       
  const uint8_t* imgBufO = orig_frame_ + border_*width_ + border_;
  const uint8_t* imgBufP = prev_frame_.get() + border_*width_ + border_;

  const int32_t width_end = ((width_ - 2*border_) & -16) + border_;

  __m128i sad_64   = _mm_setzero_si128();
  __m128i sum_64   = _mm_setzero_si128();
  __m128i sqsum_64 = _mm_setzero_si128();
  const __m128i z  = _mm_setzero_si128();

  for (uint16_t i = 0; i < (height_ - 2*border_); i += skip_num_) {
    __m128i sqsum_32  = _mm_setzero_si128();

    const uint8_t *lineO = imgBufO;
    const uint8_t *lineP = imgBufP;

    
    
    
    
    
    
    
    
    for (uint16_t j = 0; j < width_end - border_; j += 16) {
      const __m128i o = _mm_loadu_si128((__m128i*)(lineO));
      const __m128i p = _mm_loadu_si128((__m128i*)(lineP));

      lineO += 16;
      lineP += 16;

      
      sad_64 = _mm_add_epi64 (sad_64, _mm_sad_epu8(o, p));

      
      sum_64 = _mm_add_epi64 (sum_64, _mm_sad_epu8(o, z));

      
      const __m128i olo = _mm_unpacklo_epi8(o,z);
      const __m128i ohi = _mm_unpackhi_epi8(o,z);

      const __m128i sqsum_32_lo = _mm_madd_epi16(olo, olo);
      const __m128i sqsum_32_hi = _mm_madd_epi16(ohi, ohi);

      sqsum_32 = _mm_add_epi32(sqsum_32, sqsum_32_lo);
      sqsum_32 = _mm_add_epi32(sqsum_32, sqsum_32_hi);
    }

    
    sqsum_64 = _mm_add_epi64(sqsum_64,
        _mm_add_epi64(_mm_unpackhi_epi32(sqsum_32,z),
        _mm_unpacklo_epi32(sqsum_32,z)));

    imgBufO += width_ * skip_num_;
    imgBufP += width_ * skip_num_;
    num_pixels += (width_end - border_);
  }

  __m128i sad_final_128;
  __m128i sum_final_128;
  __m128i sqsum_final_128;

  
  
  _mm_store_si128 (&sad_final_128, sad_64);
  _mm_store_si128 (&sum_final_128, sum_64);
  _mm_store_si128 (&sqsum_final_128, sqsum_64);

  uint64_t *sad_final_64 = reinterpret_cast<uint64_t*>(&sad_final_128);
  uint64_t *sum_final_64 = reinterpret_cast<uint64_t*>(&sum_final_128);
  uint64_t *sqsum_final_64 = reinterpret_cast<uint64_t*>(&sqsum_final_128);

  const uint32_t pixelSum = sum_final_64[0] + sum_final_64[1];
  const uint64_t pixelSqSum = sqsum_final_64[0] + sqsum_final_64[1];
  const uint32_t tempDiffSum = sad_final_64[0] + sad_final_64[1];

  
  motion_magnitude_ = 0.0f;

  if (tempDiffSum == 0) return VPM_OK;

  
  const float tempDiffAvg = (float)tempDiffSum / (float)(num_pixels);
  const float pixelSumAvg = (float)pixelSum / (float)(num_pixels);
  const float pixelSqSumAvg = (float)pixelSqSum / (float)(num_pixels);
  float contrast = pixelSqSumAvg - (pixelSumAvg * pixelSumAvg);

  if (contrast > 0.0) {
    contrast = sqrt(contrast);
    motion_magnitude_ = tempDiffAvg/contrast;
  }

  return VPM_OK;
}

int32_t VPMContentAnalysis::ComputeSpatialMetrics_SSE2() {
  const uint8_t* imgBuf = orig_frame_ + border_*width_;
  const int32_t width_end = ((width_ - 2 * border_) & -16) + border_;

  __m128i se_32  = _mm_setzero_si128();
  __m128i sev_32 = _mm_setzero_si128();
  __m128i seh_32 = _mm_setzero_si128();
  __m128i msa_32 = _mm_setzero_si128();
  const __m128i z = _mm_setzero_si128();

  
  
  
  
  
  for (int32_t i = 0; i < (height_ - 2*border_); i += skip_num_) {
    __m128i se_16  = _mm_setzero_si128();
    __m128i sev_16 = _mm_setzero_si128();
    __m128i seh_16 = _mm_setzero_si128();
    __m128i msa_16 = _mm_setzero_si128();

    
    
    
    
    
    
    
    
    
    
    const uint8_t *lineTop = imgBuf - width_ + border_;
    const uint8_t *lineCen = imgBuf + border_;
    const uint8_t *lineBot = imgBuf + width_ + border_;

    for (int32_t j = 0; j < width_end - border_; j += 16) {
      const __m128i t = _mm_loadu_si128((__m128i*)(lineTop));
      const __m128i l = _mm_loadu_si128((__m128i*)(lineCen - 1));
      const __m128i c = _mm_loadu_si128((__m128i*)(lineCen));
      const __m128i r = _mm_loadu_si128((__m128i*)(lineCen + 1));
      const __m128i b = _mm_loadu_si128((__m128i*)(lineBot));

      lineTop += 16;
      lineCen += 16;
      lineBot += 16;

      
      __m128i clo = _mm_unpacklo_epi8(c,z);
      __m128i chi = _mm_unpackhi_epi8(c,z);

      
      const __m128i lrlo = _mm_add_epi16(_mm_unpacklo_epi8(l,z),
                                        _mm_unpacklo_epi8(r,z));
      const __m128i lrhi = _mm_add_epi16(_mm_unpackhi_epi8(l,z),
                                         _mm_unpackhi_epi8(r,z));

      
      const __m128i tblo = _mm_add_epi16(_mm_unpacklo_epi8(t,z),
                                         _mm_unpacklo_epi8(b,z));
      const __m128i tbhi = _mm_add_epi16(_mm_unpackhi_epi8(t,z),
                                         _mm_unpackhi_epi8(b,z));

      
      msa_16 = _mm_add_epi16(msa_16, _mm_add_epi16(chi, clo));

      clo = _mm_slli_epi16(clo, 1);
      chi = _mm_slli_epi16(chi, 1);
      const __m128i sevtlo = _mm_subs_epi16(clo, tblo);
      const __m128i sevthi = _mm_subs_epi16(chi, tbhi);
      const __m128i sehtlo = _mm_subs_epi16(clo, lrlo);
      const __m128i sehthi = _mm_subs_epi16(chi, lrhi);

      clo = _mm_slli_epi16(clo, 1);
      chi = _mm_slli_epi16(chi, 1);
      const __m128i setlo = _mm_subs_epi16(clo, _mm_add_epi16(lrlo, tblo));
      const __m128i sethi = _mm_subs_epi16(chi, _mm_add_epi16(lrhi, tbhi));

      
      se_16  = _mm_add_epi16(se_16, _mm_max_epi16(setlo,
          _mm_subs_epi16(z, setlo)));
      se_16  = _mm_add_epi16(se_16, _mm_max_epi16(sethi,
          _mm_subs_epi16(z, sethi)));
      sev_16 = _mm_add_epi16(sev_16, _mm_max_epi16(sevtlo,
                                             _mm_subs_epi16(z, sevtlo)));
      sev_16 = _mm_add_epi16(sev_16, _mm_max_epi16(sevthi,
          _mm_subs_epi16(z, sevthi)));
      seh_16 = _mm_add_epi16(seh_16, _mm_max_epi16(sehtlo,
            _mm_subs_epi16(z, sehtlo)));
      seh_16 = _mm_add_epi16(seh_16, _mm_max_epi16(sehthi,
            _mm_subs_epi16(z, sehthi)));
    }

    
    se_32  = _mm_add_epi32(se_32, _mm_add_epi32(_mm_unpackhi_epi16(se_16,z),
        _mm_unpacklo_epi16(se_16,z)));
    sev_32 = _mm_add_epi32(sev_32, _mm_add_epi32(_mm_unpackhi_epi16(sev_16,z),
        _mm_unpacklo_epi16(sev_16,z)));
    seh_32 = _mm_add_epi32(seh_32, _mm_add_epi32(_mm_unpackhi_epi16(seh_16,z),
        _mm_unpacklo_epi16(seh_16,z)));
    msa_32 = _mm_add_epi32(msa_32, _mm_add_epi32(_mm_unpackhi_epi16(msa_16,z),
        _mm_unpacklo_epi16(msa_16,z)));

    imgBuf += width_ * skip_num_;
  }

  __m128i se_128;
  __m128i sev_128;
  __m128i seh_128;
  __m128i msa_128;

  
  
  _mm_store_si128 (&se_128, _mm_add_epi64(_mm_unpackhi_epi32(se_32,z),
        _mm_unpacklo_epi32(se_32,z)));
  _mm_store_si128 (&sev_128, _mm_add_epi64(_mm_unpackhi_epi32(sev_32,z),
      _mm_unpacklo_epi32(sev_32,z)));
  _mm_store_si128 (&seh_128, _mm_add_epi64(_mm_unpackhi_epi32(seh_32,z),
      _mm_unpacklo_epi32(seh_32,z)));
  _mm_store_si128 (&msa_128, _mm_add_epi64(_mm_unpackhi_epi32(msa_32,z),
      _mm_unpacklo_epi32(msa_32,z)));

  uint64_t *se_64 = reinterpret_cast<uint64_t*>(&se_128);
  uint64_t *sev_64 = reinterpret_cast<uint64_t*>(&sev_128);
  uint64_t *seh_64 = reinterpret_cast<uint64_t*>(&seh_128);
  uint64_t *msa_64 = reinterpret_cast<uint64_t*>(&msa_128);

  const uint32_t spatialErrSum  = se_64[0] + se_64[1];
  const uint32_t spatialErrVSum = sev_64[0] + sev_64[1];
  const uint32_t spatialErrHSum = seh_64[0] + seh_64[1];
  const uint32_t pixelMSA = msa_64[0] + msa_64[1];

  
  const float spatialErr  = (float)(spatialErrSum >> 2);
  const float spatialErrH = (float)(spatialErrHSum >> 1);
  const float spatialErrV = (float)(spatialErrVSum >> 1);
  const float norm = (float)pixelMSA;

  
  spatial_pred_err_ = spatialErr / norm;

  
  spatial_pred_err_h_ = spatialErrH / norm;

  
  spatial_pred_err_v_ = spatialErrV / norm;

    return VPM_OK;
}

}  
