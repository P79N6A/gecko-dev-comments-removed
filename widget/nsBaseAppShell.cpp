




#include "base/message_loop.h"

#include "nsBaseAppShell.h"
#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#endif
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"




#define THREAD_EVENT_STARVATION_LIMIT PR_MillisecondsToInterval(20)

NS_IMPL_ISUPPORTS(nsBaseAppShell, nsIAppShell, nsIThreadObserver, nsIObserver)

nsBaseAppShell::nsBaseAppShell()
  : mSuspendNativeCount(0)
  , mEventloopNestingLevel(0)
  , mBlockedWait(nullptr)
  , mFavorPerf(0)
  , mNativeEventPending(false)
  , mStarvationDelay(0)
  , mSwitchTime(0)
  , mLastNativeEventTime(0)
  , mEventloopNestingState(eEventloopNone)
  , mRunningSyncSections(false)
  , mRunning(false)
  , mExiting(false)
  , mBlockNativeEvent(false)
{
}

nsBaseAppShell::~nsBaseAppShell()
{
  NS_ASSERTION(mSyncSections.IsEmpty(), "Must have run all sync sections");
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
    obsSvc->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  return NS_OK;
}


void
nsBaseAppShell::NativeEventCallback()
{
  if (!mNativeEventPending.exchange(false))
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
    
    
    
    
    mBlockNativeEvent = true;
  }

  IncrementEventloopNestingLevel();
  EventloopNestingState prevVal = mEventloopNestingState;
  NS_ProcessPendingEvents(thread, THREAD_EVENT_STARVATION_LIMIT);
  mProcessedGeckoEvents = true;
  mEventloopNestingState = prevVal;
  mBlockNativeEvent = prevBlockNativeEvent;

  
  
  if (NS_HasPendingEvents(thread))
    DoProcessMoreGeckoEvents();

  DecrementEventloopNestingLevel();
}



void
nsBaseAppShell::DoProcessMoreGeckoEvents()
{
  OnDispatchedEvent(nullptr);
}



bool
nsBaseAppShell::DoProcessNextNativeEvent(bool mayWait, uint32_t recursionDepth)
{
  
  
  
  
  
  
  
  
  
  
  
  EventloopNestingState prevVal = mEventloopNestingState;
  mEventloopNestingState = eEventloopXPCOM;

  IncrementEventloopNestingLevel();

  bool result = ProcessNextNativeEvent(mayWait);

  
  
  
  RunSyncSections(false, recursionDepth);

  DecrementEventloopNestingLevel();

  mEventloopNestingState = prevVal;
  return result;
}




NS_IMETHODIMP
nsBaseAppShell::Run(void)
{
  NS_ENSURE_STATE(!mRunning);  
  mRunning = true;

  nsIThread *thread = NS_GetCurrentThread();

  MessageLoop::current()->Run();

  NS_ProcessPendingEvents(thread);

  mRunning = false;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Exit(void)
{
  if (mRunning && !mExiting) {
    MessageLoop::current()->Quit();
  }
  mExiting = true;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::FavorPerformanceHint(bool favorPerfOverStarvation,
                                     uint32_t starvationDelay)
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
nsBaseAppShell::GetEventloopNestingLevel(uint32_t* aNestingLevelResult)
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

  if (mNativeEventPending.exchange(true))
    return NS_OK;

  
  ScheduleNativeEventCallback();
  return NS_OK;
}


NS_IMETHODIMP
nsBaseAppShell::OnProcessNextEvent(nsIThreadInternal *thr, bool mayWait,
                                   uint32_t recursionDepth)
{
  if (mBlockNativeEvent) {
    if (!mayWait)
      return NS_OK;
    
    
    
    
    mBlockNativeEvent = false;
    if (NS_HasPendingEvents(thr))
      OnDispatchedEvent(thr); 
  }

  PRIntervalTime start = PR_IntervalNow();
  PRIntervalTime limit = THREAD_EVENT_STARVATION_LIMIT;

  
  if (mBlockedWait)
    *mBlockedWait = false;

  bool *oldBlockedWait = mBlockedWait;
  mBlockedWait = &mayWait;

  
  
  
  bool needEvent = mayWait;
  
  
  mProcessedGeckoEvents = false;

  if (mFavorPerf <= 0 && start > mSwitchTime + mStarvationDelay) {
    
    PRIntervalTime now = start;
    bool keepGoing;
    do {
      mLastNativeEventTime = now;
      keepGoing = DoProcessNextNativeEvent(false, recursionDepth);
    } while (keepGoing && ((now = PR_IntervalNow()) - start) < limit);
  } else {
    
    if (start - mLastNativeEventTime > limit) {
      mLastNativeEventTime = start;
      DoProcessNextNativeEvent(false, recursionDepth);
    }
  }

  while (!NS_HasPendingEvents(thr) && !mProcessedGeckoEvents) {
    
    
    
    if (mExiting)
      mayWait = false;

    mLastNativeEventTime = PR_IntervalNow();
    if (!DoProcessNextNativeEvent(mayWait, recursionDepth) || !mayWait)
      break;
  }

  mBlockedWait = oldBlockedWait;

  
  
  
  if (needEvent && !mExiting && !NS_HasPendingEvents(thr)) {
    DispatchDummyEvent(thr);
  }

  
  RunSyncSections(true, recursionDepth);

  return NS_OK;
}

bool
nsBaseAppShell::DispatchDummyEvent(nsIThread* aTarget)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mDummyEvent)
    mDummyEvent = new nsRunnable();

  return NS_SUCCEEDED(aTarget->Dispatch(mDummyEvent, NS_DISPATCH_NORMAL));
}

