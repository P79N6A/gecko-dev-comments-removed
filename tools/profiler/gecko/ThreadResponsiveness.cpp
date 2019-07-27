




#include "ThreadResponsiveness.h"
#include "platform.h"
#include "nsComponentManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsITimer.h"
#include "mozilla/Monitor.h"
#include "ProfileEntry.h"

using mozilla::Monitor;
using mozilla::MonitorAutoLock;
using mozilla::TimeStamp;

class CheckResponsivenessTask : public nsRunnable,
                                public nsITimerCallback {
public:
  CheckResponsivenessTask()
    : mLastTracerTime(TimeStamp::Now())
    , mMonitor("CheckResponsivenessTask")
    , mTimer(nullptr)
    , mStop(false)
  {
    MOZ_COUNT_CTOR(CheckResponsivenessTask);
  }

protected:
  ~CheckResponsivenessTask()
  {
    MOZ_COUNT_DTOR(CheckResponsivenessTask);
  }

public:
  NS_IMETHOD Run() override
  {
    MonitorAutoLock mon(mMonitor);
    if (mStop)
      return NS_OK;

    
    
    
    
    
    mLastTracerTime = TimeStamp::Now();
    if (!mTimer) {
      mTimer = do_CreateInstance("@mozilla.org/timer;1");
    }
    mTimer->InitWithCallback(this, 16, nsITimer::TYPE_ONE_SHOT);

    return NS_OK;
  }

  NS_IMETHODIMP Notify(nsITimer* aTimer) final
  {
    NS_DispatchToMainThread(this);
    return NS_OK;
  }

  void Terminate() {
    MonitorAutoLock mon(mMonitor);
    mStop = true;
  }

  const TimeStamp& GetLastTracerTime() const {
    return mLastTracerTime;
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  TimeStamp mLastTracerTime;
  Monitor mMonitor;
  nsCOMPtr<nsITimer> mTimer;
  bool mStop;
};

NS_IMPL_ISUPPORTS_INHERITED(CheckResponsivenessTask, nsRunnable, nsITimerCallback)

ThreadResponsiveness::ThreadResponsiveness(ThreadProfile *aThreadProfile)
  : mThreadProfile(aThreadProfile)
  , mActiveTracerEvent(nullptr)
{
  MOZ_COUNT_CTOR(ThreadResponsiveness);
}

ThreadResponsiveness::~ThreadResponsiveness()
{
  MOZ_COUNT_DTOR(ThreadResponsiveness);
  if (mActiveTracerEvent) {
    mActiveTracerEvent->Terminate();
  }
}

void
ThreadResponsiveness::Update()
{
  if (!mActiveTracerEvent) {
    if (mThreadProfile->GetThreadInfo()->IsMainThread()) {
      mActiveTracerEvent = new CheckResponsivenessTask();
      NS_DispatchToMainThread(mActiveTracerEvent);
    } else if (mThreadProfile->GetThreadInfo()->GetThread()) {
      mActiveTracerEvent = new CheckResponsivenessTask();
      mThreadProfile->GetThreadInfo()->
        GetThread()->Dispatch(mActiveTracerEvent, NS_DISPATCH_NORMAL);
    }
  }

  if (mActiveTracerEvent) {
    mLastTracerTime = mActiveTracerEvent->GetLastTracerTime();
  }
}

