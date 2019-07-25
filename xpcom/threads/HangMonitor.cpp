




































#include "mozilla/HangMonitor.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "nsStackWalk.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#ifdef XP_WIN
#include <windows.h>
#endif

#if defined(MOZ_ENABLE_PROFILER_SPS) && defined(MOZ_PROFILING) && defined(XP_WIN)
  #define REPORT_CHROME_HANGS
#endif

namespace mozilla { namespace HangMonitor {





volatile bool gDebugDisableHangMonitor = false;

const char kHangMonitorPrefName[] = "hangmonitor.timeout";

const char kTelemetryPrefName[] = "toolkit.telemetry.enabled";




Monitor* gMonitor;


PRInt32 gTimeout;

PRThread* gThread;


bool gShutdown;



volatile PRIntervalTime gTimestamp;

#ifdef REPORT_CHROME_HANGS

static HANDLE winMainThreadHandle = NULL;


static const PRInt32 DEFAULT_CHROME_HANG_INTERVAL = 10;
#endif


int
PrefChanged(const char*, void*)
{
  PRInt32 newval = Preferences::GetInt(kHangMonitorPrefName);
#ifdef REPORT_CHROME_HANGS
  
  if (newval == 0) {
    PRBool telemetryEnabled = Preferences::GetBool(kTelemetryPrefName);
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

  return 0;
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
static void
ChromeStackWalker(void *aPC, void *aClosure)
{
  MOZ_ASSERT(aClosure);
  Telemetry::HangStack *callStack =
    reinterpret_cast< Telemetry::HangStack* >(aClosure);
  callStack->AppendElement(reinterpret_cast<uintptr_t>(aPC));
}

static void
GetChromeHangReport(Telemetry::HangStack &callStack, SharedLibraryInfo &moduleMap)
{
  MOZ_ASSERT(winMainThreadHandle);

  DWORD ret = ::SuspendThread(winMainThreadHandle);
  if (ret == -1) {
    callStack.Clear();
    moduleMap.Clear();
    return;
  }
  NS_StackWalk(ChromeStackWalker, 0, &callStack,
               reinterpret_cast<uintptr_t>(winMainThreadHandle));
  ret = ::ResumeThread(winMainThreadHandle);
  if (ret == -1) {
    callStack.Clear();
    moduleMap.Clear();
    return;
  }

  moduleMap = SharedLibraryInfo::GetInfoForSelf();
  moduleMap.SortByAddress();

  
  Telemetry::HangStack sortedStack = callStack;
  sortedStack.Sort();

  size_t moduleIndex = 0;
  size_t stackIndex = 0;
  bool unreferencedModule = true;
  while (stackIndex < sortedStack.Length() && moduleIndex < moduleMap.GetSize()) {
    uintptr_t pc = sortedStack[stackIndex];
    SharedLibrary& module = moduleMap.GetEntry(moduleIndex);
    uintptr_t moduleStart = module.GetStart();
    uintptr_t moduleEnd = module.GetEnd() - 1;
    if (moduleStart <= pc && pc <= moduleEnd) {
      
      unreferencedModule = false;
      ++stackIndex;
    } else if (pc > moduleEnd) {
      if (unreferencedModule) {
        
        moduleMap.RemoveEntries(moduleIndex, moduleIndex + 1);
      } else {
        
        unreferencedModule = true;
        ++moduleIndex;
      }
    } else {
      
      ++stackIndex;
    }
  }

  
  if (moduleIndex + 1 < moduleMap.GetSize()) {
    moduleMap.RemoveEntries(moduleIndex + 1, moduleMap.GetSize());
  }
}
#endif

void
ThreadMain(void*)
{
  MonitorAutoLock lock(*gMonitor);

  
  
  
  PRIntervalTime lastTimestamp = 0;
  int waitCount = 0;

#ifdef REPORT_CHROME_HANGS
  Telemetry::HangStack hangStack;
  SharedLibraryInfo hangModuleMap;
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
      if (waitCount == 2) {
#ifdef REPORT_CHROME_HANGS
        GetChromeHangReport(hangStack, hangModuleMap);
#else
        PRInt32 delay =
          PRInt32(PR_IntervalToSeconds(now - timestamp));
        if (delay > gTimeout) {
          MonitorAutoUnlock unlock(*gMonitor);
          Crash();
        }
#endif
      }
    }
    else {
#ifdef REPORT_CHROME_HANGS
      if (waitCount >= 2) {
        PRUint32 hangDuration = PR_IntervalToSeconds(now - lastTimestamp);
        Telemetry::RecordChromeHang(hangDuration, hangStack, hangModuleMap);
        hangStack.Clear();
        hangModuleMap.Clear();
      }
#endif
      lastTimestamp = timestamp;
      waitCount = 0;
    }

    PRIntervalTime timeout;
    if (gTimeout <= 0) {
      timeout = PR_INTERVAL_NO_TIMEOUT;
    }
    else {
      timeout = PR_MillisecondsToInterval(gTimeout * 500);
    }
    lock.Wait(timeout);
  }
}

void
Startup()
{
  
  
  
  if (GeckoProcessType_Default != XRE_GetProcessType())
    return;

  NS_ASSERTION(!gMonitor, "Hang monitor already initialized");
  gMonitor = new Monitor("HangMonitor");

  Preferences::RegisterCallback(PrefChanged, kHangMonitorPrefName, NULL);
  PrefChanged(NULL, NULL);

#ifdef REPORT_CHROME_HANGS
  Preferences::RegisterCallback(PrefChanged, kTelemetryPrefName, NULL);
  winMainThreadHandle =
    OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
  if (!winMainThreadHandle)
    return;
#endif

  
  
  
  
  Suspend();

  gThread = PR_CreateThread(PR_USER_THREAD,
                            ThreadMain,
                            NULL, PR_PRIORITY_LOW, PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD, 0);
}

void
Shutdown()
{
  if (GeckoProcessType_Default != XRE_GetProcessType())
    return;

  NS_ASSERTION(gMonitor, "Hang monitor not started");

  { 
    MonitorAutoLock lock(*gMonitor);
    gShutdown = true;
    lock.Notify();
  }

  
  if (gThread) {
    PR_JoinThread(gThread);
    gThread = NULL;
  }

  delete gMonitor;
  gMonitor = NULL;
}

void
NotifyActivity()
{
  NS_ASSERTION(NS_IsMainThread(), "HangMonitor::Notify called from off the main thread.");

  
  
  
  gTimestamp = PR_IntervalNow();
}

void
Suspend()
{
  NS_ASSERTION(NS_IsMainThread(), "HangMonitor::Suspend called from off the main thread.");

  
  gTimestamp = PR_INTERVAL_NO_WAIT;
}

} } 
