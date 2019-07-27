





#include "WorkerThread.h"

#include "mozilla/Assertions.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "nsIThreadInternal.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"

#ifdef DEBUG
#include "nsThreadManager.h"
#endif

namespace mozilla {
namespace dom {
namespace workers {

using namespace mozilla::ipc;

namespace {



const uint32_t kWorkerStackSize = 256 * sizeof(size_t) * 1024;

} 

WorkerThreadFriendKey::WorkerThreadFriendKey()
{
  MOZ_COUNT_CTOR(WorkerThreadFriendKey);
}

WorkerThreadFriendKey::~WorkerThreadFriendKey()
{
  MOZ_COUNT_DTOR(WorkerThreadFriendKey);
}

class WorkerThread::Observer final
  : public nsIThreadObserver
{
  WorkerPrivate* mWorkerPrivate;

public:
  explicit Observer(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

  NS_DECL_THREADSAFE_ISUPPORTS

private:
  ~Observer()
  {
    mWorkerPrivate->AssertIsOnWorkerThread();
  }

  NS_DECL_NSITHREADOBSERVER
};

WorkerThread::WorkerThread()
  : nsThread(nsThread::NOT_MAIN_THREAD, kWorkerStackSize)
  , mWorkerPrivateCondVar(mLock, "WorkerThread::mWorkerPrivateCondVar")
  , mWorkerPrivate(nullptr)
  , mOtherThreadsDispatchingViaEventTarget(0)
  , mAcceptingNonWorkerRunnables(true)
{
}

WorkerThread::~WorkerThread()
{
  MOZ_ASSERT(!mWorkerPrivate);
  MOZ_ASSERT(!mOtherThreadsDispatchingViaEventTarget);
  MOZ_ASSERT(mAcceptingNonWorkerRunnables);
}


already_AddRefed<WorkerThread>
WorkerThread::Create(const WorkerThreadFriendKey& )
{
  MOZ_ASSERT(nsThreadManager::get());

  nsRefPtr<WorkerThread> thread = new WorkerThread();
  if (NS_FAILED(thread->Init())) {
    NS_WARNING("Failed to create new thread!");
    return nullptr;
  }

  return thread.forget();
}

void
WorkerThread::SetWorker(const WorkerThreadFriendKey& ,
                        WorkerPrivate* aWorkerPrivate)
{
  MOZ_ASSERT(PR_GetCurrentThread() == mThread);

  if (aWorkerPrivate) {
    {
      MutexAutoLock lock(mLock);

      MOZ_ASSERT(!mWorkerPrivate);
      MOZ_ASSERT(mAcceptingNonWorkerRunnables);

      mWorkerPrivate = aWorkerPrivate;
      mAcceptingNonWorkerRunnables = false;
    }

    mObserver = new Observer(aWorkerPrivate);
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(AddObserver(mObserver)));
  } else {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(RemoveObserver(mObserver)));
    mObserver = nullptr;

    {
      MutexAutoLock lock(mLock);

      MOZ_ASSERT(mWorkerPrivate);
      MOZ_ASSERT(!mAcceptingNonWorkerRunnables);
      MOZ_ASSERT(!mOtherThreadsDispatchingViaEventTarget,
                 "XPCOM Dispatch hapenning at the same time our thread is "
                 "being unset! This should not be possible!");

      while (mOtherThreadsDispatchingViaEventTarget) {
        mWorkerPrivateCondVar.Wait();
      }

      mAcceptingNonWorkerRunnables = true;
      mWorkerPrivate = nullptr;
    }
  }
}

nsresult
WorkerThread::DispatchPrimaryRunnable(const WorkerThreadFriendKey& ,
                                      already_AddRefed<nsIRunnable>&& aRunnable)
{
  nsCOMPtr<nsIRunnable> runnable(aRunnable);

#ifdef DEBUG
  MOZ_ASSERT(PR_GetCurrentThread() != mThread);
  MOZ_ASSERT(runnable);
  {
    MutexAutoLock lock(mLock);

    MOZ_ASSERT(!mWorkerPrivate);
    MOZ_ASSERT(mAcceptingNonWorkerRunnables);
  }
#endif

  nsresult rv = nsThread::Dispatch(runnable.forget(), NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
WorkerThread::DispatchAnyThread(const WorkerThreadFriendKey& ,
                       already_AddRefed<WorkerRunnable>&& aWorkerRunnable)
{
  

#ifdef DEBUG
  {
    const bool onWorkerThread = PR_GetCurrentThread() == mThread;
    {
      MutexAutoLock lock(mLock);

      MOZ_ASSERT(mWorkerPrivate);
      MOZ_ASSERT(!mAcceptingNonWorkerRunnables);

      if (onWorkerThread) {
        mWorkerPrivate->AssertIsOnWorkerThread();
      }
    }
  }
#endif
  nsCOMPtr<nsIRunnable> runnable(aWorkerRunnable);

  nsresult rv = nsThread::Dispatch(runnable.forget(), NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(WorkerThread, nsThread)

NS_IMETHODIMP
WorkerThread::DispatchFromScript(nsIRunnable* aRunnable, uint32_t aFlags)
{
  nsCOMPtr<nsIRunnable> runnable(aRunnable);
  return Dispatch(runnable.forget(), aFlags);
}

NS_IMETHODIMP
WorkerThread::Dispatch(already_AddRefed<nsIRunnable>&& aRunnable, uint32_t aFlags)
{
  
  nsCOMPtr<nsIRunnable> runnable(aRunnable); 

  
  if (NS_WARN_IF(aFlags != NS_DISPATCH_NORMAL)) {
    return NS_ERROR_UNEXPECTED;
  }

  const bool onWorkerThread = PR_GetCurrentThread() == mThread;

#ifdef DEBUG
  if (runnable && !onWorkerThread) {
    nsCOMPtr<nsICancelableRunnable> cancelable = do_QueryInterface(runnable);

    {
      MutexAutoLock lock(mLock);

      
      if (!mAcceptingNonWorkerRunnables) {
        MOZ_ASSERT(cancelable,
                   "Only nsICancelableRunnable may be dispatched to a worker!");
      }
    }
  }
#endif

  WorkerPrivate* workerPrivate = nullptr;
  if (onWorkerThread) {
    
    MOZ_ASSERT(mWorkerPrivate);
    mWorkerPrivate->AssertIsOnWorkerThread();

    workerPrivate = mWorkerPrivate;
  } else {
    MutexAutoLock lock(mLock);

    MOZ_ASSERT(mOtherThreadsDispatchingViaEventTarget < UINT32_MAX);

    if (mWorkerPrivate) {
      workerPrivate = mWorkerPrivate;

      
      
      mOtherThreadsDispatchingViaEventTarget++;
    }
  }

  nsresult rv;
  if (runnable && onWorkerThread) {
    nsRefPtr<WorkerRunnable> workerRunnable = workerPrivate->MaybeWrapAsWorkerRunnable(runnable.forget());
    rv = nsThread::Dispatch(workerRunnable.forget(), NS_DISPATCH_NORMAL);
  } else {
    rv = nsThread::Dispatch(runnable.forget(), NS_DISPATCH_NORMAL);
  }

  if (!onWorkerThread && workerPrivate) {
    
    
    if (NS_SUCCEEDED(rv)) {
      MutexAutoLock workerLock(workerPrivate->mMutex);

      workerPrivate->mCondVar.Notify();
    }

    
    {
      MutexAutoLock lock(mLock);

      MOZ_ASSERT(mOtherThreadsDispatchingViaEventTarget);

      if (!--mOtherThreadsDispatchingViaEventTarget) {
        mWorkerPrivateCondVar.Notify();
      }
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

uint32_t
WorkerThread::RecursionDepth(const WorkerThreadFriendKey& ) const
{
  MOZ_ASSERT(PR_GetCurrentThread() == mThread);

  return mNestedEventLoopDepth;
}

NS_IMPL_ISUPPORTS(WorkerThread::Observer, nsIThreadObserver)

NS_IMETHODIMP
WorkerThread::Observer::OnDispatchedEvent(nsIThreadInternal* )
{
  MOZ_CRASH("OnDispatchedEvent() should never be called!");
}

NS_IMETHODIMP
WorkerThread::Observer::OnProcessNextEvent(nsIThreadInternal* ,
                                           bool aMayWait,
                                           uint32_t aRecursionDepth)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  
  
  
  
  
  if (aMayWait) {
    MOZ_ASSERT(aRecursionDepth == 2);
    MOZ_ASSERT(!BackgroundChild::GetForCurrentThread());
    return NS_OK;
  }

  mWorkerPrivate->OnProcessNextEvent(aRecursionDepth);
  return NS_OK;
}

NS_IMETHODIMP
WorkerThread::Observer::AfterProcessNextEvent(nsIThreadInternal* ,
                                              uint32_t aRecursionDepth,
                                              bool )
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  mWorkerPrivate->AfterProcessNextEvent(aRecursionDepth);
  return NS_OK;
}

} 
} 
} 
