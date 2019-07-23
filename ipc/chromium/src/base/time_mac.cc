



#include "base/time.h"

#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFTimeZone.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#include <time.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_cftyperef.h"

namespace base {
















const int64 Time::kTimeTToMicrosecondsOffset = GG_INT64_C(0);


Time Time::Now() {
  CFAbsoluteTime now =
      CFAbsoluteTimeGetCurrent() + kCFAbsoluteTimeIntervalSince1970;
  return Time(static_cast<int64>(now * kMicrosecondsPerSecond));
}


Time Time::NowFromSystemTime() {
  
  return Now();
}


Time Time::FromExploded(bool is_local, const Exploded& exploded) {
  CFGregorianDate date;
  date.second = exploded.second +
      exploded.millisecond / static_cast<double>(kMillisecondsPerSecond);
  date.minute = exploded.minute;
  date.hour = exploded.hour;
  date.day = exploded.day_of_month;
  date.month = exploded.month;
  date.year = exploded.year;

  scoped_cftyperef<CFTimeZoneRef>
      time_zone(is_local ? CFTimeZoneCopySystem() : NULL);
  CFAbsoluteTime seconds = CFGregorianDateGetAbsoluteTime(date, time_zone) +
      kCFAbsoluteTimeIntervalSince1970;
  return Time(static_cast<int64>(seconds * kMicrosecondsPerSecond));
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  CFAbsoluteTime seconds =
      (static_cast<double>(us_) / kMicrosecondsPerSecond) -
      kCFAbsoluteTimeIntervalSince1970;

  scoped_cftyperef<CFTimeZoneRef>
      time_zone(is_local ? CFTimeZoneCopySystem() : NULL);
  CFGregorianDate date = CFAbsoluteTimeGetGregorianDate(seconds, time_zone);

  exploded->year = date.year;
  exploded->month = date.month;
  exploded->day_of_month = date.day;
  exploded->hour = date.hour;
  exploded->minute = date.minute;
  exploded->second = date.second;
  exploded->millisecond  =
      static_cast<int>(date.second * kMillisecondsPerSecond) %
      kMillisecondsPerSecond;
}




TimeTicks TimeTicks::Now() {
  uint64_t absolute_micro;

  static mach_timebase_info_data_t timebase_info;
  if (timebase_info.denom == 0) {
    
    
    
    
    
    kern_return_t kr = mach_timebase_info(&timebase_info);
    DCHECK(kr == KERN_SUCCESS);
  }

  
  
  

  
  
  absolute_micro = mach_absolute_time() / Time::kNanosecondsPerMicrosecond *
                   timebase_info.numer / timebase_info.denom;

  
  
  

  return TimeTicks(absolute_micro);
}


TimeTicks TimeTicks::HighResNow() {
  return Now();
}

}  
