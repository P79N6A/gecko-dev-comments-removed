










#include "mozilla/Util.h"

#include "nsRefreshDriver.h"
#include "nsPresContext.h"
#include "nsComponentManagerUtils.h"
#include "prlog.h"
#include "nsAutoPtr.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "jsapi.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "nsIViewManager.h"
#include "sampler.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

using namespace mozilla;

#define DEFAULT_FRAME_RATE 60
#define DEFAULT_THROTTLED_FRAME_RATE 1

static bool sPrecisePref;

 void
nsRefreshDriver::InitializeStatics()
{
  Preferences::AddBoolVarCache(&sPrecisePref,
                               "layout.frame_rate.precise",
                               false);
}

 PRInt32
nsRefreshDriver::DefaultInterval()
{
  return NSToIntRound(1000.0 / DEFAULT_FRAME_RATE);
}



PRInt32
nsRefreshDriver::GetRefreshTimerInterval() const
{
  const char* prefName =
    mThrottled ? "layout.throttled_frame_rate" : "layout.frame_rate";
  PRInt32 rate = Preferences::GetInt(prefName, -1);
  if (rate <= 0) {
    
    rate = mThrottled ? DEFAULT_THROTTLED_FRAME_RATE : DEFAULT_FRAME_RATE;
  }
  NS_ASSERTION(rate > 0, "Must have positive rate here");
  PRInt32 interval = NSToIntRound(1000.0/rate);
  if (mThrottled) {
    interval = NS_MAX(interval, mLastTimerInterval * 2);
  }
  mLastTimerInterval = interval;
  return interval;
}

PRInt32
nsRefreshDriver::GetRefreshTimerType() const
{
  if (mThrottled) {
    return nsITimer::TYPE_ONE_SHOT;
  }
  if (HaveFrameRequestCallbacks() || sPrecisePref) {
    return nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP;
  }
  return nsITimer::TYPE_REPEATING_SLACK;
}

nsRefreshDriver::nsRefreshDriver(nsPresContext *aPresContext)
  : mPresContext(aPresContext),
    mFrozen(false),
    mThrottled(false),
    mTestControllingRefreshes(false),
    mTimerIsPrecise(false),
    mViewManagerFlushIsPending(false),
    mLastTimerInterval(0)
{
  mRequests.Init();
}

nsRefreshDriver::~nsRefreshDriver()
{
  NS_ABORT_IF_FALSE(ObserverCount() == 0,
                    "observers should have unregistered");
  NS_ABORT_IF_FALSE(!mTimer, "timer should be gone");
}



void
nsRefreshDriver::AdvanceTimeAndRefresh(PRInt64 aMilliseconds)
{
  mTestControllingRefreshes = true;
  mMostRecentRefreshEpochTime += aMilliseconds * 1000;
  mMostRecentRefresh += TimeDuration::FromMilliseconds(aMilliseconds);
  nsCxPusher pusher;
  if (pusher.PushNull()) {
    Notify(nullptr);
    pusher.Pop();
  }
}

void
nsRefreshDriver::RestoreNormalRefresh()
{
  mTestControllingRefreshes = false;
  nsCxPusher pusher;
  if (pusher.PushNull()) {
    Notify(nullptr); 
    pusher.Pop();
  }
}

TimeStamp
nsRefreshDriver::MostRecentRefresh() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted(false);

  return mMostRecentRefresh;
}

PRInt64
nsRefreshDriver::MostRecentRefreshEpochTime() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted(false);

  return mMostRecentRefreshEpochTime;
}

bool
nsRefreshDriver::AddRefreshObserver(nsARefreshObserver *aObserver,
                                    mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  bool success = array.AppendElement(aObserver) != nullptr;

  EnsureTimerStarted(false);

  return success;
}

bool
nsRefreshDriver::RemoveRefreshObserver(nsARefreshObserver *aObserver,
                                       mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.RemoveElement(aObserver);
}

bool
nsRefreshDriver::AddImageRequest(imgIRequest* aRequest)
{
  if (!mRequests.PutEntry(aRequest)) {
    return false;
  }

  EnsureTimerStarted(false);

  return true;
}

void
nsRefreshDriver::RemoveImageRequest(imgIRequest* aRequest)
{
  mRequests.RemoveEntry(aRequest);
}

void nsRefreshDriver::ClearAllImageRequests()
{
  mRequests.Clear();
}

