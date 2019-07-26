




#ifndef TimerThread_h___
#define TimerThread_h___

#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"

#include "nsTimerImpl.h"

#include "nsTArray.h"

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

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIOBSERVER
  
  NS_HIDDEN_(nsresult) Init();
  NS_HIDDEN_(nsresult) Shutdown();

  nsresult AddTimer(nsTimerImpl *aTimer);
  nsresult TimerDelayChanged(nsTimerImpl *aTimer);
  nsresult RemoveTimer(nsTimerImpl *aTimer);

#define FILTER_DURATION         1e3     /* one second */
#define FILTER_FEEDBACK_MAX     100     /* 1/10th of a second */

  void UpdateFilter(uint32_t aDelay, TimeStamp aTimeout,
                    TimeStamp aNow);

  void DoBeforeSleep();
  void DoAfterSleep();

private:
  ~TimerThread();

  int32_t mInitInProgress;
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

#define DELAY_LINE_LENGTH_LOG2  5
#define DELAY_LINE_LENGTH_MASK  ((1u << DELAY_LINE_LENGTH_LOG2) - 1)
#define DELAY_LINE_LENGTH       (1u << DELAY_LINE_LENGTH_LOG2)

  int32_t  mDelayLine[DELAY_LINE_LENGTH]; 
  uint32_t mDelayLineCounter;
  uint32_t mMinTimerPeriod;     
  TimeDuration mTimeoutAdjustment;
};

struct TimerAdditionComparator {
  TimerAdditionComparator(const mozilla::TimeStamp &aNow,
                          const mozilla::TimeDuration &aTimeoutAdjustment,
                          nsTimerImpl *aTimerToInsert) :
    now(aNow),
    timeoutAdjustment(aTimeoutAdjustment)
#ifdef DEBUG
    , timerToInsert(aTimerToInsert)
#endif
  {}

  PRBool LessThan(nsTimerImpl *fromArray, nsTimerImpl *newTimer) const {
    NS_ABORT_IF_FALSE(newTimer == timerToInsert, "Unexpected timer ordering");

    

    
    
    
    return now >= fromArray->mTimeout + timeoutAdjustment ||
           fromArray->mTimeout <= newTimer->mTimeout;
  }

  PRBool Equals(nsTimerImpl* fromArray, nsTimerImpl* newTimer) const {
    return PR_FALSE;
  }

private:
  const mozilla::TimeStamp &now;
  const mozilla::TimeDuration &timeoutAdjustment;
#ifdef DEBUG
  const nsTimerImpl * const timerToInsert;
#endif
};

#endif 
