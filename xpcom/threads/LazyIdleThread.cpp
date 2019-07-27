





#include "LazyIdleThread.h"

#include "nsIObserverService.h"

#include "GeckoProfiler.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"

#ifdef DEBUG
#define ASSERT_OWNING_THREAD()                                                 \
  PR_BEGIN_MACRO                                                               \
    nsIThread* currentThread = NS_GetCurrentThread();                          \
    if (currentThread) {                                                       \
      nsCOMPtr<nsISupports> current(do_QueryInterface(currentThread));         \
      nsCOMPtr<nsISupports> test(do_QueryInterface(mOwningThread));            \
      MOZ_ASSERT(current == test, "Wrong thread!");                            \
    }                                                                          \
  PR_END_MACRO
#else
#define ASSERT_OWNING_THREAD()
#endif

namespace mozilla {

LazyIdleThread::LazyIdleThread(uint32_t aIdleTimeoutMS,
                               const nsCSubstring& aName,
                               ShutdownMethod aShutdownMethod,
                               nsIObserver* aIdleObserver)
  : mMutex("LazyIdleThread::mMutex")
  , mOwningThread(NS_GetCurrentThread())
  , mIdleObserver(aIdleObserver)
  , mQueuedRunnables(nullptr)
  , mIdleTimeoutMS(aIdleTimeoutMS)
  , mPendingEventCount(0)
  , mIdleNotificationCount(0)
  , mShutdownMethod(aShutdownMethod)
  , mShutdown(false)
  , mThreadIsShuttingDown(false)
  , mIdleTimeoutEnabled(true)
  , mName(aName)
{
  MOZ_ASSERT(mOwningThread, "Need owning thread!");
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
  mIdleTimeoutEnabled = false;

  if (mIdleTimer && NS_FAILED(mIdleTimer->Cancel())) {
    NS_WARNING("Failed to cancel timer!");
  }

  MutexAutoLock lock(mMutex);

  
  MOZ_ASSERT(mPendingEventCount < UINT32_MAX, "Way too many!");
  mPendingEventCount++;
}

void
LazyIdleThread::EnableIdleTimeout()
{
  ASSERT_OWNING_THREAD();
  if (mIdleTimeoutEnabled) {
    return;
  }
  mIdleTimeoutEnabled = true;

  {
    MutexAutoLock lock(mMutex);

    MOZ_ASSERT(mPendingEventCount, "Mismatched calls to observer methods!");
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

  MOZ_ASSERT(mPendingEventCount < UINT32_MAX, "Way too many!");
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

  MOZ_ASSERT(!mPendingEventCount, "Shouldn't have events yet!");
  MOZ_ASSERT(!mIdleNotificationCount, "Shouldn't have idle events yet!");
  MOZ_ASSERT(!mIdleTimer, "Should have killed this long ago!");
  MOZ_ASSERT(!mThreadIsShuttingDown, "Should have cleared that!");

  nsresult rv;

  if (mShutdownMethod == AutomaticShutdown && NS_IsMainThread()) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = obs->AddObserver(this, "xpcom-shutdown-threads", false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  mIdleTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  if (NS_WARN_IF(!mIdleTimer)) {
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &LazyIdleThread::InitThread);
  if (NS_WARN_IF(!runnable)) {
    return NS_ERROR_UNEXPECTED;
  }

  rv = NS_NewThread(getter_AddRefs(mThread), runnable);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
LazyIdleThread::InitThread()
{
#if !defined(MOZILLA_XPCOMRT_API)
  char aLocal;
  profiler_register_thread(mName.get(), &aLocal);
#endif 

  PR_SetCurrentThreadName(mName.get());

  

  nsCOMPtr<nsIThreadInternal> thread(do_QueryInterface(NS_GetCurrentThread()));
  MOZ_ASSERT(thread, "This should always succeed!");

  if (NS_FAILED(thread->SetObserver(this))) {
    NS_WARNING("Failed to set thread observer!");
  }
}

void
LazyIdleThread::CleanupThread()
{
  nsCOMPtr<nsIThreadInternal> thread(do_QueryInterface(NS_GetCurrentThread()));
  MOZ_ASSERT(thread, "This should always succeed!");

  if (NS_FAILED(thread->SetObserver(nullptr))) {
    NS_WARNING("Failed to set thread observer!");
  }

  {
    MutexAutoLock lock(mMutex);

    MOZ_ASSERT(!mThreadIsShuttingDown, "Shouldn't be true ever!");
    mThreadIsShuttingDown = true;
  }

#if !defined(MOZILLA_XPCOMRT_API)
  profiler_unregister_thread();
#endif 
}

void
LazyIdleThread::ScheduleTimer()
{
  ASSERT_OWNING_THREAD();

  bool shouldSchedule;
  {
    MutexAutoLock lock(mMutex);

    MOZ_ASSERT(mIdleNotificationCount, "Should have at least one!");
    --mIdleNotificationCount;

    shouldSchedule = !mIdleNotificationCount && !mPendingEventCount;
  }

  if (mIdleTimer) {
    if (NS_FAILED(mIdleTimer->Cancel())) {
      NS_WARNING("Failed to cancel timer!");
    }

    if (shouldSchedule &&
        NS_FAILED(mIdleTimer->InitWithCallback(this, mIdleTimeoutMS,
                                               nsITimer::TYPE_ONE_SHOT))) {
      NS_WARNING("Failed to schedule timer!");
    }
  }
}

nsresult
LazyIdleThread::ShutdownThread()
{
  ASSERT_OWNING_THREAD();

  
  
  
  nsAutoTArray<nsCOMPtr<nsIRunnable>, 10> queuedRunnables;

  nsresult rv;

  
  
  
  if (mIdleTimer) {
    rv = mIdleTimer->Cancel();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    mIdleTimer = nullptr;
  }

  if (mThread) {
    if (mShutdownMethod == AutomaticShutdown && NS_IsMainThread()) {
      nsCOMPtr<nsIObserverService> obs =
        mozilla::services::GetObserverService();
      NS_WARN_IF_FALSE(obs, "Failed to get observer service!");

      if (obs &&
          NS_FAILED(obs->RemoveObserver(this, "xpcom-shutdown-threads"))) {
        NS_WARNING("Failed to remove observer!");
      }
    }

    if (mIdleObserver) {
      mIdleObserver->Observe(static_cast<nsIThread*>(this), IDLE_THREAD_TOPIC,
                             nullptr);
    }

#ifdef DEBUG
    {
      MutexAutoLock lock(mMutex);
      MOZ_ASSERT(!mThreadIsShuttingDown, "Huh?!");
    }
#endif

    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &LazyIdleThread::CleanupThread);
    if (NS_WARN_IF(!runnable)) {
      return NS_ERROR_UNEXPECTED;
    }

    PreDispatch();

    rv = mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    mQueuedRunnables = &queuedRunnables;

    if (NS_FAILED(mThread->Shutdown())) {
      NS_ERROR("Failed to shutdown the thread!");
    }

    
    mQueuedRunnables = nullptr;

    mThread = nullptr;

    {
      MutexAutoLock lock(mMutex);

      MOZ_ASSERT(!mPendingEventCount, "Huh?!");
      MOZ_ASSERT(!mIdleNotificationCount, "Huh?!");
      MOZ_ASSERT(mThreadIsShuttingDown, "Huh?!");
      mThreadIsShuttingDown = false;
    }
  }

  
  if (queuedRunnables.Length()) {
    
    if (mShutdown) {
      NS_ERROR("Runnables dispatched to LazyIdleThread will never run!");
      return NS_OK;
    }

    
    for (uint32_t index = 0; index < queuedRunnables.Length(); index++) {
      nsCOMPtr<nsIRunnable> runnable;
      runnable.swap(queuedRunnables[index]);
      MOZ_ASSERT(runnable, "Null runnable?!");

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
  MOZ_ASSERT(mRefCnt == 1, "Bad refcount!");
  delete this;
}

NS_IMPL_ADDREF(LazyIdleThread)

NS_IMETHODIMP_(MozExternalRefCountType)
LazyIdleThread::Release()
{
  nsrefcnt count = --mRefCnt;
  NS_LOG_RELEASE(this, count, "LazyIdleThread");

  if (!count) {
    
    mRefCnt = 1;

    nsCOMPtr<nsIRunnable> runnable =
      NS_NewNonOwningRunnableMethod(this, &LazyIdleThread::SelfDestruct);
    NS_WARN_IF_FALSE(runnable, "Couldn't make runnable!");

    if (NS_FAILED(NS_DispatchToCurrentThread(runnable))) {
      MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
      
      
      
      
      SelfDestruct();
    }
  }

  return count;
}

NS_IMPL_QUERY_INTERFACE(LazyIdleThread, nsIThread,
                        nsIEventTarget,
                        nsITimerCallback,
                        nsIThreadObserver,
                        nsIObserver)

NS_IMETHODIMP
LazyIdleThread::Dispatch(nsIRunnable* aEvent,
                         uint32_t aFlags)
{
  ASSERT_OWNING_THREAD();

  
  if (NS_WARN_IF(aFlags != NS_DISPATCH_NORMAL)) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (NS_WARN_IF(mShutdown)) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  if (UseRunnableQueue()) {
    mQueuedRunnables->AppendElement(aEvent);
    return NS_OK;
  }

  nsresult rv = EnsureThread();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  PreDispatch();

  return mThread->Dispatch(aEvent, aFlags);
}

NS_IMETHODIMP
LazyIdleThread::IsOnCurrentThread(bool* aIsOnCurrentThread)
{
  if (mThread) {
    return mThread->IsOnCurrentThread(aIsOnCurrentThread);
  }

  *aIsOnCurrentThread = false;
  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::GetPRThread(PRThread** aPRThread)
{
  if (mThread) {
    return mThread->GetPRThread(aPRThread);
  }

  *aPRThread = nullptr;
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
LazyIdleThread::Shutdown()
{
  ASSERT_OWNING_THREAD();

  mShutdown = true;

  nsresult rv = ShutdownThread();
  MOZ_ASSERT(!mThread, "Should have destroyed this by now!");

  mIdleObserver = nullptr;

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

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
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::OnDispatchedEvent(nsIThreadInternal* )
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");
  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::OnProcessNextEvent(nsIThreadInternal* ,
                                   bool ,
                                   uint32_t )
{
  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::AfterProcessNextEvent(nsIThreadInternal* ,
                                      uint32_t ,
                                      bool aEventWasProcessed)
{
  bool shouldNotifyIdle;
  {
    MutexAutoLock lock(mMutex);

    if (aEventWasProcessed) {
      MOZ_ASSERT(mPendingEventCount, "Mismatched calls to observer methods!");
      --mPendingEventCount;
    }

    if (mThreadIsShuttingDown) {
      
      return NS_OK;
    }

    shouldNotifyIdle = !mPendingEventCount;
    if (shouldNotifyIdle) {
      MOZ_ASSERT(mIdleNotificationCount < UINT32_MAX, "Way too many!");
      mIdleNotificationCount++;
    }
  }

  if (shouldNotifyIdle) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &LazyIdleThread::ScheduleTimer);
    if (NS_WARN_IF(!runnable)) {
      return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = mOwningThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
LazyIdleThread::Observe(nsISupports* ,
                        const char*  aTopic,
                        const char16_t* )
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  MOZ_ASSERT(mShutdownMethod == AutomaticShutdown,
             "Should not receive notifications if not AutomaticShutdown!");
  MOZ_ASSERT(!strcmp("xpcom-shutdown-threads", aTopic), "Bad topic!");

  Shutdown();
  return NS_OK;
}

} 
