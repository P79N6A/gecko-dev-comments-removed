



#include "base/time/time.h"

#include <ios>
#include <limits>
#include <ostream>
#include <sstream>

#include "base/float_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/third_party/nspr/prtime.h"

namespace base {




TimeDelta TimeDelta::Max() {
  return TimeDelta(std::numeric_limits<int64>::max());
}

int TimeDelta::InDays() const {
  if (is_max()) {
    
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(delta_ / Time::kMicrosecondsPerDay);
}

int TimeDelta::InHours() const {
  if (is_max()) {
    
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(delta_ / Time::kMicrosecondsPerHour);
}

int TimeDelta::InMinutes() const {
  if (is_max()) {
    
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(delta_ / Time::kMicrosecondsPerMinute);
}

double TimeDelta::InSecondsF() const {
  if (is_max()) {
    
    return std::numeric_limits<double>::infinity();
  }
  return static_cast<double>(delta_) / Time::kMicrosecondsPerSecond;
}

int64 TimeDelta::InSeconds() const {
  if (is_max()) {
    
    return std::numeric_limits<int64>::max();
  }
  return delta_ / Time::kMicrosecondsPerSecond;
}

double TimeDelta::InMillisecondsF() const {
  if (is_max()) {
    
    return std::numeric_limits<double>::infinity();
  }
  return static_cast<double>(delta_) / Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMilliseconds() const {
  if (is_max()) {
    
    return std::numeric_limits<int64>::max();
  }
  return delta_ / Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMillisecondsRoundedUp() const {
  if (is_max()) {
    
    return std::numeric_limits<int64>::max();
  }
  return (delta_ + Time::kMicrosecondsPerMillisecond - 1) /
      Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMicroseconds() const {
  if (is_max()) {
    
    return std::numeric_limits<int64>::max();
  }
  return delta_;
}

std::ostream& operator<<(std::ostream& os, TimeDelta time_delta) {
  return os << time_delta.InSecondsF() << "s";
}




Time Time::Max() {
  return Time(std::numeric_limits<int64>::max());
}


Time Time::FromTimeT(time_t tt) {
  if (tt == 0)
    return Time();  
  if (tt == std::numeric_limits<time_t>::max())
    return Max();
  return Time((tt * kMicrosecondsPerSecond) + kTimeTToMicrosecondsOffset);
}

time_t Time::ToTimeT() const {
  if (is_null())
    return 0;  
  if (is_max()) {
    
    return std::numeric_limits<time_t>::max();
  }
  if (std::numeric_limits<int64>::max() - kTimeTToMicrosecondsOffset <= us_) {
    DLOG(WARNING) << "Overflow when converting base::Time with internal " <<
                     "value " << us_ << " to time_t.";
    return std::numeric_limits<time_t>::max();
  }
  return (us_ - kTimeTToMicrosecondsOffset) / kMicrosecondsPerSecond;
}


Time Time::FromDoubleT(double dt) {
  if (dt == 0 || IsNaN(dt))
    return Time();  
  if (dt == std::numeric_limits<double>::infinity())
    return Max();
  return Time(static_cast<int64>((dt *
                                  static_cast<double>(kMicrosecondsPerSecond)) +
                                 kTimeTToMicrosecondsOffset));
}

double Time::ToDoubleT() const {
  if (is_null())
    return 0;  
  if (is_max()) {
    
    return std::numeric_limits<double>::infinity();
  }
  return (static_cast<double>(us_ - kTimeTToMicrosecondsOffset) /
          static_cast<double>(kMicrosecondsPerSecond));
}

#if defined(OS_POSIX)

Time Time::FromTimeSpec(const timespec& ts) {
  return FromDoubleT(ts.tv_sec +
                     static_cast<double>(ts.tv_nsec) /
                         base::Time::kNanosecondsPerSecond);
}
#endif


Time Time::FromJsTime(double ms_since_epoch) {
  
  
  if (ms_since_epoch == std::numeric_limits<double>::infinity())
    return Max();
  return Time(static_cast<int64>(ms_since_epoch * kMicrosecondsPerMillisecond) +
              kTimeTToMicrosecondsOffset);
}

double Time::ToJsTime() const {
  if (is_null()) {
    
    return 0;
  }
  if (is_max()) {
    
    return std::numeric_limits<double>::infinity();
  }
  return (static_cast<double>(us_ - kTimeTToMicrosecondsOffset) /
          kMicrosecondsPerMillisecond);
}

int64 Time::ToJavaTime() const {
  if (is_null()) {
    
    return 0;
  }
  if (is_max()) {
    
    return std::numeric_limits<int64>::max();
  }
  return ((us_ - kTimeTToMicrosecondsOffset) /
          kMicrosecondsPerMillisecond);
}


Time Time::UnixEpoch() {
  Time time;
  time.us_ = kTimeTToMicrosecondsOffset;
  return time;
}

Time Time::LocalMidnight() const {
  Exploded exploded;
  LocalExplode(&exploded);
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  return FromLocalExploded(exploded);
}


bool Time::FromStringInternal(const char* time_string,
                              bool is_local,
                              Time* parsed_time) {
  DCHECK((time_string != NULL) && (parsed_time != NULL));

  if (time_string[0] == '\0')
    return false;

  PRTime result_time = 0;
  PRStatus result = PR_ParseTimeString(time_string,
                                       is_local ? PR_FALSE : PR_TRUE,
                                       &result_time);
  if (PR_SUCCESS != result)
    return false;

  result_time += kTimeTToMicrosecondsOffset;
  *parsed_time = Time(result_time);
  return true;
}

std::ostream& operator<<(std::ostream& os, Time time) {
  Time::Exploded exploded;
  time.UTCExplode(&exploded);
  
  return os << StringPrintf("%04d-%02d-%02d %02d:%02d:%02d.%03d UTC",
                            exploded.year,
                            exploded.month,
                            exploded.day_of_month,
                            exploded.hour,
                            exploded.minute,
                            exploded.second,
                            exploded.millisecond);
}



class UnixEpochSingleton {
 public:
  UnixEpochSingleton()
      : unix_epoch_(TimeTicks::Now() - (Time::Now() - Time::UnixEpoch())) {}

  TimeTicks unix_epoch() const { return unix_epoch_; }

 private:
  const TimeTicks unix_epoch_;

  DISALLOW_COPY_AND_ASSIGN(UnixEpochSingleton);
};

static LazyInstance<UnixEpochSingleton>::Leaky
    leaky_unix_epoch_singleton_instance = LAZY_INSTANCE_INITIALIZER;


TimeTicks TimeTicks::UnixEpoch() {
  return leaky_unix_epoch_singleton_instance.Get().unix_epoch();
}

std::ostream& operator<<(std::ostream& os, TimeTicks time_ticks) {
  
  
  
  
  
  const TimeDelta as_time_delta = time_ticks - TimeTicks();
  return os << as_time_delta.InMicroseconds() << " bogo-microseconds";
}



inline bool is_in_range(int value, int lo, int hi) {
  return lo <= value && value <= hi;
}

bool Time::Exploded::HasValidValues() const {
  return is_in_range(month, 1, 12) &&
         is_in_range(day_of_week, 0, 6) &&
         is_in_range(day_of_month, 1, 31) &&
         is_in_range(hour, 0, 23) &&
         is_in_range(minute, 0, 59) &&
         is_in_range(second, 0, 60) &&
         is_in_range(millisecond, 0, 999);
}

}  
