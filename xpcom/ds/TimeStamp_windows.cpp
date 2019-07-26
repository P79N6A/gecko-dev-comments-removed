










#define FORCE_PR_LOG

#include "mozilla/TimeStamp.h"
#include "mozilla/Mutex.h"
#include "mozilla/Services.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include <pratom.h>
#include <windows.h>

#include "prlog.h"
#include <stdio.h>

#include <intrin.h>

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

static bool sHasStableTSC = false;



















static const ULONGLONG kCalibrationInterval = 4000;


















static const ULONGLONG kOverflowLimit = 100;



static const DWORD kDefaultTimeIncrement = 156001;


static const DWORD kForbidRecalibrationTime = 2000;

















#define ms2mt(x) ((x) * sFrequencyPerSec)
#define mt2ms(x) ((x) / sFrequencyPerSec)
#define mt2ms_d(x) (double(x) / sFrequencyPerSec)


static LONGLONG sFrequencyPerSec = 0;










static LONGLONG sUnderrunThreshold;
static LONGLONG sOverrunThreshold;





static LONGLONG sWakeupAdjust = 0;







static const DWORD kLockSpinCount = 4096;




CRITICAL_SECTION sTimeStampLock;








static LONGLONG sSkew = 0;





static ULONGLONG sLastGTCResult = 0;





static ULONGLONG sLastResult = 0;




static ULONGLONG sLastCalibrated;




static ULONGLONG sFallbackTime = 0;










static union CalibrationFlags {
  struct {
    bool fallBackToGTC;
    bool forceRecalibrate;
  } flags;
  uint32_t dwordValue;
} sCalibrationFlags;


namespace mozilla {


static ULONGLONG
CalibratedPerformanceCounter();

typedef ULONGLONG (WINAPI* GetTickCount64_t)();
static GetTickCount64_t sGetTickCount64 = nullptr;

static inline ULONGLONG
InterlockedRead64(volatile ULONGLONG* destination)
{
#ifdef _WIN64
  
  return *destination;
#else
  
  return _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*> (destination), 0, 0);
#endif
}





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







class StandbyObserver MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

public:
  StandbyObserver()
  {
    LOG(("TimeStamp: StandByObserver::StandByObserver()"));
  }

  ~StandbyObserver()
  {
    LOG(("TimeStamp: StandByObserver::~StandByObserver()"));
  }

  static inline void Ensure()
  {
    if (sInitialized)
      return;

    
    
    if (!NS_IsMainThread())
      return;

    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (!obs)
      return; 

    sInitialized = true;

    nsRefPtr<StandbyObserver> observer = new StandbyObserver();
    obs->AddObserver(observer, "wake_notification", false);

    
    
  }

private:
  static bool sInitialized;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(StandbyObserver, nsIObserver)

bool
StandbyObserver::sInitialized = false;

NS_IMETHODIMP
StandbyObserver::Observe(nsISupports *subject,
                         const char *topic,
                         const PRUnichar *data)
{
  AutoCriticalSection lock(&sTimeStampLock);

  CalibrationFlags value;
  value.dwordValue = sCalibrationFlags.dwordValue;

  if (value.flags.fallBackToGTC &&
      ((sGetTickCount64() - sFallbackTime) > kForbidRecalibrationTime)) {
    LOG(("Disallowing recalibration since the time from fallback is too long"));
    return NS_OK;
  }

  
  
  value.flags.forceRecalibrate = value.flags.fallBackToGTC;
  value.flags.fallBackToGTC = false;
  sCalibrationFlags.dwordValue = value.dwordValue; 

  LOG(("TimeStamp: system has woken up, reset GTC fallback"));

  return NS_OK;
}






static void
InitThresholds()
{
  DWORD timeAdjustment = 0, timeIncrement = 0;
  BOOL timeAdjustmentDisabled;
  GetSystemTimeAdjustment(&timeAdjustment,
                          &timeIncrement,
                          &timeAdjustmentDisabled);

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
    LONGLONG((-2) * ticksPerGetTickCountResolutionCeiling);

  
  sOverrunThreshold =
    LONGLONG((+2) * ticksPerGetTickCountResolution);
}

