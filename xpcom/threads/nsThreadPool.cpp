





#include "nsIClassInfoImpl.h"
#include "nsThreadPool.h"
#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"
#include "prinrval.h"
#include "prlog.h"

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetThreadPoolLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("nsThreadPool");
  }
  return sLog;
}
#endif
#ifdef LOG
#undef LOG
#endif
#define LOG(args) PR_LOG(GetThreadPoolLog(), PR_LOG_DEBUG, args)







#define DEFAULT_THREAD_LIMIT 4
#define DEFAULT_IDLE_THREAD_LIMIT 1
#define DEFAULT_IDLE_THREAD_TIMEOUT PR_SecondsToInterval(60)

NS_IMPL_ADDREF(nsThreadPool)
NS_IMPL_RELEASE(nsThreadPool)
NS_IMPL_CLASSINFO(nsThreadPool, nullptr, nsIClassInfo::THREADSAFE,
                  NS_THREADPOOL_CID)
NS_IMPL_QUERY_INTERFACE_CI(nsThreadPool, nsIThreadPool, nsIEventTarget,
                           nsIRunnable)
NS_IMPL_CI_INTERFACE_GETTER(nsThreadPool, nsIThreadPool, nsIEventTarget)

nsThreadPool::nsThreadPool()
  : mThreadLimit(DEFAULT_THREAD_LIMIT)
  , mIdleThreadLimit(DEFAULT_IDLE_THREAD_LIMIT)
  , mIdleThreadTimeout(DEFAULT_IDLE_THREAD_TIMEOUT)
  , mIdleCount(0)
  , mStackSize(nsIThreadManager::DEFAULT_STACK_SIZE)
  , mShutdown(false)
{
}

nsThreadPool::~nsThreadPool()
{
  
  
  MOZ_ASSERT(mThreads.IsEmpty());
}

