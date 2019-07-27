





#include "mozilla/HangMonitor.h"

#include <set>

#include "mozilla/Atomics.h"
#include "mozilla/BackgroundHangMonitor.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "mozilla/ProcessedStack.h"
#include "mozilla/Telemetry.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/UniquePtr.h"
#include "nsReadableUtils.h"
#include "nsStackWalk.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#ifdef XP_WIN
#include <windows.h>
#endif

#if defined(MOZ_ENABLE_PROFILER_SPS) && defined(MOZ_PROFILING) && defined(XP_WIN)
  #define REPORT_CHROME_HANGS
#endif

namespace mozilla {
namespace HangMonitor {





volatile bool gDebugDisableHangMonitor = false;

const char kHangMonitorPrefName[] = "hangmonitor.timeout";

#ifdef REPORT_CHROME_HANGS
const char kTelemetryPrefName[] = "toolkit.telemetry.enabled";
#endif




Monitor* gMonitor;


int32_t gTimeout;

PRThread* gThread;


bool gShutdown;



Atomic<PRIntervalTime> gTimestamp(PR_INTERVAL_NO_WAIT);

#ifdef REPORT_CHROME_HANGS

static HANDLE winMainThreadHandle = nullptr;


static const int32_t DEFAULT_CHROME_HANG_INTERVAL = 5;


static const int32_t MAX_CALL_STACK_PCS = 400;


static StaticAutoPtr<std::set<Annotator*>> gAnnotators;
#endif


void
PrefChanged(const char*, void*)
{
  int32_t newval = Preferences::GetInt(kHangMonitorPrefName);
#ifdef REPORT_CHROME_HANGS
  
  if (newval == 0) {
    bool telemetryEnabled = Preferences::GetBool(kTelemetryPrefName);
    if (telemetryEnabled) {
      newval = DEFAULT_CHROME_HANG_INTERVAL;
    }
  }
#endif
  MonitorAutoLock lock(*gMonitor);
  if (newval != gTimeout) {
    gTimeout = newval;
    lock.Notify();
  }
}

void
Crash()
{
  if (gDebugDisableHangMonitor) {
    return;
  }

#ifdef XP_WIN
  if (::IsDebuggerPresent()) {
    return;
  }
#endif

#ifdef MOZ_CRASHREPORTER
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("Hang"),
                                     NS_LITERAL_CSTRING("1"));
#endif

  NS_RUNTIMEABORT("HangMonitor triggered");
}

#ifdef REPORT_CHROME_HANGS
class ChromeHangAnnotations : public HangAnnotations
{
public:
  ChromeHangAnnotations();
  ~ChromeHangAnnotations();

  void AddAnnotation(const nsAString& aName, const int32_t aData) MOZ_OVERRIDE;
  void AddAnnotation(const nsAString& aName, const double aData) MOZ_OVERRIDE;
  void AddAnnotation(const nsAString& aName, const nsAString& aData) MOZ_OVERRIDE;
  void AddAnnotation(const nsAString& aName, const nsACString& aData) MOZ_OVERRIDE;
  void AddAnnotation(const nsAString& aName, const bool aData) MOZ_OVERRIDE;

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  bool IsEmpty() const MOZ_OVERRIDE;
  bool GetEnumerator(Enumerator** aOutEnum) MOZ_OVERRIDE;

