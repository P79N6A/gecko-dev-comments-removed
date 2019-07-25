






































#include "LazyIdleThread.h"

#include "nsIObserverService.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

#ifdef DEBUG
#define ASSERT_OWNING_THREAD()                                                 \
  PR_BEGIN_MACRO                                                               \
    nsIThread* currentThread = NS_GetCurrentThread();                          \
    if (currentThread) {                                                       \
      nsCOMPtr<nsISupports> current(do_QueryInterface(currentThread));         \
      nsCOMPtr<nsISupports> test(do_QueryInterface(mOwningThread));            \
      NS_ASSERTION(current == test, "Wrong thread!");                          \
    }                                                                          \
  PR_END_MACRO
#else
#define ASSERT_OWNING_THREAD()
#endif

USING_INDEXEDDB_NAMESPACE

using mozilla::MutexAutoLock;

LazyIdleThread::LazyIdleThread(PRUint32 aIdleTimeoutMS,
                               ShutdownMethod aShutdownMethod,
                               nsIObserver* aIdleObserver)
: mMutex("LazyIdleThread::mMutex"),
  mOwningThread(NS_GetCurrentThread()),
  mIdleObserver(aIdleObserver),
  mQueuedRunnables(nsnull),
  mIdleTimeoutMS(aIdleTimeoutMS),
  mPendingEventCount(0),
  mIdleNotificationCount(0),
  mShutdownMethod(aShutdownMethod),
  mShutdown(PR_FALSE),
  mThreadIsShuttingDown(PR_FALSE),
  mIdleTimeoutEnabled(PR_TRUE)
{
  NS_ASSERTION(mOwningThread, "This should never fail!");
}

LazyIdleThread::~LazyIdleThread()
{
  ASSERT_OWNING_THREAD();

  Shutdown();
}

void
LazyIdleThread::SetWeakIdleObserver(nsIObserver* aObserver)
{
  ASSERT_OWNING_THREAD();

  if (mShutdown) {
    NS_WARN_IF_FALSE(!aObserver,
                     "Setting an observer after Shutdown was called!");
    return;
  }

  mIdleObserver = aObserver;
}

void
LazyIdleThread::DisableIdleTimeout()
{
  ASSERT_OWNING_THREAD();
  if (!mIdleTimeoutEnabled) {
    return;
  }
  mIdleTimeoutEnabled = PR_FALSE;

  if (mIdleTimer && NS_FAILED(mIdleTimer->Cancel())) {
    NS_WARNING("Failed to cancel timer!");
  }

  MutexAutoLock lock(mMutex);

  
  NS_ASSERTION(mPendingEventCount < PR_UINT32_MAX, "Way too many!");
  mPendingEventCount++;
}

