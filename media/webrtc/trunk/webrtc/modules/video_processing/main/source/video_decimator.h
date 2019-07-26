









#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_VIDEO_DECIMATOR_H
#define WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_VIDEO_DECIMATOR_H

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMVideoDecimator {
 public:
  VPMVideoDecimator();
  ~VPMVideoDecimator();

  void Reset();

  void EnableTemporalDecimation(bool enable);

  int32_t SetMaxFramerate(uint32_t max_frame_rate);
  int32_t SetTargetframe_rate(uint32_t frame_rate);

  bool DropFrame();

  void UpdateIncomingframe_rate();

  
  uint32_t Decimatedframe_rate();

  
  uint32_t Inputframe_rate();

 private:
  void ProcessIncomingframe_rate(int64_t now);

  enum { kFrameCountHistory_size = 90};
  enum { kFrameHistoryWindowMs = 2000};

  
  int32_t overshoot_modifier_;
  uint32_t drop_count_;
  uint32_t keep_count_;
  uint32_t target_frame_rate_;
  float incoming_frame_rate_;
  uint32_t max_frame_rate_;
  int64_t incoming_frame_times_[kFrameCountHistory_size];
  bool enable_temporal_decimation_;
};

}  

#endif  