  typedef std::pair<nsString, nsString> AnnotationType;
  typedef std::vector<AnnotationType> VectorType;
  typedef VectorType::const_iterator IteratorType;

private:
  VectorType  mAnnotations;
};

ChromeHangAnnotations::ChromeHangAnnotations()
{
  MOZ_COUNT_CTOR(ChromeHangAnnotations);
}

ChromeHangAnnotations::~ChromeHangAnnotations()
{
  MOZ_COUNT_DTOR(ChromeHangAnnotations);
}

void
ChromeHangAnnotations::AddAnnotation(const nsAString& aName, const int32_t aData)
{
  nsString dataString;
  dataString.AppendInt(aData);
  AnnotationType annotation = std::make_pair(nsString(aName), dataString);
  mAnnotations.push_back(annotation);
}

void
ChromeHangAnnotations::AddAnnotation(const nsAString& aName, const double aData)
{
  nsString dataString;
  dataString.AppendFloat(aData);
  AnnotationType annotation = std::make_pair(nsString(aName), dataString);
  mAnnotations.push_back(annotation);
}

void
ChromeHangAnnotations::AddAnnotation(const nsAString& aName, const nsAString& aData)
{
  AnnotationType annotation = std::make_pair(nsString(aName), nsString(aData));
  mAnnotations.push_back(annotation);
}

void
ChromeHangAnnotations::AddAnnotation(const nsAString& aName, const nsACString& aData)
{
  nsString dataString;
  AppendUTF8toUTF16(aData, dataString);
  AnnotationType annotation = std::make_pair(nsString(aName), dataString);
  mAnnotations.push_back(annotation);
}

void
ChromeHangAnnotations::AddAnnotation(const nsAString& aName, const bool aData)
{
  nsString dataString;
  dataString += aData ? NS_LITERAL_STRING("true") : NS_LITERAL_STRING("false");
  AnnotationType annotation = std::make_pair(nsString(aName), dataString);
  mAnnotations.push_back(annotation);
}






class ChromeHangAnnotationEnumerator : public HangAnnotations::Enumerator
{
public:
  ChromeHangAnnotationEnumerator(const ChromeHangAnnotations::VectorType& aAnnotations);
  ~ChromeHangAnnotationEnumerator();

  virtual bool Next(nsAString& aOutName, nsAString& aOutValue);

private:
  ChromeHangAnnotations::IteratorType mIterator;
  ChromeHangAnnotations::IteratorType mEnd;
};

ChromeHangAnnotationEnumerator::ChromeHangAnnotationEnumerator(
                          const ChromeHangAnnotations::VectorType& aAnnotations)
  : mIterator(aAnnotations.begin())
  , mEnd(aAnnotations.end())
{
  MOZ_COUNT_CTOR(ChromeHangAnnotationEnumerator);
}

ChromeHangAnnotationEnumerator::~ChromeHangAnnotationEnumerator()
{
  MOZ_COUNT_DTOR(ChromeHangAnnotationEnumerator);
}

bool
ChromeHangAnnotationEnumerator::Next(nsAString& aOutName, nsAString& aOutValue)
{
  aOutName.Truncate();
  aOutValue.Truncate();
  if (mIterator == mEnd) {
    return false;
  }
  aOutName = mIterator->first;
  aOutValue = mIterator->second;
  ++mIterator;
  return true;
}

bool
ChromeHangAnnotations::IsEmpty() const
{
  return mAnnotations.empty();
}

size_t
ChromeHangAnnotations::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t result = sizeof(mAnnotations) +
                  mAnnotations.capacity() * sizeof(AnnotationType);
  for (IteratorType i = mAnnotations.begin(), e = mAnnotations.end(); i != e;
       ++i) {
    result += i->first.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    result += i->second.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
  }

  return result;
}

bool
ChromeHangAnnotations::GetEnumerator(HangAnnotations::Enumerator** aOutEnum)
{
  if (!aOutEnum) {
    return false;
  }
  *aOutEnum = nullptr;
  if (mAnnotations.empty()) {
    return false;
  }
  *aOutEnum = new ChromeHangAnnotationEnumerator(mAnnotations);
  return true;
}

static void
ChromeStackWalker(uint32_t aFrameNumber, void* aPC, void* aSP, void* aClosure)
{
  MOZ_ASSERT(aClosure);
  std::vector<uintptr_t>* stack =
    static_cast<std::vector<uintptr_t>*>(aClosure);
  if (stack->size() == MAX_CALL_STACK_PCS) {
    return;
  }
  MOZ_ASSERT(stack->size() < MAX_CALL_STACK_PCS);
  stack->push_back(reinterpret_cast<uintptr_t>(aPC));
}

