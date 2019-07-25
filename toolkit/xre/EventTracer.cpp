






































































#include "EventTracer.h"

#include <stdio.h>

#include "mozilla/TimeStamp.h"
#include "mozilla/WidgetTraceEvent.h"
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
  
  
  const PRIntervalTime kMeasureInterval = PR_MillisecondsToInterval(50);

  FILE* log = NULL;
  char* envfile = PR_GetEnv("MOZ_INSTRUMENT_EVENT_LOOP_OUTPUT");
  if (envfile) {
    log = fopen(envfile, "w");
  }
  if (log == NULL)
    log = stdout;

  fprintf(log, "MOZ_EVENT_TRACE start %llu\n", PR_Now() / PR_USEC_PER_MSEC);

  while (!sExit) {
    TimeStamp start(TimeStamp::Now());
    PRIntervalTime next_sleep = kMeasureInterval;

    
    
    
    if (FireAndWaitForTracerEvent()) {
      TimeDuration duration = TimeStamp::Now() - start;
      
      if (duration.ToMilliseconds() > kMeasureInterval) {
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
  if (sTracerThread)
    PR_JoinThread(sTracerThread);
  sTracerThread = NULL;

  
  CleanUpWidgetTracing();
}

}  
