









#ifndef WEBRTC_VIDEO_ENGINE_OVERUSE_FRAME_DETECTOR_H_
#define WEBRTC_VIDEO_ENGINE_OVERUSE_FRAME_DETECTOR_H_

#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"

namespace webrtc {

class Clock;
class CpuOveruseObserver;
class CriticalSectionWrapper;
class VCMExpFilter;


#ifdef WEBRTC_LINUX
const float kOveruseStdDevMs = 15.0f;
const float kNormalUseStdDevMs = 7.0f;
#elif WEBRTC_MAC
const float kOveruseStdDevMs = 24.0f;
const float kNormalUseStdDevMs = 14.0f;
#else
const float kOveruseStdDevMs = 17.0f;
const float kNormalUseStdDevMs = 10.0f;
#endif


class Statistics {
 public:
  Statistics();

  void AddSample(float sample_ms);
  void Reset();

  float Mean() const;
  float StdDev() const;
  uint64_t Count() const;

 private:
  float InitialMean() const;
  float InitialVariance() const;

  float sum_;
  uint64_t count_;
  scoped_ptr<VCMExpFilter> filtered_samples_;
  scoped_ptr<VCMExpFilter> filtered_variance_;
};


class OveruseFrameDetector : public Module {
 public:
  explicit OveruseFrameDetector(Clock* clock,
                                float normaluse_stddev_ms,
                                float overuse_stddev_ms);
  ~OveruseFrameDetector();

  
  
  void SetObserver(CpuOveruseObserver* observer);

  
  void FrameCaptured(int width, int height);

  
  void FrameProcessingStarted();

  
  void FrameEncoded(int encode_time_ms);

  
  
  int last_capture_jitter_ms() const;

  
  
  int AvgEncodeTimeMs() const;

  
  
  
  int EncodeUsagePercent() const;

  
  
  
  
  
  
  int AvgCaptureQueueDelayMsPerS() const;
  int CaptureQueueDelayMsPerS() const;

  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;

 private:
  FRIEND_TEST_ALL_PREFIXES(OveruseFrameDetectorTest, TriggerOveruse);
  FRIEND_TEST_ALL_PREFIXES(OveruseFrameDetectorTest, OveruseAndRecover);
  FRIEND_TEST_ALL_PREFIXES(OveruseFrameDetectorTest, DoubleOveruseAndRecover);
  FRIEND_TEST_ALL_PREFIXES(
      OveruseFrameDetectorTest, TriggerNormalUsageWithMinProcessCount);
  FRIEND_TEST_ALL_PREFIXES(
      OveruseFrameDetectorTest, ConstantOveruseGivesNoNormalUsage);
  FRIEND_TEST_ALL_PREFIXES(OveruseFrameDetectorTest, LastCaptureJitter);

  void set_min_process_count_before_reporting(int64_t count) {
    min_process_count_before_reporting_ = count;
  }

  class EncodeTimeAvg;
  class EncodeUsage;
  class CaptureQueueDelay;

  bool IsOverusing();
  bool IsUnderusing(int64_t time_now);

  bool DetectFrameTimeout(int64_t now) const;

  
  scoped_ptr<CriticalSectionWrapper> crit_;

  
  const float normaluse_stddev_ms_;
  const float overuse_stddev_ms_;

  int64_t min_process_count_before_reporting_;

  
  CpuOveruseObserver* observer_;

  Clock* clock_;
  int64_t next_process_time_;
  int64_t num_process_times_;

  Statistics capture_deltas_;
  int64_t last_capture_time_;

  int64_t last_overuse_time_;
  int checks_above_threshold_;

  int64_t last_rampup_time_;
  bool in_quick_rampup_;
  int current_rampup_delay_ms_;

  
  int num_pixels_;

  int last_capture_jitter_ms_;

  int64_t last_encode_sample_ms_;
  scoped_ptr<EncodeTimeAvg> encode_time_;
  scoped_ptr<EncodeUsage> encode_usage_;

  scoped_ptr<CaptureQueueDelay> capture_queue_delay_;

  DISALLOW_COPY_AND_ASSIGN(OveruseFrameDetector);
};

}  

#endif  
