












#include "webrtc/common_audio/resampler/sinc_resampler.h"

#include <arm_neon.h>

namespace webrtc {

float SincResampler::Convolve_NEON(const float* input_ptr, const float* k1,
                                   const float* k2,
                                   double kernel_interpolation_factor) {
  float32x4_t m_input;
  float32x4_t m_sums1 = vmovq_n_f32(0);
  float32x4_t m_sums2 = vmovq_n_f32(0);

  const float* upper = input_ptr + kKernelSize;
  for (; input_ptr < upper; ) {
    m_input = vld1q_f32(input_ptr);
    input_ptr += 4;
    m_sums1 = vmlaq_f32(m_sums1, m_input, vld1q_f32(k1));
    k1 += 4;
    m_sums2 = vmlaq_f32(m_sums2, m_input, vld1q_f32(k2));
    k2 += 4;
  }

  
  m_sums1 = vmlaq_f32(
      vmulq_f32(m_sums1, vmovq_n_f32(1.0 - kernel_interpolation_factor)),
      m_sums2, vmovq_n_f32(kernel_interpolation_factor));

  
  float32x2_t m_half = vadd_f32(vget_high_f32(m_sums1), vget_low_f32(m_sums1));
  return vget_lane_f32(vpadd_f32(m_half, m_half), 0);
}

}  
