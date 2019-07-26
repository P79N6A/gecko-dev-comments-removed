


















































#include "GeckoProfiler.h"

#include "EventTracer.h"

#include <stdio.h>

#include "mozilla/TimeStamp.h"
#include "mozilla/WidgetTraceEvent.h"
#include "nsDebug.h"
#include "MainThreadUtils.h"
#include <limits.h>
#include <prenv.h>
#include <prinrval.h>
#include <prthread.h>
#include <prtime.h>

#ifdef MOZ_WIDGET_GONK
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#endif

using mozilla::TimeDuration;
using mozilla::TimeStamp;
using mozilla::FireAndWaitForTracerEvent;

namespace {
  struct TracerStartClosure {
    mozilla::Atomic<int> mLogTracing;
  };
}

static PRThread* sTracerThread = nullptr;
static mozilla::Atomic<int> sExit(0);
static int sStartCount = 0;
static bool sLogging = false;
static TracerStartClosure *sTracerStartClosure = nullptr;

#ifdef MOZ_WIDGET_GONK
class EventLoopLagDispatcher : public nsRunnable
{
  public:
    explicit EventLoopLagDispatcher(int aLag)
      : mLag(aLag) {}

    NS_IMETHODIMP Run()
    {
      nsCOMPtr<nsIObserverService> obsService =
        mozilla::services::GetObserverService();
      if (!obsService) {
        return NS_ERROR_FAILURE;
      }

      nsAutoString value;
      value.AppendInt(mLag);
      return obsService->NotifyObservers(nullptr, "event-loop-lag", value.get());
    }

  private:
    int mLag;
};
#endif











static void TracerThread(void *arg)
{
  PR_SetCurrentThreadName("Event Tracer");

  TracerStartClosure* threadArgs = static_cast<TracerStartClosure*>(arg);

  
  
  
  PRIntervalTime threshold = PR_MillisecondsToInterval(20);
  
  PRIntervalTime interval = PR_MillisecondsToInterval(10);

  sExit = false;
  FILE* log = nullptr;
  char* envfile = PR_GetEnv("MOZ_INSTRUMENT_EVENT_LOOP_OUTPUT");
  if (envfile) {
    log = fopen(envfile, "w");
  }
  if (log == nullptr)
    log = stdout;

  char* thresholdenv = PR_GetEnv("MOZ_INSTRUMENT_EVENT_LOOP_THRESHOLD");
  if (thresholdenv && *thresholdenv) {
    int val = atoi(thresholdenv);
    if (val != 0 && val != INT_MAX && val != INT_MIN) {
      threshold = PR_MillisecondsToInterval(val);
    }
  }

  char* intervalenv = PR_GetEnv("MOZ_INSTRUMENT_EVENT_LOOP_INTERVAL");
  if (intervalenv && *intervalenv) {
    int val = atoi(intervalenv);
    if (val != 0 && val != INT_MAX && val != INT_MIN) {
      interval = PR_MillisecondsToInterval(val);
    }
  }

  if (threadArgs->mLogTracing) {
    long long now = PR_Now() / PR_USEC_PER_MSEC;
    fprintf(log, "MOZ_EVENT_TRACE start %llu\n", now);
  }

  while (!sExit) {
    TimeStamp start(TimeStamp::Now());
    profiler_responsiveness(start);
    PRIntervalTime next_sleep = interval;

    
    
    
    if (FireAndWaitForTracerEvent()) {
      TimeDuration duration = TimeStamp::Now() - start;
      
      long long now = PR_Now() / PR_USEC_PER_MSEC;
      if (threadArgs->mLogTracing && duration.ToMilliseconds() > threshold) {
        fprintf(log, "MOZ_EVENT_TRACE sample %llu %lf\n",
                now,
                duration.ToMilliseconds());
#ifdef MOZ_WIDGET_GONK
        NS_DispatchToMainThread(
         new EventLoopLagDispatcher(int(duration.ToSecondsSigDigits() * 1000)));
#endif
      }

      if (next_sleep > duration.ToMilliseconds()) {
        next_sleep -= int(duration.ToMilliseconds());
      }
      else {
        
        
        next_sleep = 0;
      }
    }

    if (next_sleep != 0 && !sExit) {
      PR_Sleep(next_sleep);
    }
  }

  if (threadArgs->mLogTracing) {
    long long now = PR_Now() / PR_USEC_PER_MSEC;
    fprintf(log, "MOZ_EVENT_TRACE stop %llu\n", now);
  }

  if (log != stdout)
    fclose(log);

  delete threadArgs;
}


namespace mozilla {

bool InitEventTracing(bool aLog)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  sStartCount++;

  
  
  if (sTracerThread) {
    if (aLog)
      sTracerStartClosure->mLogTracing = 1;
    return true;
  }

  
  if (!InitWidgetTracing())
    return false;

  
  sTracerStartClosure = new TracerStartClosure();
  sTracerStartClosure->mLogTracing = aLog;

  
  
  NS_ABORT_IF_FALSE(!sTracerThread, "Event tracing already initialized!");
  sTracerThread = PR_CreateThread(PR_USER_THREAD,
                                  TracerThread,
                                  sTracerStartClosure,
                                  PR_PRIORITY_NORMAL,
                                  PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD,
                                  0);
  return sTracerThread != nullptr;
}

void ShutdownEventTracing()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  sStartCount--;
  if (sStartCount > 0)
    return;

  if (!sTracerThread)
    return;

  sExit = 1;
  
  SignalTracerThread();

  if (sTracerThread)
    PR_JoinThread(sTracerThread);
  sTracerThread = nullptr;
  sTracerStartClosure = nullptr;

  
  CleanUpWidgetTracing();
}

}  
