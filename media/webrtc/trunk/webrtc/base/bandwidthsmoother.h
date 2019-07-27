









#ifndef WEBRTC_BASE_BANDWIDTHSMOOTHER_H_
#define WEBRTC_BASE_BANDWIDTHSMOOTHER_H_

#include "webrtc/base/rollingaccumulator.h"
#include "webrtc/base/timeutils.h"

namespace rtc {













class BandwidthSmoother {
 public:
  BandwidthSmoother(int initial_bandwidth_guess,
                    uint32 time_between_increase,
                    double percent_increase,
                    size_t samples_count_to_average,
                    double min_sample_count_percent);

  
  
  
  bool Sample(uint32 sample_time, int bandwidth);

  int get_bandwidth_estimation() const {
    return bandwidth_estimation_;
  }

 private:
  uint32 time_between_increase_;
  double percent_increase_;
  uint32 time_at_last_change_;
  int bandwidth_estimation_;
  RollingAccumulator<int> accumulator_;
  double min_sample_count_percent_;
};

}  

#endif  