static void
GetChromeHangReport(Telemetry::ProcessedStack& aStack,
                    int32_t& aSystemUptime,
                    int32_t& aFirefoxUptime)
{
  MOZ_ASSERT(winMainThreadHandle);

  
  
  std::vector<uintptr_t> rawStack;
  rawStack.reserve(MAX_CALL_STACK_PCS);
  DWORD ret = ::SuspendThread(winMainThreadHandle);
  if (ret == -1) {
    return;
  }
  NS_StackWalk(ChromeStackWalker,  0,  0,
               reinterpret_cast<void*>(&rawStack),
               reinterpret_cast<uintptr_t>(winMainThreadHandle), nullptr);
  ret = ::ResumeThread(winMainThreadHandle);
  if (ret == -1) {
    return;
  }
  aStack = Telemetry::GetStackAndModules(rawStack);

  
  aSystemUptime = ((GetTickCount() / 1000) - (gTimeout * 2)) / 60;

  
  bool error;
  TimeStamp processCreation = TimeStamp::ProcessCreation(error);
  if (!error) {
    TimeDuration td = TimeStamp::Now() - processCreation;
    aFirefoxUptime = (static_cast<int32_t>(td.ToSeconds()) - (gTimeout * 2)) / 60;
  } else {
    aFirefoxUptime = -1;
  }
}

static void
ChromeHangAnnotatorCallout(ChromeHangAnnotations& aAnnotations)
{
  gMonitor->AssertCurrentThreadOwns();
  MOZ_ASSERT(gAnnotators);
  if (!gAnnotators) {
    return;
  }
  for (std::set<Annotator*>::iterator i = gAnnotators->begin(),
                                      e = gAnnotators->end();
       i != e; ++i) {
    (*i)->AnnotateHang(aAnnotations);
  }
}

#endif

void
ThreadMain(void*)
{
  PR_SetCurrentThreadName("Hang Monitor");

  MonitorAutoLock lock(*gMonitor);

  
  
  
  PRIntervalTime lastTimestamp = 0;
  int waitCount = 0;

#ifdef REPORT_CHROME_HANGS
  Telemetry::ProcessedStack stack;
  int32_t systemUptime = -1;
  int32_t firefoxUptime = -1;
  auto annotations = MakeUnique<ChromeHangAnnotations>();
#endif

  while (true) {
    if (gShutdown) {
      return; 
    }

    
    PRIntervalTime timestamp = gTimestamp;

    PRIntervalTime now = PR_IntervalNow();

    if (timestamp != PR_INTERVAL_NO_WAIT &&
        now < timestamp) {
      
      timestamp = 1; 
    }

    if (timestamp != PR_INTERVAL_NO_WAIT &&
        timestamp == lastTimestamp &&
        gTimeout > 0) {
      ++waitCount;
#ifdef REPORT_CHROME_HANGS
      
      
      if (waitCount == 2) {
        GetChromeHangReport(stack, systemUptime, firefoxUptime);
        ChromeHangAnnotatorCallout(*annotations);
      }
#else
      
      
      if (waitCount >= 2) {
        int32_t delay =
          int32_t(PR_IntervalToSeconds(now - timestamp));
        if (delay >= gTimeout) {
          MonitorAutoUnlock unlock(*gMonitor);
          Crash();
        }
      }
#endif
    } else {
#ifdef REPORT_CHROME_HANGS
      if (waitCount >= 2) {
        uint32_t hangDuration = PR_IntervalToSeconds(now - lastTimestamp);
        Telemetry::RecordChromeHang(hangDuration, stack, systemUptime,
                                    firefoxUptime, Move(annotations));
        stack.Clear();
        annotations = MakeUnique<ChromeHangAnnotations>();
      }
#endif
      lastTimestamp = timestamp;
      waitCount = 0;
    }

    PRIntervalTime timeout;
    if (gTimeout <= 0) {
      timeout = PR_INTERVAL_NO_TIMEOUT;
    } else {
      timeout = PR_MillisecondsToInterval(gTimeout * 500);
    }
    lock.Wait(timeout);
  }
}

void
Startup()
{
  
  
  
  if (GeckoProcessType_Default != XRE_GetProcessType()) {
    return;
  }

  MOZ_ASSERT(!gMonitor, "Hang monitor already initialized");
  gMonitor = new Monitor("HangMonitor");

  Preferences::RegisterCallback(PrefChanged, kHangMonitorPrefName, nullptr);
  PrefChanged(nullptr, nullptr);

#ifdef REPORT_CHROME_HANGS
  Preferences::RegisterCallback(PrefChanged, kTelemetryPrefName, nullptr);
  winMainThreadHandle =
    OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
  if (!winMainThreadHandle) {
    return;
  }
  gAnnotators = new std::set<Annotator*>();
#endif

  
  
  
  
  Suspend();

  gThread = PR_CreateThread(PR_USER_THREAD,
                            ThreadMain,
                            nullptr, PR_PRIORITY_LOW, PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD, 0);
}

