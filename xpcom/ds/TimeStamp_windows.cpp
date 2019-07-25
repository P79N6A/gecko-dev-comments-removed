










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

#if defined(PR_LOGGING)









  PRLogModuleInfo* timeStampLog = PR_NewLogModule("TimeStampWindows");
  #define LOG(x)  PR_LOG(timeStampLog, PR_LOG_DEBUG, x)
#else
  #define LOG(x)
#endif 


static volatile ULONGLONG sResolution;
static volatile ULONGLONG sResolutionSigDigs;
static const double   kNsPerSecd  = 1000000000.0;
static const LONGLONG kNsPerSec   = 1000000000;
static const LONGLONG kNsPerMillisec = 1000000;



















static const ULONGLONG kCalibrationInterval = 4000;


















static const ULONGLONG kOverflowLimit = 100;



static const DWORD kDefaultTimeIncrement = 156001;

















#define ms2mt(x) ((x) * sFrequencyPerSec)
#define mt2ms(x) ((x) / sFrequencyPerSec)
#define mt2ms_d(x) (double(x) / sFrequencyPerSec)


static LONGLONG sFrequencyPerSec = 0;










static LONGLONG sUnderrunThreshold;
static LONGLONG sOverrunThreshold;







static const DWORD kLockSpinCount = 4096;




CRITICAL_SECTION sTimeStampLock;








static LONGLONG sSkew = 0;





static ULONGLONG sLastGTCResult = 0;





static ULONGLONG sLastResult = 0;




static ULONGLONG sLastCalibrated;



static bool sFallBackToGTC = false;



static bool sForceRecalibrate = false;


namespace mozilla {


static ULONGLONG
CalibratedPerformanceCounter();






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







class StandbyObserver : public nsIObserver
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

  
  
  sFallBackToGTC = false;
  sForceRecalibrate = true;
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
    (PRInt64(timeIncrement) * sFrequencyPerSec) / 10000LL;

  
  LONGLONG ticksPerGetTickCountResolutionCeiling =
    (PRInt64(timeIncrementCeil) * sFrequencyPerSec) / 10000LL;


  
  
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





static inline ULONGLONG
TickCount64(DWORD now)
{
  ULONGLONG lastResultHiPart = sLastGTCResult & (~0ULL << 32);
  ULONGLONG result = lastResultHiPart | ULONGLONG(now);

  
  
  
  

  if (sLastGTCResult > result) {
    if ((sLastGTCResult - result) > (1ULL << 31))
      result += 1ULL << 32;
    else
      result = sLastGTCResult;
  }

  sLastGTCResult = result;
  return result;
}


static inline ULONGLONG
PerformanceCounter()
{
  LARGE_INTEGER pc;
  ::QueryPerformanceCounter(&pc);
  return pc.QuadPart * 1000ULL;
}


static inline void
RecordFlaw()
{
  sFallBackToGTC = true;

  LOG(("TimeStamp: falling back to GTC :("));

#if 0
  
  
  
  
  
  
  
  
  
  
  
  
  
  InitResolution();
#endif
}









static inline bool
CheckCalibration(LONGLONG overflow, ULONGLONG qpc, ULONGLONG gtc)
{
  if (sFallBackToGTC) {
    
    return false;
  }

  ULONGLONG sinceLastCalibration = gtc - sLastCalibrated;

  if (overflow && !sForceRecalibrate) {
    
    
    
    ULONGLONG trend = LONGLONG(overflow *
      (double(kCalibrationInterval) / sinceLastCalibration));

    LOG(("TimeStamp: calibration after %llus with overflow %1.4fms"
         ", adjusted trend per calibration interval is %1.4fms",
         sinceLastCalibration / 1000,
         mt2ms_d(overflow),
         mt2ms_d(trend)));

    if (trend > ms2mt(kOverflowLimit)) {
      
      
      RecordFlaw();
      return false;
    }
  }

  if (sinceLastCalibration > kCalibrationInterval || sForceRecalibrate) {
    
    sSkew = qpc - ms2mt(gtc);
    sLastCalibrated = gtc;
    LOG(("TimeStamp: new skew is %1.2fms (force:%d)",
      mt2ms_d(sSkew), sForceRecalibrate));

    sForceRecalibrate = false;
  }

  return true;
}



static ULONGLONG
CalibratedPerformanceCounter()
{
  
  
  StandbyObserver::Ensure();

  
  
  

  ULONGLONG qpc = PerformanceCounter();
  DWORD gtcw = GetTickCount();

  AutoCriticalSection lock(&sTimeStampLock);

  
  ULONGLONG gtc = TickCount64(gtcw);

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

  if (result > sLastResult)
    sLastResult = result;

  return sLastResult;
}





double
TimeDuration::ToSeconds() const
{
  return double(mValue) / (sFrequencyPerSec * 1000ULL);
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
  return TimeDuration::FromTicks(PRInt64(ms2mt(aMilliseconds)));
}

TimeDuration
TimeDuration::Resolution()
{
  AutoCriticalSection lock(&sTimeStampLock);

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

nsresult
TimeStamp::Startup()
{
  

  InitializeCriticalSectionAndSpinCount(&sTimeStampLock, kLockSpinCount);

  LARGE_INTEGER freq;
  BOOL QPCAvailable = ::QueryPerformanceFrequency(&freq);
  if (!QPCAvailable) {
    
    sFrequencyPerSec = 1;
    sFallBackToGTC = true;
    InitResolution();

    LOG(("TimeStamp: using GetTickCount"));
    return NS_OK;
  }

  sFrequencyPerSec = freq.QuadPart;

  ULONGLONG qpc = PerformanceCounter();
  sLastCalibrated = TickCount64(::GetTickCount());
  sSkew = qpc - ms2mt(sLastCalibrated);

  InitThresholds();
  InitResolution();

  LOG(("TimeStamp: initial skew is %1.2fms", mt2ms_d(sSkew)));

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
  return TimeStamp(PRUint64(CalibratedPerformanceCounter()));
}

} 
