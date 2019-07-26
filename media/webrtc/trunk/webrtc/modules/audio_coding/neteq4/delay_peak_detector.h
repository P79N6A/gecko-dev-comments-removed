









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DELAY_PEAK_DETECTOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DELAY_PEAK_DETECTOR_H_

#include <string.h>  

#include <list>

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

class DelayPeakDetector {
 public:
  DelayPeakDetector();
  virtual ~DelayPeakDetector() {}
  virtual void Reset();

  
  
  virtual void SetPacketAudioLength(int length_ms);

  
  
  virtual bool peak_found() { return peak_found_; }

  
  
  virtual int MaxPeakHeight() const;

  
  
  virtual int MaxPeakPeriod() const;

  
  
  
  virtual bool Update(int inter_arrival_time, int target_level);

  
  
  
  virtual void IncrementCounter(int inc_ms);

 private:
  static const size_t kMaxNumPeaks = 8;
  static const size_t kMinPeaksToTrigger = 2;
  static const int kPeakHeightMs = 78;
  static const int kMaxPeakPeriodMs = 10000;

  typedef struct {
    int period_ms;
    int peak_height_packets;
  } Peak;

  bool CheckPeakConditions();

  std::list<Peak> peak_history_;
  bool peak_found_;
  int peak_detection_threshold_;
  int peak_period_counter_ms_;

  DISALLOW_COPY_AND_ASSIGN(DelayPeakDetector);
};

}  
#endif  
