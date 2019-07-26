








#include "webrtc/test/statistics.h"

#include <math.h>

namespace webrtc {
namespace test {

Statistics::Statistics() : sum_(0.0), sum_squared_(0.0), count_(0) {}

void Statistics::AddSample(double sample) {
  sum_ += sample;
  sum_squared_ += sample * sample;
  ++count_;
}

double Statistics::Mean() const {
  if (count_ == 0)
    return 0.0;
  return sum_ / count_;
}

double Statistics::Variance() const {
  if (count_ == 0)
    return 0.0;
  return sum_squared_ / count_ - Mean() * Mean();
}

double Statistics::StandardDeviation() const {
  return sqrt(Variance());
}
}  
}  