void
nsRefreshDriver::EnsureTimerStarted(bool aAdjustingTimer)
{
  if (mTimer || mFrozen || !mPresContext) {
    
    
    return;
  }

  if (!aAdjustingTimer) {
    
    
    
    
    
    
    UpdateMostRecentRefresh();
  }

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!mTimer) {
    return;
  }

  PRInt32 timerType = GetRefreshTimerType();
  mTimerIsPrecise = (timerType == nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);

  nsresult rv = mTimer->InitWithCallback(this,
                                         GetRefreshTimerInterval(),
                                         timerType);
  if (NS_FAILED(rv)) {
    mTimer = nullptr;
  }
}

void
nsRefreshDriver::StopTimer()
{
  if (!mTimer) {
    return;
  }

  mTimer->Cancel();
  mTimer = nullptr;
}

PRUint32
nsRefreshDriver::ObserverCount() const
{
  PRUint32 sum = 0;
  for (PRUint32 i = 0; i < ArrayLength(mObservers); ++i) {
    sum += mObservers[i].Length();
  }

  
  
  
  
  sum += mStyleFlushObservers.Length();
  sum += mLayoutFlushObservers.Length();
  sum += mFrameRequestCallbackDocs.Length();
  sum += mViewManagerFlushIsPending;
  return sum;
}

PRUint32
nsRefreshDriver::ImageRequestCount() const
{
  return mRequests.Count();
}

void
nsRefreshDriver::UpdateMostRecentRefresh()
{
  if (mTestControllingRefreshes) {
    return;
  }

  
  mMostRecentRefreshEpochTime = JS_Now();
  mMostRecentRefresh = TimeStamp::Now();
}

nsRefreshDriver::ObserverArray&
nsRefreshDriver::ArrayFor(mozFlushType aFlushType)
{
  switch (aFlushType) {
    case Flush_Style:
      return mObservers[0];
    case Flush_Layout:
      return mObservers[1];
    case Flush_Display:
      return mObservers[2];
    default:
      NS_ABORT_IF_FALSE(false, "bad flush type");
      return *static_cast<ObserverArray*>(nullptr);
  }
}





NS_IMPL_ISUPPORTS1(nsRefreshDriver, nsITimerCallback)





NS_IMETHODIMP
nsRefreshDriver::Notify(nsITimer *aTimer)
{
  SAMPLE_LABEL("nsRefreshDriver", "Notify");

  NS_PRECONDITION(!mFrozen, "Why are we notified while frozen?");
  NS_PRECONDITION(mPresContext, "Why are we notified after disconnection?");
  NS_PRECONDITION(!nsContentUtils::GetCurrentJSContext(),
                  "Shouldn't have a JSContext on the stack");

  if (mTestControllingRefreshes && aTimer) {
    
    return NS_OK;
  }

  UpdateMostRecentRefresh();

  nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
  if (!presShell || (ObserverCount() == 0 && ImageRequestCount() == 0)) {
    
    
    
    
    
    
    
    StopTimer();
    return NS_OK;
  }

  





  for (PRUint32 i = 0; i < ArrayLength(mObservers); ++i) {
    ObserverArray::EndLimitedIterator etor(mObservers[i]);
    while (etor.HasMore()) {
      nsRefPtr<nsARefreshObserver> obs = etor.GetNext();
      obs->WillRefresh(mMostRecentRefresh);
      
      if (!mPresContext || !mPresContext->GetPresShell()) {
        StopTimer();
        return NS_OK;
      }
    }

    if (i == 0) {
      
      nsIDocument::FrameRequestCallbackList frameRequestCallbacks;
      for (PRUint32 i = 0; i < mFrameRequestCallbackDocs.Length(); ++i) {
        mFrameRequestCallbackDocs[i]->
          TakeFrameRequestCallbacks(frameRequestCallbacks);
      }
      
      
      mFrameRequestCallbackDocs.Clear();

      PRInt64 eventTime = mMostRecentRefreshEpochTime / PR_USEC_PER_MSEC;
      for (PRUint32 i = 0; i < frameRequestCallbacks.Length(); ++i) {
        nsAutoMicroTask mt;
        frameRequestCallbacks[i]->Sample(eventTime);
      }

      
      if (mPresContext && mPresContext->GetPresShell()) {
        nsAutoTArray<nsIPresShell*, 16> observers;
        observers.AppendElements(mStyleFlushObservers);
        for (PRUint32 j = observers.Length();
             j && mPresContext && mPresContext->GetPresShell(); --j) {
          
          
          nsIPresShell* shell = observers[j - 1];
          if (!mStyleFlushObservers.Contains(shell))
            continue;
          NS_ADDREF(shell);
          mStyleFlushObservers.RemoveElement(shell);
          shell->FrameConstructor()->mObservingRefreshDriver = false;
          shell->FlushPendingNotifications(Flush_Style);
          NS_RELEASE(shell);
        }
      }
    } else if  (i == 1) {
      
      if (mPresContext && mPresContext->GetPresShell()) {
        nsAutoTArray<nsIPresShell*, 16> observers;
        observers.AppendElements(mLayoutFlushObservers);
        for (PRUint32 j = observers.Length();
             j && mPresContext && mPresContext->GetPresShell(); --j) {
          
          
          nsIPresShell* shell = observers[j - 1];
          if (!mLayoutFlushObservers.Contains(shell))
            continue;
          NS_ADDREF(shell);
          mLayoutFlushObservers.RemoveElement(shell);
          shell->mReflowScheduled = false;
          shell->mSuppressInterruptibleReflows = false;
          shell->FlushPendingNotifications(Flush_InterruptibleLayout);
          NS_RELEASE(shell);
        }
      }
    }
  }

  




  ImageRequestParameters parms = {mMostRecentRefresh};
  if (mRequests.Count()) {
    mRequests.EnumerateEntries(nsRefreshDriver::ImageRequestEnumerator, &parms);
    EnsureTimerStarted(false);
  }

  if (mViewManagerFlushIsPending) {
    mViewManagerFlushIsPending = false;
    mPresContext->GetPresShell()->GetViewManager()->ProcessPendingUpdates();
  }

  if (mThrottled ||
      (mTimerIsPrecise !=
       (GetRefreshTimerType() == nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP))) {
    
    
    
    
    
    

    
    
    
    StopTimer();
    EnsureTimerStarted(true);
  }

  return NS_OK;
}

