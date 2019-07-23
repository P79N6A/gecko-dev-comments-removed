





































#include "nsBaseAppShell.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"

#include "base/message_loop.h"




#define THREAD_EVENT_STARVATION_LIMIT PR_MillisecondsToInterval(20)

NS_IMPL_THREADSAFE_ISUPPORTS3(nsBaseAppShell, nsIAppShell, nsIThreadObserver,
                              nsIObserver)

nsBaseAppShell::nsBaseAppShell()
  : mSuspendNativeCount(0)
  , mBlockedWait(nsnull)
  , mFavorPerf(0)
  , mNativeEventPending(0)
  , mEventloopNestingLevel(0)
  , mStarvationDelay(0)
  , mSwitchTime(0)
  , mLastNativeEventTime(0)
  , mEventloopNestingState(eEventloopNone)
  , mRunning(PR_FALSE)
  , mExiting(PR_FALSE)
  , mBlockNativeEvent(PR_FALSE)
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

  
  
  
  
  if (mEventloopNestingState == eEventloopXPCOM) {
    mEventloopNestingState = eEventloopOther;
    
    
    return;
  }

  
  

  nsIThread *thread = NS_GetCurrentThread();
  PRBool prevBlockNativeEvent = mBlockNativeEvent;
  if (mEventloopNestingState == eEventloopOther) {
    if (!NS_HasPendingEvents(thread))
      return;
    
    
    
    
    mBlockNativeEvent = PR_TRUE;
  }

  ++mEventloopNestingLevel;
  EventloopNestingState prevVal = mEventloopNestingState;
  NS_ProcessPendingEvents(thread, THREAD_EVENT_STARVATION_LIMIT);
  mEventloopNestingState = prevVal;
  mBlockNativeEvent = prevBlockNativeEvent;

  
  
  if (NS_HasPendingEvents(thread))
    OnDispatchedEvent(nsnull);

  --mEventloopNestingLevel;
}

PRBool
nsBaseAppShell::DoProcessNextNativeEvent(PRBool mayWait)
{
  
  
  
  
  
  
  
  
  
  
  
  EventloopNestingState prevVal = mEventloopNestingState;
  mEventloopNestingState = eEventloopXPCOM;

  ++mEventloopNestingLevel;
  PRBool result = ProcessNextNativeEvent(mayWait);
  --mEventloopNestingLevel;

  mEventloopNestingState = prevVal;
  return result;
}




NS_IMETHODIMP
nsBaseAppShell::Run(void)
{
  NS_ENSURE_STATE(!mRunning);  
  mRunning = PR_TRUE;

  MessageLoop::current()->Run();

  NS_ProcessPendingEvents(NS_GetCurrentThread());

  mRunning = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Exit(void)
{
  if (mRunning && !mExiting) {
    MessageLoop::current()->Quit();
  }
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
nsBaseAppShell::SuspendNative()
{
  ++mSuspendNativeCount;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::ResumeNative()
{
  --mSuspendNativeCount;
  NS_ASSERTION(mSuspendNativeCount >= 0, "Unbalanced call to nsBaseAppShell::ResumeNative!");
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::GetEventloopNestingLevel(PRUint32* aNestingLevelResult)
{
  NS_ENSURE_ARG_POINTER(aNestingLevelResult);

  *aNestingLevelResult = mEventloopNestingLevel;

  return NS_OK;
}





NS_IMETHODIMP
nsBaseAppShell::OnDispatchedEvent(nsIThreadInternal *thr)
{
  if (mBlockNativeEvent)
    return NS_OK;

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
  if (mBlockNativeEvent) {
    if (!mayWait)
      return NS_OK;
    
    
    
    
    mBlockNativeEvent = PR_FALSE;
    if (NS_HasPendingEvents(thr))
      OnDispatchedEvent(thr); 
  }

  PRIntervalTime start = PR_IntervalNow();
  PRIntervalTime limit = THREAD_EVENT_STARVATION_LIMIT;

  
  if (mBlockedWait)
    *mBlockedWait = PR_FALSE;

  PRBool *oldBlockedWait = mBlockedWait;
  mBlockedWait = &mayWait;

  
  
  
  PRBool needEvent = mayWait;

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

  while (!NS_HasPendingEvents(thr)) {
    
    
    
    if (mExiting)
      mayWait = PR_FALSE;

    mLastNativeEventTime = PR_IntervalNow();
    if (!DoProcessNextNativeEvent(mayWait) || !mayWait)
      break;
  }

  mBlockedWait = oldBlockedWait;

  
  
  
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
