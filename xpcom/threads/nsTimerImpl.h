





#ifndef nsTimerImpl_h___
#define nsTimerImpl_h___

#include "nsITimer.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"

#include "nsCOMPtr.h"

#include "prlog.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"

#ifdef MOZ_TASK_TRACER
#include "TracedTaskCommon.h"
#endif

#if defined(PR_LOGGING)
extern PRLogModuleInfo* GetTimerLog();
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

class nsTimerImpl final : public nsITimer
{
public:
  typedef mozilla::TimeStamp TimeStamp;

  nsTimerImpl();

  static nsresult Startup();
  static void Shutdown();

  friend class TimerThread;
  friend struct TimerAdditionComparator;

  void Fire();
  
  static already_AddRefed<nsTimerImpl> PostTimerEvent(
    already_AddRefed<nsTimerImpl> aTimerRef);
  void SetDelayInternal(uint32_t aDelay);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMER

  int32_t GetGeneration()
  {
    return mGeneration;
  }

#ifdef MOZ_TASK_TRACER
  void DispatchTracedTask()
  {
    mTracedTask = mozilla::tasktracer::CreateFakeTracedTask(*(int**)(this));
  }
#endif

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override;

private:
  enum class CallbackType : uint8_t {
    Unknown = 0,
    Interface = 1,
    Function = 2,
    Observer = 3,
  };

  ~nsTimerImpl();
  nsresult InitCommon(uint32_t aType, uint32_t aDelay);

  void ReleaseCallback()
  {
    
    
    
    CallbackType cbType = mCallbackType;
    mCallbackType = CallbackType::Unknown;

    if (cbType == CallbackType::Interface) {
      NS_RELEASE(mCallback.i);
    } else if (cbType == CallbackType::Observer) {
      NS_RELEASE(mCallback.o);
    }
  }

  bool IsRepeating() const
  {
    static_assert(TYPE_ONE_SHOT < TYPE_REPEATING_SLACK,
                  "invalid ordering of timer types!");
    static_assert(TYPE_REPEATING_SLACK < TYPE_REPEATING_PRECISE,
                  "invalid ordering of timer types!");
    static_assert(TYPE_REPEATING_PRECISE < TYPE_REPEATING_PRECISE_CAN_SKIP,
                  "invalid ordering of timer types!");
    return mType >= TYPE_REPEATING_SLACK;
  }

  bool IsRepeatingPrecisely() const
  {
    return mType >= TYPE_REPEATING_PRECISE;
  }

  nsCOMPtr<nsIEventTarget> mEventTarget;

  void*                 mClosure;

  union CallbackUnion
  {
    nsTimerCallbackFunc c;
    nsITimerCallback*   i;
    nsIObserver*        o;
  } mCallback;

  
  
  nsCOMPtr<nsITimerCallback> mTimerCallbackWhileFiring;

  
  CallbackType          mCallbackType;

  
  
  uint8_t               mType;
  bool                  mFiring;


  
  
  
  
  bool                  mArmed;
  bool                  mCanceled;

  
  
  
  int32_t               mGeneration;

  uint32_t              mDelay;
  TimeStamp             mTimeout;

#ifdef MOZ_TASK_TRACER
  nsRefPtr<mozilla::tasktracer::FakeTracedTask> mTracedTask;
#endif

#ifdef DEBUG_TIMERS
  TimeStamp             mStart, mStart2;
  static double         sDeltaSum;
  static double         sDeltaSumSquared;
  static double         sDeltaNum;
#endif

};

#endif
