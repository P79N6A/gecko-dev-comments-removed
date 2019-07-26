




#ifndef nsTimerImpl_h___
#define nsTimerImpl_h___



#include "nsITimer.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"

#include "nsCOMPtr.h"

#include "prlog.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo *GetTimerLog();
#define DEBUG_TIMERS 1
#else
#undef DEBUG_TIMERS
#endif

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

class nsTimerImpl MOZ_FINAL : public nsITimer
{
public:
  typedef mozilla::TimeStamp TimeStamp;

  nsTimerImpl();

  static NS_HIDDEN_(nsresult) Startup();
  static NS_HIDDEN_(void) Shutdown();

  friend class TimerThread;
  friend struct TimerAdditionComparator;

  void Fire();
  nsresult PostTimerEvent();
  void SetDelayInternal(uint32_t aDelay);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMER

  int32_t GetGeneration() { return mGeneration; }

private:
  ~nsTimerImpl();
  nsresult InitCommon(uint32_t aType, uint32_t aDelay);

  void ReleaseCallback()
  {
    
    
    
    uint8_t cbType = mCallbackType;
    mCallbackType = CALLBACK_TYPE_UNKNOWN; 

    if (cbType == CALLBACK_TYPE_INTERFACE)
      NS_RELEASE(mCallback.i);
    else if (cbType == CALLBACK_TYPE_OBSERVER)
      NS_RELEASE(mCallback.o);
  }

  bool IsRepeating() const {
    PR_STATIC_ASSERT(TYPE_ONE_SHOT < TYPE_REPEATING_SLACK);
    PR_STATIC_ASSERT(TYPE_REPEATING_SLACK < TYPE_REPEATING_PRECISE);
    PR_STATIC_ASSERT(TYPE_REPEATING_PRECISE < TYPE_REPEATING_PRECISE_CAN_SKIP);
    return mType >= TYPE_REPEATING_SLACK;
  }

  bool IsRepeatingPrecisely() const {
    return mType >= TYPE_REPEATING_PRECISE;
  }

  nsCOMPtr<nsIEventTarget> mEventTarget;

  void *                mClosure;

  union CallbackUnion {
    nsTimerCallbackFunc c;
    nsITimerCallback *  i;
    nsIObserver *       o;
  } mCallback;

  
  
  nsCOMPtr<nsITimerCallback> mTimerCallbackWhileFiring;

  
  uint8_t               mCallbackType;

  
  
  uint8_t               mType;
  bool                  mFiring;


  
  
  
  
  bool                  mArmed;
  bool                  mCanceled;

  
  
  
  int32_t               mGeneration;

  uint32_t              mDelay;
  TimeStamp             mTimeout;

#ifdef DEBUG_TIMERS
  TimeStamp             mStart, mStart2;
  static double         sDeltaSum;
  static double         sDeltaSumSquared;
  static double         sDeltaNum;
#endif

};

#endif 
