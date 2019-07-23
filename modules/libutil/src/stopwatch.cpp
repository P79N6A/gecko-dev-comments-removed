#include "stopwatch.h"
#include <stdio.h>
#include <time.h>
#ifdef XP_UNIX
#include <unistd.h>
#include <sys/times.h>
#endif
#ifdef XP_WIN
#include "windows.h"
#endif
#include "nsDebug.h"




#define MILLISECOND_RESOLUTION
#ifdef MILLISECOND_RESOLUTION
double gTicks = 1.0e-4; 
#else
double gTicks = 1.0e-7; 
#endif

Stopwatch::Stopwatch() {

#ifdef R__UNIX
   if (!gTicks) gTicks = (clock_t)sysconf(_SC_CLK_TCK);
#endif
   fState         = kUndefined;   
   fTotalCpuTime  = 0;
   fTotalRealTime = 0;   
   mCreatedStack = PR_FALSE;
   mSavedStates = nsnull;
   Start();
}

Stopwatch::~Stopwatch() {
  EState* state = 0;
  if (mSavedStates) {
    while ((state = (EState*) mSavedStates->Pop())) {
      delete state;
    } 
    delete mSavedStates;
  }
}

void Stopwatch::Start(PRBool reset) {
   if (reset) {
      fTotalCpuTime  = 0;
      fTotalRealTime = 0;
   }
   if (fState != kRunning) {
#ifndef R__UNIX
      fStartRealTime = GetRealTime();
      fStartCpuTime  = GetCPUTime();
#else
      struct tms cpt;
      fStartRealTime = (double)times(&cpt) / gTicks;
      fStartCpuTime  = (double)(cpt.tms_utime+cpt.tms_stime) / gTicks;
#endif
   }
   fState = kRunning;
}

void Stopwatch::Stop() {

#ifndef R__UNIX
   fStopRealTime = GetRealTime();
   fStopCpuTime  = GetCPUTime();
#else
   struct tms cpt;
   fStopRealTime = (double)times(&cpt) / gTicks;
   fStopCpuTime  = (double)(cpt.tms_utime+cpt.tms_stime) / gTicks;
#endif
   if (fState == kRunning) {
      fTotalCpuTime  += fStopCpuTime  - fStartCpuTime;
      fTotalRealTime += fStopRealTime - fStartRealTime;
   }
   fState = kStopped;
}


void Stopwatch::SaveState() {
  if (!mCreatedStack) {
    mSavedStates = new nsDeque(nsnull);
    if (!mSavedStates)
      return;
    mCreatedStack = PR_TRUE;
  }
  EState* state = new EState();
  if (state) {
    *state = fState;
    mSavedStates->PushFront((void*) state);
  }
}

void Stopwatch::RestoreState() {
  EState* state = nsnull;
  state = (EState*) mSavedStates->Pop();
  if (state) {
    if (*state == kRunning && fState == kStopped)
      Start(PR_FALSE);
    else if (*state == kStopped && fState == kRunning)
      Stop();
    delete state;
  }
  else {
    NS_WARNING("Stopwatch::RestoreState(): The saved state stack is empty.\n");
  }
}

void Stopwatch::Continue() {

  if (fState != kUndefined) {

    if (fState == kStopped) {
      fTotalCpuTime  -= fStopCpuTime  - fStartCpuTime;
      fTotalRealTime -= fStopRealTime - fStartRealTime;
    }

    fState = kRunning;
  }
}




double Stopwatch::RealTime() {

  if (fState != kUndefined) {
    if (fState == kRunning)
      Stop();
  }

#ifdef MILLISECOND_RESOLUTION
  return fTotalRealTime/1000;
#else
  return fTotalRealTime;
#endif
}



double Stopwatch::RealTimeInMilliseconds() {

  if (fState != kUndefined) {
    if (fState == kRunning)
      Stop();
  }

#ifdef MILLISECOND_RESOLUTION
  return fTotalRealTime;
#else
  return fTotalRealTime * 1000; 
#endif
}



double Stopwatch::CpuTime() {
  if (fState != kUndefined) {

    if (fState == kRunning)
      Stop();

  }
#ifdef MILLISECOND_RESOLUTION
  return fTotalCpuTime / 1000;  
#else
  return fTotalCpuTime;
#endif
}


double Stopwatch::GetRealTime(){ 
#if defined(R__MAC)

   return(double)clock() / 1000000L;
#elif defined(R__UNIX)
   struct tms cpt;
   return (double)times(&cpt) / gTicks;
#elif defined(R__VMS)
  return(double)clock()/gTicks;
#elif defined(WIN32)
  union     {FILETIME ftFileTime;
             __int64  ftInt64;
            } ftRealTime; 
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st,&ftRealTime.ftFileTime);
  return (double)ftRealTime.ftInt64 * gTicks;
#endif
}


double Stopwatch::GetCPUTime(){ 
#if defined(R__MAC)

   return(double)clock();
#elif defined(R__UNIX)
   struct tms cpt;
   times(&cpt);
   return (double)(cpt.tms_utime+cpt.tms_stime) / gTicks;
#elif defined(R__VMS)
   return(double)clock()/gTicks;
#elif defined(WINCE)
   return 0;
#elif defined(WIN32)

  DWORD       ret;
  FILETIME    ftCreate,       
              ftExit;         

  union     {FILETIME ftFileTime;
             __int64  ftInt64;
            } ftKernel; 

  union     {FILETIME ftFileTime;
             __int64  ftInt64;
            } ftUser;   

  HANDLE hProcess = GetCurrentProcess();
  ret = GetProcessTimes (hProcess, &ftCreate, &ftExit,
                                   &ftKernel.ftFileTime,
                                   &ftUser.ftFileTime);
  if (ret != PR_TRUE){
    ret = GetLastError ();
#ifdef DEBUG
    printf("%s 0x%lx\n"," Error on GetProcessTimes", (int)ret);
#endif
    }

  








    return (double) (ftKernel.ftInt64 + ftUser.ftInt64) * gTicks;

#endif
}


void Stopwatch::Print(void) {
   

   double  realt = RealTimeInMilliseconds();

   int  hours = int(realt / 3600000);
   realt -= hours * 3600000;
   int  min   = int(realt / 60000);
   realt -= min * 60000;
   int  sec   = int(realt/1000);
   realt -= sec * 1000;
#ifdef MOZ_PERF_METRICS
  int ms = int(realt);
   RAPTOR_STOPWATCH_TRACE(("Real time %d:%d:%d.%d, CP time %.3f\n", hours, min, sec, ms, CpuTime()));
#elif defined(DEBUG)
  int ms = int(realt);
   printf("Real time %d:%d:%d.%d, CP time %.3f\n", hours, min, sec, ms, CpuTime());
#endif
}
