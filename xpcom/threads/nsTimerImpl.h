







































#ifndef nsTimerImpl_h___
#define nsTimerImpl_h___



#include "nsITimer.h"
#include "nsVoidArray.h"
#include "nsIThread.h"
#include "nsIObserver.h"

#include "nsCOMPtr.h"

#include "prlog.h"

#if defined(PR_LOGGING)
static PRLogModuleInfo *gTimerLog = PR_NewLogModule("nsTimerImpl");
#define DEBUG_TIMERS 1
#else
#undef DEBUG_TIMERS
#endif

#define NS_TIMER_CLASSNAME "Timer"
#define NS_TIMER_CID \
{ /* 5ff24248-1dd2-11b2-8427-fbab44f29bc8 */         \
     0x5ff24248,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0x84, 0x27, 0xfb, 0xab, 0x44, 0xf2, 0x9b, 0xc8} \
}

enum {
  CALLBACK_TYPE_UNKNOWN   = 0,
  CALLBACK_TYPE_INTERFACE = 1,
  CALLBACK_TYPE_FUNC      = 2,
  CALLBACK_TYPE_OBSERVER  = 3
};


#define DELAY_INTERVAL_LIMIT    PR_BIT(8 * sizeof(PRIntervalTime) - 1)


#define DELAY_INTERVAL_MAX      (DELAY_INTERVAL_LIMIT - 1)


#define TIMER_LESS_THAN(t, u)   ((t) - (u) > DELAY_INTERVAL_LIMIT)

class nsTimerImpl : public nsITimer
{
public:

  nsTimerImpl();

  static NS_HIDDEN_(nsresult) Startup();
  static NS_HIDDEN_(void) Shutdown();

  friend class TimerThread;

  void Fire();
  void PostTimerEvent();
  void SetDelayInternal(PRUint32 aDelay);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMER

  PRInt32 GetGeneration() { return mGeneration; }

private:
  ~nsTimerImpl();
  nsresult InitCommon(PRUint32 aType, PRUint32 aDelay);

  void ReleaseCallback()
  {
    if (mCallbackType == CALLBACK_TYPE_INTERFACE)
      NS_RELEASE(mCallback.i);
    else if (mCallbackType == CALLBACK_TYPE_OBSERVER)
      NS_RELEASE(mCallback.o);
  }

  nsCOMPtr<nsIThread>   mCallingThread;

  void *                mClosure;

  union {
    nsTimerCallbackFunc c;
    nsITimerCallback *  i;
    nsIObserver *       o;
  } mCallback;

  
  PRUint8               mCallbackType;

  
  
  PRUint8               mType;
  PRPackedBool          mFiring;


  
  
  
  
  PRBool                mArmed;
  PRBool                mCanceled;

  
  
  
  PRInt32               mGeneration;

  PRUint32              mDelay;
  PRIntervalTime        mTimeout;

#ifdef DEBUG_TIMERS
  PRIntervalTime        mStart, mStart2;
  static double         sDeltaSum;
  static double         sDeltaSumSquared;
  static double         sDeltaNum;
#endif

};

#endif 
