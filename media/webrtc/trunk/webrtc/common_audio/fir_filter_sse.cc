









#include "webrtc/common_audio/fir_filter_sse.h"

#include <assert.h>
#include <string.h>
#include <xmmintrin.h>

#include "webrtc/system_wrappers/interface/aligned_malloc.h"

namespace webrtc {

FIRFilterSSE2::FIRFilterSSE2(const float* coefficients,
                             size_t coefficients_length,
                             size_t max_input_length)
    :  
      coefficients_length_((coefficients_length + 3) & ~0x03),
      state_length_(coefficients_length_ - 1),
      coefficients_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * coefficients_length_, 16))),
      state_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * (max_input_length + state_length_),
                        16))) {
  
  size_t padding = coefficients_length_ - coefficients_length;
  memset(coefficients_.get(), 0, padding * sizeof(coefficients_[0]));
  
  
  for (size_t i = 0; i < coefficients_length; ++i) {
    coefficients_[i + padding] = coefficients[coefficients_length - i - 1];
  }
  memset(state_.get(),
         0,
         (max_input_length + state_length_) * sizeof(state_[0]));
}

void FIRFilterSSE2::Filter(const float* in, size_t length, float* out) {
  assert(length > 0);

  memcpy(&state_[state_length_], in, length * sizeof(*in));

  
  
  for (size_t i = 0; i < length; ++i) {
    float* in_ptr = &state_[i];
    float* coef_ptr = coefficients_.get();

    __m128 m_sum = _mm_setzero_ps();
    __m128 m_in;

    
    
    if (reinterpret_cast<uintptr_t>(in_ptr) & 0x0F) {
      for (size_t j = 0; j < coefficients_length_; j += 4) {
        m_in = _mm_loadu_ps(in_ptr + j);
        m_sum = _mm_add_ps(m_sum, _mm_mul_ps(m_in, _mm_load_ps(coef_ptr + j)));
      }
    } else {
      for (size_t j = 0; j < coefficients_length_; j += 4) {
        m_in = _mm_load_ps(in_ptr + j);
        m_sum = _mm_add_ps(m_sum, _mm_mul_ps(m_in, _mm_load_ps(coef_ptr + j)));
      }
    }
    m_sum = _mm_add_ps(_mm_movehl_ps(m_sum, m_sum), m_sum);
    _mm_store_ss(out + i, _mm_add_ss(m_sum, _mm_shuffle_ps(m_sum, m_sum, 1)));
  }

  
  memmove(state_.get(), &state_[length], state_length_ * sizeof(state_[0]));
}

}  
