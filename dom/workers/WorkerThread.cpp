





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

class WorkerThread::Observer MOZ_FINAL
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
: nsThread(nsThread::NOT_MAIN_THREAD, kWorkerStackSize),
  mWorkerPrivate(nullptr)
#ifdef DEBUG
  , mAcceptingNonWorkerRunnables(true)
#endif
{
}

WorkerThread::~WorkerThread()
{
}


already_AddRefed<WorkerThread>
WorkerThread::Create()
{
  MOZ_ASSERT(nsThreadManager::get());

  nsRefPtr<WorkerThread> thread = new WorkerThread();
  if (NS_FAILED(thread->Init())) {
    NS_WARNING("Failed to create new thread!");
    return nullptr;
  }

  NS_SetThreadName(thread, "DOM Worker");

  return thread.forget();
}

void
WorkerThread::SetWorker(WorkerPrivate* aWorkerPrivate)
{
  MOZ_ASSERT(PR_GetCurrentThread() == mThread);
  MOZ_ASSERT_IF(aWorkerPrivate, !mWorkerPrivate);
  MOZ_ASSERT_IF(!aWorkerPrivate, mWorkerPrivate);

  

  if (mWorkerPrivate) {
    MOZ_ASSERT(mObserver);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(RemoveObserver(mObserver)));

    mObserver = nullptr;
    mWorkerPrivate->SetThread(nullptr);
  }

  mWorkerPrivate = aWorkerPrivate;

  if (mWorkerPrivate) {
    mWorkerPrivate->SetThread(this);

    nsRefPtr<Observer> observer = new Observer(mWorkerPrivate);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(AddObserver(observer)));

    mObserver.swap(observer);
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(WorkerThread, nsThread)

NS_IMETHODIMP
WorkerThread::Dispatch(nsIRunnable* aRunnable, uint32_t aFlags)
{
  

#ifdef DEBUG
  if (PR_GetCurrentThread() == mThread) {
    MOZ_ASSERT(mWorkerPrivate);
    mWorkerPrivate->AssertIsOnWorkerThread();
  }
  else if (aRunnable && !IsAcceptingNonWorkerRunnables()) {
    
    nsCOMPtr<nsICancelableRunnable> cancelable = do_QueryInterface(aRunnable);
    MOZ_ASSERT(cancelable,
               "Should have been wrapped by the worker's event target!");
  }
#endif

  
  if (NS_WARN_IF(aFlags != NS_DISPATCH_NORMAL)) {
    return NS_ERROR_UNEXPECTED;
  }

  nsIRunnable* runnableToDispatch;
  nsRefPtr<WorkerRunnable> workerRunnable;

  if (aRunnable && PR_GetCurrentThread() == mThread) {
    
    workerRunnable = mWorkerPrivate->MaybeWrapAsWorkerRunnable(aRunnable);
    runnableToDispatch = workerRunnable;
  }
  else {
    runnableToDispatch = aRunnable;
  }

  nsresult rv = nsThread::Dispatch(runnableToDispatch, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
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
