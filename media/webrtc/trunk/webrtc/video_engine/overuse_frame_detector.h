









#ifndef WEBRTC_VIDEO_ENGINE_OVERUSE_FRAME_DETECTOR_H_
#define WEBRTC_VIDEO_ENGINE_OVERUSE_FRAME_DETECTOR_H_

#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Clock;
class CpuOveruseObserver;
class CriticalSectionWrapper;
class VCMExpFilter;


#ifdef WEBRTC_LINUX
const float kOveruseStdDevMs = 15.0f;
const float kNormalUseStdDevMs = 7.0f;
#elif WEBRTC_MAC
const float kOveruseStdDevMs = 22.0f;
const float kNormalUseStdDevMs = 12.0f;
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

  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;

 private:
  bool IsOverusing();
  bool IsUnderusing(int64_t time_now);

  
  scoped_ptr<CriticalSectionWrapper> crit_;

  
  const float normaluse_stddev_ms_;
  const float overuse_stddev_ms_;

  
  CpuOveruseObserver* observer_;

  Clock* clock_;
  int64_t next_process_time_;

  Statistics capture_deltas_;
  int64_t last_capture_time_;

  int64_t last_overuse_time_;
  int checks_above_threshold_;

  int64_t last_rampup_time_;
  bool in_quick_rampup_;
  int current_rampup_delay_ms_;

  
  int num_pixels_;

  DISALLOW_COPY_AND_ASSIGN(OveruseFrameDetector);
};

}  

#endif  
