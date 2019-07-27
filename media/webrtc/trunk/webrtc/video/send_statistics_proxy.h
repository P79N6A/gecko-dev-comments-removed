









#ifndef WEBRTC_VIDEO_SEND_STATISTICS_PROXY_H_
#define WEBRTC_VIDEO_SEND_STATISTICS_PROXY_H_

#include <string>

#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_capture.h"
#include "webrtc/video_send_stream.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;

class SendStatisticsProxy : public RtcpStatisticsCallback,
                            public StreamDataCountersCallback,
                            public BitrateStatisticsObserver,
                            public FrameCountObserver,
                            public ViEEncoderObserver,
                            public ViECaptureObserver,
                            public SendSideDelayObserver {
 public:
  explicit SendStatisticsProxy(const VideoSendStream::Config& config);
  virtual ~SendStatisticsProxy();

  VideoSendStream::Stats GetStats() const;

 protected:
  
  virtual void StatisticsUpdated(const RtcpStatistics& statistics,
                                 uint32_t ssrc) OVERRIDE;
  
  virtual void DataCountersUpdated(const StreamDataCounters& counters,
                                   uint32_t ssrc) OVERRIDE;

  
  virtual void Notify(const BitrateStatistics& total_stats,
                      const BitrateStatistics& retransmit_stats,
                      uint32_t ssrc) OVERRIDE;

  
  virtual void FrameCountUpdated(FrameType frame_type,
                                 uint32_t frame_count,
                                 const unsigned int ssrc) OVERRIDE;

  
  virtual void OutgoingRate(const int video_channel,
                            const unsigned int framerate,
                            const unsigned int bitrate) OVERRIDE;

  virtual void SuspendChange(int video_channel, bool is_suspended) OVERRIDE;

  
  virtual void BrightnessAlarm(const int capture_id,
                               const Brightness brightness) OVERRIDE {}

  virtual void CapturedFrameRate(const int capture_id,
                                 const unsigned char frame_rate) OVERRIDE;

  virtual void NoPictureAlarm(const int capture_id,
                              const CaptureAlarm alarm) OVERRIDE {}

  virtual void SendSideDelayUpdated(int avg_delay_ms,
                                    int max_delay_ms,
                                    uint32_t ssrc) OVERRIDE;

 private:
  SsrcStats* GetStatsEntry(uint32_t ssrc) EXCLUSIVE_LOCKS_REQUIRED(crit_);

  const VideoSendStream::Config config_;
  scoped_ptr<CriticalSectionWrapper> crit_;
  VideoSendStream::Stats stats_ GUARDED_BY(crit_);
};

}  
#endif  
