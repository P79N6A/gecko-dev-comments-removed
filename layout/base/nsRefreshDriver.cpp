










































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








#define DEFAULT_FRAME_RATE 60



static PRInt32
GetRefreshTimerInterval()
{
  PRInt32 rate = nsContentUtils::GetIntPref("layout.frame_rate", -1);
  if (rate <= 0) {
    
    rate = DEFAULT_FRAME_RATE;
  }
  NS_ASSERTION(rate > 0, "Must have positive rate here");
  return NSToIntRound(1000.0/rate);
}

static PRInt32
GetRefreshTimerType()
{
  PRBool precise =
    nsContentUtils::GetBoolPref("layout.frame_rate.precise", PR_FALSE);
  return precise ? nsITimer::TYPE_REPEATING_PRECISE
                 : nsITimer::TYPE_REPEATING_SLACK;
}

nsRefreshDriver::nsRefreshDriver(nsPresContext *aPresContext)
  : mPresContext(aPresContext),
    mFrozen(PR_FALSE)
{
}

nsRefreshDriver::~nsRefreshDriver()
{
  NS_ABORT_IF_FALSE(ObserverCount() == 0,
                    "observers should have unregistered");
  NS_ABORT_IF_FALSE(!mTimer, "timer should be gone");
}

TimeStamp
nsRefreshDriver::MostRecentRefresh() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted();

  return mMostRecentRefresh;
}

PRInt64
nsRefreshDriver::MostRecentRefreshEpochTime() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted();

  return mMostRecentRefreshEpochTime;
}

PRBool
nsRefreshDriver::AddRefreshObserver(nsARefreshObserver *aObserver,
                                    mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  PRBool success = array.AppendElement(aObserver) != nsnull;

  EnsureTimerStarted();

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
nsRefreshDriver::EnsureTimerStarted()
{
  if (mTimer || mFrozen || !mPresContext) {
    
    
    return;
  }

  UpdateMostRecentRefresh();

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!mTimer) {
    return;
  }

  nsresult rv = mTimer->InitWithCallback(this, GetRefreshTimerInterval(),
                                         GetRefreshTimerType());
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
  return sum;
}

void
nsRefreshDriver::UpdateMostRecentRefresh()
{
  
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
nsRefreshDriver::Notify(nsITimer * )
{
  NS_PRECONDITION(!mFrozen, "Why are we notified while frozen?");
  NS_PRECONDITION(mPresContext, "Why are we notified after disconnection?");

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
      
      
      
      nsTArray<nsIDocument*> targets;
      targets.SwapElements(mBeforePaintTargets);
      PRInt64 eventTime = mMostRecentRefreshEpochTime / PR_USEC_PER_MSEC;
      for (PRUint32 i = 0; i < targets.Length(); ++i) {
        targets[i]->BeforePaintEventFiring();
      }
      for (PRUint32 i = 0; i < targets.Length(); ++i) {
        nsEvent ev(PR_TRUE, NS_BEFOREPAINT);
        ev.time = eventTime;
        nsEventDispatcher::Dispatch(targets[i], nsnull, &ev);
      }

      
      while (!mStyleFlushObservers.IsEmpty() &&
             mPresContext && mPresContext->GetPresShell()) {
        PRUint32 idx = mStyleFlushObservers.Length() - 1;
        nsCOMPtr<nsIPresShell> shell = mStyleFlushObservers[idx];
        mStyleFlushObservers.RemoveElementAt(idx);
        shell->FrameConstructor()->mObservingRefreshDriver = PR_FALSE;
        shell->FlushPendingNotifications(Flush_Style);
      }
    } else if  (i == 1) {
      
      while (!mLayoutFlushObservers.IsEmpty() &&
             mPresContext && mPresContext->GetPresShell()) {
        PRUint32 idx = mLayoutFlushObservers.Length() - 1;
        nsCOMPtr<nsIPresShell> shell = mLayoutFlushObservers[idx];
        mLayoutFlushObservers.RemoveElementAt(idx);
        shell->mReflowScheduled = PR_FALSE;
        shell->mSuppressInterruptibleReflows = PR_FALSE;
        shell->FlushPendingNotifications(Flush_InterruptibleLayout);
      }
    }
  }

  return NS_OK;
}

void
nsRefreshDriver::Freeze()
{
  NS_ASSERTION(!mFrozen, "Freeze called on already-frozen refresh driver");
  StopTimer();
  mFrozen = PR_TRUE;
}

void
nsRefreshDriver::Thaw()
{
  NS_ASSERTION(mFrozen, "Thaw called on an unfrozen refresh driver");
  mFrozen = PR_FALSE;
  if (ObserverCount()) {
    NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, &nsRefreshDriver::DoRefresh));
    EnsureTimerStarted();
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
  EnsureTimerStarted();
  return appended;
}

void
nsRefreshDriver::RevokeBeforePaintEvent(nsIDocument* aDocument)
{
  mBeforePaintTargets.RemoveElement(aDocument);
}
