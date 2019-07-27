



#include "base/time/time.h"

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#if defined(OS_ANDROID) && !defined(__LP64__)
#include <time64.h>
#endif
#include <unistd.h>

#include <limits>
#include <ostream>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/port.h"
#include "build/build_config.h"

#if defined(OS_ANDROID)
#include "base/os_compat_android.h"
#elif defined(OS_NACL)
#include "base/os_compat_nacl.h"
#endif

#if !defined(OS_MACOSX)
#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"
#endif

namespace {

#if !defined(OS_MACOSX)


base::LazyInstance<base::Lock>::Leaky
    g_sys_time_to_time_struct_lock = LAZY_INSTANCE_INITIALIZER;




#if defined(OS_ANDROID) && !defined(__LP64__)
typedef time64_t SysTime;

SysTime SysTimeFromTimeStruct(struct tm* timestruct, bool is_local) {
  base::AutoLock locked(g_sys_time_to_time_struct_lock.Get());
  if (is_local)
    return mktime64(timestruct);
  else
    return timegm64(timestruct);
}

void SysTimeToTimeStruct(SysTime t, struct tm* timestruct, bool is_local) {
  base::AutoLock locked(g_sys_time_to_time_struct_lock.Get());
  if (is_local)
    localtime64_r(&t, timestruct);
  else
    gmtime64_r(&t, timestruct);
}

#else  
typedef time_t SysTime;

SysTime SysTimeFromTimeStruct(struct tm* timestruct, bool is_local) {
  base::AutoLock locked(g_sys_time_to_time_struct_lock.Get());
  if (is_local)
    return mktime(timestruct);
  else
    return timegm(timestruct);
}

void SysTimeToTimeStruct(SysTime t, struct tm* timestruct, bool is_local) {
  base::AutoLock locked(g_sys_time_to_time_struct_lock.Get());
  if (is_local)
    localtime_r(&t, timestruct);
  else
    gmtime_r(&t, timestruct);
}
#endif  




#if (defined(OS_POSIX) &&                                               \
     defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0) || \
    defined(OS_BSD) || defined(OS_ANDROID)
base::TimeTicks ClockNow(clockid_t clk_id) {
  uint64_t absolute_micro;

  struct timespec ts;
  if (clock_gettime(clk_id, &ts) != 0) {
    NOTREACHED() << "clock_gettime(" << clk_id << ") failed.";
    return base::TimeTicks();
  }

  absolute_micro =
      (static_cast<int64>(ts.tv_sec) * base::Time::kMicrosecondsPerSecond) +
      (static_cast<int64>(ts.tv_nsec / base::Time::kNanosecondsPerMicrosecond));

  return base::TimeTicks::FromInternalValue(absolute_micro);
}
#else  
#error No usable tick clock function on this platform.
#endif  
#endif  

}  

