









#ifndef WEBRTC_VIDEO_ENGINE_OVERUSE_FRAME_DETECTOR_H_
#define WEBRTC_VIDEO_ENGINE_OVERUSE_FRAME_DETECTOR_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/exp_filter.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/video_engine/include/vie_base.h"

namespace webrtc {

class Clock;
class CpuOveruseObserver;
class CriticalSectionWrapper;


class Statistics {
 public:
  Statistics();

  void AddSample(float sample_ms);
  void Reset();
  void SetOptions(const CpuOveruseOptions& options);

  float Mean() const;
  float StdDev() const;
  uint64_t Count() const;

 private:
  float InitialMean() const;
  float InitialVariance() const;

  float sum_;
  uint64_t count_;
  CpuOveruseOptions options_;
  scoped_ptr<rtc::ExpFilter> filtered_samples_;
  scoped_ptr<rtc::ExpFilter> filtered_variance_;
};


class OveruseFrameDetector : public Module {
 public:
  explicit OveruseFrameDetector(Clock* clock);
  ~OveruseFrameDetector();

  
  
  void SetObserver(CpuOveruseObserver* observer);

  
  void SetOptions(const CpuOveruseOptions& options);

  
  void FrameCaptured(int width, int height, int64_t capture_time_ms);

  
  void FrameProcessingStarted();

  
  void FrameEncoded(int encode_time_ms);

  
  void FrameSent(int64_t capture_time_ms);

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void GetCpuOveruseMetrics(CpuOveruseMetrics* metrics) const;

  
  int CaptureQueueDelayMsPerS() const;
  int LastProcessingTimeMs() const;
  int FramesInQueue() const;

  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;

 private:
  class EncodeTimeAvg;
  class SendProcessingUsage;
  class CaptureQueueDelay;
  class FrameQueue;

  void AddProcessingTime(int elapsed_ms);

  bool IsOverusing();
  bool IsUnderusing(int64_t time_now);

  bool FrameTimeoutDetected(int64_t now) const;
  bool FrameSizeChanged(int num_pixels) const;

  void ResetAll(int num_pixels);

  
  scoped_ptr<CriticalSectionWrapper> crit_;

  
  CpuOveruseObserver* observer_;

  CpuOveruseOptions options_;

  Clock* clock_;
  int64_t next_process_time_;
  int64_t num_process_times_;

  Statistics capture_deltas_;
  int64_t last_capture_time_;

  int64_t last_overuse_time_;
  int checks_above_threshold_;
  int num_overuse_detections_;

  int64_t last_rampup_time_;
  bool in_quick_rampup_;
  int current_rampup_delay_ms_;

  
  int num_pixels_;

  int64_t last_encode_sample_ms_;
  scoped_ptr<EncodeTimeAvg> encode_time_;
  scoped_ptr<SendProcessingUsage> usage_;
  scoped_ptr<FrameQueue> frame_queue_;
  int64_t last_sample_time_ms_;

  scoped_ptr<CaptureQueueDelay> capture_queue_delay_;

  DISALLOW_COPY_AND_ASSIGN(OveruseFrameDetector);
};

}  

#endif  