PLDHashOperator
nsRefreshDriver::ImageRequestEnumerator(nsISupportsHashKey* aEntry,
                                        void* aUserArg)
{
  ImageRequestParameters* parms =
    static_cast<ImageRequestParameters*> (aUserArg);
  mozilla::TimeStamp mostRecentRefresh = parms->ts;
  imgIRequest* req = static_cast<imgIRequest*>(aEntry->GetKey());
  NS_ABORT_IF_FALSE(req, "Unable to retrieve the image request");
  nsCOMPtr<imgIContainer> image;
  req->GetImage(getter_AddRefs(image));
  if (image) {
    image->RequestRefresh(mostRecentRefresh);
  }

  return PL_DHASH_NEXT;
}

void
nsRefreshDriver::Freeze()
{
  NS_ASSERTION(!mFrozen, "Freeze called on already-frozen refresh driver");
  StopTimer();
  mFrozen = true;
}

void
nsRefreshDriver::Thaw()
{
  NS_ASSERTION(mFrozen, "Thaw called on an unfrozen refresh driver");
  mFrozen = false;
  if (ObserverCount() || ImageRequestCount()) {
    
    
    
    
    NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, &nsRefreshDriver::DoRefresh));
    EnsureTimerStarted(false);
  }
}

void
nsRefreshDriver::SetThrottled(bool aThrottled)
{
  if (aThrottled != mThrottled) {
    mThrottled = aThrottled;
    if (mTimer) {
      
      
      StopTimer();
      EnsureTimerStarted(true);
    }
  }
}

void
nsRefreshDriver::DoRefresh()
{
  
  if (!mFrozen && mPresContext && mTimer) {
    Notify(nullptr);
  }
}

#ifdef DEBUG
bool
nsRefreshDriver::IsRefreshObserver(nsARefreshObserver *aObserver,
                                   mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.Contains(aObserver);
}
#endif

void
nsRefreshDriver::ScheduleFrameRequestCallbacks(nsIDocument* aDocument)
{
  NS_ASSERTION(mFrameRequestCallbackDocs.IndexOf(aDocument) ==
               mFrameRequestCallbackDocs.NoIndex,
               "Don't schedule the same document multiple times");
  mFrameRequestCallbackDocs.AppendElement(aDocument);
  
  
  EnsureTimerStarted(false);
}

void
nsRefreshDriver::RevokeFrameRequestCallbacks(nsIDocument* aDocument)
{
  mFrameRequestCallbackDocs.RemoveElement(aDocument);
  
  
}
