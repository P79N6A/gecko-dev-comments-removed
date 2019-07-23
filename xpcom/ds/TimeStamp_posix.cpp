














































#include <time.h>

#include "mozilla/TimeStamp.h"


static PRUint64 sResolution;
static PRUint64 sResolutionSigDigs;

static const PRUint16 kNsPerUs   =       1000;
static const PRUint64 kNsPerMs   =    1000000;
static const PRUint64 kNsPerSec  = 1000000000; 
static const double kNsPerSecd   = 1000000000.0;

static PRUint64
TimespecToNs(const struct timespec& ts)
{
  PRUint64 baseNs = PRUint64(ts.tv_sec) * kNsPerSec;
  return baseNs + PRUint64(ts.tv_nsec);
}

static PRUint64
ClockTimeNs()
{
  struct timespec ts;
  
  
  clock_gettime(CLOCK_MONOTONIC, &ts);

  
  
  
  
  
  
  return TimespecToNs(ts);
}

static PRUint64
ClockResolutionNs()
{
  
  
  
  
  
  

  PRUint64 start = ClockTimeNs();
  PRUint64 end = ClockTimeNs();
  PRUint64 minres = (end - start);

  
  
  
  for (int i = 0; i < 9; ++i) {
    start = ClockTimeNs();
    end = ClockTimeNs();

    PRUint64 candidate = (start - end);
    if (candidate < minres)
      minres = candidate;
  }

  if (0 == minres) {
    
    
    struct timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);

    minres = TimespecToNs(ts);
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
  return double(mValue) / kNsPerSecd;
}

double
TimeDuration::ToSecondsSigDigits() const
{
  
  PRInt64 valueSigDigs = sResolution * (mValue / sResolution);
  
  valueSigDigs = sResolutionSigDigs * (valueSigDigs / sResolutionSigDigs);
  return double(valueSigDigs) / kNsPerSecd;
}

TimeDuration
TimeDuration::FromSeconds(PRInt32 aSeconds)
{
  return TimeDuration::FromTicks((PRInt64(aSeconds) * PRInt64(kNsPerSec)));
}

TimeDuration
TimeDuration::FromMilliseconds(PRInt32 aMilliseconds)
{
  return TimeDuration::FromTicks(PRInt64(aMilliseconds) * PRInt64(kNsPerMs));
}

TimeDuration
TimeDuration::Resolution()
{
  return TimeDuration::FromTicks(sResolution);
}


nsresult
TimeStamp::Startup()
{
  struct timespec dummy;
  if (0 != clock_gettime(CLOCK_MONOTONIC, &dummy))
      NS_RUNTIMEABORT("CLOCK_MONOTONIC is absent!");

  sResolution = ClockResolutionNs();

  
  
  for (sResolutionSigDigs = 1;
       !(sResolutionSigDigs == sResolution
         || 10*sResolutionSigDigs > sResolution);
       sResolutionSigDigs *= 10);

  return NS_OK;
}

void
TimeStamp::Shutdown()
{
}

TimeStamp
TimeStamp::Now()
{
  return TimeStamp(ClockTimeNs());
}

}
