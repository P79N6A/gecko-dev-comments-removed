









#include "webrtc/base/exp_filter.h"

#include <math.h>

namespace rtc {

const float ExpFilter::kValueUndefined = -1.0f;

void ExpFilter::Reset(float alpha) {
  alpha_ = alpha;
  filtered_ = kValueUndefined;
}

float ExpFilter::Apply(float exp, float sample) {
  if (filtered_ == kValueUndefined) {
    
    filtered_ = sample;
  } else if (exp == 1.0) {
    filtered_ = alpha_ * filtered_ + (1 - alpha_) * sample;
  } else {
    float alpha = pow(alpha_, exp);
    filtered_ = alpha * filtered_ + (1 - alpha) * sample;
  }
  if (max_ != kValueUndefined && filtered_ > max_) {
    filtered_ = max_;
  }
  return filtered_;
}

void ExpFilter::UpdateBase(float alpha) {
  alpha_ = alpha;
}
}  
