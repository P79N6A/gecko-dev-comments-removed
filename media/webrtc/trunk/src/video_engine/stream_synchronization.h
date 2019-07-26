









#ifndef WEBRTC_VIDEO_ENGINE_STREAM_SYNCHRONIZATION_H_
#define WEBRTC_VIDEO_ENGINE_STREAM_SYNCHRONIZATION_H_

#include "typedefs.h"  

namespace webrtc {

struct ViESyncDelay;

class StreamSynchronization {
 public:
  struct Measurements {
    Measurements()
        : received_ntp_secs(0),
          received_ntp_frac(0),
          rtcp_arrivaltime_secs(0),
          rtcp_arrivaltime_frac(0) {}
    uint32_t received_ntp_secs;
    uint32_t received_ntp_frac;
    uint32_t rtcp_arrivaltime_secs;
    uint32_t rtcp_arrivaltime_frac;
  };

  StreamSynchronization(int audio_channel_id, int video_channel_id);
  ~StreamSynchronization();

  int ComputeDelays(const Measurements& audio,
                    int current_audio_delay_ms,
                    int* extra_audio_delay_ms,
                    const Measurements& video,
                    int* total_video_delay_target_ms);

 private:
  ViESyncDelay* channel_delay_;
  int audio_channel_id_;
  int video_channel_id_;
};

}  

#endif  
