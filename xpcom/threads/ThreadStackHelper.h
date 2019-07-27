





#ifndef mozilla_ThreadStackHelper_h
#define mozilla_ThreadStackHelper_h

#include "mozilla/ThreadHangStats.h"

#include "GeckoProfiler.h"

#include <stddef.h>

#if defined(XP_LINUX)
#include <signal.h>
#include <semaphore.h>
#include <sys/types.h>
#elif defined(XP_WIN)
#include <windows.h>
#elif defined(XP_MACOSX)
#include <mach/mach.h>
#endif

namespace mozilla {











class ThreadStackHelper
{
public:
  typedef Telemetry::HangStack Stack;

private:
#ifdef MOZ_ENABLE_PROFILER_SPS
  const PseudoStack* const mPseudoStack;
#endif
  Stack* mStackToFill;
  size_t mMaxStackSize;
  size_t mMaxBufferSize;

  bool PrepareStackBuffer(Stack& aStack);
  void FillStackBuffer();
#ifdef MOZ_ENABLE_PROFILER_SPS
  const char* AppendJSEntry(const volatile StackEntry* aEntry,
                            intptr_t& aAvailableBufferSize,
                            const char* aPrevLabel);
#endif

public:
  


  static void Startup();
  


  static void Shutdown();

  


  ThreadStackHelper();

  ~ThreadStackHelper();

  





  void GetStack(Stack& aStack);

#if defined(XP_LINUX)
private:
  static int sInitialized;
  static int sFillStackSignum;

  static void FillStackHandler(int aSignal, siginfo_t* aInfo, void* aContext);

  sem_t mSem;
  pid_t mThreadID;

#elif defined(XP_WIN)
private:
  bool mInitialized;
  HANDLE mThreadID;

#elif defined(XP_MACOSX)
private:
  thread_act_t mThreadID;

#endif
};

} 

#endif 
