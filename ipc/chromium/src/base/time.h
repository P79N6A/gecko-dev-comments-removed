




















#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <time.h>

#include "base/basictypes.h"

#if defined(OS_WIN)


#include <windows.h>
#endif

namespace base {

class Time;
class TimeTicks;


class PageLoadTrackerUnitTest;



class TimeDelta {
 public:
  TimeDelta() : delta_(0) {
  }

  
  static TimeDelta FromDays(int64_t days);
  static TimeDelta FromHours(int64_t hours);
  static TimeDelta FromMinutes(int64_t minutes);
  static TimeDelta FromSeconds(int64_t secs);
  static TimeDelta FromMilliseconds(int64_t ms);
  static TimeDelta FromMicroseconds(int64_t us);

  
  
  
  int64_t ToInternalValue() const {
    return delta_;
  }

  
  
  int InDays() const;
  int InHours() const;
  int InMinutes() const;
  double InSecondsF() const;
  int64_t InSeconds() const;
  double InMillisecondsF() const;
  int64_t InMilliseconds() const;
  int64_t InMicroseconds() const;

  TimeDelta& operator=(TimeDelta other) {
    delta_ = other.delta_;
    return *this;
  }

  
  TimeDelta operator+(TimeDelta other) const {
    return TimeDelta(delta_ + other.delta_);
  }
  TimeDelta operator-(TimeDelta other) const {
    return TimeDelta(delta_ - other.delta_);
  }

  TimeDelta& operator+=(TimeDelta other) {
    delta_ += other.delta_;
    return *this;
  }
  TimeDelta& operator-=(TimeDelta other) {
    delta_ -= other.delta_;
    return *this;
  }
  TimeDelta operator-() const {
    return TimeDelta(-delta_);
  }

  
  
  TimeDelta operator*(int64_t a) const {
    return TimeDelta(delta_ * a);
  }
  TimeDelta operator/(int64_t a) const {
    return TimeDelta(delta_ / a);
  }
  TimeDelta& operator*=(int64_t a) {
    delta_ *= a;
    return *this;
  }
  TimeDelta& operator/=(int64_t a) {
    delta_ /= a;
    return *this;
  }
  int64_t operator/(TimeDelta a) const {
    return delta_ / a.delta_;
  }

  
  Time operator+(Time t) const;
  TimeTicks operator+(TimeTicks t) const;

  
  bool operator==(TimeDelta other) const {
    return delta_ == other.delta_;
  }
  bool operator!=(TimeDelta other) const {
    return delta_ != other.delta_;
  }
  bool operator<(TimeDelta other) const {
    return delta_ < other.delta_;
  }
  bool operator<=(TimeDelta other) const {
    return delta_ <= other.delta_;
  }
  bool operator>(TimeDelta other) const {
    return delta_ > other.delta_;
  }
  bool operator>=(TimeDelta other) const {
    return delta_ >= other.delta_;
  }

 private:
  friend class Time;
  friend class TimeTicks;
  friend TimeDelta operator*(int64_t a, TimeDelta td);

  
  
  
  explicit TimeDelta(int64_t delta_us) : delta_(delta_us) {
  }

  
  int64_t delta_;
};

inline TimeDelta operator*(int64_t a, TimeDelta td) {
  return TimeDelta(a * td.delta_);
}




class Time {
 public:
  static const int64_t kMillisecondsPerSecond = 1000;
  static const int64_t kMicrosecondsPerMillisecond = 1000;
  static const int64_t kMicrosecondsPerSecond = kMicrosecondsPerMillisecond *
                                              kMillisecondsPerSecond;
  static const int64_t kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
  static const int64_t kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
  static const int64_t kMicrosecondsPerDay = kMicrosecondsPerHour * 24;
  static const int64_t kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
  static const int64_t kNanosecondsPerMicrosecond = 1000;
  static const int64_t kNanosecondsPerSecond = kNanosecondsPerMicrosecond *
                                             kMicrosecondsPerSecond;

  
  
  
  struct Exploded {
    int year;                 
    signed char month;        
    signed char day_of_week;  
    signed char day_of_month; 
    signed char hour;         
    signed char minute;       
    signed char second;       
                              
    int millisecond;          
  };

  
  explicit Time() : us_(0) {
  }

  
  bool is_null() const {
    return us_ == 0;
  }

  
  
  
  static Time Now();

  
  
  
  
  static Time NowFromSystemTime();

  
  
  
  static Time FromTimeT(time_t tt);
  time_t ToTimeT() const;

  
  
  static Time FromDoubleT(double dt);
  double ToDoubleT() const;


#if defined(OS_WIN)
  static Time FromFileTime(FILETIME ft);
  FILETIME ToFileTime() const;
#endif

  
  
  static Time FromUTCExploded(const Exploded& exploded) {
    return FromExploded(false, exploded);
  }
  static Time FromLocalExploded(const Exploded& exploded) {
    return FromExploded(true, exploded);
  }

  
  
  
  
  static Time FromInternalValue(int64_t us) {
    return Time(us);
  }

  
  
  
  
  
  