void
Shutdown()
{
  if (GeckoProcessType_Default != XRE_GetProcessType()) {
    return;
  }

  MOZ_ASSERT(gMonitor, "Hang monitor not started");

  {
    
    MonitorAutoLock lock(*gMonitor);
    gShutdown = true;
    lock.Notify();
  }

  
  if (gThread) {
    PR_JoinThread(gThread);
    gThread = nullptr;
  }

  delete gMonitor;
  gMonitor = nullptr;

#ifdef REPORT_CHROME_HANGS
  
  gAnnotators = nullptr;
#endif
}

static bool
IsUIMessageWaiting()
{
#ifndef XP_WIN
  return false;
#else
#define NS_WM_IMEFIRST WM_IME_SETCONTEXT
#define NS_WM_IMELAST  WM_IME_KEYUP
  BOOL haveUIMessageWaiting = FALSE;
  MSG msg;
  haveUIMessageWaiting |= ::PeekMessageW(&msg, nullptr, WM_KEYFIRST,
                                         WM_IME_KEYLAST, PM_NOREMOVE);
  haveUIMessageWaiting |= ::PeekMessageW(&msg, nullptr, NS_WM_IMEFIRST,
                                         NS_WM_IMELAST, PM_NOREMOVE);
  haveUIMessageWaiting |= ::PeekMessageW(&msg, nullptr, WM_MOUSEFIRST,
                                         WM_MOUSELAST, PM_NOREMOVE);
  return haveUIMessageWaiting;
#endif
}

void
NotifyActivity(ActivityType aActivityType)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "HangMonitor::Notify called from off the main thread.");

  
  if (aActivityType == kGeneralActivity) {
    aActivityType = IsUIMessageWaiting() ? kActivityUIAVail :
                                           kActivityNoUIAVail;
  }

  
  static uint32_t cumulativeUILagMS = 0;
  switch (aActivityType) {
    case kActivityNoUIAVail:
      cumulativeUILagMS = 0;
      break;
    case kActivityUIAVail:
    case kUIActivity:
      if (gTimestamp != PR_INTERVAL_NO_WAIT) {
        cumulativeUILagMS += PR_IntervalToMilliseconds(PR_IntervalNow() -
                                                       gTimestamp);
      }
      break;
    default:
      break;
  }

  
  
  
  gTimestamp = PR_IntervalNow();

  
  
  if (aActivityType == kUIActivity) {
    
    
    static const uint32_t kUIResponsivenessThresholdMS = 50;
    if (cumulativeUILagMS > kUIResponsivenessThresholdMS) {
      mozilla::Telemetry::Accumulate(mozilla::Telemetry::EVENTLOOP_UI_LAG_EXP_MS,
                                     cumulativeUILagMS);
    }
    cumulativeUILagMS = 0;
  }

  if (gThread && !gShutdown) {
    mozilla::BackgroundHangMonitor().NotifyActivity();
  }
}

void
Suspend()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "HangMonitor::Suspend called from off the main thread.");

  
  gTimestamp = PR_INTERVAL_NO_WAIT;

  if (gThread && !gShutdown) {
    mozilla::BackgroundHangMonitor().NotifyWait();
  }
}

void
RegisterAnnotator(Annotator& aAnnotator)
{
#ifdef REPORT_CHROME_HANGS
  if (GeckoProcessType_Default != XRE_GetProcessType()) {
    return;
  }
  MonitorAutoLock lock(*gMonitor);
  MOZ_ASSERT(gAnnotators);
  gAnnotators->insert(&aAnnotator);
#endif
}

void
UnregisterAnnotator(Annotator& aAnnotator)
{
#ifdef REPORT_CHROME_HANGS
  if (GeckoProcessType_Default != XRE_GetProcessType()) {
    return;
  }
  MonitorAutoLock lock(*gMonitor);
  MOZ_ASSERT(gAnnotators);
  gAnnotators->erase(&aAnnotator);
#endif
}

} 
} 
