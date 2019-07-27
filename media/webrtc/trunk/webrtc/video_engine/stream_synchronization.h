









#ifndef WEBRTC_VIDEO_ENGINE_STREAM_SYNCHRONIZATION_H_
#define WEBRTC_VIDEO_ENGINE_STREAM_SYNCHRONIZATION_H_

#include <list>

#include "webrtc/system_wrappers/interface/rtp_to_ntp.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct ViESyncDelay;

class StreamSynchronization {
 public:
  struct Measurements {
    Measurements() : rtcp(), latest_receive_time_ms(0), latest_timestamp(0) {}
    RtcpList rtcp;
    int64_t latest_receive_time_ms;
    uint32_t latest_timestamp;
  };

  StreamSynchronization(int audio_channel_id, int video_channel_id);
  ~StreamSynchronization();

  bool ComputeDelays(int relative_delay_ms,
                     int current_audio_delay_ms,
                     int* extra_audio_delay_ms,
                     int* total_video_delay_target_ms);

  
  
  
  static bool ComputeRelativeDelay(const Measurements& audio_measurement,
                                   const Measurements& video_measurement,
                                   int* relative_delay_ms);
  
  
  void SetTargetBufferingDelay(int target_delay_ms);

 private:
  ViESyncDelay* channel_delay_;
  int audio_channel_id_;
  int video_channel_id_;
  int base_target_delay_ms_;
  int avg_diff_ms_;
};
}  

#endif  
