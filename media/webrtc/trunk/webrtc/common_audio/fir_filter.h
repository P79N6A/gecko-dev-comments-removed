









#ifndef WEBRTC_COMMON_AUDIO_FIR_FILTER_H_
#define WEBRTC_COMMON_AUDIO_FIR_FILTER_H_

#include <string.h>

namespace webrtc {


class FIRFilter {
 public:
  
  
  
  
  
  
  static FIRFilter* Create(const float* coefficients,
                           size_t coefficients_length,
                           size_t max_input_length);

  virtual ~FIRFilter() {}

  
  
  virtual void Filter(const float* in, size_t length, float* out) = 0;
};

}  

#endif  
