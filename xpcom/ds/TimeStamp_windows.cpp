








#include "mozilla/MathAlgorithms.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsWindowsHelpers.h"
#include <windows.h>

#include "nsCRT.h"
#include "prlog.h"
#include "prprf.h"
#include <stdio.h>

#include <intrin.h>

#if defined(PR_LOGGING)









static PRLogModuleInfo*
GetTimeStampLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("TimeStampWindows");
  }
  return sLog;
}
  #define LOG(x)  PR_LOG(GetTimeStampLog(), PR_LOG_DEBUG, x)
#else
  #define LOG(x)
#endif 


static volatile ULONGLONG sResolution;
static volatile ULONGLONG sResolutionSigDigs;
static const double   kNsPerSecd  = 1000000000.0;
static const LONGLONG kNsPerSec   = 1000000000;
static const LONGLONG kNsPerMillisec = 1000000;









static const uint32_t kFailureFreeInterval = 5000;

static const uint32_t kMaxFailuresPerInterval = 4;


static const uint32_t kFailureThreshold = 50;



static const DWORD kDefaultTimeIncrement = 156001;

















#define ms2mt(x) ((x) * sFrequencyPerSec)
#define mt2ms(x) ((x) / sFrequencyPerSec)
#define mt2ms_f(x) (double(x) / sFrequencyPerSec)


static LONGLONG sFrequencyPerSec = 0;




static const LONGLONG kGTCTickLeapTolerance = 4;












static LONGLONG sGTCResulutionThreshold;







static const uint32_t kHardFailureLimit = 2000;

static LONGLONG sHardFailureLimit;


static LONGLONG sFailureFreeInterval;
static LONGLONG sFailureThreshold;






static bool sHasStableTSC = false;







static bool volatile sUseQPC = true;







static const DWORD kLockSpinCount = 4096;




static CRITICAL_SECTION sTimeStampLock;














static ULONGLONG sFaultIntoleranceCheckpoint = 0;





static DWORD sLastGTCResult = 0;



static DWORD sLastGTCRollover = 0;

