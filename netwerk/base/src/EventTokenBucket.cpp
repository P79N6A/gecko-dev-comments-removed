





#include "EventTokenBucket.h"

#include "nsNetUtil.h"
#include "nsSocketTransportService2.h"

extern PRThread *gSocketThread;

namespace mozilla {
namespace net {





class TokenBucketCancelable : public nsICancelable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICANCELABLE

  TokenBucketCancelable(class ATokenBucketEvent *event);
  virtual ~TokenBucketCancelable() {}
  void Fire();

private:
  friend class EventTokenBucket;
  ATokenBucketEvent *mEvent;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(TokenBucketCancelable, nsICancelable)

TokenBucketCancelable::TokenBucketCancelable(ATokenBucketEvent *event)
  : mEvent(event)
{
}

NS_IMETHODIMP
TokenBucketCancelable::Cancel(nsresult reason)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  mEvent = nullptr;
  return NS_OK;
}

void
TokenBucketCancelable::Fire()
{
  if (!mEvent)
    return;

  ATokenBucketEvent *event = mEvent;
  mEvent = nullptr;
  event->OnTokenBucketAdmitted();
}





NS_IMPL_THREADSAFE_ISUPPORTS1(EventTokenBucket, nsITimerCallback)


EventTokenBucket::EventTokenBucket(uint32_t eventsPerSecond,
                                   uint32_t burstSize)
  : mUnitCost(kUsecPerSec)
  , mMaxCredit(kUsecPerSec)
  , mCredit(kUsecPerSec)
  , mPaused(false)
  , mStopped(false)
  , mTimerArmed(false)
{
  MOZ_COUNT_CTOR(EventTokenBucket);
  mLastUpdate = TimeStamp::Now();

  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;
  nsCOMPtr<nsIEventTarget> sts;
  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  if (NS_SUCCEEDED(rv))
    sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mTimer)
    mTimer->SetTarget(sts);
  SetRate(eventsPerSecond, burstSize);
}

EventTokenBucket::~EventTokenBucket()
{
  SOCKET_LOG(("EventTokenBucket::dtor %p events=%d\n",
              this, mEvents.GetSize()));

  MOZ_COUNT_DTOR(EventTokenBucket);
  if (mTimer && mTimerArmed)
    mTimer->Cancel();

  
  while (mEvents.GetSize()) {
    nsRefPtr<TokenBucketCancelable> cancelable = 
      dont_AddRef(static_cast<TokenBucketCancelable *>(mEvents.PopFront()));
    cancelable->Fire();
  }
}

void
EventTokenBucket::SetRate(uint32_t eventsPerSecond,
                          uint32_t burstSize)
{
  SOCKET_LOG(("EventTokenBucket::SetRate %p %u %u\n",
              this, eventsPerSecond, burstSize));

  if (eventsPerSecond > kMaxHz) {
    eventsPerSecond = kMaxHz;
    SOCKET_LOG(("  eventsPerSecond out of range\n"));
  }

  if (!eventsPerSecond) {
    eventsPerSecond = 1;
    SOCKET_LOG(("  eventsPerSecond out of range\n"));
  }

  mUnitCost = kUsecPerSec / eventsPerSecond;
  mMaxCredit = mUnitCost * burstSize;
  if (mMaxCredit > kUsecPerSec * 60 * 15) {
    SOCKET_LOG(("  burstSize out of range\n"));
    mMaxCredit = kUsecPerSec * 60 * 15;
  }
  mCredit = mMaxCredit;
  mLastUpdate = TimeStamp::Now();
}

void
EventTokenBucket::ClearCredits()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  SOCKET_LOG(("EventTokenBucket::ClearCredits %p\n", this));
  mCredit = 0;
}

uint32_t
EventTokenBucket::BurstEventsAvailable()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  return static_cast<uint32_t>(mCredit / mUnitCost);
}

uint32_t
EventTokenBucket::QueuedEvents()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  return mEvents.GetSize();
}

