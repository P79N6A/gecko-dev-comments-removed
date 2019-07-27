









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_MEDIA_OPTIMIZATION_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_MEDIA_OPTIMIZATION_H_

#include <list>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/source/media_opt_util.h"
#include "webrtc/modules/video_coding/main/source/qm_select.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {


class Clock;
class FrameDropper;
class VCMContentMetricsProcessing;

namespace media_optimization {


class MediaOptimization {
 public:
  MediaOptimization(int32_t id, Clock* clock);
  ~MediaOptimization();

  
  
  void Reset();

  
  void SetEncodingData(VideoCodecType send_codec_type,
                       int32_t max_bit_rate,
                       uint32_t frame_rate,
                       uint32_t bit_rate,
                       uint16_t width,
                       uint16_t height,
                       uint8_t divisor,
                       int num_temporal_layers,
                       int32_t mtu);

  
  
  
  
  
  
  
  
  uint32_t SetTargetRates(uint32_t target_bitrate,
                          uint8_t fraction_lost,
                          uint32_t round_trip_time_ms,
                          VCMProtectionCallback* protection_callback,
                          VCMQMSettingsCallback* qmsettings_callback);

  void EnableProtectionMethod(bool enable, VCMProtectionMethodEnum method);
  void EnableQM(bool enable);
  void EnableFrameDropper(bool enable);

  
  
  
  void SuspendBelowMinBitrate(int threshold_bps, int window_bps);
  bool IsVideoSuspended() const;

  bool DropFrame();

  void UpdateContentData(const VideoContentMetrics* content_metrics);

  
  int32_t UpdateWithEncodedData(int encoded_length,
                                uint32_t timestamp,
                                FrameType encoded_frame_type);

  
  void SetCPULoadState(CPULoadState state);

  uint32_t InputFrameRate();
  uint32_t SentFrameRate();
  uint32_t SentBitRate();
  VCMFrameCount SentFrameCount();

 private:
  enum {
    kFrameCountHistorySize = 90
  };
  enum {
    kFrameHistoryWinMs = 2000
  };
  enum {
    kBitrateAverageWinMs = 1000
  };

  struct EncodedFrameSample;
  typedef std::list<EncodedFrameSample> FrameSampleList;

  void UpdateIncomingFrameRate();
  void PurgeOldFrameSamples(int64_t now_ms);
  void UpdateSentBitrate(int64_t now_ms);
  void UpdateSentFramerate();

  
  int32_t SelectQuality(VCMQMSettingsCallback* qmsettings_callback);

  
  
  bool QMUpdate(VCMResolutionScale* qm,
                VCMQMSettingsCallback* qmsettings_callback);

  
  bool CheckStatusForQMchange();

  void ProcessIncomingFrameRate(int64_t now);

  
  
  
  void CheckSuspendConditions();

  int32_t id_;
  Clock* clock_;
  int32_t max_bit_rate_;
  VideoCodecType send_codec_type_;
  uint16_t codec_width_;
  uint16_t codec_height_;
  uint16_t min_width_;
  uint16_t min_height_;
  float user_frame_rate_;
  scoped_ptr<FrameDropper> frame_dropper_;
  scoped_ptr<VCMLossProtectionLogic> loss_prot_logic_;
  uint8_t fraction_lost_;
  uint32_t send_statistics_[4];
  uint32_t send_statistics_zero_encode_;
  int32_t max_payload_size_;
  int target_bit_rate_;
  float incoming_frame_rate_;
  int64_t incoming_frame_times_[kFrameCountHistorySize];
  bool enable_qm_;
  std::list<EncodedFrameSample> encoded_frame_samples_;
  uint32_t avg_sent_bit_rate_bps_;
  uint32_t avg_sent_framerate_;
  uint32_t key_frame_cnt_;
  uint32_t delta_frame_cnt_;
  scoped_ptr<VCMContentMetricsProcessing> content_;
  scoped_ptr<VCMQmResolution> qm_resolution_;
  int64_t last_qm_update_time_;
  int64_t last_change_time_;  
  int num_layers_;
  bool suspension_enabled_;
  bool video_suspended_;
  int suspension_threshold_bps_;
  int suspension_window_bps_;
  CPULoadState loadstate_;
};
}  
}  

#endif
