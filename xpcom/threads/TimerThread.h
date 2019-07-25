




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
#define DELAY_LINE_LENGTH_MASK  PR_BITMASK(DELAY_LINE_LENGTH_LOG2)
#define DELAY_LINE_LENGTH       PR_BIT(DELAY_LINE_LENGTH_LOG2)

  int32_t  mDelayLine[DELAY_LINE_LENGTH]; 
  uint32_t mDelayLineCounter;
  uint32_t mMinTimerPeriod;     
  TimeDuration mTimeoutAdjustment;
};

#endif 
