









#ifndef WEBRTC_COMMON_AUDIO_FIR_FILTER_NEON_H_
#define WEBRTC_COMMON_AUDIO_FIR_FILTER_NEON_H_

#include "webrtc/common_audio/fir_filter.h"
#include "webrtc/system_wrappers/interface/aligned_malloc.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class FIRFilterNEON : public FIRFilter {
 public:
  FIRFilterNEON(const float* coefficients,
                size_t coefficients_length,
                size_t max_input_length);

  virtual void Filter(const float* in, size_t length, float* out) OVERRIDE;

 private:
  size_t coefficients_length_;
  size_t state_length_;
  scoped_ptr<float[], AlignedFreeDeleter> coefficients_;
  scoped_ptr<float[], AlignedFreeDeleter> state_;
};

}  

#endif  
