




#include "MediaDecoderStateMachineScheduler.h"
#include "SharedThreadPool.h"
#include "mozilla/Preferences.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsITimer.h"
#include "nsComponentManagerUtils.h"
#include "VideoUtils.h"

namespace {
class TimerEvent : public nsITimerCallback, public nsRunnable {
  typedef mozilla::MediaDecoderStateMachineScheduler Scheduler;
  NS_DECL_ISUPPORTS_INHERITED
public:
  TimerEvent(Scheduler* aScheduler, int aTimerId)
    : mScheduler(aScheduler), mTimerId(aTimerId) {}

  NS_IMETHOD Run() MOZ_OVERRIDE {
    return mScheduler->TimeoutExpired(mTimerId);
  }

  NS_IMETHOD Notify(nsITimer* aTimer) MOZ_OVERRIDE {
    return mScheduler->TimeoutExpired(mTimerId);
  }
private:
  ~TimerEvent() {}
  Scheduler* const mScheduler;
  const int mTimerId;
};

NS_IMPL_ISUPPORTS_INHERITED(TimerEvent, nsRunnable, nsITimerCallback);
} 

static already_AddRefed<nsIEventTarget>
CreateStateMachineThread()
{
  using mozilla::SharedThreadPool;
  using mozilla::RefPtr;
  RefPtr<SharedThreadPool> threadPool(
      SharedThreadPool::Get(NS_LITERAL_CSTRING("Media State Machine"), 1));
  nsCOMPtr<nsIEventTarget> rv = threadPool.get();
  return rv.forget();
}

namespace mozilla {

MediaDecoderStateMachineScheduler::MediaDecoderStateMachineScheduler(
    ReentrantMonitor& aMonitor,
    nsresult (*aTimeoutCallback)(void*),
    void* aClosure, bool aRealTime)
  : mTimeoutCallback(aTimeoutCallback)
  , mClosure(aClosure)
  
  , mRealTime(aRealTime &&
              Preferences::GetBool("media.realtime_decoder.enabled", false))
  , mMonitor(aMonitor)
  , mEventTarget(CreateStateMachineThread())
  , mTimer(do_CreateInstance("@mozilla.org/timer;1"))
  , mTimerId(0)
  , mState(SCHEDULER_STATE_NONE)
  , mInRunningStateMachine(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(MediaDecoderStateMachineScheduler);
}

MediaDecoderStateMachineScheduler::~MediaDecoderStateMachineScheduler()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_DTOR(MediaDecoderStateMachineScheduler);
}

nsresult
MediaDecoderStateMachineScheduler::Init()
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE(mEventTarget, NS_ERROR_FAILURE);
  nsresult rv = mTimer->SetTarget(mEventTarget);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
MediaDecoderStateMachineScheduler::Schedule(int64_t aUsecs)
{
  mMonitor.AssertCurrentThreadIn();

  if (NS_WARN_IF(mState == SCHEDULER_STATE_SHUTDOWN)) {
    return NS_ERROR_FAILURE;
  }

  aUsecs = std::max<int64_t>(aUsecs, 0);

  TimeStamp timeout = TimeStamp::Now() +
    TimeDuration::FromMilliseconds(static_cast<double>(aUsecs) / USECS_PER_MS);

  if (!mTimeout.IsNull() && timeout >= mTimeout) {
    
    
    return NS_OK;
  }

  uint32_t ms = static_cast<uint32_t>((aUsecs / USECS_PER_MS) & 0xFFFFFFFF);
  if (IsRealTime() && ms > 40) {
    ms = 40;
  }

  
  

  nsresult rv = NS_ERROR_FAILURE;
  nsRefPtr<TimerEvent> event = new TimerEvent(this, mTimerId+1);

  if (ms == 0) {
    
    
    
    rv = mEventTarget->Dispatch(event, NS_DISPATCH_NORMAL);
  } else if (OnStateMachineThread()) {
    rv = mTimer->InitWithCallback(event, ms, nsITimer::TYPE_ONE_SHOT);
  } else {
    MOZ_ASSERT(false, "non-zero delay timer should be only "
                      "scheduled in state machine thread");
  }

  if (NS_SUCCEEDED(rv)) {
    mTimeout = timeout;
    ++mTimerId;
  } else {
    NS_WARNING("Failed to schedule state machine");
  }

  return rv;
}

nsresult
MediaDecoderStateMachineScheduler::TimeoutExpired(int aTimerId)
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  MOZ_ASSERT(OnStateMachineThread());
  MOZ_ASSERT(!mInRunningStateMachine,
             "State machine cycles must run in sequence!");

  mInRunningStateMachine = true;
  
  nsresult rv = NS_OK;
  if (mTimerId == aTimerId) {
    ResetTimer();
    rv = mTimeoutCallback(mClosure);
  }
  mInRunningStateMachine = false;

  return rv;
}

void
MediaDecoderStateMachineScheduler::ScheduleAndShutdown()
{
  mMonitor.AssertCurrentThreadIn();
  
  Schedule();
  
  
  mState = SCHEDULER_STATE_SHUTDOWN;
}

bool
MediaDecoderStateMachineScheduler::OnStateMachineThread() const
{
  bool rv = false;
  mEventTarget->IsOnCurrentThread(&rv);
  return rv;
}

bool
MediaDecoderStateMachineScheduler::IsScheduled() const
{
  mMonitor.AssertCurrentThreadIn();
  return !mTimeout.IsNull();
}

void
MediaDecoderStateMachineScheduler::ResetTimer()
{
  mMonitor.AssertCurrentThreadIn();
  mTimer->Cancel();
  mTimeout = TimeStamp();
}

} 
