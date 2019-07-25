






































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
  if (!mShutdown) {
    NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");
    Shutdown();
  }
}

void
LazyIdleThread::SetIdleObserver(nsIObserver* aObserver)
{
  if (mShutdown) {
    if (aObserver) {
      NS_WARNING("Setting an observer after Shutdown was called!");
      return;
    }
  }
  else {
    NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");
  }

  mIdleObserver = aObserver;
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

    if (mIdleObserver) {
      mIdleObserver->Observe(mThread, IDLE_THREAD_TOPIC, nsnull);
    }

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




void
LazyIdleThread::CancelTimer(nsITimer* aTimer)
{
  NS_ASSERTION(aTimer, "Null timer!");
  if (NS_FAILED(mOwningThread->Dispatch(new CancelTimerRunnable(aTimer),
                                        NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch CancelTimerRunnable!");
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS5(LazyIdleThread, nsIThread,
                                              nsIEventTarget,
                                              nsITimerCallback,
                                              nsIThreadObserver,
                                              nsIObserver)




NS_IMETHODIMP
LazyIdleThread::Dispatch(nsIRunnable* aEvent,
                         PRUint32 aFlags)
{
  NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

  nsresult rv = EnsureThread();
  NS_ENSURE_SUCCESS(rv, rv);

  return mThread->Dispatch(aEvent, aFlags);
}

NS_IMETHODIMP
LazyIdleThread::IsOnCurrentThread(PRBool* aIsOnCurrentThread)
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
  if (!mShutdown) {
    NS_ASSERTION(NS_GetCurrentThread() == mOwningThread, "Wrong thread!");

    ShutdownThread();

    mShutdown = PR_TRUE;
    mOwningThread = nsnull;
    mIdleObserver = nsnull;
  }

  return NS_OK;
}





NS_IMETHODIMP
LazyIdleThread::HasPendingEvents(PRBool* aHasPendingEvents)
{
  nsCOMPtr<nsIThread> thisThread(NS_GetCurrentThread());
  NS_ASSERTION(thisThread, "This should never be null!");

  return thisThread->HasPendingEvents(aHasPendingEvents);
}





NS_IMETHODIMP
LazyIdleThread::ProcessNextEvent(PRBool aMayWait,
                                 PRBool* aEventWasProcessed)
{
  nsCOMPtr<nsIThread> thisThread(NS_GetCurrentThread());
  NS_ASSERTION(thisThread, "This should never be null!");

  return thisThread->ProcessNextEvent(aMayWait, aEventWasProcessed);
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
    CancelTimer(timer);
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

  if (NS_UNLIKELY(mIdleTimer)) {
    rv = mIdleTimer->Cancel();
  }
  else {
    mIdleTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mIdleTimer->SetTarget(mOwningThread);
  }
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
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  Shutdown();
  return NS_OK;
}
