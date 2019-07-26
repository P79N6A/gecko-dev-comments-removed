









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_TIMING_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_TIMING_H_

#include "webrtc/modules/video_coding/main/source/codec_timer.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;
class VCMTimestampExtrapolator;

class VCMTiming {
 public:
  
  
  VCMTiming(Clock* clock,
            int32_t vcm_id = 0,
            int32_t timing_id = 0,
            VCMTiming* master_timing = NULL);
  ~VCMTiming();

  
  void Reset();
  void ResetDecodeTime();

  
  void set_render_delay(uint32_t render_delay_ms);

  
  
  void SetJitterDelay(uint32_t required_delay_ms);

  
  void set_min_playout_delay(uint32_t min_playout_delay);

  
  
  
  void UpdateCurrentDelay(uint32_t frame_timestamp);

  
  
  
  
  void UpdateCurrentDelay(int64_t render_time_ms,
                          int64_t actual_decode_time_ms);

  
  
  int32_t StopDecodeTimer(uint32_t time_stamp,
                          int64_t start_time_ms,
                          int64_t now_ms);

  
  
  void IncomingTimestamp(uint32_t time_stamp, int64_t last_packet_time_ms);
  
  
  
  int64_t RenderTimeMs(uint32_t frame_timestamp, int64_t now_ms) const;

  
  
  uint32_t MaxWaitingTime(int64_t render_time_ms, int64_t now_ms) const;

  
  
  uint32_t TargetVideoDelay() const;

  
  
  bool EnoughTimeToDecode(uint32_t available_processing_time_ms) const;

  
  void GetTimings(int* decode_ms,
                  int* max_decode_ms,
                  int* current_delay_ms,
                  int* target_delay_ms,
                  int* jitter_buffer_ms,
                  int* min_playout_delay_ms,
                  int* render_delay_ms) const;

  enum { kDefaultRenderDelayMs = 10 };
  enum { kDelayMaxChangeMsPerS = 100 };

 protected:
  int32_t MaxDecodeTimeMs(FrameType frame_type = kVideoFrameDelta) const;
  int64_t RenderTimeMsInternal(uint32_t frame_timestamp, int64_t now_ms) const;
  uint32_t TargetDelayInternal() const;

 private:
  CriticalSectionWrapper* crit_sect_;
  int32_t vcm_id_;
  Clock* clock_;
  int32_t timing_id_;
  bool master_;
  VCMTimestampExtrapolator* ts_extrapolator_;
  VCMCodecTimer codec_timer_;
  uint32_t render_delay_ms_;
  uint32_t min_playout_delay_ms_;
  uint32_t jitter_delay_ms_;
  uint32_t current_delay_ms_;
  int last_decode_ms_;
  uint32_t prev_frame_timestamp_;
};
}  

#endif  
