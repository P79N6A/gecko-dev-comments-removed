

















#include <mach/mach_time.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <time.h>
#include <unistd.h>

#include "mozilla/TimeStamp.h"
#include "nsDebug.h"


static uint64_t sResolution;
static uint64_t sResolutionSigDigs;

static const uint64_t kNsPerMs   =    1000000;
static const uint64_t kUsPerSec  =    1000000;
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
    if (candidate < minres) {
      minres = candidate;
    }
  }

  if (0 == minres) {
    
    
    minres = 1 * kNsPerMs;
  }

  return minres;
}

namespace mozilla {

double
BaseTimeDurationPlatformUtils::ToSeconds(int64_t aTicks)
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  return (aTicks * sNsPerTick) / kNsPerSecd;
}

double
BaseTimeDurationPlatformUtils::ToSecondsSigDigits(int64_t aTicks)
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  
  int64_t valueSigDigs = sResolution * (aTicks / sResolution);
  
  valueSigDigs = sResolutionSigDigs * (valueSigDigs / sResolutionSigDigs);
  return (valueSigDigs * sNsPerTick) / kNsPerSecd;
}

int64_t
BaseTimeDurationPlatformUtils::TicksFromMilliseconds(double aMilliseconds)
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  double result = (aMilliseconds * kNsPerMsd) / sNsPerTick;
  if (result > INT64_MAX) {
    return INT64_MAX;
  } else if (result < INT64_MIN) {
    return INT64_MIN;
  }

  return result;
}

int64_t
BaseTimeDurationPlatformUtils::ResolutionInTicks()
{
  NS_ABORT_IF_FALSE(gInitialized, "calling TimeDuration too early");
  return static_cast<int64_t>(sResolution);
}

nsresult
TimeStamp::Startup()
{
  if (gInitialized) {
    return NS_OK;
  }

  mach_timebase_info_data_t timebaseInfo;
  
  
  
  kern_return_t kr = mach_timebase_info(&timebaseInfo);
  if (kr != KERN_SUCCESS) {
    NS_RUNTIMEABORT("mach_timebase_info failed");
  }

  sNsPerTick = double(timebaseInfo.numer) / timebaseInfo.denom;

  sResolution = ClockResolutionNs();

  
  
  for (sResolutionSigDigs = 1;
       !(sResolutionSigDigs == sResolution ||
         10 * sResolutionSigDigs > sResolution);
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




uint64_t
TimeStamp::ComputeProcessUptime()
{
  struct timeval tv;
  int rv = gettimeofday(&tv, nullptr);

  if (rv == -1) {
    return 0;
  }

  int mib[] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_PID,
    getpid(),
  };
  u_int mibLen = sizeof(mib) / sizeof(mib[0]);

  struct kinfo_proc proc;
  size_t bufferSize = sizeof(proc);
  rv = sysctl(mib, mibLen, &proc, &bufferSize, nullptr, 0);

  if (rv == -1) {
    return 0;
  }

  uint64_t startTime =
    ((uint64_t)proc.kp_proc.p_un.__p_starttime.tv_sec * kUsPerSec) +
    proc.kp_proc.p_un.__p_starttime.tv_usec;
  uint64_t now = (tv.tv_sec * kUsPerSec) + tv.tv_usec;

  if (startTime > now) {
    return 0;
  }

  return now - startTime;
}

} 
