




































#include "mozilla/HangMonitor.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#ifdef XP_WIN
#include <windows.h>
#endif

namespace mozilla { namespace HangMonitor {





volatile bool gDebugDisableHangMonitor = false;

const char kHangMonitorPrefName[] = "hangmonitor.timeout";




Monitor* gMonitor;


PRInt32 gTimeout;

PRThread* gThread;


bool gShutdown;



volatile PRIntervalTime gTimestamp;


int
PrefChanged(const char*, void*)
{
  PRInt32 newval = Preferences::GetInt(kHangMonitorPrefName);
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

void
ThreadMain(void*)
{
  MonitorAutoLock lock(*gMonitor);

  
  
  
  PRIntervalTime lastTimestamp = 0;
  int waitCount = 0;

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
        PRInt32 delay =
          PRInt32(PR_IntervalToSeconds(now - timestamp));
        if (delay > gTimeout) {
          MonitorAutoUnlock unlock(*gMonitor);
          Crash();
        }
      }
    }
    else {
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