namespace base {

struct timespec TimeDelta::ToTimeSpec() const {
  int64 microseconds = InMicroseconds();
  time_t seconds = 0;
  if (microseconds >= Time::kMicrosecondsPerSecond) {
    seconds = InSeconds();
    microseconds -= seconds * Time::kMicrosecondsPerSecond;
  }
  struct timespec result =
      {seconds,
       static_cast<long>(microseconds * Time::kNanosecondsPerMicrosecond)};
  return result;
}

#if !defined(OS_MACOSX)












static const int64 kWindowsEpochDeltaSeconds = GG_INT64_C(11644473600);


const int64 Time::kWindowsEpochDeltaMicroseconds =
    kWindowsEpochDeltaSeconds * Time::kMicrosecondsPerSecond;




const int64 Time::kTimeTToMicrosecondsOffset = kWindowsEpochDeltaMicroseconds;


Time Time::Now() {
  struct timeval tv;
  struct timezone tz = { 0, 0 };  
  if (gettimeofday(&tv, &tz) != 0) {
    DCHECK(0) << "Could not determine time of day";
    PLOG(ERROR) << "Call to gettimeofday failed.";
    
    
    return Time();
  }
  
  
  
  return Time((tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec) +
      kWindowsEpochDeltaMicroseconds);
}


Time Time::NowFromSystemTime() {
  
  return Now();
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  
  
  
  int64 microseconds = us_ - kWindowsEpochDeltaMicroseconds;
  
  int64 milliseconds;  
  SysTime seconds;  
  int millisecond;  
  if (microseconds >= 0) {
    
    milliseconds = microseconds / kMicrosecondsPerMillisecond;
    seconds = milliseconds / kMillisecondsPerSecond;
    millisecond = milliseconds % kMillisecondsPerSecond;
  } else {
    
    milliseconds = (microseconds - kMicrosecondsPerMillisecond + 1) /
                   kMicrosecondsPerMillisecond;
    seconds = (milliseconds - kMillisecondsPerSecond + 1) /
              kMillisecondsPerSecond;
    
    millisecond = milliseconds % kMillisecondsPerSecond;
    if (millisecond < 0)
      millisecond += kMillisecondsPerSecond;
  }

  struct tm timestruct;
  SysTimeToTimeStruct(seconds, &timestruct, is_local);

  exploded->year         = timestruct.tm_year + 1900;
  exploded->month        = timestruct.tm_mon + 1;
  exploded->day_of_week  = timestruct.tm_wday;
  exploded->day_of_month = timestruct.tm_mday;
  exploded->hour         = timestruct.tm_hour;
  exploded->minute       = timestruct.tm_min;
  exploded->second       = timestruct.tm_sec;
  exploded->millisecond  = millisecond;
}


Time Time::FromExploded(bool is_local, const Exploded& exploded) {
  struct tm timestruct;
  timestruct.tm_sec    = exploded.second;
  timestruct.tm_min    = exploded.minute;
  timestruct.tm_hour   = exploded.hour;
  timestruct.tm_mday   = exploded.day_of_month;
  timestruct.tm_mon    = exploded.month - 1;
  timestruct.tm_year   = exploded.year - 1900;
  timestruct.tm_wday   = exploded.day_of_week;  
  timestruct.tm_yday   = 0;     
  timestruct.tm_isdst  = -1;    
#if !defined(OS_NACL) && !defined(OS_SOLARIS)
  timestruct.tm_gmtoff = 0;     
  timestruct.tm_zone   = NULL;  
#endif


  int64 milliseconds;
  SysTime seconds;

  
  
  
  
  

  
  struct tm timestruct0 = timestruct;

  seconds = SysTimeFromTimeStruct(&timestruct, is_local);
  if (seconds == -1) {
    
    
    timestruct = timestruct0;
    timestruct.tm_isdst = 0;
    int64 seconds_isdst0 = SysTimeFromTimeStruct(&timestruct, is_local);

    timestruct = timestruct0;
    timestruct.tm_isdst = 1;
    int64 seconds_isdst1 = SysTimeFromTimeStruct(&timestruct, is_local);

    
    
    if (seconds_isdst0 < 0)
      seconds = seconds_isdst1;
    else if (seconds_isdst1 < 0)
      seconds = seconds_isdst0;
    else
      seconds = std::min(seconds_isdst0, seconds_isdst1);
  }

  
  
  
  
  if (seconds == -1 &&
      (exploded.year < 1969 || exploded.year > 1970)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    const int64 min_seconds = (sizeof(SysTime) < sizeof(int64))
                                  ? std::numeric_limits<SysTime>::min()
                                  : std::numeric_limits<int32_t>::min();
    const int64 max_seconds = (sizeof(SysTime) < sizeof(int64))
                                  ? std::numeric_limits<SysTime>::max()
                                  : std::numeric_limits<int32_t>::max();
    if (exploded.year < 1969) {
      milliseconds = min_seconds * kMillisecondsPerSecond;
    } else {
      milliseconds = max_seconds * kMillisecondsPerSecond;
      milliseconds += (kMillisecondsPerSecond - 1);
    }
  } else {
    milliseconds = seconds * kMillisecondsPerSecond + exploded.millisecond;
  }

  
  return Time((milliseconds * kMicrosecondsPerMillisecond) +
      kWindowsEpochDeltaMicroseconds);
}



TimeTicks TimeTicks::Now() {
  return ClockNow(CLOCK_MONOTONIC);
}


TimeTicks TimeTicks::HighResNow() {
  return Now();
}


bool TimeTicks::IsHighResNowFastAndReliable() {
  return true;
}


TimeTicks TimeTicks::ThreadNow() {
#if (defined(_POSIX_THREAD_CPUTIME) && (_POSIX_THREAD_CPUTIME >= 0)) || \
    defined(OS_ANDROID)
  return ClockNow(CLOCK_THREAD_CPUTIME_ID);
#else
  NOTREACHED();
  return TimeTicks();
#endif
}


#if defined(OS_CHROMEOS)

TimeTicks TimeTicks::NowFromSystemTraceTime() {
  uint64_t absolute_micro;

  struct timespec ts;
  if (clock_gettime(kClockSystemTrace, &ts) != 0) {
    
    return HighResNow();
  }

  absolute_micro =
      (static_cast<int64>(ts.tv_sec) * Time::kMicrosecondsPerSecond) +
      (static_cast<int64>(ts.tv_nsec) / Time::kNanosecondsPerMicrosecond);

  return TimeTicks(absolute_micro);
}

#else  


TimeTicks TimeTicks::NowFromSystemTraceTime() {
  return HighResNow();
}

#endif  

#endif  


Time Time::FromTimeVal(struct timeval t) {
  DCHECK_LT(t.tv_usec, static_cast<int>(Time::kMicrosecondsPerSecond));
  DCHECK_GE(t.tv_usec, 0);
  if (t.tv_usec == 0 && t.tv_sec == 0)
    return Time();
  if (t.tv_usec == static_cast<suseconds_t>(Time::kMicrosecondsPerSecond) - 1 &&
      t.tv_sec == std::numeric_limits<time_t>::max())
    return Max();
  return Time(
      (static_cast<int64>(t.tv_sec) * Time::kMicrosecondsPerSecond) +
      t.tv_usec +
      kTimeTToMicrosecondsOffset);
}

struct timeval Time::ToTimeVal() const {
  struct timeval result;
  if (is_null()) {
    result.tv_sec = 0;
    result.tv_usec = 0;
    return result;
  }
  if (is_max()) {
    result.tv_sec = std::numeric_limits<time_t>::max();
    result.tv_usec = static_cast<suseconds_t>(Time::kMicrosecondsPerSecond) - 1;
    return result;
  }
  int64 us = us_ - kTimeTToMicrosecondsOffset;
  result.tv_sec = us / Time::kMicrosecondsPerSecond;
  result.tv_usec = us % Time::kMicrosecondsPerSecond;
  return result;
}

}  
