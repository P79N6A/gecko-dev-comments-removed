





#ifndef mozilla_ReentrantMonitor_h
#define mozilla_ReentrantMonitor_h

#include "prmon.h"

#ifdef MOZILLA_INTERNAL_API
#include "GeckoProfiler.h"
#endif 

#include "mozilla/BlockingResourceBase.h"











namespace mozilla {








class ReentrantMonitor : BlockingResourceBase
{
public:
  



  explicit ReentrantMonitor(const char* aName)
    : BlockingResourceBase(aName, eReentrantMonitor)
#ifdef DEBUG
    , mEntryCount(0)
#endif
  {
    MOZ_COUNT_CTOR(ReentrantMonitor);
    mReentrantMonitor = PR_NewMonitor();
    if (!mReentrantMonitor) {
      NS_RUNTIMEABORT("Can't allocate mozilla::ReentrantMonitor");
    }
  }

  


  ~ReentrantMonitor()
  {
    NS_ASSERTION(mReentrantMonitor,
                 "improperly constructed ReentrantMonitor or double free");
    PR_DestroyMonitor(mReentrantMonitor);
    mReentrantMonitor = 0;
    MOZ_COUNT_DTOR(ReentrantMonitor);
  }

#ifndef DEBUG
  



  void Enter() { PR_EnterMonitor(mReentrantMonitor); }

  



  void Exit() { PR_ExitMonitor(mReentrantMonitor); }

  



  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT)
  {
#ifdef MOZILLA_INTERNAL_API
    GeckoProfilerSleepRAII profiler_sleep;
#endif 
    return PR_Wait(mReentrantMonitor, aInterval) == PR_SUCCESS ?
      NS_OK : NS_ERROR_FAILURE;
  }

#else 
  void Enter();
  void Exit();
  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT);

#endif  

  



  nsresult Notify()
  {
    return PR_Notify(mReentrantMonitor) == PR_SUCCESS ? NS_OK :
                                                        NS_ERROR_FAILURE;
  }

  



  nsresult NotifyAll()
  {
    return PR_NotifyAll(mReentrantMonitor) == PR_SUCCESS ? NS_OK :
                                                           NS_ERROR_FAILURE;
  }

#ifdef DEBUG
  



  void AssertCurrentThreadIn()
  {
    PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mReentrantMonitor);
  }

  



  void AssertNotCurrentThreadIn()
  {
    
  }

#else
  void AssertCurrentThreadIn() {}
  void AssertNotCurrentThreadIn() {}

#endif  

private:
  ReentrantMonitor();
  ReentrantMonitor(const ReentrantMonitor&);
  ReentrantMonitor& operator=(const ReentrantMonitor&);

  PRMonitor* mReentrantMonitor;
#ifdef DEBUG
  int32_t mEntryCount;
#endif
};









class MOZ_STACK_CLASS ReentrantMonitorAutoEnter
{
public:
  






  explicit ReentrantMonitorAutoEnter(mozilla::ReentrantMonitor& aReentrantMonitor)
    : mReentrantMonitor(&aReentrantMonitor)
  {
    NS_ASSERTION(mReentrantMonitor, "null monitor");
    mReentrantMonitor->Enter();
  }

  ~ReentrantMonitorAutoEnter(void)
  {
    mReentrantMonitor->Exit();
  }

  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT)
  {
    return mReentrantMonitor->Wait(aInterval);
  }

  nsresult Notify() { return mReentrantMonitor->Notify(); }
  nsresult NotifyAll() { return mReentrantMonitor->NotifyAll(); }

private:
  ReentrantMonitorAutoEnter();
  ReentrantMonitorAutoEnter(const ReentrantMonitorAutoEnter&);
  ReentrantMonitorAutoEnter& operator=(const ReentrantMonitorAutoEnter&);
  static void* operator new(size_t) CPP_THROW_NEW;
  static void operator delete(void*);

  mozilla::ReentrantMonitor* mReentrantMonitor;
};








class MOZ_STACK_CLASS ReentrantMonitorAutoExit
{
public:
  








  explicit ReentrantMonitorAutoExit(ReentrantMonitor& aReentrantMonitor)
    : mReentrantMonitor(&aReentrantMonitor)
  {
    NS_ASSERTION(mReentrantMonitor, "null monitor");
    mReentrantMonitor->AssertCurrentThreadIn();
    mReentrantMonitor->Exit();
  }

  ~ReentrantMonitorAutoExit(void)
  {
    mReentrantMonitor->Enter();
  }

private:
  ReentrantMonitorAutoExit();
  ReentrantMonitorAutoExit(const ReentrantMonitorAutoExit&);
  ReentrantMonitorAutoExit& operator=(const ReentrantMonitorAutoExit&);
  static void* operator new(size_t) CPP_THROW_NEW;
  static void operator delete(void*);

  ReentrantMonitor* mReentrantMonitor;
};

} 


#endif 
