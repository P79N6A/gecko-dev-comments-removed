









#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"

#include <algorithm>  

namespace webrtc {








DelayPeakDetector::DelayPeakDetector()
  : peak_found_(false),
    peak_detection_threshold_(0),
    peak_period_counter_ms_(-1) {
}

void DelayPeakDetector::Reset() {
  peak_period_counter_ms_ = -1;  
  peak_found_ = false;
  peak_history_.clear();
}


void DelayPeakDetector::SetPacketAudioLength(int length_ms) {
  if (length_ms > 0) {
    peak_detection_threshold_ = kPeakHeightMs / length_ms;
  }
}

int DelayPeakDetector::MaxPeakHeight() const {
  int max_height = -1;  
  std::list<Peak>::const_iterator it;
  for (it = peak_history_.begin(); it != peak_history_.end(); ++it) {
    max_height = std::max(max_height, it->peak_height_packets);
  }
  return max_height;
}

int DelayPeakDetector::MaxPeakPeriod() const {
  int max_period = -1;  
  std::list<Peak>::const_iterator it;
  for (it = peak_history_.begin(); it != peak_history_.end(); ++it) {
    max_period = std::max(max_period, it->period_ms);
  }
  return max_period;
}

bool DelayPeakDetector::Update(int inter_arrival_time, int target_level) {
  if (inter_arrival_time > target_level + peak_detection_threshold_ ||
      inter_arrival_time > 2 * target_level) {
    
    if (peak_period_counter_ms_ == -1) {
      
      peak_period_counter_ms_ = 0;
    } else if (peak_period_counter_ms_ <= kMaxPeakPeriodMs) {
      
      
      Peak peak_data;
      peak_data.period_ms = peak_period_counter_ms_;
      peak_data.peak_height_packets = inter_arrival_time;
      peak_history_.push_back(peak_data);
      while (peak_history_.size() > kMaxNumPeaks) {
        
        peak_history_.pop_front();
      }
      peak_period_counter_ms_ = 0;
    } else if (peak_period_counter_ms_ <= 2 * kMaxPeakPeriodMs) {
      
      
      peak_period_counter_ms_ = 0;
    } else {
      
      
      
      Reset();
    }
  }
  return CheckPeakConditions();
}

void DelayPeakDetector::IncrementCounter(int inc_ms) {
  if (peak_period_counter_ms_ >= 0) {
    peak_period_counter_ms_ += inc_ms;
  }
}

bool DelayPeakDetector::CheckPeakConditions() {
  size_t s = peak_history_.size();
  if (s >= kMinPeaksToTrigger &&
      peak_period_counter_ms_ <= 2 * MaxPeakPeriod()) {
    peak_found_ = true;
  } else {
    peak_found_ = false;
  }
  return peak_found_;
}
}  
