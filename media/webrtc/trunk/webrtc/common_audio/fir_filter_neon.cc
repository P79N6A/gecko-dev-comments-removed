









#include "webrtc/common_audio/fir_filter_neon.h"

#include <arm_neon.h>
#include <assert.h>
#include <string.h>

#include "webrtc/system_wrappers/interface/aligned_malloc.h"

namespace webrtc {

FIRFilterNEON::FIRFilterNEON(const float* coefficients,
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
  memset(coefficients_.get(), 0.f, padding * sizeof(coefficients_[0]));
  
  
  for (size_t i = 0; i < coefficients_length; ++i) {
    coefficients_[i + padding] = coefficients[coefficients_length - i - 1];
  }
  memset(state_.get(),
         0.f,
         (max_input_length + state_length_) * sizeof(state_[0]));
}

void FIRFilterNEON::Filter(const float* in, size_t length, float* out) {
  assert(length > 0);

  memcpy(&state_[state_length_], in, length * sizeof(*in));

  
  
  for (size_t i = 0; i < length; ++i) {
    float* in_ptr = &state_[i];
    float* coef_ptr = coefficients_.get();

    float32x4_t m_sum = vmovq_n_f32(0);
    float32x4_t m_in;

    for (size_t j = 0; j < coefficients_length_; j += 4) {
       m_in = vld1q_f32(in_ptr + j);
       m_sum = vmlaq_f32(m_sum, m_in, vld1q_f32(coef_ptr + j));
    }

    float32x2_t m_half = vadd_f32(vget_high_f32(m_sum), vget_low_f32(m_sum));
    out[i] = vget_lane_f32(vpadd_f32(m_half, m_half), 0);
  }

  
  memmove(state_.get(), &state_[length], state_length_ * sizeof(state_[0]));
}

}  
