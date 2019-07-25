

















































#include <mach/mach_time.h>
#include <time.h>

#include "mozilla/TimeStamp.h"


static PRUint64 sResolution;
static PRUint64 sResolutionSigDigs;

static const PRUint16 kNsPerUs   =       1000;
static const PRUint64 kNsPerMs   =    1000000;
static const PRUint64 kNsPerSec  = 1000000000;
static const double kNsPerMsd    =    1000000.0;
static const double kNsPerSecd   = 1000000000.0;

static double sNsPerTick;

static PRUint64
ClockTime()
{
  
  
  
  
  
  
  
  return mach_absolute_time();
}

static PRUint64
ClockResolutionNs()
{
  PRUint64 start = ClockTime();
  PRUint64 end = ClockTime();
  PRUint64 minres = (end - start);

  
  
  
  for (int i = 0; i < 9; ++i) {
    start = ClockTime();
    end = ClockTime();

    PRUint64 candidate = (start - end);
    if (candidate < minres)
      minres = candidate;
  }

  if (0 == minres) {
    
    
    minres = 1 * kNsPerMs;
  }

  return minres;
}

namespace mozilla {

double
TimeDuration::ToSeconds() const
{
  return (mValue * sNsPerTick) / kNsPerSecd;
}

double
TimeDuration::ToSecondsSigDigits() const
{
  
  PRInt64 valueSigDigs = sResolution * (mValue / sResolution);
  
  valueSigDigs = sResolutionSigDigs * (valueSigDigs / sResolutionSigDigs);
  return (valueSigDigs * sNsPerTick) / kNsPerSecd;
}

TimeDuration
TimeDuration::FromMilliseconds(double aMilliseconds)
{
  return TimeDuration::FromTicks(PRInt64((aMilliseconds * kNsPerMsd) / sNsPerTick));
}

TimeDuration
TimeDuration::Resolution()
{
  return TimeDuration::FromTicks(PRInt64(sResolution));
}

struct TimeStampInitialization
{
  TimeStampInitialization() {
    TimeStamp::Startup();
  }
  ~TimeStampInitialization() {
    TimeStamp::Shutdown();
  }
};

static TimeStampInitialization initOnce;
static bool gInitialized = false;

nsresult
TimeStamp::Startup()
{
  if (gInitialized)
    return NS_OK;

  mach_timebase_info_data_t timebaseInfo;
  
  
  
  kern_return_t kr = mach_timebase_info(&timebaseInfo);
  if (kr != KERN_SUCCESS)
    NS_RUNTIMEABORT("mach_timebase_info failed");

  sNsPerTick = double(timebaseInfo.numer) / timebaseInfo.denom;

  sResolution = ClockResolutionNs();

  
  
  for (sResolutionSigDigs = 1;
       !(sResolutionSigDigs == sResolution
         || 10*sResolutionSigDigs > sResolution);
       sResolutionSigDigs *= 10);

  gInitialized = PR_TRUE;
  return NS_OK;
}

void
TimeStamp::Shutdown()
{
}

TimeStamp
TimeStamp::Now()
{
  return TimeStamp(ClockTime());
}

}
