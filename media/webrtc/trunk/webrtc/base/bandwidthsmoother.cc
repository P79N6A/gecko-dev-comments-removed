









#include "webrtc/base/bandwidthsmoother.h"

#include <limits.h>

namespace rtc {

BandwidthSmoother::BandwidthSmoother(int initial_bandwidth_guess,
                                     uint32 time_between_increase,
                                     double percent_increase,
                                     size_t samples_count_to_average,
                                     double min_sample_count_percent)
    : time_between_increase_(time_between_increase),
      percent_increase_(rtc::_max(1.0, percent_increase)),
      time_at_last_change_(0),
      bandwidth_estimation_(initial_bandwidth_guess),
      accumulator_(samples_count_to_average),
      min_sample_count_percent_(
          rtc::_min(1.0,
                          rtc::_max(0.0, min_sample_count_percent))) {
}



bool BandwidthSmoother::Sample(uint32 sample_time, int bandwidth) {
  if (bandwidth < 0) {
    return false;
  }

  accumulator_.AddSample(bandwidth);

  if (accumulator_.count() < static_cast<size_t>(
          accumulator_.max_count() * min_sample_count_percent_)) {
    
    return false;
  }

  
  const int mean_bandwidth = static_cast<int>(accumulator_.ComputeMean());

  if (mean_bandwidth < bandwidth_estimation_) {
    time_at_last_change_ = sample_time;
    bandwidth_estimation_ = mean_bandwidth;
    return true;
  }

  const int old_bandwidth_estimation = bandwidth_estimation_;
  const double increase_threshold_d = percent_increase_ * bandwidth_estimation_;
  if (increase_threshold_d > INT_MAX) {
    
    return false;
  }

  const int increase_threshold = static_cast<int>(increase_threshold_d);
  if (mean_bandwidth < increase_threshold) {
    time_at_last_change_ = sample_time;
    
    
    
  } else if (sample_time >= time_at_last_change_ + time_between_increase_) {
    time_at_last_change_ = sample_time;
    if (increase_threshold == 0) {
      
      
      bandwidth_estimation_ = mean_bandwidth;
    } else {
      bandwidth_estimation_ = increase_threshold;
    }
  }
  

  return old_bandwidth_estimation != bandwidth_estimation_;
}

}  
