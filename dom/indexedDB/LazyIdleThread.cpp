






































#include "LazyIdleThread.h"

#include "nsIObserverService.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE

using mozilla::MutexAutoLock;

namespace {




class CancelTimerRunnable : public nsRunnable
{
public:
  CancelTimerRunnable(nsITimer* aTimer)
  : mTimer(aTimer)
  { }

  NS_IMETHOD Run() {
    return mTimer->Cancel();
  }

private:
  nsCOMPtr<nsITimer> mTimer;
};

} 

LazyIdleThread::LazyIdleThread(PRUint32 aIdleTimeoutMS)
: mMutex("LazyIdleThread::mMutex"),
  mOwningThread(NS_GetCurrentThread()),
  mIdleTimeoutMS(aIdleTimeoutMS),
  mShutdown(PR_FALSE),
  mThreadHasTimedOut(PR_FALSE)
{
  NS_ASSERTION(mOwningThread, "This should never fail!");
}

LazyIdleThread::~LazyIdleThread()
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  Shutdown();
}




nsresult
LazyIdleThread::EnsureThread()
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  if (mShutdown) {
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
  }

  if (mThread) {
    return NS_OK;
  }

  if (!mOwningThread) {
    NS_ASSERTION(mOwningThread, "This should never be null!");
  }

  nsresult rv;

  if (NS_IsMainThread()) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->AddObserver(this, "xpcom-shutdown-threads", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

#ifdef DEBUG
  nsCOMPtr<nsIThreadObserver> oldObserver;
  nsresult rv = thread->GetObserver(getter_AddRefs(oldObserver));
  NS_ASSERTION(NS_SUCCEEDED(rv) && !oldObserver, "Already have an observer!");
#endif

  if (NS_FAILED(thread->SetObserver(this))) {
    NS_WARNING("Failed to set thread observer!");
  }
}





void
LazyIdleThread::ShutdownThread()
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  if (mThread) {
    if (NS_FAILED(mThread->Shutdown())) {
      NS_WARNING("Failed to shutdown thread!");
    }
    mThread = nsnull;
  }

  
  if (mIdleTimer) {
    if (NS_FAILED(mIdleTimer->Cancel())) {
      NS_WARNING("Failed to cancel timer!");
    }
    mIdleTimer = nsnull;
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS3(LazyIdleThread, nsITimerCallback,
                                              nsIThreadObserver,
                                              nsIObserver)




nsresult
LazyIdleThread::Dispatch(nsIRunnable* aEvent)
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  nsresult rv = EnsureThread();
  NS_ENSURE_SUCCESS(rv, rv);

  return mThread->Dispatch(aEvent, NS_DISPATCH_NORMAL);
}




void
LazyIdleThread::Shutdown()
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  mShutdown = PR_TRUE;

  ShutdownThread();
}






NS_IMETHODIMP
LazyIdleThread::Notify(nsITimer* aTimer)
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  {
    MutexAutoLock lock(mMutex);

    
    
    if (aTimer != mIdleTimer) {
      return NS_OK;
    }

    
    mIdleTimer = nsnull;
    mThreadHasTimedOut = PR_TRUE;
  }

  ShutdownThread();

  
  mThreadHasTimedOut = PR_FALSE;
  return NS_OK;
}





NS_IMETHODIMP
LazyIdleThread::OnDispatchedEvent(nsIThreadInternal* )
{
  

  nsCOMPtr<nsITimer> timer;
  {
    MutexAutoLock lock(mMutex);

    if (mThreadHasTimedOut) {
      return NS_OK;
    }

    timer.swap(mIdleTimer);
  }

  if (timer) {
    nsresult rv = mOwningThread->Dispatch(new CancelTimerRunnable(timer),
                                          NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}




NS_IMETHODIMP
LazyIdleThread::OnProcessNextEvent(nsIThreadInternal* ,
                                   PRBool ,
                                   PRUint32 )
{
  return NS_OK;
}





NS_IMETHODIMP
LazyIdleThread::AfterProcessNextEvent(nsIThreadInternal* aThread,
                                      PRUint32 )
{
  PRBool hasPendingEvents;
  nsresult rv = aThread->HasPendingEvents(&hasPendingEvents);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasPendingEvents) {
    return NS_OK;
  }

  MutexAutoLock lock(mMutex);

  if (mThreadHasTimedOut) {
    return NS_OK;
  }

  NS_ASSERTION(!mIdleTimer, "Should have been cleared!");

  mIdleTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mIdleTimer->SetTarget(mOwningThread);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mIdleTimer->InitWithCallback(this, mIdleTimeoutMS,
                                    nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMETHODIMP
LazyIdleThread::Observe(nsISupports* ,
                        const char* ,
                        const PRUnichar* )
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  ShutdownThread();
  return NS_OK;
}
