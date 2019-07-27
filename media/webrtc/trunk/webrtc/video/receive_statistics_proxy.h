









#ifndef WEBRTC_VIDEO_RECEIVE_STATISTICS_PROXY_H_
#define WEBRTC_VIDEO_RECEIVE_STATISTICS_PROXY_H_

#include <string>

#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/frame_callback.h"
#include "webrtc/modules/remote_bitrate_estimator/rate_statistics.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"
#include "webrtc/video_receive_stream.h"
#include "webrtc/video_renderer.h"

namespace webrtc {

class Clock;
class CriticalSectionWrapper;
class ViECodec;
class ViEDecoderObserver;

namespace internal {

class ReceiveStatisticsProxy : public ViEDecoderObserver,
                               public RtcpStatisticsCallback,
                               public StreamDataCountersCallback {
 public:
  ReceiveStatisticsProxy(uint32_t ssrc,
                         Clock* clock,
                         ViERTP_RTCP* rtp_rtcp,
                         ViECodec* codec,
                         int channel);
  virtual ~ReceiveStatisticsProxy();

  VideoReceiveStream::Stats GetStats() const;

  void OnDecodedFrame();
  void OnRenderedFrame();

  
  virtual void IncomingCodecChanged(const int video_channel,
                                    const VideoCodec& video_codec) OVERRIDE {}
  virtual void IncomingRate(const int video_channel,
                            const unsigned int framerate,
                            const unsigned int bitrate_bps) OVERRIDE;
  virtual void DecoderTiming(int decode_ms,
                             int max_decode_ms,
                             int current_delay_ms,
                             int target_delay_ms,
                             int jitter_buffer_ms,
                             int min_playout_delay_ms,
                             int render_delay_ms) OVERRIDE {}
  virtual void RequestNewKeyFrame(const int video_channel) OVERRIDE {}

  
  virtual void StatisticsUpdated(const webrtc::RtcpStatistics& statistics,
                                 uint32_t ssrc) OVERRIDE;

  
  virtual void DataCountersUpdated(const webrtc::StreamDataCounters& counters,
                                   uint32_t ssrc) OVERRIDE;

 private:
  std::string GetCName() const;

  const int channel_;
  Clock* const clock_;
  ViECodec* const codec_;
  ViERTP_RTCP* const rtp_rtcp_;

  scoped_ptr<CriticalSectionWrapper> crit_;
  VideoReceiveStream::Stats stats_ GUARDED_BY(crit_);
  RateStatistics decode_fps_estimator_ GUARDED_BY(crit_);
  RateStatistics renders_fps_estimator_ GUARDED_BY(crit_);
};

}  
}  
#endif  