void
nsBaseAppShell::IncrementEventloopNestingLevel()
{
  ++mEventloopNestingLevel;
#if defined(MOZ_CRASHREPORTER)
  CrashReporter::SetEventloopNestingLevel(mEventloopNestingLevel);
#endif
}

void
nsBaseAppShell::DecrementEventloopNestingLevel()
{
  --mEventloopNestingLevel;
#if defined(MOZ_CRASHREPORTER)
  CrashReporter::SetEventloopNestingLevel(mEventloopNestingLevel);
#endif
}

void
nsBaseAppShell::RunSyncSectionsInternal(bool aStable,
                                        uint32_t aThreadRecursionLevel)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mSyncSections.IsEmpty(), "Nothing to do!");

  
  
  MOZ_RELEASE_ASSERT(!mRunningSyncSections);
  mRunningSyncSections = true;

  
  
  
  
  
  
  
  

  nsTArray<SyncSection> pendingSyncSections;

  for (uint32_t i = 0; i < mSyncSections.Length(); i++) {
    SyncSection& section = mSyncSections[i];
    if ((aStable && section.mStable) ||
        (!section.mStable &&
         section.mEventloopNestingLevel == mEventloopNestingLevel &&
         section.mThreadRecursionLevel == aThreadRecursionLevel)) {
      section.mRunnable->Run();
    }
    else {
      
      SyncSection* pending = pendingSyncSections.AppendElement();
      section.Forget(pending);
    }
  }

  mSyncSections.SwapElements(pendingSyncSections);
  mRunningSyncSections = false;
}

void
nsBaseAppShell::ScheduleSyncSection(nsIRunnable* aRunnable, bool aStable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  nsIThread* thread = NS_GetCurrentThread();

  
  SyncSection* section = mSyncSections.AppendElement();
  section->mStable = aStable;
  section->mRunnable = aRunnable;

  
  
  
  
  if (!aStable) {
    section->mEventloopNestingLevel = mEventloopNestingLevel;

    nsCOMPtr<nsIThreadInternal> threadInternal = do_QueryInterface(thread);
    NS_ASSERTION(threadInternal, "This should never fail!");

    uint32_t recursionLevel;
    if (NS_FAILED(threadInternal->GetRecursionDepth(&recursionLevel))) {
      NS_ERROR("This should never fail!");
    }

    
    
    section->mThreadRecursionLevel = recursionLevel ? recursionLevel - 1 : 0;
  }

  
  if (!NS_HasPendingEvents(thread) && !DispatchDummyEvent(thread)) {
    RunSyncSections(true, 0);
  }
}


NS_IMETHODIMP
nsBaseAppShell::AfterProcessNextEvent(nsIThreadInternal *thr,
                                      uint32_t recursionDepth,
                                      bool eventWasProcessed)
{
  
  RunSyncSections(true, recursionDepth);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Observe(nsISupports *subject, const char *topic,
                        const char16_t *data)
{
  NS_ASSERTION(!strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID), "oops");
  Exit();
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::RunInStableState(nsIRunnable* aRunnable)
{
  ScheduleSyncSection(aRunnable, true);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::RunBeforeNextEvent(nsIRunnable* aRunnable)
{
  ScheduleSyncSection(aRunnable, false);
  return NS_OK;
}
