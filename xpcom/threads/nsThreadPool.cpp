





































#include "nsIProxyObjectManager.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadPool.h"
#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "prinrval.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("nsThreadPool");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)







#define DEFAULT_THREAD_LIMIT 4
#define DEFAULT_IDLE_THREAD_LIMIT 1
#define DEFAULT_IDLE_THREAD_TIMEOUT PR_SecondsToInterval(60)

NS_IMPL_THREADSAFE_ADDREF(nsThreadPool)
NS_IMPL_THREADSAFE_RELEASE(nsThreadPool)
NS_IMPL_QUERY_INTERFACE3_CI(nsThreadPool, nsIThreadPool, nsIEventTarget,
                            nsIRunnable)
NS_IMPL_CI_INTERFACE_GETTER2(nsThreadPool, nsIThreadPool, nsIEventTarget)

nsThreadPool::nsThreadPool()
  : mThreadLimit(DEFAULT_THREAD_LIMIT)
  , mIdleThreadLimit(DEFAULT_IDLE_THREAD_LIMIT)
  , mIdleThreadTimeout(DEFAULT_IDLE_THREAD_TIMEOUT)
  , mIdleCount(0)
  , mShutdown(PR_FALSE)
{
}

nsThreadPool::~nsThreadPool()
{
  Shutdown();
}

nsresult
nsThreadPool::PutEvent(nsIRunnable *event)
{
  
 
  PRBool spawnThread = PR_FALSE;
  {
    nsAutoMonitor mon(mEvents.Monitor());

    LOG(("THRD-P(%p) put [%d %d %d]\n", this, mIdleCount, mThreads.Count(),
         mThreadLimit));
    NS_ASSERTION(mIdleCount <= (PRUint32) mThreads.Count(), "oops");

    
    if (mIdleCount == 0 && mThreads.Count() < (PRInt32) mThreadLimit)
      spawnThread = PR_TRUE;

    mEvents.PutEvent(event);
  }

  LOG(("THRD-P(%p) put [spawn=%d]\n", this, spawnThread));
  if (!spawnThread)
    return NS_OK;

  nsCOMPtr<nsIThread> thread;
  nsThreadManager::get()->NewThread(0, getter_AddRefs(thread));
  NS_ENSURE_STATE(thread);

  PRBool killThread = PR_FALSE;
  {
    nsAutoMonitor mon(mEvents.Monitor());
    if (mThreads.Count() < (PRInt32) mThreadLimit) {
      mThreads.AppendObject(thread);
    } else {
      killThread = PR_TRUE;  
    }
  }
  LOG(("THRD-P(%p) put [%p kill=%d]\n", this, thread.get(), killThread));
  if (killThread) {
    thread->Shutdown();
  } else {
    thread->Dispatch(this, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

void
nsThreadPool::ShutdownThread(nsIThread *thread)
{
  LOG(("THRD-P(%p) shutdown async [%p]\n", this, thread));

  
  

  NS_ASSERTION(!NS_IsMainThread(), "wrong thread");

  nsCOMPtr<nsIThread> doomed;
  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, NS_GET_IID(nsIThread), thread,
                       NS_PROXY_ASYNC, getter_AddRefs(doomed));
  if (doomed) {
    doomed->Shutdown();
  } else {
    NS_WARNING("failed to construct proxy to main thread");
  }
}

NS_IMETHODIMP
nsThreadPool::Run()
{
  LOG(("THRD-P(%p) enter\n", this));

  nsCOMPtr<nsIThread> current;
  nsThreadManager::get()->GetCurrentThread(getter_AddRefs(current));

  PRBool shutdownThreadOnExit = PR_FALSE;
  PRBool exitThread = PR_FALSE;
  PRBool wasIdle = PR_FALSE;
  PRIntervalTime idleSince;

  do {
    nsCOMPtr<nsIRunnable> event;
    {
      nsAutoMonitor mon(mEvents.Monitor());
      if (!mEvents.GetPendingEvent(getter_AddRefs(event))) {
        PRIntervalTime now     = PR_IntervalNow();
        PRIntervalTime timeout = PR_MillisecondsToInterval(mIdleThreadTimeout);

        
        if (mShutdown) {
          exitThread = PR_TRUE;
        } else {
          if (wasIdle) {
            
            if (mIdleCount > mIdleThreadLimit || (now - idleSince) >= timeout)
              exitThread = PR_TRUE;
          } else {
            
            if (mIdleCount == mIdleThreadLimit) {
              exitThread = PR_TRUE;
            } else {
              ++mIdleCount;
              idleSince = now;
              wasIdle = PR_TRUE;
            }
          }
        }

        if (exitThread) {
          if (wasIdle)
            --mIdleCount;
          shutdownThreadOnExit = mThreads.RemoveObject(current);
        } else {
          PRIntervalTime delta = timeout - (now - idleSince);
          LOG(("THRD-P(%p) waiting [%d]\n", this, delta));
          mon.Wait(delta);
        }
      } else if (wasIdle) {
        wasIdle = PR_FALSE;
        --mIdleCount;
      }
    }
    if (event) {
      LOG(("THRD-P(%p) running [%p]\n", this, event.get()));
      event->Run();
    }
  } while (!exitThread);

  if (shutdownThreadOnExit)
    ShutdownThread(current);

  LOG(("THRD-P(%p) leave\n", this));
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Dispatch(nsIRunnable *event, PRUint32 flags)
{
  LOG(("THRD-P(%p) dispatch [%p %x]\n", this, event, flags));

  NS_ENSURE_STATE(!mShutdown);

  if (flags & DISPATCH_SYNC) {
    nsCOMPtr<nsIThread> thread;
    nsThreadManager::get()->GetCurrentThread(getter_AddRefs(thread));
    NS_ENSURE_STATE(thread);

    nsRefPtr<nsThreadSyncDispatch> wrapper =
        new nsThreadSyncDispatch(thread, event);
    PutEvent(wrapper);

    while (wrapper->IsPending())
      NS_ProcessNextEvent(thread);
  } else {
    NS_ASSERTION(flags == NS_DISPATCH_NORMAL, "unexpected dispatch flags");
    PutEvent(event);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::IsOnCurrentThread(PRBool *result)
{
  
  
  NS_NOTREACHED("implement me");

  *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Shutdown()
{
  nsCOMArray<nsIThread> threads;
  {
    nsAutoMonitor mon(mEvents.Monitor());
    mShutdown = PR_TRUE;
    mon.NotifyAll();

    threads.AppendObjects(mThreads);
  }

  
  
  

  for (PRInt32 i = 0; i < threads.Count(); ++i)
    threads[i]->Shutdown();

  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetThreadLimit(PRUint32 *value)
{
  *value = mThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetThreadLimit(PRUint32 value)
{
  nsAutoMonitor mon(mEvents.Monitor());
  mThreadLimit = value;
  if (mIdleThreadLimit > mThreadLimit)
    mIdleThreadLimit = mThreadLimit;
  mon.NotifyAll();  
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadLimit(PRUint32 *value)
{
  *value = mIdleThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadLimit(PRUint32 value)
{
  nsAutoMonitor mon(mEvents.Monitor());
  mIdleThreadLimit = value;
  if (mIdleThreadLimit > mThreadLimit)
    mIdleThreadLimit = mThreadLimit;
  mon.NotifyAll();  
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadTimeout(PRUint32 *value)
{
  *value = mIdleThreadTimeout;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadTimeout(PRUint32 value)
{
  nsAutoMonitor mon(mEvents.Monitor());
  mIdleThreadTimeout = value;
  mon.NotifyAll();  
  return NS_OK;
}
