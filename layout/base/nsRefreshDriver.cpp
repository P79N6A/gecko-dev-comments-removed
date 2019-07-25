










































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

using mozilla::TimeStamp;
using mozilla::TimeDuration;

#define DEFAULT_FRAME_RATE 60
#define DEFAULT_THROTTLED_FRAME_RATE 1

static PRBool sPrecisePref;

 void
nsRefreshDriver::InitializeStatics()
{
  nsContentUtils::AddBoolPrefVarCache("layout.frame_rate.precise",
                                      &sPrecisePref,
                                      PR_FALSE);
}


PRInt32
nsRefreshDriver::GetRefreshTimerInterval() const
{
  const char* prefName =
    mThrottled ? "layout.throttled_frame_rate" : "layout.frame_rate";
  PRInt32 rate = nsContentUtils::GetIntPref(prefName, -1);
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
  if (HaveAnimationFrameListeners() || sPrecisePref) {
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
    mLastTimerInterval(0)
{
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
    Notify(nsnull);
    pusher.Pop();
  }
}

void
nsRefreshDriver::RestoreNormalRefresh()
{
  mTestControllingRefreshes = false;
  nsCxPusher pusher;
  if (pusher.PushNull()) {
    Notify(nsnull); 
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

PRBool
nsRefreshDriver::AddRefreshObserver(nsARefreshObserver *aObserver,
                                    mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  PRBool success = array.AppendElement(aObserver) != nsnull;

  EnsureTimerStarted(false);

  return success;
}

PRBool
nsRefreshDriver::RemoveRefreshObserver(nsARefreshObserver *aObserver,
                                       mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.RemoveElement(aObserver);
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
    mTimer = nsnull;
  }
}

void
nsRefreshDriver::StopTimer()
{
  if (!mTimer) {
    return;
  }

  mTimer->Cancel();
  mTimer = nsnull;
}

PRUint32
nsRefreshDriver::ObserverCount() const
{
  PRUint32 sum = 0;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mObservers); ++i) {
    sum += mObservers[i].Length();
  }
  
  
  
  
  sum += mStyleFlushObservers.Length();
  sum += mLayoutFlushObservers.Length();
  sum += mBeforePaintTargets.Length();
  sum += mAnimationFrameListenerDocs.Length();
  return sum;
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
      NS_ABORT_IF_FALSE(PR_FALSE, "bad flush type");
      return *static_cast<ObserverArray*>(nsnull);
  }
}





NS_IMPL_ISUPPORTS1(nsRefreshDriver, nsITimerCallback)





NS_IMETHODIMP
nsRefreshDriver::Notify(nsITimer *aTimer)
{
  NS_PRECONDITION(!mFrozen, "Why are we notified while frozen?");
  NS_PRECONDITION(mPresContext, "Why are we notified after disconnection?");
  NS_PRECONDITION(!nsContentUtils::GetCurrentJSContext(),
                  "Shouldn't have a JSContext on the stack");

  if (mTestControllingRefreshes && aTimer) {
    
    return NS_OK;
  }

  UpdateMostRecentRefresh();

  nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
  if (!presShell || ObserverCount() == 0) {
    
    
    
    
    
    
    
    StopTimer();
    return NS_OK;
  }

  





  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mObservers); ++i) {
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
      
      
      
      nsTArray< nsCOMPtr<nsIDocument> > targets;
      targets.SwapElements(mBeforePaintTargets);
      for (PRUint32 i = 0; i < targets.Length(); ++i) {
        targets[i]->BeforePaintEventFiring();
      }

      
      nsIDocument::AnimationListenerList animationListeners;
      for (PRUint32 i = 0; i < mAnimationFrameListenerDocs.Length(); ++i) {
        mAnimationFrameListenerDocs[i]->
          TakeAnimationFrameListeners(animationListeners);
      }
      
      
      mAnimationFrameListenerDocs.Clear();

      PRInt64 eventTime = mMostRecentRefreshEpochTime / PR_USEC_PER_MSEC;
      for (PRUint32 i = 0; i < targets.Length(); ++i) {
        nsEvent ev(PR_TRUE, NS_BEFOREPAINT);
        ev.time = eventTime;
        nsEventDispatcher::Dispatch(targets[i], nsnull, &ev);
      }

      for (PRUint32 i = 0; i < animationListeners.Length(); ++i) {
        animationListeners[i]->OnBeforePaint(eventTime);
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
          shell->FrameConstructor()->mObservingRefreshDriver = PR_FALSE;
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
          shell->mReflowScheduled = PR_FALSE;
          shell->mSuppressInterruptibleReflows = PR_FALSE;
          shell->FlushPendingNotifications(Flush_InterruptibleLayout);
          NS_RELEASE(shell);
        }
      }
    }
  }

  if (mThrottled ||
      (mTimerIsPrecise !=
       (GetRefreshTimerType() == nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP))) {
    
    
    
    
    
    

    
    
    
    StopTimer();
    EnsureTimerStarted(true);
  }

  return NS_OK;
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
  if (ObserverCount()) {
    
    
    
    
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
    Notify(nsnull);
  }
}

#ifdef DEBUG
PRBool
nsRefreshDriver::IsRefreshObserver(nsARefreshObserver *aObserver,
                                   mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.Contains(aObserver);
}
#endif

PRBool
nsRefreshDriver::ScheduleBeforePaintEvent(nsIDocument* aDocument)
{
  NS_ASSERTION(mBeforePaintTargets.IndexOf(aDocument) ==
               mBeforePaintTargets.NoIndex,
               "Shouldn't have a paint event posted for this document");
  PRBool appended = mBeforePaintTargets.AppendElement(aDocument) != nsnull;
  EnsureTimerStarted(false);
  return appended;
}

void
nsRefreshDriver::ScheduleAnimationFrameListeners(nsIDocument* aDocument)
{
  NS_ASSERTION(mAnimationFrameListenerDocs.IndexOf(aDocument) ==
               mAnimationFrameListenerDocs.NoIndex,
               "Don't schedule the same document multiple times");
  mAnimationFrameListenerDocs.AppendElement(aDocument);
  
  
  EnsureTimerStarted(false);
}

void
nsRefreshDriver::RevokeBeforePaintEvent(nsIDocument* aDocument)
{
  mBeforePaintTargets.RemoveElement(aDocument);
}

void
nsRefreshDriver::RevokeAnimationFrameListeners(nsIDocument* aDocument)
{
  mAnimationFrameListenerDocs.RemoveElement(aDocument);
  
  
}
