










































#include "nsRefreshDriver.h"
#include "nsPresContext.h"
#include "nsComponentManagerUtils.h"
#include "prlog.h"
#include "nsAutoPtr.h"
#include "nsCSSFrameConstructor.h"








#define REFRESH_INTERVAL_MILLISECONDS 20

using mozilla::TimeStamp;

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

  nsresult rv = mTimer->InitWithCallback(this, REFRESH_INTERVAL_MILLISECONDS,
                                         nsITimer::TYPE_REPEATING_SLACK);
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
  return sum;
}

void
nsRefreshDriver::UpdateMostRecentRefresh()
{
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
