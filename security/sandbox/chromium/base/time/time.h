

























#ifndef BASE_TIME_TIME_H_
#define BASE_TIME_TIME_H_

#include <time.h>

#include <iosfwd>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "build/build_config.h"

#if defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>

#undef TYPE_BOOL
#endif

#if defined(OS_POSIX)
#include <unistd.h>
#include <sys/time.h>
#endif

#if defined(OS_WIN)


#include <windows.h>
#endif

#include <limits>

namespace base {

class Time;
class TimeTicks;



class BASE_EXPORT TimeDelta {
 public:
  TimeDelta() : delta_(0) {
  }

  
  static TimeDelta FromDays(int days);
  static TimeDelta FromHours(int hours);
  static TimeDelta FromMinutes(int minutes);
  static TimeDelta FromSeconds(int64 secs);
  static TimeDelta FromMilliseconds(int64 ms);
  static TimeDelta FromSecondsD(double secs);
  static TimeDelta FromMillisecondsD(double ms);
  static TimeDelta FromMicroseconds(int64 us);
#if defined(OS_WIN)
  static TimeDelta FromQPCValue(LONGLONG qpc_value);
#endif

  
  
  
  
  static TimeDelta FromInternalValue(int64 delta) {
    return TimeDelta(delta);
  }

  
  
  
  static TimeDelta Max();

  
  
  
  
  int64 ToInternalValue() const {
    return delta_;
  }

  
  bool is_max() const {
    return delta_ == std::numeric_limits<int64>::max();
  }

#if defined(OS_POSIX)
  struct timespec ToTimeSpec() const;
#endif

  
  
  
  
  
  int InDays() const;
  int InHours() const;
  int InMinutes() const;
  double InSecondsF() const;
  int64 InSeconds() const;
  double InMillisecondsF() const;
  int64 InMilliseconds() const;
  int64 InMillisecondsRoundedUp() const;
  int64 InMicroseconds() const;

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

  
  
  TimeDelta operator*(int64 a) const {
    return TimeDelta(delta_ * a);
  }
  TimeDelta operator/(int64 a) const {
    return TimeDelta(delta_ / a);
  }
  TimeDelta& operator*=(int64 a) {
    delta_ *= a;
    return *this;
  }
  TimeDelta& operator/=(int64 a) {
    delta_ /= a;
    return *this;
  }
  int64 operator/(TimeDelta a) const {
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
  friend TimeDelta operator*(int64 a, TimeDelta td);

  
  
  
  explicit TimeDelta(int64 delta_us) : delta_(delta_us) {
  }

  
  int64 delta_;
};

inline TimeDelta operator*(int64 a, TimeDelta td) {
  return TimeDelta(a * td.delta_);
}


BASE_EXPORT std::ostream& operator<<(std::ostream& os, TimeDelta time_delta);




class BASE_EXPORT Time {
 public:
  static const int64 kMillisecondsPerSecond = 1000;
  static const int64 kMicrosecondsPerMillisecond = 1000;
  static const int64 kMicrosecondsPerSecond = kMicrosecondsPerMillisecond *
                                              kMillisecondsPerSecond;
  static const int64 kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
  static const int64 kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
  static const int64 kMicrosecondsPerDay = kMicrosecondsPerHour * 24;
  static const int64 kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
  static const int64 kNanosecondsPerMicrosecond = 1000;
  static const int64 kNanosecondsPerSecond = kNanosecondsPerMicrosecond *
                                             kMicrosecondsPerSecond;

  
  
  static const int64 kTimeTToMicrosecondsOffset;

#if !defined(OS_WIN)
  
  
  
  
  
  static const int64 kWindowsEpochDeltaMicroseconds;
#else
  
  
  
  static const int64 kQPCOverflowThreshold = 0x8637BD05AF7;
#endif

  
  
  
  struct BASE_EXPORT Exploded {
    int year;          
    int month;         
    int day_of_week;   
    int day_of_month;  
    int hour;          
    int minute;        
    int second;        
                       
    int millisecond;   

    
    
    
    bool HasValidValues() const;
  };

  
  Time() : us_(0) {
  }

  
  bool is_null() const {
    return us_ == 0;
  }

  
  bool is_max() const {
    return us_ == std::numeric_limits<int64>::max();
  }

  
  static Time UnixEpoch();

  
  
  
  static Time Now();

  
  
  static Time Max();

  
  
  
  
  static Time NowFromSystemTime();

  
  
  
  static Time FromTimeT(time_t tt);
  time_t ToTimeT() const;

  
  
  
  
  
  static Time FromDoubleT(double dt);
  double ToDoubleT() const;

#if defined(OS_POSIX)
  
  
  
  
  static Time FromTimeSpec(const timespec& ts);
#endif

  
  
  
  static Time FromJsTime(double ms_since_epoch);
  double ToJsTime() const;

  
  
  int64 ToJavaTime() const;

#if defined(OS_POSIX)
  static Time FromTimeVal(struct timeval t);
  struct timeval ToTimeVal() const;
#endif

#if defined(OS_MACOSX)
  static Time FromCFAbsoluteTime(CFAbsoluteTime t);
  CFAbsoluteTime ToCFAbsoluteTime() const;
#endif

#if defined(OS_WIN)
  static Time FromFileTime(FILETIME ft);
  FILETIME ToFileTime() const;

  
  
  
  static const int kMinLowResolutionThresholdMs = 16;

  
  static void EnableHighResolutionTimer(bool enable);

  
  
  
  
  
  
  
  static bool ActivateHighResolutionTimer(bool activate);

  
  
  
  static bool IsHighResolutionTimerInUse();
#endif

  
  
