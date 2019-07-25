














#include <time.h>

#include "mozilla/TimeStamp.h"


static uint64_t sResolution;
static uint64_t sResolutionSigDigs;

static const uint16_t kNsPerUs   =       1000;
static const uint64_t kNsPerMs   =    1000000;
static const uint64_t kNsPerSec  = 1000000000; 
static const double kNsPerMsd    =    1000000.0;
static const double kNsPerSecd   = 1000000000.0;

static uint64_t
TimespecToNs(const struct timespec& ts)
{
  uint64_t baseNs = uint64_t(ts.tv_sec) * kNsPerSec;
  return baseNs + uint64_t(ts.tv_nsec);
}

static uint64_t
ClockTimeNs()
{
  struct timespec ts;
  
  
  clock_gettime(CLOCK_MONOTONIC, &ts);

  
  
  
  
  
  
  return TimespecToNs(ts);
}

static uint64_t
ClockResolutionNs()
{
  
  
  
  
  
  

  uint64_t start = ClockTimeNs();
  uint64_t end = ClockTimeNs();
  uint64_t minres = (end - start);

  
  
  
  for (int i = 0; i < 9; ++i) {
    start = ClockTimeNs();
    end = ClockTimeNs();

    uint64_t candidate = (start - end);
    if (candidate < minres)
      minres = candidate;
  }

  if (0 == minres) {
    
    
    struct timespec ts;
    if (0 == clock_getres(CLOCK_MONOTONIC, &ts)) {
      minres = TimespecToNs(ts);
    }
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
  
  int64_t valueSigDigs = sResolution * (mValue / sResolution);
  
  valueSigDigs = sResolutionSigDigs * (valueSigDigs / sResolutionSigDigs);
  return double(valueSigDigs) / kNsPerSecd;
}

TimeDuration
TimeDuration::FromMilliseconds(double aMilliseconds)
{
  return TimeDuration::FromTicks(aMilliseconds * kNsPerMsd);
}

TimeDuration
TimeDuration::Resolution()
{
  return TimeDuration::FromTicks(int64_t(sResolution));
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

  struct timespec dummy;
  if (0 != clock_gettime(CLOCK_MONOTONIC, &dummy))
      NS_RUNTIMEABORT("CLOCK_MONOTONIC is absent!");

  sResolution = ClockResolutionNs();

  
  
  for (sResolutionSigDigs = 1;
       !(sResolutionSigDigs == sResolution
         || 10*sResolutionSigDigs > sResolution);
       sResolutionSigDigs *= 10);

  gInitialized = true;
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
