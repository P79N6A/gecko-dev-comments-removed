





#include "MediaTimer.h"

#include <math.h>

#include "nsComponentManagerUtils.h"
#include "nsThreadUtils.h"

namespace mozilla {

NS_IMPL_ADDREF(MediaTimer)
NS_IMPL_RELEASE_WITH_DESTROY(MediaTimer, DispatchDestroy())

MediaTimer::MediaTimer()
  : mMonitor("MediaTimer Monitor")
  , mTimer(do_CreateInstance("@mozilla.org/timer;1"))
  , mUpdateScheduled(false)
{
  
  
  RefPtr<SharedThreadPool> threadPool(
    SharedThreadPool::Get(NS_LITERAL_CSTRING("MediaTimer"), 1));
  mThread = threadPool.get();
  mTimer->SetTarget(mThread);
}

void
MediaTimer::DispatchDestroy()
{
  nsCOMPtr<nsIRunnable> task = NS_NewNonOwningRunnableMethod(this, &MediaTimer::Destroy);
  nsresult rv = mThread->Dispatch(task, NS_DISPATCH_NORMAL);
  MOZ_DIAGNOSTIC_ASSERT(NS_SUCCEEDED(rv));
  (void) rv;
}

void
MediaTimer::Destroy()
{
  MOZ_ASSERT(OnMediaTimerThread());

  
  
  
  while (!mEntries.empty()) {
    mEntries.top().mPromise->Reject(false, __func__);
    mEntries.pop();
  }

  
  CancelTimerIfArmed();

  delete this;
}

bool
MediaTimer::OnMediaTimerThread()
{
  bool rv = false;
  mThread->IsOnCurrentThread(&rv);
  return rv;
}

nsRefPtr<MediaTimerPromise>
MediaTimer::WaitUntil(const TimeStamp& aTimeStamp, const char* aCallSite)
{
  MonitorAutoLock mon(mMonitor);
  Entry e(aTimeStamp, aCallSite);
  nsRefPtr<MediaTimerPromise> p = e.mPromise.get();
  mEntries.push(e);
  ScheduleUpdate();
  return p;
}

void
MediaTimer::ScheduleUpdate()
{
  mMonitor.AssertCurrentThreadOwns();
  if (mUpdateScheduled) {
    return;
  }
  mUpdateScheduled = true;

  nsCOMPtr<nsIRunnable> task = NS_NewRunnableMethod(this, &MediaTimer::Update);
  nsresult rv = mThread->Dispatch(task, NS_DISPATCH_NORMAL);
  MOZ_DIAGNOSTIC_ASSERT(NS_SUCCEEDED(rv));
  (void) rv;
}

void
MediaTimer::Update()
{
  MonitorAutoLock mon(mMonitor);
  UpdateLocked();
}

void
MediaTimer::UpdateLocked()
{
  MOZ_ASSERT(OnMediaTimerThread());
  mMonitor.AssertCurrentThreadOwns();
  mUpdateScheduled = false;

  
  TimeStamp now = TimeStamp::Now();
  while (!mEntries.empty() && mEntries.top().mTimeStamp < now) {
    mEntries.top().mPromise->Resolve(true, __func__);
    mEntries.pop();
  }

  
  if (mEntries.empty()) {
    CancelTimerIfArmed();
    return;
  }

  
  if (!TimerIsArmed() || mEntries.top().mTimeStamp < mCurrentTimerTarget) {
    CancelTimerIfArmed();
    ArmTimer(mEntries.top().mTimeStamp, now);
  }
}








 void
MediaTimer::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  static_cast<MediaTimer*>(aClosure)->TimerFired();
}

void
MediaTimer::TimerFired()
{
  MonitorAutoLock mon(mMonitor);
  MOZ_ASSERT(OnMediaTimerThread());
  mCurrentTimerTarget = TimeStamp();
  UpdateLocked();
}

void
MediaTimer::ArmTimer(const TimeStamp& aTarget, const TimeStamp& aNow)
{
  MOZ_DIAGNOSTIC_ASSERT(!TimerIsArmed());
  MOZ_DIAGNOSTIC_ASSERT(aTarget > aNow);

  
  
  unsigned long delay = std::ceil((aTarget - aNow).ToMilliseconds());
  mCurrentTimerTarget = aTarget;
  nsresult rv = mTimer->InitWithFuncCallback(&TimerCallback, this, delay, nsITimer::TYPE_ONE_SHOT);
  MOZ_DIAGNOSTIC_ASSERT(NS_SUCCEEDED(rv));
  (void) rv;
}

} 
