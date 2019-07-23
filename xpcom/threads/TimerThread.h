







































#ifndef TimerThread_h___
#define TimerThread_h___

#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"

#include "nsTimerImpl.h"

#include "nsVoidArray.h"

#include "prcvar.h"
#include "prinrval.h"
#include "prlock.h"

class TimerThread : public nsIRunnable,
                    public nsIObserver
{
public:
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

  void UpdateFilter(PRUint32 aDelay, PRIntervalTime aTimeout,
                    PRIntervalTime aNow);

  void DoBeforeSleep();
  void DoAfterSleep();

private:
  ~TimerThread();

  PRInt32 mInitInProgress;
  PRBool  mInitialized;

  
  
  
  PRInt32 AddTimerInternal(nsTimerImpl *aTimer);
  PRBool  RemoveTimerInternal(nsTimerImpl *aTimer);

  nsCOMPtr<nsIThread> mThread;
  PRLock *mLock;
  PRCondVar *mCondVar;

  PRPackedBool mShutdown;
  PRPackedBool mWaiting;
  PRPackedBool mSleeping;
  
  nsVoidArray mTimers;

#define DELAY_LINE_LENGTH_LOG2  5
#define DELAY_LINE_LENGTH_MASK  PR_BITMASK(DELAY_LINE_LENGTH_LOG2)
#define DELAY_LINE_LENGTH       PR_BIT(DELAY_LINE_LENGTH_LOG2)

  PRInt32  mDelayLine[DELAY_LINE_LENGTH];
  PRUint32 mDelayLineCounter;
  PRUint32 mMinTimerPeriod;     
  PRInt32  mTimeoutAdjustment;
};

#endif 