static void
InitResolution()
{
  
  
  

  ULONGLONG minres = ~0ULL;
  int loops = 10;
  do {
    ULONGLONG start = CalibratedPerformanceCounter();
    ULONGLONG end = CalibratedPerformanceCounter();

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





static ULONGLONG WINAPI
GetTickCount64Fallback()
{
  ULONGLONG old, newValue;
  do {
    old = InterlockedRead64(&sLastGTCResult);
    ULONGLONG oldTop = old & 0xffffffff00000000;
    ULONG oldBottom = old & 0xffffffff;
    ULONG newBottom = GetTickCount();
    if (newBottom < oldBottom) {
        
        newValue = (oldTop + (1ULL<<32)) | newBottom;
    } else {
        newValue = oldTop | newBottom;
    }
  } while (old != _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*> (&sLastGTCResult),
                                                newValue, old));

  return newValue;
}


static inline ULONGLONG
PerformanceCounter()
{
  LARGE_INTEGER pc;
  ::QueryPerformanceCounter(&pc);
  return pc.QuadPart * 1000ULL;
}


static inline void
RecordFlaw(ULONGLONG gtc)
{
  sCalibrationFlags.flags.fallBackToGTC = true;
  sFallbackTime = gtc;

  LOG(("TimeStamp: falling back to GTC at %llu :(", gtc));

#if 0
  
  
  
  
  
  
  
  
  
  
  
  
  
  InitResolution();
#endif
}









static inline bool
CheckCalibration(LONGLONG overflow, ULONGLONG qpc, ULONGLONG gtc)
{
  CalibrationFlags value;
  value.dwordValue = sCalibrationFlags.dwordValue; 
  if (value.flags.fallBackToGTC) {
    
    return false;
  }

  ULONGLONG sinceLastCalibration = gtc - sLastCalibrated;

  if (overflow && !value.flags.forceRecalibrate) {
    
    
    
    ULONGLONG trend = LONGLONG(overflow *
      (double(kCalibrationInterval) / sinceLastCalibration));

    LOG(("TimeStamp: calibration after %llus with overflow %1.4fms"
         ", adjusted trend per calibration interval is %1.4fms",
         sinceLastCalibration / 1000,
         mt2ms_d(overflow),
         mt2ms_d(trend)));

    if (trend > ms2mt(kOverflowLimit)) {
      
      
      AutoCriticalSection lock(&sTimeStampLock);
      RecordFlaw(gtc);
      return false;
    }
  }

  if (sinceLastCalibration > kCalibrationInterval || value.flags.forceRecalibrate) {
    
    AutoCriticalSection lock(&sTimeStampLock);

    
    
    
    
    
    if (value.flags.forceRecalibrate)
      sWakeupAdjust += sSkew - (qpc - ms2mt(gtc));

    sSkew = qpc - ms2mt(gtc);
    sLastCalibrated = gtc;
    LOG(("TimeStamp: new skew is %1.2fms, wakeup adjust is %1.2fms (force:%d)",
      mt2ms_d(sSkew), mt2ms_d(sWakeupAdjust), value.flags.forceRecalibrate));

    sCalibrationFlags.flags.forceRecalibrate = false;
  }

  return true;
}

















ULONGLONG
AtomicStoreIfGreaterThan(ULONGLONG* destination, ULONGLONG newValue)
{
  ULONGLONG readValue;
  do {
    readValue = InterlockedRead64(destination);
    if (readValue > newValue)
      return readValue;
  } while (readValue != _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*> (destination),
                                                      newValue, readValue));

  return newValue;
}



static ULONGLONG
CalibratedPerformanceCounter()
{
  
  
  StandbyObserver::Ensure();

  
  
  

  ULONGLONG qpc = PerformanceCounter() + sWakeupAdjust;

  
  ULONGLONG gtc = sGetTickCount64();

  LONGLONG diff = qpc - ms2mt(gtc) - sSkew;
  LONGLONG overflow = 0;

  if (diff < sUnderrunThreshold) {
    overflow = sUnderrunThreshold - diff;
  }
  else if (diff > sOverrunThreshold) {
    overflow = diff - sOverrunThreshold;
  }

  ULONGLONG result = qpc;
  if (!CheckCalibration(overflow, qpc, gtc)) {
    
    result = ms2mt(gtc) + sSkew;
  }

#if 0
  LOG(("TimeStamp: result = %1.2fms, diff = %1.4fms",
      mt2ms_d(result), mt2ms_d(diff)));
#endif

  return AtomicStoreIfGreaterThan(&sLastResult, result);
}





double
TimeDuration::ToSeconds() const
{
  
  return double(mValue) / (double(sFrequencyPerSec) * 1000.0);
}

double
TimeDuration::ToSecondsSigDigits() const
{
  AutoCriticalSection lock(&sTimeStampLock);

  
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
  AutoCriticalSection lock(&sTimeStampLock);

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
  

  HMODULE kernelDLL = GetModuleHandleW(L"kernel32.dll");
  sGetTickCount64 = reinterpret_cast<GetTickCount64_t>
    (GetProcAddress(kernelDLL, "GetTickCount64"));
  if (!sGetTickCount64) {
    
    
    sGetTickCount64 = GetTickCount64Fallback;
  }

  InitializeCriticalSectionAndSpinCount(&sTimeStampLock, kLockSpinCount);

  LARGE_INTEGER freq;
  BOOL QPCAvailable = ::QueryPerformanceFrequency(&freq);
  if (!QPCAvailable) {
    
    sFrequencyPerSec = 1;
    sCalibrationFlags.flags.fallBackToGTC = true;
    InitResolution();

    LOG(("TimeStamp: using GetTickCount"));
    return NS_OK;
  }

  sFrequencyPerSec = freq.QuadPart;

  ULONGLONG qpc = PerformanceCounter();
  sLastCalibrated = sGetTickCount64();
  sSkew = qpc - ms2mt(sLastCalibrated);

  InitThresholds();
  InitResolution();

  sHasStableTSC = HasStableTSC();

  LOG(("TimeStamp: initial skew is %1.2fms, sHasStableTSC=%d", mt2ms_d(sSkew), sHasStableTSC));

  return NS_OK;
}

void
TimeStamp::Shutdown()
{
  DeleteCriticalSection(&sTimeStampLock);
}

TimeStamp
TimeStamp::Now()
{
  if (sHasStableTSC) {
    return TimeStamp(uint64_t(PerformanceCounter()));
  }
  return TimeStamp(uint64_t(CalibratedPerformanceCounter()));
}

} 
