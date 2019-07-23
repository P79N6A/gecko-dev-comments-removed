




































#include "nsBaseAppShell.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"




#define THREAD_EVENT_STARVATION_LIMIT PR_MillisecondsToInterval(20)

NS_IMPL_THREADSAFE_ISUPPORTS3(nsBaseAppShell, nsIAppShell, nsIThreadObserver,
                              nsIObserver)

nsBaseAppShell::nsBaseAppShell()
  : mFavorPerf(0)
  , mNativeEventPending(PR_FALSE)
  , mStarvationDelay(0)
  , mSwitchTime(0)
  , mLastNativeEventTime(0)
  , mRunWasCalled(PR_FALSE)
  , mExiting(PR_FALSE)
  , mProcessingNextNativeEvent(PR_FALSE)
{
}

nsresult
nsBaseAppShell::Init()
{
  

  nsCOMPtr<nsIThreadInternal> threadInt =
      do_QueryInterface(NS_GetCurrentThread());
  NS_ENSURE_STATE(threadInt);

  threadInt->SetObserver(this);

  nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService("@mozilla.org/observer-service;1");
  if (obsSvc)
    obsSvc->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  return NS_OK;
}

void
nsBaseAppShell::NativeEventCallback()
{
  PRInt32 hasPending = PR_AtomicSet(&mNativeEventPending, 0);
  if (hasPending == 0)
    return;

  
  
  
  
  
  
  
  if (mProcessingNextNativeEvent) {
#if 0
    
    
    
    mProcessingNextNativeEvent = PR_FALSE;
    if (NS_HasPendingEvents(NS_GetCurrentThread()))
      OnDispatchedEvent(nsnull);
#endif
    return;
  }

  
  

  nsIThread *thread = NS_GetCurrentThread();
  NS_ProcessPendingEvents(thread, THREAD_EVENT_STARVATION_LIMIT);

  
  
  if (NS_HasPendingEvents(thread))
    OnDispatchedEvent(nsnull);
}

PRBool
nsBaseAppShell::DoProcessNextNativeEvent(PRBool mayWait)
{
  
  
  
  
  
  
  
  
  
  
  
  PRBool prevVal = mProcessingNextNativeEvent;

  mProcessingNextNativeEvent = PR_TRUE;
  PRBool result = ProcessNextNativeEvent(mayWait); 
  mProcessingNextNativeEvent = prevVal;
  return result;
}




NS_IMETHODIMP
nsBaseAppShell::Run(void)
{
  nsIThread *thread = NS_GetCurrentThread();

  NS_ENSURE_STATE(!mRunWasCalled);  
  mRunWasCalled = PR_TRUE;

  while (!mExiting)
    NS_ProcessNextEvent(thread);

  NS_ProcessPendingEvents(thread);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Exit(void)
{
  mExiting = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::FavorPerformanceHint(PRBool favorPerfOverStarvation,
                                     PRUint32 starvationDelay)
{
  mStarvationDelay = PR_MillisecondsToInterval(starvationDelay);
  if (favorPerfOverStarvation) {
    ++mFavorPerf;
  } else {
    --mFavorPerf;
    mSwitchTime = PR_IntervalNow();
  }
  return NS_OK;
}





NS_IMETHODIMP
nsBaseAppShell::OnDispatchedEvent(nsIThreadInternal *thr)
{
  PRInt32 lastVal = PR_AtomicSet(&mNativeEventPending, 1);
  if (lastVal == 1)
    return NS_OK;
    
  ScheduleNativeEventCallback();
  return NS_OK;
}


NS_IMETHODIMP
nsBaseAppShell::OnProcessNextEvent(nsIThreadInternal *thr, PRBool mayWait,
                                   PRUint32 recursionDepth)
{
  PRIntervalTime start = PR_IntervalNow();
  PRIntervalTime limit = THREAD_EVENT_STARVATION_LIMIT;

  if (mFavorPerf <= 0 && start > mSwitchTime + mStarvationDelay) {
    
    PRIntervalTime now = start;
    PRBool keepGoing;
    do {
      mLastNativeEventTime = now;
      keepGoing = DoProcessNextNativeEvent(PR_FALSE);
    } while (keepGoing && ((now = PR_IntervalNow()) - start) < limit);
  } else {
    
    if (start - mLastNativeEventTime > limit) {
      mLastNativeEventTime = start;
      DoProcessNextNativeEvent(PR_FALSE);
    }
  }

  
  
  
  PRBool needEvent = mayWait;

  while (!NS_HasPendingEvents(thr)) {
    
    
    if (mExiting && mayWait)
      mayWait = PR_FALSE;

    mLastNativeEventTime = PR_IntervalNow();
    if (!DoProcessNextNativeEvent(mayWait) || !mayWait)
      break;
  }

  
  
  
  if (needEvent && !NS_HasPendingEvents(thr)) {  
    if (!mDummyEvent)
      mDummyEvent = new nsRunnable();
    thr->Dispatch(mDummyEvent, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsBaseAppShell::AfterProcessNextEvent(nsIThreadInternal *thr,
                                      PRUint32 recursionDepth)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Observe(nsISupports *subject, const char *topic,
                        const PRUnichar *data)
{
  NS_ASSERTION(!strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID), "oops");
  Exit();
  return NS_OK;
}