void
EventTokenBucket::Pause()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  SOCKET_LOG(("EventTokenBucket::Pause %p\n", this));
  if (mPaused || mStopped)
    return;

  mPaused = true;
  if (mTimerArmed) {
    mTimer->Cancel();
    mTimerArmed = false;
  }
}

void
EventTokenBucket::UnPause()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  SOCKET_LOG(("EventTokenBucket::UnPause %p\n", this));
  if (!mPaused || mStopped)
    return;

  mPaused = false;
  DispatchEvents();
  UpdateTimer();
}

nsresult
EventTokenBucket::SubmitEvent(ATokenBucketEvent *event, nsICancelable **cancelable)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  SOCKET_LOG(("EventTokenBucket::SubmitEvent %p\n", this));

  if (mStopped || !mTimer)
    return NS_ERROR_FAILURE;

  UpdateCredits();

  nsRefPtr<TokenBucketCancelable> cancelEvent = new TokenBucketCancelable(event);
  
  

  NS_ADDREF(*cancelable = cancelEvent.get());

  if (mPaused || !TryImmediateDispatch(cancelEvent.get())) {
    
    SOCKET_LOG(("   queued\n"));
    mEvents.Push(cancelEvent.forget().get());
    UpdateTimer();
  }
  else {
    SOCKET_LOG(("   dispatched synchronously\n"));
  }

  return NS_OK;
}

bool
EventTokenBucket::TryImmediateDispatch(TokenBucketCancelable *cancelable)
{
  if (mCredit < mUnitCost)
    return false;

  mCredit -= mUnitCost;
  cancelable->Fire();
  return true;
}

void
EventTokenBucket::DispatchEvents()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  SOCKET_LOG(("EventTokenBucket::DispatchEvents %p %d\n", this, mPaused));
  if (mPaused || mStopped)
    return;

  while (mEvents.GetSize() && mUnitCost <= mCredit) {
    nsRefPtr<TokenBucketCancelable> cancelable = 
      dont_AddRef(static_cast<TokenBucketCancelable *>(mEvents.PopFront()));
    if (cancelable->mEvent) {
      SOCKET_LOG(("EventTokenBucket::DispachEvents [%p] "
                  "Dispatching queue token bucket event cost=%lu credit=%lu\n",
                  this, mUnitCost, mCredit));
      mCredit -= mUnitCost;
      cancelable->Fire();
    }
  }
}
 
void
EventTokenBucket::UpdateTimer()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  if (mTimerArmed || mPaused || mStopped || !mEvents.GetSize() || !mTimer)
    return;

  if (mCredit >= mUnitCost)
    return;

  
  
  
  
  uint64_t deficit = mUnitCost - mCredit;
  uint64_t msecWait = (deficit + (kUsecPerMsec - 1)) / kUsecPerMsec;

  if (msecWait < 4) 
    msecWait = 4;
  else if (msecWait > 60000) 
    msecWait = 60000;

  SOCKET_LOG(("EventTokenBucket::UpdateTimer %p for %dms\n",
              this, msecWait));
  nsresult rv = mTimer->InitWithCallback(this, static_cast<uint32_t>(msecWait),
                                         nsITimer::TYPE_ONE_SHOT);
  mTimerArmed = NS_SUCCEEDED(rv);
}

NS_IMETHODIMP
EventTokenBucket::Notify(nsITimer *timer)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  SOCKET_LOG(("EventTokenBucket::Notify() %p\n", this));
  mTimerArmed = false;
  if (mStopped)
    return NS_OK;

  UpdateCredits();
  DispatchEvents();
  UpdateTimer();

  return NS_OK;
}

void
EventTokenBucket::UpdateCredits()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  TimeStamp now = TimeStamp::Now();
  TimeDuration elapsed = now - mLastUpdate;
  mLastUpdate = now;

  mCredit += static_cast<uint64_t>(elapsed.ToMicroseconds());
  if (mCredit > mMaxCredit)
    mCredit = mMaxCredit;
  SOCKET_LOG(("EventTokenBucket::UpdateCredits %p to %lu (%lu each.. %3.2f)\n",
              this, mCredit, mUnitCost, (double)mCredit / mUnitCost));
}

} 
} 
