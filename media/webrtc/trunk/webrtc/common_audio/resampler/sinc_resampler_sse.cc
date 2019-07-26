












#include "webrtc/common_audio/resampler/sinc_resampler.h"

#include <xmmintrin.h>

namespace webrtc {

float SincResampler::Convolve_SSE(const float* input_ptr, const float* k1,
                                  const float* k2,
                                  double kernel_interpolation_factor) {
  __m128 m_input;
  __m128 m_sums1 = _mm_setzero_ps();
  __m128 m_sums2 = _mm_setzero_ps();

  
  
  if (reinterpret_cast<uintptr_t>(input_ptr) & 0x0F) {
    for (int i = 0; i < kKernelSize; i += 4) {
      m_input = _mm_loadu_ps(input_ptr + i);
      m_sums1 = _mm_add_ps(m_sums1, _mm_mul_ps(m_input, _mm_load_ps(k1 + i)));
      m_sums2 = _mm_add_ps(m_sums2, _mm_mul_ps(m_input, _mm_load_ps(k2 + i)));
    }
  } else {
    for (int i = 0; i < kKernelSize; i += 4) {
      m_input = _mm_load_ps(input_ptr + i);
      m_sums1 = _mm_add_ps(m_sums1, _mm_mul_ps(m_input, _mm_load_ps(k1 + i)));
      m_sums2 = _mm_add_ps(m_sums2, _mm_mul_ps(m_input, _mm_load_ps(k2 + i)));
    }
  }

  
  m_sums1 = _mm_mul_ps(m_sums1, _mm_set_ps1(1.0 - kernel_interpolation_factor));
  m_sums2 = _mm_mul_ps(m_sums2, _mm_set_ps1(kernel_interpolation_factor));
  m_sums1 = _mm_add_ps(m_sums1, m_sums2);

  
  float result;
  m_sums2 = _mm_add_ps(_mm_movehl_ps(m_sums1, m_sums1), m_sums1);
  _mm_store_ss(&result, _mm_add_ss(m_sums2, _mm_shuffle_ps(
      m_sums2, m_sums2, 1)));

  return result;
}

}  
