








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_STATISTICS_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_STATISTICS_H_

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class Statistics {
 public:
  Statistics();

  void AddSample(double sample);

  double Mean() const;
  double Variance() const;
  double StandardDeviation() const;

 private:
  double sum_;
  double sum_squared_;
  uint64_t count_;
};
}  
}  

#endif  
