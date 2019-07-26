




#ifndef TimerThread_h___
#define TimerThread_h___

#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"

#include "nsTimerImpl.h"
#include "nsThreadUtils.h"

#include "nsTArray.h"

#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"

class TimerThread MOZ_FINAL : public nsIRunnable,
                              public nsIObserver
{
public:
  typedef mozilla::Monitor Monitor;
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  TimerThread();
  NS_HIDDEN_(nsresult) InitLocks();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIOBSERVER
  
  NS_HIDDEN_(nsresult) Init();
  NS_HIDDEN_(nsresult) Shutdown();

  nsresult AddTimer(nsTimerImpl *aTimer);
  nsresult TimerDelayChanged(nsTimerImpl *aTimer);
  nsresult RemoveTimer(nsTimerImpl *aTimer);

  void DoBeforeSleep();
  void DoAfterSleep();

  bool IsOnTimerThread() const
  {
    return mThread == NS_GetCurrentThread();
  }

private:
  ~TimerThread();

  mozilla::Atomic<int32_t> mInitInProgress;
  bool    mInitialized;

  
  
  
  int32_t AddTimerInternal(nsTimerImpl *aTimer);
  bool    RemoveTimerInternal(nsTimerImpl *aTimer);
  void    ReleaseTimerInternal(nsTimerImpl *aTimer);

  nsCOMPtr<nsIThread> mThread;
  Monitor mMonitor;

  bool mShutdown;
  bool mWaiting;
  bool mSleeping;
  
  nsTArray<nsTimerImpl*> mTimers;
};

struct TimerAdditionComparator {
  TimerAdditionComparator(const mozilla::TimeStamp &aNow,
                          nsTimerImpl *aTimerToInsert) :
    now(aNow)
#ifdef DEBUG
    , timerToInsert(aTimerToInsert)
#endif
  {}

  bool LessThan(nsTimerImpl *fromArray, nsTimerImpl *newTimer) const {
    NS_ABORT_IF_FALSE(newTimer == timerToInsert, "Unexpected timer ordering");

    
    return fromArray->mTimeout <= now ||
           fromArray->mTimeout <= newTimer->mTimeout;
  }

  bool Equals(nsTimerImpl* fromArray, nsTimerImpl* newTimer) const {
    return false;
  }

private:
  const mozilla::TimeStamp &now;
#ifdef DEBUG
  const nsTimerImpl * const timerToInsert;
#endif
};

#endif 
