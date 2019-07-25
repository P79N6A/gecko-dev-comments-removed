


















































































#include "sampler.h"

#include "EventTracer.h"

#include <stdio.h>

#include "mozilla/TimeStamp.h"
#include "mozilla/WidgetTraceEvent.h"
#include <limits.h>
#include <prenv.h>
#include <prinrval.h>
#include <prthread.h>
#include <prtime.h>

using mozilla::TimeDuration;
using mozilla::TimeStamp;
using mozilla::FireAndWaitForTracerEvent;

namespace {

PRThread* sTracerThread = NULL;
bool sExit = false;











void TracerThread(void *arg)
{
  
  
  
  PRIntervalTime threshold = PR_MillisecondsToInterval(20);
  
  PRIntervalTime interval = PR_MillisecondsToInterval(10);

  FILE* log = NULL;
  char* envfile = PR_GetEnv("MOZ_INSTRUMENT_EVENT_LOOP_OUTPUT");
  if (envfile) {
    log = fopen(envfile, "w");
  }
  if (log == NULL)
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

  fprintf(log, "MOZ_EVENT_TRACE start %llu\n", PR_Now() / PR_USEC_PER_MSEC);

  while (!sExit) {
    TimeStamp start(TimeStamp::Now());
    SAMPLER_RESPONSIVENESS(start);
    PRIntervalTime next_sleep = interval;

    
    
    
    if (FireAndWaitForTracerEvent()) {
      TimeDuration duration = TimeStamp::Now() - start;
      
      if (duration.ToMilliseconds() > threshold) {
        fprintf(log, "MOZ_EVENT_TRACE sample %llu %d\n",
                PR_Now() / PR_USEC_PER_MSEC,
                int(duration.ToSecondsSigDigits() * 1000));
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

  fprintf(log, "MOZ_EVENT_TRACE stop %llu\n", PR_Now() / PR_USEC_PER_MSEC);

  if (log != stdout)
    fclose(log);
}

} 

namespace mozilla {

bool InitEventTracing()
{
  
  if (!InitWidgetTracing())
    return false;

  
  
  NS_ABORT_IF_FALSE(!sTracerThread, "Event tracing already initialized!");
  sTracerThread = PR_CreateThread(PR_USER_THREAD,
                                  TracerThread,
                                  NULL,
                                  PR_PRIORITY_NORMAL,
                                  PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD,
                                  0);
  return sTracerThread != NULL;
}

void ShutdownEventTracing()
{
  sExit = true;
  
  SignalTracerThread();

  if (sTracerThread)
    PR_JoinThread(sTracerThread);
  sTracerThread = NULL;

  
  CleanUpWidgetTracing();
}

}  
