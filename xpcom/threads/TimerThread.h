







































#ifndef TimerThread_h___
#define TimerThread_h___

#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"

#include "nsTimerImpl.h"

#include "nsTArray.h"

#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"

class TimerThread : public nsIRunnable,
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

  void UpdateFilter(PRUint32 aDelay, TimeStamp aTimeout,
                    TimeStamp aNow);

  void DoBeforeSleep();
  void DoAfterSleep();

private:
  ~TimerThread();

  PRInt32 mInitInProgress;
  PRBool  mInitialized;

  
  
  
  PRInt32 AddTimerInternal(nsTimerImpl *aTimer);
  PRBool  RemoveTimerInternal(nsTimerImpl *aTimer);
  void    ReleaseTimerInternal(nsTimerImpl *aTimer);

  nsCOMPtr<nsIThread> mThread;
  Monitor mMonitor;

  PRPackedBool mShutdown;
  PRPackedBool mWaiting;
  PRPackedBool mSleeping;
  
  nsTArray<nsTimerImpl*> mTimers;

#define DELAY_LINE_LENGTH_LOG2  5
#define DELAY_LINE_LENGTH_MASK  PR_BITMASK(DELAY_LINE_LENGTH_LOG2)
#define DELAY_LINE_LENGTH       PR_BIT(DELAY_LINE_LENGTH_LOG2)

  PRInt32  mDelayLine[DELAY_LINE_LENGTH]; 
  PRUint32 mDelayLineCounter;
  PRUint32 mMinTimerPeriod;     
  TimeDuration mTimeoutAdjustment;
};

#endif 
