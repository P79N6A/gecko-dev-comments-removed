














#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#if defined(__DragonFly__) || defined(__FreeBSD__) \
    || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if defined(__DragonFly__) || defined(__FreeBSD__)
#include <sys/user.h>
#endif

#if defined(__NetBSD__)
#undef KERN_PROC
#define KERN_PROC KERN_PROC2
#define KINFO_PROC struct kinfo_proc2
#else
#define KINFO_PROC struct kinfo_proc
#endif

#if defined(__DragonFly__)
#define KP_START_SEC kp_start.tv_sec
#define KP_START_USEC kp_start.tv_usec
#elif defined(__FreeBSD__)
#define KP_START_SEC ki_start.tv_sec
#define KP_START_USEC ki_start.tv_usec
#else
#define KP_START_SEC p_ustart_sec
#define KP_START_USEC p_ustart_usec
#endif

#include "mozilla/Snprintf.h"
#include "mozilla/TimeStamp.h"
#include <pthread.h>


static uint64_t sResolution;
static uint64_t sResolutionSigDigs;

static const uint16_t kNsPerUs   =       1000;
static const uint64_t kNsPerMs   =    1000000;
static const uint64_t kNsPerSec  = 1000000000;
static const double kNsPerMsd    =    1000000.0;
static const double kNsPerSecd   = 1000000000.0;

static uint64_t
TimespecToNs(const struct timespec& aTs)
{
  uint64_t baseNs = uint64_t(aTs.tv_sec) * kNsPerSec;
  return baseNs + uint64_t(aTs.tv_nsec);
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
    if (candidate < minres) {
      minres = candidate;
    }
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
BaseTimeDurationPlatformUtils::ToSeconds(int64_t aTicks)
{
  return double(aTicks) / kNsPerSecd;
}

double
BaseTimeDurationPlatformUtils::ToSecondsSigDigits(int64_t aTicks)
{
  
  int64_t valueSigDigs = sResolution * (aTicks / sResolution);
  
  valueSigDigs = sResolutionSigDigs * (valueSigDigs / sResolutionSigDigs);
  return double(valueSigDigs) / kNsPerSecd;
}

int64_t
BaseTimeDurationPlatformUtils::TicksFromMilliseconds(double aMilliseconds)
{
  double result = aMilliseconds * kNsPerMsd;
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
  return static_cast<int64_t>(sResolution);
}

static bool gInitialized = false;

void
TimeStamp::Startup()
{
  if (gInitialized) {
    return;
  }

  struct timespec dummy;
  if (clock_gettime(CLOCK_MONOTONIC, &dummy) != 0) {
    MOZ_CRASH("CLOCK_MONOTONIC is absent!");
  }

  sResolution = ClockResolutionNs();

  
  
  for (sResolutionSigDigs = 1;
       !(sResolutionSigDigs == sResolution ||
         10 * sResolutionSigDigs > sResolution);
       sResolutionSigDigs *= 10);

  gInitialized = true;

  return;
}

void
TimeStamp::Shutdown()
{
}

TimeStamp
TimeStamp::Now(bool aHighResolution)
{
  return TimeStamp(ClockTimeNs());
}

#if defined(XP_LINUX) || defined(ANDROID)





static uint64_t
JiffiesSinceBoot(const char* aFile)
{
  char stat[512];

  FILE* f = fopen(aFile, "r");
  if (!f) {
    return 0;
  }

  int n = fread(&stat, 1, sizeof(stat) - 1, f);

  fclose(f);

  if (n <= 0) {
    return 0;
  }

  stat[n] = 0;

  long long unsigned startTime = 0; 
  char* s = strrchr(stat, ')');

  if (!s) {
    return 0;
  }

  int rv = sscanf(s + 2,
                  "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u "
                  "%*u %*u %*u %*d %*d %*d %*d %*d %*d %llu",
                  &startTime);

  if (rv != 1 || !startTime) {
    return 0;
  }

  return startTime;
}







static void*
ComputeProcessUptimeThread(void* aTime)
{
  uint64_t* uptime = static_cast<uint64_t*>(aTime);
  long hz = sysconf(_SC_CLK_TCK);

  *uptime = 0;

  if (!hz) {
    return nullptr;
  }

  char threadStat[40];
  snprintf_literal(threadStat, "/proc/self/task/%d/stat", (pid_t)syscall(__NR_gettid));

  uint64_t threadJiffies = JiffiesSinceBoot(threadStat);
  uint64_t selfJiffies = JiffiesSinceBoot("/proc/self/stat");

  if (!threadJiffies || !selfJiffies) {
    return nullptr;
  }

  *uptime = ((threadJiffies - selfJiffies) * kNsPerSec) / hz;
  return nullptr;
}




uint64_t
TimeStamp::ComputeProcessUptime()
{
  uint64_t uptime = 0;
  pthread_t uptime_pthread;

  if (pthread_create(&uptime_pthread, nullptr, ComputeProcessUptimeThread, &uptime)) {
    MOZ_CRASH("Failed to create process uptime thread.");
    return 0;
  }

  pthread_join(uptime_pthread, NULL);

  return uptime / kNsPerUs;
}

#elif defined(__DragonFly__) || defined(__FreeBSD__) \
      || defined(__NetBSD__) || defined(__OpenBSD__)




uint64_t
TimeStamp::ComputeProcessUptime()
{
  struct timespec ts;
  int rv = clock_gettime(CLOCK_REALTIME, &ts);

  if (rv == -1) {
    return 0;
  }

  int mib[] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_PID,
    getpid(),
#if defined(__NetBSD__) || defined(__OpenBSD__)
    sizeof(KINFO_PROC),
    1,
#endif
  };
  u_int mibLen = sizeof(mib) / sizeof(mib[0]);

  KINFO_PROC proc;
  size_t bufferSize = sizeof(proc);
  rv = sysctl(mib, mibLen, &proc, &bufferSize, nullptr, 0);

  if (rv == -1) {
    return 0;
  }

  uint64_t startTime = ((uint64_t)proc.KP_START_SEC * kNsPerSec) +
    (proc.KP_START_USEC * kNsPerUs);
  uint64_t now = ((uint64_t)ts.tv_sec * kNsPerSec) + ts.tv_nsec;

  if (startTime > now) {
    return 0;
  }

  return (now - startTime) / kNsPerUs;
}

#else

uint64_t
TimeStamp::ComputeProcessUptime()
{
  return 0;
}

#endif

} 
