




































#ifndef __NSTIMER_H
#define __NSTIMER_H

#ifdef MOZ_PERF_METRICS

#include "stopwatch.h"

class nsStackBasedTimer 
{
public:
  nsStackBasedTimer(Stopwatch* aStopwatch) { sw = aStopwatch; }
  ~nsStackBasedTimer() { if (sw) sw->Stop(); }

  void Start(PRBool aReset) { if (sw) sw->Start(aReset); }
  void Stop(void) { if (sw) sw->Stop(); }
  void Reset(void) { if (sw) sw->Reset(); }
  void SaveState(void) { if (sw) sw->SaveState(); }
  void RestoreState(void) { if (sw) sw->RestoreState(); }
  void Print(void) { if (sw) sw->Print(); }
  
private:
  Stopwatch* sw;
};


#define ENABLE_DEBUG_OUTPUT PR_FALSE








#ifdef XP_MAC

#ifdef MOZ_TIMER_USE_MAC_ISDK

#include "InstrumentationHelpers.h"
#  define MOZ_TIMER_DECLARE(name)  
#  define MOZ_TIMER_CREATE(name)   \
  static InstTraceClassRef name = 0;  StInstrumentationLog __traceLog("Creating name..."), name)

#  define MOZ_TIMER_RESET(name, msg)
#  define MOZ_TIMER_START(name, msg)
#  define MOZ_TIMER_STOP(name, msg)
#  define MOZ_TIMER_SAVE(name, msg)
#  define MOZ_TIMER_RESTORE(name, msg)
#  define MOZ_TIMER_LOG(msg) \
  do { __traceLog.LogMiddleEvent(); } while(0)

#  define MOZ_TIMER_DEBUGLOG(msg) \
  if (ENABLE_DEBUG_OUTPUT) printf msg

#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data) \
  do { traceLog.LogMiddleEventWithData((msg), (data)); } while (0)

#else

#define MOZ_TIMER_USE_STOPWATCH

#endif  

#endif  



#ifdef XP_WIN

#ifdef MOZ_TIMER_USE_QUANTIFY

#include "pure.h"
#include "prprf.h"

#  define MOZ_TIMER_DECLARE(name)
#  define MOZ_TIMER_CREATE(name)
#  define MOZ_TIMER_RESET(name)  \
  QuantifyClearData()

#  define MOZ_TIMER_START(name)  \
  QuantifyStartRecordingData()
  
#  define MOZ_TIMER_STOP(name) \
  QuantifyStopRecordingData()

#  define MOZ_TIMER_SAVE(name)
#  define MOZ_TIMER_RESTORE(name)

#  define MOZ_TIMER_PRINT(name)
 
#  define MOZ_TIMER_LOG(msg)    \
do {                            \
  char* str = PR_smprintf msg;  \
  QuantifyAddAnnotation(str);   \
  PR_smprintf_free(str);        \
} while (0)

#  define MOZ_TIMER_DEBUGLOG(msg) \
  if (ENABLE_DEBUG_OUTPUT) printf msg

#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data)

#else

#define MOZ_TIMER_USE_STOPWATCH

#endif  

#endif  


#ifdef XP_UNIX

#define MOZ_TIMER_USE_STOPWATCH

#endif  

#ifdef MOZ_TIMER_USE_STOPWATCH

#  define MOZ_TIMER_DECLARE(name)  \
  Stopwatch name;

#  define MOZ_TIMER_CREATE(name)    \
  static Stopwatch __sw_name;  nsStackBasedTimer name(&__sw_name)

#  define MOZ_TIMER_RESET(name)  \
  name.Reset();

#  define MOZ_TIMER_START(name)  \
  name.Start(PR_FALSE);

#  define MOZ_TIMER_STOP(name) \
  name.Stop();

#  define MOZ_TIMER_SAVE(name) \
  name.SaveState();

#  define MOZ_TIMER_RESTORE(name)  \
  name.RestoreState();

#  define MOZ_TIMER_PRINT(name)   \
  name.Print();

#  define MOZ_TIMER_LOG(msg)  \
  printf msg

#  define MOZ_TIMER_DEBUGLOG(msg) \
  if (ENABLE_DEBUG_OUTPUT) printf msg

#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data)

#endif 

#else
#  define MOZ_TIMER_DECLARE(name)
#  define MOZ_TIMER_CREATE(name)
#  define MOZ_TIMER_RESET(name)
#  define MOZ_TIMER_START(name)
#  define MOZ_TIMER_STOP(name)
#  define MOZ_TIMER_SAVE(name)
#  define MOZ_TIMER_RESTORE(name)
#  define MOZ_TIMER_PRINT(name)
#  define MOZ_TIMER_LOG(msg)
#  define MOZ_TIMER_DEBUGLOG(msg)
#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data)
#endif  

#endif  

