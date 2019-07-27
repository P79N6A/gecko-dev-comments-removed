









#include "webrtc/base/ratetracker.h"
#include "webrtc/base/timeutils.h"

namespace rtc {

RateTracker::RateTracker()
    : total_units_(0), units_second_(0),
      last_units_second_time_(static_cast<uint32>(-1)),
      last_units_second_calc_(0) {
}

size_t RateTracker::total_units() const {
  return total_units_;
}

size_t RateTracker::units_second() {
  
  
  
  
  uint32 current_time = Time();
  if (last_units_second_time_ != static_cast<uint32>(-1)) {
    int delta = rtc::TimeDiff(current_time, last_units_second_time_);
    if (delta >= 1000) {
      int fraction_time = delta % 1000;
      int seconds = delta / 1000;
      int fraction_units =
          static_cast<int>(total_units_ - last_units_second_calc_) *
              fraction_time / delta;
      
      units_second_ =
          (total_units_ - last_units_second_calc_ - fraction_units) / seconds;
      last_units_second_time_ = current_time - fraction_time;
      last_units_second_calc_ = total_units_ - fraction_units;
    }
  }
  if (last_units_second_time_ == static_cast<uint32>(-1)) {
    last_units_second_time_ = current_time;
    last_units_second_calc_ = total_units_;
  }

  return units_second_;
}

void RateTracker::Update(size_t units) {
  total_units_ += units;
}

uint32 RateTracker::Time() const {
  return rtc::Time();
}

}  
