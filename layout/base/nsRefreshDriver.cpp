









































#include "nsRefreshDriver.h"
#include "nsPresContext.h"
#include "nsComponentManagerUtils.h"
#include "prlog.h"








#define REFRESH_INTERVAL_MILLISECONDS 20

using mozilla::TimeStamp;

nsRefreshDriver::nsRefreshDriver()
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
  PRBool success = array.RemoveElement(aObserver);

  if (ObserverCount() == 0) {
    StopTimer();
  }

  return success;
}

void
nsRefreshDriver::EnsureTimerStarted()
{
  if (mTimer) {
    
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





NS_IMPL_ADDREF_USING_AGGREGATOR(nsRefreshDriver,
                                nsPresContext::FromRefreshDriver(this))
NS_IMPL_RELEASE_USING_AGGREGATOR(nsRefreshDriver,
                                 nsPresContext::FromRefreshDriver(this))
NS_IMPL_QUERY_INTERFACE1(nsRefreshDriver, nsITimerCallback)





NS_IMETHODIMP
nsRefreshDriver::Notify(nsITimer *aTimer)
{
  UpdateMostRecentRefresh();

  nsPresContext *presContext = nsPresContext::FromRefreshDriver(this);
  nsCOMPtr<nsIPresShell> presShell = presContext->GetPresShell();
  if (!presShell) {
    
    StopTimer();
    return NS_OK;
  }

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mObservers); ++i) {
    ObserverArray::EndLimitedIterator etor(mObservers[i]);
    while (etor.HasMore()) {
      etor.GetNext()->WillRefresh(mMostRecentRefresh);
    }
    if (i == 0) {
      
      
      
      
      
      
      
      
      presShell->FlushPendingNotifications(Flush_Style);
    }
  }

  if (ObserverCount() == 0) {
    StopTimer();
  }

  return NS_OK;
}
