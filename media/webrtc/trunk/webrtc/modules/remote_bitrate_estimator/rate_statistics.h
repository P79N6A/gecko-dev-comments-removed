









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_RATE_STATISTICS_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_RATE_STATISTICS_H_

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RateStatistics {
 public:
  
  
  
  RateStatistics(uint32_t window_size_ms, float scale);
  ~RateStatistics();

  void Reset();
  void Update(uint32_t count, int64_t now_ms);
  uint32_t Rate(int64_t now_ms);

 private:
  void EraseOld(int64_t now_ms);

  
  
  const int num_buckets_;
  scoped_array<uint32_t> buckets_;

  
  uint32_t accumulated_count_;

  
  int64_t oldest_time_;

  
  int oldest_index_;

  
  const float scale_;
};
}  

#endif  