nsresult
nsThreadPool::PutEvent(nsIRunnable* aEvent)
{
  

  bool spawnThread = false;
  uint32_t stackSize = 0;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());

    LOG(("THRD-P(%p) put [%d %d %d]\n", this, mIdleCount, mThreads.Count(),
         mThreadLimit));
    MOZ_ASSERT(mIdleCount <= (uint32_t)mThreads.Count(), "oops");

    
    if (mIdleCount == 0 && mThreads.Count() < (int32_t)mThreadLimit) {
      spawnThread = true;
    }

    mEvents.PutEvent(aEvent);
    stackSize = mStackSize;
  }

  LOG(("THRD-P(%p) put [spawn=%d]\n", this, spawnThread));
  if (!spawnThread) {
    return NS_OK;
  }

  nsCOMPtr<nsIThread> thread;
  nsThreadManager::get()->NewThread(0,
                                    stackSize,
                                    getter_AddRefs(thread));
  if (NS_WARN_IF(!thread)) {
    return NS_ERROR_UNEXPECTED;
  }

  bool killThread = false;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    if (mThreads.Count() < (int32_t)mThreadLimit) {
      mThreads.AppendObject(thread);
    } else {
      killThread = true;  
    }
  }
  LOG(("THRD-P(%p) put [%p kill=%d]\n", this, thread.get(), killThread));
  if (killThread) {
    
    
    
    
    

    nsRefPtr<nsIRunnable> r = NS_NewRunnableMethod(thread,
                                                   &nsIThread::Shutdown);
    NS_DispatchToCurrentThread(r);
  } else {
    thread->Dispatch(this, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

void
nsThreadPool::ShutdownThread(nsIThread* aThread)
{
  LOG(("THRD-P(%p) shutdown async [%p]\n", this, aThread));

  
  

  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");

  nsRefPtr<nsIRunnable> r = NS_NewRunnableMethod(aThread, &nsIThread::Shutdown);
  NS_DispatchToMainThread(r);
}

NS_IMETHODIMP
nsThreadPool::Run()
{
  LOG(("THRD-P(%p) enter\n", this));

  mThreadNaming.SetThreadPoolName(mName);

  nsCOMPtr<nsIThread> current;
  nsThreadManager::get()->GetCurrentThread(getter_AddRefs(current));

  bool shutdownThreadOnExit = false;
  bool exitThread = false;
  bool wasIdle = false;
  PRIntervalTime idleSince;

  nsCOMPtr<nsIThreadPoolListener> listener;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    listener = mListener;
  }

  if (listener) {
    listener->OnThreadCreated();
  }

  do {
    nsCOMPtr<nsIRunnable> event;
    {
      ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
      if (!mEvents.GetPendingEvent(getter_AddRefs(event))) {
        PRIntervalTime now     = PR_IntervalNow();
        PRIntervalTime timeout = PR_MillisecondsToInterval(mIdleThreadTimeout);

        
        if (mShutdown) {
          exitThread = true;
        } else {
          if (wasIdle) {
            
            if (mIdleCount > mIdleThreadLimit || (now - idleSince) >= timeout) {
              exitThread = true;
            }
          } else {
            
            if (mIdleCount == mIdleThreadLimit) {
              exitThread = true;
            } else {
              ++mIdleCount;
              idleSince = now;
              wasIdle = true;
            }
          }
        }

        if (exitThread) {
          if (wasIdle) {
            --mIdleCount;
          }
          shutdownThreadOnExit = mThreads.RemoveObject(current);
        } else {
          PRIntervalTime delta = timeout - (now - idleSince);
          LOG(("THRD-P(%p) waiting [%d]\n", this, delta));
          mon.Wait(delta);
        }
      } else if (wasIdle) {
        wasIdle = false;
        --mIdleCount;
      }
    }
    if (event) {
      LOG(("THRD-P(%p) running [%p]\n", this, event.get()));
      event->Run();
    }
  } while (!exitThread);

  if (listener) {
    listener->OnThreadShuttingDown();
  }

  if (shutdownThreadOnExit) {
    ShutdownThread(current);
  }

  LOG(("THRD-P(%p) leave\n", this));
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Dispatch(nsIRunnable* aEvent, uint32_t aFlags)
{
  LOG(("THRD-P(%p) dispatch [%p %x]\n", this, aEvent, aFlags));

  if (NS_WARN_IF(mShutdown)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (aFlags & DISPATCH_SYNC) {
    nsCOMPtr<nsIThread> thread;
    nsThreadManager::get()->GetCurrentThread(getter_AddRefs(thread));
    if (NS_WARN_IF(!thread)) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    nsRefPtr<nsThreadSyncDispatch> wrapper =
      new nsThreadSyncDispatch(thread, aEvent);
    PutEvent(wrapper);

    while (wrapper->IsPending()) {
      NS_ProcessNextEvent(thread);
    }
  } else {
    NS_ASSERTION(aFlags == NS_DISPATCH_NORMAL, "unexpected dispatch flags");
    PutEvent(aEvent);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::IsOnCurrentThread(bool* aResult)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  nsIThread* thread = NS_GetCurrentThread();
  for (uint32_t i = 0; i < static_cast<uint32_t>(mThreads.Count()); ++i) {
    if (mThreads[i] == thread) {
      *aResult = true;
      return NS_OK;
    }
  }
  *aResult = false;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Shutdown()
{
  nsCOMArray<nsIThread> threads;
  nsCOMPtr<nsIThreadPoolListener> listener;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    mShutdown = true;
    mon.NotifyAll();

    threads.AppendObjects(mThreads);
    mThreads.Clear();

    
    
    
    mListener.swap(listener);
  }

  
  

  for (int32_t i = 0; i < threads.Count(); ++i) {
    threads[i]->Shutdown();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetThreadLimit(uint32_t* aValue)
{
  *aValue = mThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetThreadLimit(uint32_t aValue)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  mThreadLimit = aValue;
  if (mIdleThreadLimit > mThreadLimit) {
    mIdleThreadLimit = mThreadLimit;
  }

  if (static_cast<uint32_t>(mThreads.Count()) > mThreadLimit) {
    mon.NotifyAll();  
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadLimit(uint32_t* aValue)
{
  *aValue = mIdleThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadLimit(uint32_t aValue)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  mIdleThreadLimit = aValue;
  if (mIdleThreadLimit > mThreadLimit) {
    mIdleThreadLimit = mThreadLimit;
  }

  
  if (mIdleCount > mIdleThreadLimit) {
    mon.NotifyAll();  
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadTimeout(uint32_t* aValue)
{
  *aValue = mIdleThreadTimeout;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadTimeout(uint32_t aValue)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  uint32_t oldTimeout = mIdleThreadTimeout;
  mIdleThreadTimeout = aValue;

  
  if (mIdleThreadTimeout < oldTimeout && mIdleCount > 0) {
    mon.NotifyAll();  
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetThreadStackSize(uint32_t* aValue)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  *aValue = mStackSize;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetThreadStackSize(uint32_t aValue)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  mStackSize = aValue;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetListener(nsIThreadPoolListener** aListener)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  NS_IF_ADDREF(*aListener = mListener);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetListener(nsIThreadPoolListener* aListener)
{
  nsCOMPtr<nsIThreadPoolListener> swappedListener(aListener);
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    mListener.swap(swappedListener);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetName(const nsACString& aName)
{
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    if (mThreads.Count()) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  mName = aName;
  return NS_OK;
}
