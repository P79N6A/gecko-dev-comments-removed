























#include "pkix/Time.h"
#include "pkixutil.h"
#ifdef WIN32
#include "windows.h"
#else
#include "sys/time.h"
#endif

namespace mozilla { namespace pkix {

Time
Now()
{
  uint64_t seconds;

#ifdef WIN32
  
  
  
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t ft64 = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) |
                  ft.dwLowDateTime;
  seconds = (DaysBeforeYear(1601) * Time::ONE_DAY_IN_SECONDS) +
            ft64 / (1000u * 1000u * 1000u / 100u);
#else
  
  
  
  timeval tv;
  (void) gettimeofday(&tv, nullptr);
  seconds = (DaysBeforeYear(1970) * Time::ONE_DAY_IN_SECONDS) + tv.tv_sec;
#endif

  return TimeFromElapsedSecondsAD(seconds);
}

Time
TimeFromEpochInSeconds(uint64_t secondsSinceEpoch)
{
  uint64_t seconds = (DaysBeforeYear(1970) * Time::ONE_DAY_IN_SECONDS) +
                     secondsSinceEpoch;
  return TimeFromElapsedSecondsAD(seconds);
}

} } 
