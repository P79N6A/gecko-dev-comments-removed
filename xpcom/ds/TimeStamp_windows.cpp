










#define FORCE_PR_LOG

#include "mozilla/MathAlgorithms.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include <windows.h>

#include "prlog.h"
#include <stdio.h>

#include <intrin.h>

#if defined(PR_LOGGING)









static PRLogModuleInfo*
GetTimeStampLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("TimeStampWindows");
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









static const uint32_t kQPCHardFailureDetectionInterval = 2000;



















static const ULONGLONG kOverflowLimit = 50;



static const DWORD kDefaultTimeIncrement = 156001;

















#define ms2mt(x) ((x) * sFrequencyPerSec)
#define mt2ms(x) ((x) / sFrequencyPerSec)
#define mt2ms_f(x) (double(x) / sFrequencyPerSec)


static LONGLONG sFrequencyPerSec = 0;










static LONGLONG sUnderrunThreshold;
static LONGLONG sOverrunThreshold;





static LONGLONG sQPCHardFailureDetectionInterval;


static bool sHasStableTSC = false;







static bool volatile sUseQPC = true;







static const DWORD kLockSpinCount = 4096;




static CRITICAL_SECTION sTimeStampLock;





static DWORD sLastGTCResult = 0;



static DWORD sLastGTCRollover = 0;

namespace mozilla {

typedef ULONGLONG (WINAPI* GetTickCount64_t)();
static GetTickCount64_t sGetTickCount64 = nullptr;





class AutoCriticalSection
{
public:
  AutoCriticalSection(LPCRITICAL_SECTION section)
    : mSection(section)
  {
    ::EnterCriticalSection(mSection);
  }
  ~AutoCriticalSection()
  {
    ::LeaveCriticalSection(mSection);
  }
private:
  LPCRITICAL_SECTION mSection;
};



static ULONGLONG WINAPI
MozGetTickCount64()
{
  DWORD GTC = ::GetTickCount();

  
  AutoCriticalSection lock(&sTimeStampLock);

  
  
  if ((sLastGTCResult > GTC) && ((sLastGTCResult - GTC) > (1UL << 30)))
    ++sLastGTCRollover;

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

  if (!timeIncrement)
    timeIncrement = kDefaultTimeIncrement;

  
  
  DWORD timeIncrementCeil = timeIncrement;
  
  timeIncrementCeil -= 1;
  
  timeIncrementCeil /= 10000;
  
  timeIncrementCeil += 1;
  
  timeIncrementCeil *= 10000;

  
  LONGLONG ticksPerGetTickCountResolution =
    (int64_t(timeIncrement) * sFrequencyPerSec) / 10000LL;

  
  LONGLONG ticksPerGetTickCountResolutionCeiling =
    (int64_t(timeIncrementCeil) * sFrequencyPerSec) / 10000LL;

  
  
  
  
  sUnderrunThreshold =
    LONGLONG((-4) * ticksPerGetTickCountResolutionCeiling);

  
  sOverrunThreshold =
    LONGLONG((+4) * ticksPerGetTickCountResolution);

  sQPCHardFailureDetectionInterval =
    LONGLONG(kQPCHardFailureDetectionInterval) * sFrequencyPerSec;
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
    if (candidate < minres)
      minres = candidate;
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
       !(sigDigs == result
         || 10*sigDigs > result);
       sigDigs *= 10);

  sResolutionSigDigs = sigDigs;
}





TimeStampValue::TimeStampValue(_SomethingVeryRandomHere* nullValue)
  : mGTC(0)
  , mQPC(0)
  , mHasQPC(false)
  , mIsNull(true)
{
  MOZ_ASSERT(!nullValue);
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



bool
TimeStampValue::CheckQPC(int64_t aDuration, const TimeStampValue &aOther) const
{
  if (!mHasQPC || !aOther.mHasQPC) 
    return false;

  if (sHasStableTSC) 
    return true;

  if (!sUseQPC) 
    return false;

  
  aDuration = Abs(aDuration);

  

  LONGLONG skew1 = mGTC - mQPC;
  LONGLONG skew2 = aOther.mGTC - aOther.mQPC;

  LONGLONG diff = skew1 - skew2;
  LONGLONG overflow;

  if (diff < sUnderrunThreshold)
    overflow = sUnderrunThreshold - diff;
  else if (diff > sOverrunThreshold)
    overflow = diff - sOverrunThreshold;
  else
    return true;

  ULONGLONG trend;
  if (aDuration)
    trend = LONGLONG(overflow * (double(sQPCHardFailureDetectionInterval) / aDuration));
  else
    trend = overflow;

  LOG(("TimeStamp: QPC check after %llums with overflow %1.4fms"
       ", adjusted trend per interval is %1.4fms",
       mt2ms(aDuration),
       mt2ms_f(overflow),
       mt2ms_f(trend)));

  if (trend <= ms2mt(kOverflowLimit)) {
    
    return true;
  }

  
  LOG(("TimeStamp: QPC found highly jittering"));

  if (aDuration < sQPCHardFailureDetectionInterval) {
    
    
    sUseQPC = false;
    LOG(("TimeStamp: QPC disabled"));
  }

  return false;
}

uint64_t
TimeStampValue::operator-(const TimeStampValue &aOther) const
{
  if (mIsNull && aOther.mIsNull)
    return uint64_t(0);

  if (CheckQPC(int64_t(mGTC - aOther.mGTC), aOther))
    return mQPC - aOther.mQPC;

  return mGTC - aOther.mGTC;
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
  return TimeDuration::FromTicks(int64_t(ms2mt(aMilliseconds)));
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

static bool
HasStableTSC()
{
  union {
    int regs[4];
    struct {
      int nIds;
      char cpuString[12];
    };
  } cpuInfo;

  __cpuid(cpuInfo.regs, 0);
  
  
  
  if (_strnicmp(cpuInfo.cpuString, "GenuntelineI", sizeof(cpuInfo.cpuString)))
    return false;

  int regs[4];

  
  __cpuid(regs, 0x80000000);
  if (regs[0] < 0x80000007)
    return false;

  __cpuid(regs, 0x80000007);
  
  
  return regs[3] & (1 << 8);
}

nsresult
TimeStamp::Startup()
{
  

  HMODULE kernelDLL = GetModuleHandleW(L"kernel32.dll");
  sGetTickCount64 = reinterpret_cast<GetTickCount64_t>
    (GetProcAddress(kernelDLL, "GetTickCount64"));
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

} 