  static bool FromString(const wchar_t* time_string, Time* parsed_time);

  
  
  
  int64_t ToInternalValue() const {
    return us_;
  }

  
  
  void UTCExplode(Exploded* exploded) const {
    return Explode(false, exploded);
  }
  void LocalExplode(Exploded* exploded) const {
    return Explode(true, exploded);
  }

  
  
  Time LocalMidnight() const;

  Time& operator=(Time other) {
    us_ = other.us_;
    return *this;
  }

  
  TimeDelta operator-(Time other) const {
    return TimeDelta(us_ - other.us_);
  }

  
  Time& operator+=(TimeDelta delta) {
    us_ += delta.delta_;
    return *this;
  }
  Time& operator-=(TimeDelta delta) {
    us_ -= delta.delta_;
    return *this;
  }

  
  Time operator+(TimeDelta delta) const {
    return Time(us_ + delta.delta_);
  }
  Time operator-(TimeDelta delta) const {
    return Time(us_ - delta.delta_);
  }

  
  bool operator==(Time other) const {
    return us_ == other.us_;
  }
  bool operator!=(Time other) const {
    return us_ != other.us_;
  }
  bool operator<(Time other) const {
    return us_ < other.us_;
  }
  bool operator<=(Time other) const {
    return us_ <= other.us_;
  }
  bool operator>(Time other) const {
    return us_ > other.us_;
  }
  bool operator>=(Time other) const {
    return us_ >= other.us_;
  }

 private:
  friend class TimeDelta;

  
  
  void Explode(bool is_local, Exploded* exploded) const;

  
  
  static Time FromExploded(bool is_local, const Exploded& exploded);

  explicit Time(int64_t us) : us_(us) {
  }

  
  
  static const int64_t kTimeTToMicrosecondsOffset;

  
  int64_t us_;
};

inline Time TimeDelta::operator+(Time t) const {
  return Time(t.us_ + delta_);
}




inline TimeDelta TimeDelta::FromDays(int64_t days) {
  return TimeDelta(days * Time::kMicrosecondsPerDay);
}


inline TimeDelta TimeDelta::FromHours(int64_t hours) {
  return TimeDelta(hours * Time::kMicrosecondsPerHour);
}


inline TimeDelta TimeDelta::FromMinutes(int64_t minutes) {
  return TimeDelta(minutes * Time::kMicrosecondsPerMinute);
}


inline TimeDelta TimeDelta::FromSeconds(int64_t secs) {
  return TimeDelta(secs * Time::kMicrosecondsPerSecond);
}


inline TimeDelta TimeDelta::FromMilliseconds(int64_t ms) {
  return TimeDelta(ms * Time::kMicrosecondsPerMillisecond);
}


inline TimeDelta TimeDelta::FromMicroseconds(int64_t us) {
  return TimeDelta(us);
}



class TimeTicks {
 public:
  TimeTicks() : ticks_(0) {
  }

  
  
  
  static TimeTicks Now();

  
  
  
  
  static TimeTicks HighResNow();

  
  bool is_null() const {
    return ticks_ == 0;
  }

  
  int64_t ToInternalValue() const {
    return ticks_;
  }

  TimeTicks& operator=(TimeTicks other) {
    ticks_ = other.ticks_;
    return *this;
  }

  
  TimeDelta operator-(TimeTicks other) const {
    return TimeDelta(ticks_ - other.ticks_);
  }

  
  TimeTicks& operator+=(TimeDelta delta) {
    ticks_ += delta.delta_;
    return *this;
  }
  TimeTicks& operator-=(TimeDelta delta) {
    ticks_ -= delta.delta_;
    return *this;
  }

  
  TimeTicks operator+(TimeDelta delta) const {
    return TimeTicks(ticks_ + delta.delta_);
  }
  TimeTicks operator-(TimeDelta delta) const {
    return TimeTicks(ticks_ - delta.delta_);
  }

  
  bool operator==(TimeTicks other) const {
    return ticks_ == other.ticks_;
  }
  bool operator!=(TimeTicks other) const {
    return ticks_ != other.ticks_;
  }
  bool operator<(TimeTicks other) const {
    return ticks_ < other.ticks_;
  }
  bool operator<=(TimeTicks other) const {
    return ticks_ <= other.ticks_;
  }
  bool operator>(TimeTicks other) const {
    return ticks_ > other.ticks_;
  }
  bool operator>=(TimeTicks other) const {
    return ticks_ >= other.ticks_;
  }

 protected:
  friend class TimeDelta;
  friend class PageLoadTrackerUnitTest;

  
  
  explicit TimeTicks(int64_t ticks) : ticks_(ticks) {
  }

  
  int64_t ticks_;

#if defined(OS_WIN)
  typedef DWORD (*TickFunctionType)(void);
  static TickFunctionType SetMockTickFunction(TickFunctionType ticker);
#endif
};

inline TimeTicks TimeDelta::operator+(TimeTicks t) const {
  return TimeTicks(t.ticks_ + delta_);
}

}  

#endif  
