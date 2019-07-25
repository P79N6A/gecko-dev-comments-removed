





































#include "base/message_loop.h"

#include "nsBaseAppShell.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"




#define THREAD_EVENT_STARVATION_LIMIT PR_MillisecondsToInterval(20)

NS_IMPL_THREADSAFE_ISUPPORTS3(nsBaseAppShell, nsIAppShell, nsIThreadObserver,
                              nsIObserver)

nsBaseAppShell::nsBaseAppShell()
  : mSuspendNativeCount(0)
  , mEventloopNestingLevel(0)
  , mBlockedWait(nsnull)
  , mFavorPerf(0)
  , mNativeEventPending(0)
  , mStarvationDelay(0)
  , mSwitchTime(0)
  , mLastNativeEventTime(0)
  , mEventloopNestingState(eEventloopNone)
  , mRunning(PR_FALSE)
  , mExiting(PR_FALSE)
  , mBlockNativeEvent(PR_FALSE)
{
}

nsBaseAppShell::~nsBaseAppShell()
{
  NS_ASSERTION(mSyncSections.Count() == 0, "Must have run all sync sections");
}

nsresult
nsBaseAppShell::Init()
{
  

  nsCOMPtr<nsIThreadInternal> threadInt =
      do_QueryInterface(NS_GetCurrentThread());
  NS_ENSURE_STATE(threadInt);

  threadInt->SetObserver(this);

  nsCOMPtr<nsIObserverService> obsSvc =
    mozilla::services::GetObserverService();
  if (obsSvc)
    obsSvc->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  return NS_OK;
}


void
nsBaseAppShell::NativeEventCallback()
{
  PRInt32 hasPending = PR_ATOMIC_SET(&mNativeEventPending, 0);
  if (hasPending == 0)
    return;

  
  
  
  
  if (mEventloopNestingState == eEventloopXPCOM) {
    mEventloopNestingState = eEventloopOther;
    
    
    return;
  }

  
  

  nsIThread *thread = NS_GetCurrentThread();
  bool prevBlockNativeEvent = mBlockNativeEvent;
  if (mEventloopNestingState == eEventloopOther) {
    if (!NS_HasPendingEvents(thread))
      return;
    
    
    
    
    mBlockNativeEvent = PR_TRUE;
  }

  ++mEventloopNestingLevel;
  EventloopNestingState prevVal = mEventloopNestingState;
  NS_ProcessPendingEvents(thread, THREAD_EVENT_STARVATION_LIMIT);
  mProcessedGeckoEvents = PR_TRUE;
  mEventloopNestingState = prevVal;
  mBlockNativeEvent = prevBlockNativeEvent;

  
  
  if (NS_HasPendingEvents(thread))
    DoProcessMoreGeckoEvents();

  --mEventloopNestingLevel;
}



void
nsBaseAppShell::DoProcessMoreGeckoEvents()
{
  OnDispatchedEvent(nsnull);
}



bool
nsBaseAppShell::DoProcessNextNativeEvent(bool mayWait)
{
  
  
  
  
  
  
  
  
  
  
  
  EventloopNestingState prevVal = mEventloopNestingState;
  mEventloopNestingState = eEventloopXPCOM;

  ++mEventloopNestingLevel;
  bool result = ProcessNextNativeEvent(mayWait);
  --mEventloopNestingLevel;

  mEventloopNestingState = prevVal;
  return result;
}




NS_IMETHODIMP
nsBaseAppShell::Run(void)
{
  NS_ENSURE_STATE(!mRunning);  
  mRunning = PR_TRUE;

  nsIThread *thread = NS_GetCurrentThread();

  MessageLoop::current()->Run();

  NS_ProcessPendingEvents(thread);

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
nsBaseAppShell::FavorPerformanceHint(bool favorPerfOverStarvation,
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

  PRInt32 lastVal = PR_ATOMIC_SET(&mNativeEventPending, 1);
  if (lastVal == 1)
    return NS_OK;

  
  ScheduleNativeEventCallback();
  return NS_OK;
}


NS_IMETHODIMP
nsBaseAppShell::OnProcessNextEvent(nsIThreadInternal *thr, bool mayWait,
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

  bool *oldBlockedWait = mBlockedWait;
  mBlockedWait = &mayWait;

  
  
  
  bool needEvent = mayWait;
  
  
  mProcessedGeckoEvents = PR_FALSE;

  if (mFavorPerf <= 0 && start > mSwitchTime + mStarvationDelay) {
    
    PRIntervalTime now = start;
    bool keepGoing;
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

  while (!NS_HasPendingEvents(thr) && !mProcessedGeckoEvents) {
    
    
    
    if (mExiting)
      mayWait = PR_FALSE;

    mLastNativeEventTime = PR_IntervalNow();
    if (!DoProcessNextNativeEvent(mayWait) || !mayWait)
      break;
  }

  mBlockedWait = oldBlockedWait;

  
  
  
  if (needEvent && !mExiting && !NS_HasPendingEvents(thr)) {  
    if (!mDummyEvent)
      mDummyEvent = new nsRunnable();
    thr->Dispatch(mDummyEvent, NS_DISPATCH_NORMAL);
  }

  
  RunSyncSections();

  return NS_OK;
}

void
nsBaseAppShell::RunSyncSections()
{
  if (mSyncSections.Count() == 0) {
    return;
  }
  
  
  
  
  
  for (PRInt32 i = 0; i < mSyncSections.Count(); i++) {
    mSyncSections[i]->Run();
  }
  mSyncSections.Clear();
}


NS_IMETHODIMP
nsBaseAppShell::AfterProcessNextEvent(nsIThreadInternal *thr,
                                      PRUint32 recursionDepth)
{
  
  RunSyncSections();
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

NS_IMETHODIMP
nsBaseAppShell::RunInStableState(nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  mSyncSections.AppendObject(aRunnable);

  
  nsIThread* thread = NS_GetCurrentThread(); 
  if (!NS_HasPendingEvents(thread) &&
       NS_FAILED(thread->Dispatch(new nsRunnable(), NS_DISPATCH_NORMAL)))
  {
    
    
    RunSyncSections();
  }
  return NS_OK;
}