  static Time FromUTCExploded(const Exploded& exploded) {
    return FromExploded(false, exploded);
  }
  static Time FromLocalExploded(const Exploded& exploded) {
    return FromExploded(true, exploded);
  }

  
  
  
  
  static Time FromInternalValue(int64 us) {
    return Time(us);
  }

  
  
  
  
  
  
  
  
  static bool FromString(const char* time_string, Time* parsed_time) {
    return FromStringInternal(time_string, true, parsed_time);
  }
  static bool FromUTCString(const char* time_string, Time* parsed_time) {
    return FromStringInternal(time_string, false, parsed_time);
  }

  
  
  
  int64 ToInternalValue() const {
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

  explicit Time(int64 us) : us_(us) {
  }

  
  
  void Explode(bool is_local, Exploded* exploded) const;

  
  
  static Time FromExploded(bool is_local, const Exploded& exploded);

  
  
  
  
  
  
  
  static bool FromStringInternal(const char* time_string,
                                 bool is_local,
                                 Time* parsed_time);

  
  int64 us_;
};




inline TimeDelta TimeDelta::FromDays(int days) {
  
  if (days == std::numeric_limits<int>::max())
    return Max();
  return TimeDelta(days * Time::kMicrosecondsPerDay);
}


inline TimeDelta TimeDelta::FromHours(int hours) {
  
  if (hours == std::numeric_limits<int>::max())
    return Max();
  return TimeDelta(hours * Time::kMicrosecondsPerHour);
}


inline TimeDelta TimeDelta::FromMinutes(int minutes) {
  
  if (minutes == std::numeric_limits<int>::max())
    return Max();
  return TimeDelta(minutes * Time::kMicrosecondsPerMinute);
}


inline TimeDelta TimeDelta::FromSeconds(int64 secs) {
  
  if (secs == std::numeric_limits<int64>::max())
    return Max();
  return TimeDelta(secs * Time::kMicrosecondsPerSecond);
}


inline TimeDelta TimeDelta::FromMilliseconds(int64 ms) {
  
  if (ms == std::numeric_limits<int64>::max())
    return Max();
  return TimeDelta(ms * Time::kMicrosecondsPerMillisecond);
}


inline TimeDelta TimeDelta::FromSecondsD(double secs) {
  
  if (secs == std::numeric_limits<double>::infinity())
    return Max();
  return TimeDelta(static_cast<int64>(secs * Time::kMicrosecondsPerSecond));
}


inline TimeDelta TimeDelta::FromMillisecondsD(double ms) {
  
  if (ms == std::numeric_limits<double>::infinity())
    return Max();
  return TimeDelta(static_cast<int64>(ms * Time::kMicrosecondsPerMillisecond));
}


inline TimeDelta TimeDelta::FromMicroseconds(int64 us) {
  
  if (us == std::numeric_limits<int64>::max())
    return Max();
  return TimeDelta(us);
}

inline Time TimeDelta::operator+(Time t) const {
  return Time(t.us_ + delta_);
}


BASE_EXPORT std::ostream& operator<<(std::ostream& os, Time time);



class BASE_EXPORT TimeTicks {
 public:
  
#if defined(OS_LINUX)
  
  
  
  static const clockid_t kClockSystemTrace = 11;
#endif

  TimeTicks() : ticks_(0) {
  }

  
  
  
  static TimeTicks Now();

  
  
  
  
  static TimeTicks HighResNow();

  static bool IsHighResNowFastAndReliable();

  
  static bool IsThreadNowSupported() {
#if (defined(_POSIX_THREAD_CPUTIME) && (_POSIX_THREAD_CPUTIME >= 0)) || \
    (defined(OS_MACOSX) && !defined(OS_IOS)) || defined(OS_ANDROID)
    return true;
#else
    return false;
#endif
  }

  
  
  
  
  
  static TimeTicks ThreadNow();

  
  
  
  
  
  static TimeTicks NowFromSystemTraceTime();

#if defined(OS_WIN)
  
  static int64 GetQPCDriftMicroseconds();

  static TimeTicks FromQPCValue(LONGLONG qpc_value);

  
  
  static bool IsHighResClockWorking();

  
  static TimeTicks UnprotectedNow();
#endif

  
  bool is_null() const {
    return ticks_ == 0;
  }

  
  
  
  
  static TimeTicks FromInternalValue(int64 ticks) {
    return TimeTicks(ticks);
  }

  
  
  
  
  
  
  static TimeTicks UnixEpoch();

  
  
  int64 ToInternalValue() const {
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

  
  
  explicit TimeTicks(int64 ticks) : ticks_(ticks) {
  }

  
  int64 ticks_;

#if defined(OS_WIN)
  typedef DWORD (*TickFunctionType)(void);
  static TickFunctionType SetMockTickFunction(TickFunctionType ticker);
#endif
};

inline TimeTicks TimeDelta::operator+(TimeTicks t) const {
  return TimeTicks(t.ticks_ + delta_);
}


BASE_EXPORT std::ostream& operator<<(std::ostream& os, TimeTicks time_ticks);

}  

#endif  
