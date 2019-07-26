

















#include <mach/mach_time.h>
#include <time.h>

#include "mozilla/TimeStamp.h"


static uint64_t sResolution;
static uint64_t sResolutionSigDigs;

static const uint16_t kNsPerUs   =       1000;
static const uint64_t kNsPerMs   =    1000000;
static const uint64_t kNsPerSec  = 1000000000;
static const double kNsPerMsd    =    1000000.0;
static const double kNsPerSecd   = 1000000000.0;

static bool gInitialized = false;
static double sNsPerTick;

static uint64_t
ClockTime()
{
  
  
  
  
  
  
  
  return mach_absolute_time();
}

static uint64_t
ClockResolutionNs()
{
  uint64_t start = ClockTime();
  uint64_t end = ClockTime();
  uint64_t minres = (end - start);

  
  
  
  for (int i = 0; i < 9; ++i) {
    start = ClockTime();
    end = ClockTime();

    uint64_t candidate = (start - end);
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
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  return (mValue * sNsPerTick) / kNsPerSecd;
}

double
TimeDuration::ToSecondsSigDigits() const
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  
  int64_t valueSigDigs = sResolution * (mValue / sResolution);
  
  valueSigDigs = sResolutionSigDigs * (valueSigDigs / sResolutionSigDigs);
  return (valueSigDigs * sNsPerTick) / kNsPerSecd;
}

TimeDuration
TimeDuration::FromMilliseconds(double aMilliseconds)
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  return TimeDuration::FromTicks(int64_t((aMilliseconds * kNsPerMsd) / sNsPerTick));
}

TimeDuration
TimeDuration::Resolution()
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
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

  gInitialized = true;
  return NS_OK;
}

void
TimeStamp::Shutdown()
{
}

TimeStamp
TimeStamp::Now(bool aHighResolution)
{
  return TimeStamp(ClockTime());
}

}
