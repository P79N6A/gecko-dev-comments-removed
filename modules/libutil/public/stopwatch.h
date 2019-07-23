#ifndef __STOPWATCH_H
#define __STOPWATCH_H
#include "nscore.h"
#include "prlog.h"
#include "nsDeque.h"

#ifdef XP_MAC
#define R__MAC
#endif

#ifdef XP_UNIX
#define R__UNIX
#endif

#ifdef MOZ_PERF_METRICS
#  define NS_RESET_AND_START_STOPWATCH(_sw)          \
    _sw.Start(PR_TRUE);

#  define NS_START_STOPWATCH(_sw)                    \
    _sw.Start(PR_FALSE);

#  define NS_STOP_STOPWATCH(_sw)                     \
    _sw.Stop();

#  define NS_SAVE_STOPWATCH_STATE(_sw)               \
    _sw.SaveState();

#  define NS_RESTORE_STOPWATCH_STATE(_sw)            \
    _sw.RestoreState();

#else
#  define NS_RESET_AND_START_STOPWATCH(_sw) 
#  define NS_START_STOPWATCH(_sw)
#  define NS_STOP_STOPWATCH(_sw)
#  define NS_SAVE_STOPWATCH_STATE(_sw)
#  define NS_RESTORE_STOPWATCH_STATE(_sw)
#endif


#ifdef MOZ_PERF_METRICS

static PRLogModuleInfo* gLogStopwatchModule = PR_NewLogModule("timing");

#if 0
#define RAPTOR_TRACE_STOPWATCHES        0x1

#define RAPTOR_STOPWATCH_TRACE(_args)                               \
  PR_BEGIN_MACRO                                                    \
  PR_LOG(gLogStopwatchModule, RAPTOR_TRACE_STOPWATCHES, _args);     \
  PR_END_MACRO
#endif

#define RAPTOR_STOPWATCH_TRACE(_args)      \
  PR_BEGIN_MACRO                           \
  printf _args ;                           \
  PR_END_MACRO

#else
#define RAPTOR_TRACE_STOPWATCHES 
#define RAPTOR_STOPWATCH_TRACE(_args) 
#endif

#ifdef DEBUG_STOPWATCH
#define RAPTOR_STOPWATCH_DEBUGTRACE(_args)      \
  PR_BEGIN_MACRO                                \
  printf _args ;                                \
  PR_END_MACRO
#else
#define RAPTOR_STOPWATCH_DEBUGTRACE(_args) 
#endif

class Stopwatch {

private:
   enum EState { kUndefined, kStopped, kRunning };

   double         fStartRealTime;   
   double         fStopRealTime;    
   double         fStartCpuTime;    
   double         fStopCpuTime;     
   double         fTotalCpuTime;    
   double         fTotalRealTime;   
   EState         fState;           
   nsDeque*       mSavedStates;     
   PRBool         mCreatedStack;    

public:
   Stopwatch();
   virtual ~Stopwatch();

   void           Start(PRBool reset = PR_TRUE);
   void           Stop();
   void           Continue();
   void           SaveState();      
   void           RestoreState();   
   double         RealTime();
   double         RealTimeInMilliseconds();
   void           Reset() { ResetCpuTime(); ResetRealTime(); }
   void           ResetCpuTime(double aTime = 0) { Stop();  fTotalCpuTime = aTime; }
   void           ResetRealTime(double aTime = 0) { Stop(); fTotalRealTime = aTime; }
   double         CpuTime();
   void           Print(void);
   static double  GetRealTime();
   static double  GetCPUTime();

};
#endif
