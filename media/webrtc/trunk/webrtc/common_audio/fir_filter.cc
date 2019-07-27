









#include "webrtc/common_audio/fir_filter.h"

#include <assert.h>
#include <string.h>

#include "webrtc/common_audio/fir_filter_neon.h"
#include "webrtc/common_audio/fir_filter_sse.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class FIRFilterC : public FIRFilter {
 public:
  FIRFilterC(const float* coefficients,
             size_t coefficients_length);

  virtual void Filter(const float* in, size_t length, float* out) OVERRIDE;

 private:
  size_t coefficients_length_;
  size_t state_length_;
  scoped_ptr<float[]> coefficients_;
  scoped_ptr<float[]> state_;
};

FIRFilter* FIRFilter::Create(const float* coefficients,
                             size_t coefficients_length,
                             size_t max_input_length) {
  if (!coefficients || coefficients_length <= 0 || max_input_length <= 0) {
    assert(false);
    return NULL;
  }

  FIRFilter* filter = NULL;

#if defined(WEBRTC_ARCH_X86_FAMILY)
#if defined(__SSE2__)
  filter =
      new FIRFilterSSE2(coefficients, coefficients_length, max_input_length);
#else
  
  if (WebRtc_GetCPUInfo(kSSE2)) {
    filter =
        new FIRFilterSSE2(coefficients, coefficients_length, max_input_length);
  } else {
    filter = new FIRFilterC(coefficients, coefficients_length);
  }
#endif
#elif defined(WEBRTC_ARCH_ARM_V7)
#if defined(WEBRTC_ARCH_ARM_NEON)
  filter =
      new FIRFilterNEON(coefficients, coefficients_length, max_input_length);
#else
  
  if (WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON) {
    filter =
        new FIRFilterNEON(coefficients, coefficients_length, max_input_length);
  } else {
    filter = new FIRFilterC(coefficients, coefficients_length);
  }
#endif
#else
  filter = new FIRFilterC(coefficients, coefficients_length);
#endif

  return filter;
}

FIRFilterC::FIRFilterC(const float* coefficients, size_t coefficients_length)
    : coefficients_length_(coefficients_length),
      state_length_(coefficients_length - 1),
      coefficients_(new float[coefficients_length_]),
      state_(new float[state_length_]) {
  for (size_t i = 0; i < coefficients_length_; ++i) {
    coefficients_[i] = coefficients[coefficients_length_ - i - 1];
  }
  memset(state_.get(), 0, state_length_ * sizeof(state_[0]));
}

void FIRFilterC::Filter(const float* in, size_t length, float* out) {
  assert(length > 0);

  
  
  for (size_t i = 0; i < length; ++i) {
    out[i] = 0.f;
    size_t j;
    for (j = 0; state_length_ > i && j < state_length_ - i; ++j) {
      out[i] += state_[i + j] * coefficients_[j];
    }
    for (; j < coefficients_length_; ++j) {
      out[i] += in[j + i - state_length_] * coefficients_[j];
    }
  }

  
  if (length >= state_length_) {
    memcpy(
        state_.get(), &in[length - state_length_], state_length_ * sizeof(*in));
  } else {
    memmove(state_.get(),
            &state_[length],
            (state_length_ - length) * sizeof(state_[0]));
    memcpy(&state_[state_length_ - length], in, length * sizeof(*in));
  }
}

}  