namespace mozilla {

typedef ULONGLONG (WINAPI* GetTickCount64_t)();
static GetTickCount64_t sGetTickCount64 = nullptr;



static ULONGLONG WINAPI
MozGetTickCount64()
{
  DWORD GTC = ::GetTickCount();

  
  AutoCriticalSection lock(&sTimeStampLock);

  
  
  if ((sLastGTCResult > GTC) && ((sLastGTCResult - GTC) > (1UL << 30))) {
    ++sLastGTCRollover;
  }

  sLastGTCResult = GTC;
  return ULONGLONG(sLastGTCRollover) << 32 | sLastGTCResult;
}


static inline ULONGLONG
PerformanceCounter()
{
  LARGE_INTEGER pc;
  ::QueryPerformanceCounter(&pc);
  return pc.QuadPart * 1000ULL;
}

static void
InitThresholds()
{
  DWORD timeAdjustment = 0, timeIncrement = 0;
  BOOL timeAdjustmentDisabled;
  GetSystemTimeAdjustment(&timeAdjustment,
                          &timeIncrement,
                          &timeAdjustmentDisabled);

  LOG(("TimeStamp: timeIncrement=%d [100ns]", timeIncrement));

  if (!timeIncrement) {
    timeIncrement = kDefaultTimeIncrement;
  }

  
  
  DWORD timeIncrementCeil = timeIncrement;
  
  timeIncrementCeil -= 1;
  
  timeIncrementCeil /= 10000;
  
  timeIncrementCeil += 1;
  
  timeIncrementCeil *= 10000;

  
  LONGLONG ticksPerGetTickCountResolutionCeiling =
    (int64_t(timeIncrementCeil) * sFrequencyPerSec) / 10000LL;

  
  sGTCResulutionThreshold =
    LONGLONG(kGTCTickLeapTolerance * ticksPerGetTickCountResolutionCeiling);

  sHardFailureLimit = ms2mt(kHardFailureLimit);
  sFailureFreeInterval = ms2mt(kFailureFreeInterval);
  sFailureThreshold = ms2mt(kFailureThreshold);
}

static void
InitResolution()
{
  
  
  

  ULONGLONG minres = ~0ULL;
  int loops = 10;
  do {
    ULONGLONG start = PerformanceCounter();
    ULONGLONG end = PerformanceCounter();

    ULONGLONG candidate = (end - start);
    if (candidate < minres) {
      minres = candidate;
    }
  } while (--loops && minres);

  if (0 == minres) {
    minres = 1;
  }

  
  
  ULONGLONG result = mt2ms(minres * kNsPerMillisec);
  if (0 == result) {
    result = 1;
  }

  sResolution = result;

  
  
  ULONGLONG sigDigs;
  for (sigDigs = 1;
       !(sigDigs == result || 10 * sigDigs > result);
       sigDigs *= 10);

  sResolutionSigDigs = sigDigs;
}





TimeStampValue::TimeStampValue(ULONGLONG aGTC, ULONGLONG aQPC, bool aHasQPC)
  : mGTC(aGTC)
  , mQPC(aQPC)
  , mHasQPC(aHasQPC)
  , mIsNull(false)
{
}

TimeStampValue&
TimeStampValue::operator+=(const int64_t aOther)
{
  mGTC += aOther;
  mQPC += aOther;
  return *this;
}

TimeStampValue&
TimeStampValue::operator-=(const int64_t aOther)
{
  mGTC -= aOther;
  mQPC -= aOther;
  return *this;
}



uint64_t
TimeStampValue::CheckQPC(const TimeStampValue& aOther) const
{
  uint64_t deltaGTC = mGTC - aOther.mGTC;

  if (!mHasQPC || !aOther.mHasQPC) { 
    return deltaGTC;
  }

  uint64_t deltaQPC = mQPC - aOther.mQPC;

  if (sHasStableTSC) { 
    return deltaQPC;
  }

  
  int64_t diff = DeprecatedAbs(int64_t(deltaQPC) - int64_t(deltaGTC));
  if (diff <= sGTCResulutionThreshold) {
    return deltaQPC;
  }

  
  int64_t duration = DeprecatedAbs(int64_t(deltaGTC));
  int64_t overflow = diff - sGTCResulutionThreshold;

  LOG(("TimeStamp: QPC check after %llums with overflow %1.4fms",
       mt2ms(duration), mt2ms_f(overflow)));

  if (overflow <= sFailureThreshold) {  
    return deltaQPC;
  }

  

  if (!sUseQPC) { 
    return deltaGTC;
  }

  LOG(("TimeStamp: QPC jittered over failure threshold"));

  if (duration < sHardFailureLimit) {
    
    
    uint64_t now = ms2mt(sGetTickCount64());

    AutoCriticalSection lock(&sTimeStampLock);

    if (sFaultIntoleranceCheckpoint && sFaultIntoleranceCheckpoint > now) {
      
      
      
      uint64_t failureCount =
        (sFaultIntoleranceCheckpoint - now + sFailureFreeInterval - 1) /
        sFailureFreeInterval;
      if (failureCount > kMaxFailuresPerInterval) {
        sUseQPC = false;
        LOG(("TimeStamp: QPC disabled"));
      } else {
        
        
        ++failureCount;
        sFaultIntoleranceCheckpoint = now + failureCount * sFailureFreeInterval;
        LOG(("TimeStamp: recording %dth QPC failure", failureCount));
      }
    } else {
      
      sFaultIntoleranceCheckpoint = now + sFailureFreeInterval;
      LOG(("TimeStamp: recording 1st QPC failure"));
    }
  }

  return deltaGTC;
}

uint64_t
TimeStampValue::operator-(const TimeStampValue& aOther) const
{
  if (mIsNull && aOther.mIsNull) {
    return uint64_t(0);
  }

  return CheckQPC(aOther);
}





double
TimeDuration::ToSeconds() const
{
  
  return double(mValue) / (double(sFrequencyPerSec) * 1000.0);
}

double
TimeDuration::ToSecondsSigDigits() const
{
  
  LONGLONG resolution = sResolution;
  LONGLONG resolutionSigDigs = sResolutionSigDigs;
  LONGLONG valueSigDigs = resolution * (mValue / resolution);
  
  valueSigDigs = resolutionSigDigs * (valueSigDigs / resolutionSigDigs);
  return double(valueSigDigs) / kNsPerSecd;
}

TimeDuration
TimeDuration::FromMilliseconds(double aMilliseconds)
{
  return TimeDuration::FromTicks(ms2mt(aMilliseconds));
}

TimeDuration
TimeDuration::Resolution()
{
  return TimeDuration::FromTicks(int64_t(sResolution));
}

static bool
HasStableTSC()
{
  union
  {
    int regs[4];
    struct
    {
      int nIds;
      char cpuString[12];
    };
  } cpuInfo;

  __cpuid(cpuInfo.regs, 0);
  
  
  
  if (_strnicmp(cpuInfo.cpuString, "GenuntelineI",
                sizeof(cpuInfo.cpuString))) {
    return false;
  }

  int regs[4];

  
  __cpuid(regs, 0x80000000);
  if (regs[0] < 0x80000007) {
    return false;
  }

  __cpuid(regs, 0x80000007);
  
  
  return regs[3] & (1 << 8);
}

nsresult
TimeStamp::Startup()
{
  

  HMODULE kernelDLL = GetModuleHandleW(L"kernel32.dll");
  sGetTickCount64 = reinterpret_cast<GetTickCount64_t>(
    GetProcAddress(kernelDLL, "GetTickCount64"));
  if (!sGetTickCount64) {
    
    
    sGetTickCount64 = MozGetTickCount64;
  }

  InitializeCriticalSectionAndSpinCount(&sTimeStampLock, kLockSpinCount);

  sHasStableTSC = HasStableTSC();
  LOG(("TimeStamp: HasStableTSC=%d", sHasStableTSC));

  LARGE_INTEGER freq;
  sUseQPC = ::QueryPerformanceFrequency(&freq);
  if (!sUseQPC) {
    
    InitResolution();

    LOG(("TimeStamp: using GetTickCount"));
    return NS_OK;
  }

  sFrequencyPerSec = freq.QuadPart;
  LOG(("TimeStamp: QPC frequency=%llu", sFrequencyPerSec));

  InitThresholds();
  InitResolution();

  return NS_OK;
}

void
TimeStamp::Shutdown()
{
  DeleteCriticalSection(&sTimeStampLock);
}

TimeStamp
TimeStamp::Now(bool aHighResolution)
{
  
  bool useQPC = (aHighResolution && sUseQPC);

  
  ULONGLONG QPC = useQPC ? PerformanceCounter() : uint64_t(0);
  ULONGLONG GTC = ms2mt(sGetTickCount64());
  return TimeStamp(TimeStampValue(GTC, QPC, useQPC));
}




uint64_t
TimeStamp::ComputeProcessUptime()
{
  SYSTEMTIME nowSys;
  GetSystemTime(&nowSys);

  FILETIME now;
  bool success = SystemTimeToFileTime(&nowSys, &now);

  if (!success) {
    return 0;
  }

  FILETIME start, foo, bar, baz;
  success = GetProcessTimes(GetCurrentProcess(), &start, &foo, &bar, &baz);

  if (!success) {
    return 0;
  }

  ULARGE_INTEGER startUsec = {
    start.dwLowDateTime,
    start.dwHighDateTime
  };
  ULARGE_INTEGER nowUsec = {
    now.dwLowDateTime,
    now.dwHighDateTime
  };

  return (nowUsec.QuadPart - startUsec.QuadPart) / 10ULL;
}

} 