void
LazyIdleThread::EnableIdleTimeout()
{
  ASSERT_OWNING_THREAD();
  if (mIdleTimeoutEnabled) {
    return;
  }
  mIdleTimeoutEnabled = PR_TRUE;

  {
    MutexAutoLock lock(mMutex);

    NS_ASSERTION(mPendingEventCount, "Mismatched calls to observer methods!");
    --mPendingEventCount;
  }

  if (mThread) {
    nsCOMPtr<nsIRunnable> runnable(new nsRunnable());
    if (NS_FAILED(Dispatch(runnable, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch!");
    }
  }
}

void
LazyIdleThread::PreDispatch()
{
  MutexAutoLock lock(mMutex);

  NS_ASSERTION(mPendingEventCount < PR_UINT32_MAX, "Way too many!");
  mPendingEventCount++;
}

nsresult
LazyIdleThread::EnsureThread()
{
  ASSERT_OWNING_THREAD();

  if (mShutdown) {
    return NS_ERROR_UNEXPECTED;
  }

  if (mThread) {
    return NS_OK;
  }

  NS_ASSERTION(!mPendingEventCount, "Shouldn't have events yet!");
  NS_ASSERTION(!mIdleNotificationCount, "Shouldn't have idle events yet!");
  NS_ASSERTION(!mIdleTimer, "Should have killed this long ago!");
  NS_ASSERTION(!mThreadIsShuttingDown, "Should have cleared that!");

  nsresult rv;

  if (mShutdownMethod == AutomaticShutdown && NS_IsMainThread()) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->AddObserver(this, "xpcom-shutdown-threads", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mIdleTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_TRUE(mIdleTimer, NS_ERROR_FAILURE);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &LazyIdleThread::InitThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

  rv = NS_NewThread(getter_AddRefs(mThread), runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
LazyIdleThread::InitThread()
{
  

  nsCOMPtr<nsIThreadInternal> thread(do_QueryInterface(NS_GetCurrentThread()));
  NS_ASSERTION(thread, "This should always succeed!");

  if (NS_FAILED(thread->SetObserver(this))) {
    NS_WARNING("Failed to set thread observer!");
  }
}

void
LazyIdleThread::CleanupThread()
{
  nsCOMPtr<nsIThreadInternal> thread(do_QueryInterface(NS_GetCurrentThread()));
  NS_ASSERTION(thread, "This should always succeed!");

  if (NS_FAILED(thread->SetObserver(nsnull))) {
    NS_WARNING("Failed to set thread observer!");
  }

  MutexAutoLock lock(mMutex);

  NS_ASSERTION(!mThreadIsShuttingDown, "Shouldn't be true ever!");
  mThreadIsShuttingDown = PR_TRUE;
}

void
LazyIdleThread::ScheduleTimer()
{
  ASSERT_OWNING_THREAD();

  bool shouldSchedule;
  {
    MutexAutoLock lock(mMutex);

    NS_ASSERTION(mIdleNotificationCount, "Should have at least one!");
    --mIdleNotificationCount;

    shouldSchedule = !mIdleNotificationCount && !mPendingEventCount;
  }

  if (NS_FAILED(mIdleTimer->Cancel())) {
    NS_WARNING("Failed to cancel timer!");
  }

  if (shouldSchedule &&
      NS_FAILED(mIdleTimer->InitWithCallback(this, mIdleTimeoutMS,
                                             nsITimer::TYPE_ONE_SHOT))) {
    NS_WARNING("Failed to schedule timer!");
  }
}

nsresult
LazyIdleThread::ShutdownThread()
{
  ASSERT_OWNING_THREAD();

  
  
  
  nsAutoTArray<nsCOMPtr<nsIRunnable>, 10> queuedRunnables;

  nsresult rv;

  if (mThread) {
    if (mShutdownMethod == AutomaticShutdown && NS_IsMainThread()) {
      nsCOMPtr<nsIObserverService> obs =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
      NS_WARN_IF_FALSE(obs, "Failed to get observer service!");

      if (obs &&
          NS_FAILED(obs->RemoveObserver(this, "xpcom-shutdown-threads"))) {
        NS_WARNING("Failed to remove observer!");
      }
    }

    if (mIdleObserver) {
      mIdleObserver->Observe(static_cast<nsIThread*>(this), IDLE_THREAD_TOPIC,
                             nsnull);
    }

#ifdef DEBUG
    {
      MutexAutoLock lock(mMutex);
      NS_ASSERTION(!mThreadIsShuttingDown, "Huh?!");
    }
#endif

    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &LazyIdleThread::CleanupThread);
    NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

    PreDispatch();

    rv = mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mQueuedRunnables = &queuedRunnables;

    if (NS_FAILED(mThread->Shutdown())) {
      NS_ERROR("Failed to shutdown the thread!");
    }

    
    mQueuedRunnables = nsnull;

    mThread = nsnull;

    {
      MutexAutoLock lock(mMutex);

      NS_ASSERTION(!mPendingEventCount, "Huh?!");
      NS_ASSERTION(!mIdleNotificationCount, "Huh?!");
      NS_ASSERTION(mThreadIsShuttingDown, "Huh?!");
      mThreadIsShuttingDown = PR_FALSE;
    }
  }

  if (mIdleTimer) {
    rv = mIdleTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    mIdleTimer = nsnull;
  }

  
  if (queuedRunnables.Length()) {
    
    if (mShutdown) {
      NS_ERROR("Runnables dispatched to LazyIdleThread will never run!");
      return NS_OK;
    }

    
    for (PRUint32 index = 0; index < queuedRunnables.Length(); index++) {
      nsCOMPtr<nsIRunnable> runnable;
      runnable.swap(queuedRunnables[index]);
      NS_ASSERTION(runnable, "Null runnable?!");

      if (NS_FAILED(Dispatch(runnable, NS_DISPATCH_NORMAL))) {
        NS_ERROR("Failed to re-dispatch queued runnable!");
      }
    }
  }

  return NS_OK;
}

void
LazyIdleThread::SelfDestruct()
{
  NS_ASSERTION(mRefCnt == 1, "Bad refcount!");
  delete this;
}

NS_IMPL_THREADSAFE_ADDREF(LazyIdleThread)

NS_IMETHODIMP_(nsrefcnt)
LazyIdleThread::Release()
{
  nsrefcnt count = NS_AtomicDecrementRefcnt(mRefCnt);
  NS_LOG_RELEASE(this, count, "LazyIdleThread");

  if (!count) {
    
    mRefCnt = 1;

    nsCOMPtr<nsIRunnable> runnable =
      NS_NewNonOwningRunnableMethod(this, &LazyIdleThread::SelfDestruct);
    NS_WARN_IF_FALSE(runnable, "Couldn't make runnable!");

    if (NS_FAILED(NS_DispatchToCurrentThread(runnable))) {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
      
      
      
      
      SelfDestruct();
    }
  }

  return count;
}

NS_IMPL_THREADSAFE_QUERY_INTERFACE5(LazyIdleThread, nsIThread,
                                                    nsIEventTarget,
                                                    nsITimerCallback,
                                                    nsIThreadObserver,
                                                    nsIObserver)

NS_IMETHODIMP
LazyIdleThread::Dispatch(nsIRunnable* aEvent,
                         PRUint32 aFlags)
{
  ASSERT_OWNING_THREAD();

  
  NS_ENSURE_TRUE(aFlags == NS_DISPATCH_NORMAL, NS_ERROR_NOT_IMPLEMENTED);

  
  
  if (UseRunnableQueue()) {
    mQueuedRunnables->AppendElement(aEvent);
    return NS_OK;
  }

  nsresult rv = EnsureThread();
  NS_ENSURE_SUCCESS(rv, rv);

  PreDispatch();

  return mThread->Dispatch(aEvent, aFlags);
}

NS_IMETHODIMP
LazyIdleThread::IsOnCurrentThread(bool* aIsOnCurrentThread)
{
  if (mThread) {
    return mThread->IsOnCurrentThread(aIsOnCurrentThread);
  }

  *aIsOnCurrentThread = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::GetPRThread(PRThread** aPRThread)
{
  if (mThread) {
    return mThread->GetPRThread(aPRThread);
  }

  *aPRThread = nsnull;
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
LazyIdleThread::Shutdown()
{
  ASSERT_OWNING_THREAD();

  mShutdown = PR_TRUE;

  nsresult rv = ShutdownThread();
  NS_ASSERTION(!mThread, "Should have destroyed this by now!");

  mIdleObserver = nsnull;

  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::HasPendingEvents(bool* aHasPendingEvents)
{
  
  
  NS_NOTREACHED("Shouldn't ever call this!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LazyIdleThread::ProcessNextEvent(bool aMayWait,
                                 bool* aEventWasProcessed)
{
  
  
  NS_NOTREACHED("Shouldn't ever call this!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LazyIdleThread::Notify(nsITimer* aTimer)
{
  ASSERT_OWNING_THREAD();

  {
    MutexAutoLock lock(mMutex);

    if (mPendingEventCount || mIdleNotificationCount) {
      
      
      return NS_OK;
    }
  }

  nsresult rv = ShutdownThread();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::OnDispatchedEvent(nsIThreadInternal* )
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");
  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::OnProcessNextEvent(nsIThreadInternal* ,
                                   bool ,
                                   PRUint32 )
{
  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::AfterProcessNextEvent(nsIThreadInternal* ,
                                      PRUint32 )
{
  bool shouldNotifyIdle;
  {
    MutexAutoLock lock(mMutex);

    NS_ASSERTION(mPendingEventCount, "Mismatched calls to observer methods!");
    --mPendingEventCount;

    if (mThreadIsShuttingDown) {
      
      return NS_OK;
    }

    shouldNotifyIdle = !mPendingEventCount;
    if (shouldNotifyIdle) {
      NS_ASSERTION(mIdleNotificationCount < PR_UINT32_MAX, "Way too many!");
      mIdleNotificationCount++;
    }
  }

  if (shouldNotifyIdle) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &LazyIdleThread::ScheduleTimer);
    NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

    nsresult rv = mOwningThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::Observe(nsISupports* ,
                        const char*  aTopic,
                        const PRUnichar* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mShutdownMethod == AutomaticShutdown,
               "Should not receive notifications if not AutomaticShutdown!");
  NS_ASSERTION(!strcmp("xpcom-shutdown-threads", aTopic), "Bad topic!");

  Shutdown();
  return NS_OK;
}
