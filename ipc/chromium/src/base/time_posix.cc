



#include "base/time.h"

#ifdef OS_MACOSX
#include <mach/mach_time.h>
#endif
#include <sys/time.h>
#include <time.h>

#include <limits>

#include "base/basictypes.h"
#include "base/logging.h"

namespace base {










const int64 Time::kTimeTToMicrosecondsOffset = GG_INT64_C(0);


Time Time::Now() {
  struct timeval tv;
  struct timezone tz = { 0, 0 };  
  if (gettimeofday(&tv, &tz) != 0) {
    DCHECK(0) << "Could not determine time of day";
  }
  
  
  return tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec;
}


Time Time::NowFromSystemTime() {
  
  return Now();
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
  timestruct.tm_gmtoff = 0;     
  timestruct.tm_zone   = NULL;  

  time_t seconds;
  if (is_local)
    seconds = mktime(&timestruct);
  else
    seconds = timegm(&timestruct);

  int64 milliseconds;
  
  
  
  
  if (seconds == -1 &&
      (exploded.year < 1969 || exploded.year > 1970)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (exploded.year < 1969) {
      milliseconds = std::numeric_limits<time_t>::min() *
                     kMillisecondsPerSecond;
    } else {
      milliseconds = (std::numeric_limits<time_t>::max() *
                      kMillisecondsPerSecond) +
                     kMillisecondsPerSecond - 1;
    }
  } else {
    milliseconds = seconds * kMillisecondsPerSecond + exploded.millisecond;
  }

  return Time(milliseconds * kMicrosecondsPerMillisecond);
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  
  
  int64 milliseconds = us_ / kMicrosecondsPerMillisecond;
  time_t seconds = milliseconds / kMillisecondsPerSecond;

  struct tm timestruct;
  if (is_local)
    localtime_r(&seconds, &timestruct);
  else
    gmtime_r(&seconds, &timestruct);

  exploded->year         = timestruct.tm_year + 1900;
  exploded->month        = timestruct.tm_mon + 1;
  exploded->day_of_week  = timestruct.tm_wday;
  exploded->day_of_month = timestruct.tm_mday;
  exploded->hour         = timestruct.tm_hour;
  exploded->minute       = timestruct.tm_min;
  exploded->second       = timestruct.tm_sec;
  exploded->millisecond  = milliseconds % kMillisecondsPerSecond;
}




TimeTicks TimeTicks::Now() {
  uint64_t absolute_micro;

#if defined(OS_MACOSX)
  static mach_timebase_info_data_t timebase_info;
  if (timebase_info.denom == 0) {
    
    
    
    
    
    kern_return_t kr = mach_timebase_info(&timebase_info);
    DCHECK(kr == KERN_SUCCESS);
  }

  
  
  

  
  
  absolute_micro = mach_absolute_time() / Time::kNanosecondsPerMicrosecond *
                   timebase_info.numer / timebase_info.denom;

  
  
  

#elif defined(OS_POSIX) && \
      defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0

  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    NOTREACHED() << "clock_gettime(CLOCK_MONOTONIC) failed.";
    return TimeTicks();
  }

  absolute_micro =
      (static_cast<int64>(ts.tv_sec) * Time::kMicrosecondsPerSecond) +
      (static_cast<int64>(ts.tv_nsec) / Time::kNanosecondsPerMicrosecond);

#else  
#error No usable tick clock function on this platform.
#endif  

  return TimeTicks(absolute_micro);
}


TimeTicks TimeTicks::HighResNow() {
  return Now();
}

}  
