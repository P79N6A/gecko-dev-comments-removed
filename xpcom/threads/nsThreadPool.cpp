





#include "nsIClassInfoImpl.h"
#include "nsThreadPool.h"
#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"
#include "prinrval.h"
#include "prlog.h"
#include "mozilla/DebugOnly.h"

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo *
GetThreadPoolLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("nsThreadPool");
  return sLog;
}
#endif
#define LOG(args) PR_LOG(GetThreadPoolLog(), PR_LOG_DEBUG, args)







#define DEFAULT_THREAD_LIMIT 4
#define DEFAULT_IDLE_THREAD_LIMIT 1
#define DEFAULT_IDLE_THREAD_TIMEOUT PR_SecondsToInterval(60)

NS_IMPL_ADDREF(nsThreadPool)
NS_IMPL_RELEASE(nsThreadPool)
NS_IMPL_CLASSINFO(nsThreadPool, nullptr, nsIClassInfo::THREADSAFE,
                  NS_THREADPOOL_CID)
NS_IMPL_QUERY_INTERFACE2_CI(nsThreadPool, nsIThreadPool, nsIEventTarget)
NS_IMPL_CI_INTERFACE_GETTER2(nsThreadPool, nsIThreadPool, nsIEventTarget)

nsThreadPool::nsThreadPool()
  : mThreadLimit(DEFAULT_THREAD_LIMIT)
  , mIdleThreadLimit(DEFAULT_IDLE_THREAD_LIMIT)
  , mIdleThreadTimeout(DEFAULT_IDLE_THREAD_TIMEOUT)
  , mIdleCount(0)
  , mShutdown(false)
{
}

nsThreadPool::~nsThreadPool()
{
  
  
  MOZ_ASSERT(mThreads.IsEmpty());
}

nsresult
nsThreadPool::PutEvent(nsIRunnable *event)
{
  
 
  bool spawnThread = false;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());

    LOG(("THRD-P(%p) put [%d %u %d]\n", this, mIdleCount, mThreads.Length(),
         mThreadLimit));
    MOZ_ASSERT(mIdleCount <= mThreads.Length(), "oops");

    
    if (mIdleCount == 0 && mThreads.Length() < mThreadLimit) {
      
      mThreads.AppendElement<PRThread*>(nullptr);
      spawnThread = true;
    } else {
      mEvents.PutEvent(event);
    }
  }

  LOG(("THRD-P(%p) put [spawn=%d]\n", this, spawnThread));
  if (!spawnThread)
    return NS_OK;

  NS_ADDREF_THIS(); 
  PRThread* thread = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                                     PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                     PR_JOINABLE_THREAD, 0);

  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    if (!thread) {
      NS_WARNING("PR_CreateThread() failed");
      NS_RELEASE_THIS();
      mThreads.RemoveElement<PRThread*>(nullptr);
      if (mThreads.IsEmpty())
        return NS_ERROR_OUT_OF_MEMORY;
    }

    mEvents.PutEvent(event);
  }

  return NS_OK;
}

void
nsThreadPool::ShutdownThread(PRThread *thread)
{
  LOG(("THRD-P(%p) shutdown async [%p]\n", this, thread));

  
  

  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");

  class JoinEvent MOZ_FINAL : public nsRunnable
  {
  public:
    explicit JoinEvent(PRThread *thread) : mThread(thread) { }

    NS_IMETHODIMP Run() MOZ_OVERRIDE
    {
      DebugOnly<PRStatus> status = PR_JoinThread(mThread);
      MOZ_ASSERT(status == PR_SUCCESS, "PR_JoinThread failed");
      return NS_OK;
    }

  private:
    PRThread *mThread;
  };

  nsRefPtr<nsIRunnable> r = new JoinEvent(thread);
  NS_DispatchToMainThread(r);
}

 void
nsThreadPool::ThreadFunc(void *arg)
{
  auto self = static_cast<nsThreadPool*>(arg);  
  self->Run();
  NS_RELEASE(self); 
}

void
nsThreadPool::Run()
{
  LOG(("THRD-P(%p) enter\n", this));

  mThreadNaming.SetThreadPoolName(mName);

  PRThread* current = PR_GetCurrentThread();

  nsCOMPtr<nsIThreadPoolListener> listener;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());

    auto index = mThreads.IndexOf<PRThread*>(nullptr);
    MOZ_ASSERT(index != mThreads.NoIndex, "mThreads entry has gone!");
    mThreads[index] = current;

    listener = mListener;
  }

  if (listener) {
    listener->OnThreadCreated();
  }

  bool shutdownThreadOnExit = false;
  bool exitThread = false;
  bool wasIdle = false;
  PRIntervalTime idleSince;

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
            
            if (mIdleCount > mIdleThreadLimit || (now - idleSince) >= timeout)
              exitThread = true;
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
          if (wasIdle)
            --mIdleCount;
          shutdownThreadOnExit = mThreads.RemoveElement(current);
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
  return;
}

NS_IMETHODIMP
nsThreadPool::Dispatch(nsIRunnable *event, uint32_t flags)
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
nsThreadPool::IsOnCurrentThread(bool *result)
{
  
  
  NS_NOTREACHED("implement me");

  *result = false;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Shutdown()
{
  nsTArray<PRThread*> threads;
  nsCOMPtr<nsIThreadPoolListener> listener;
  {
    ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
    mShutdown = true;
    mon.NotifyAll();

    
    
    
    mListener.swap(listener);

    while (mThreads.Length()) {
      
      
      ReentrantMonitorAutoExit mon(mEvents.GetReentrantMonitor());
      
      NS_ProcessNextEvent();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetThreadLimit(uint32_t *value)
{
  *value = mThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetThreadLimit(uint32_t value)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  mThreadLimit = value;
  if (mIdleThreadLimit > mThreadLimit)
    mIdleThreadLimit = mThreadLimit;

  if (mThreads.Length() > mThreadLimit) {
    mon.NotifyAll();  
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadLimit(uint32_t *value)
{
  *value = mIdleThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadLimit(uint32_t value)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  mIdleThreadLimit = value;
  if (mIdleThreadLimit > mThreadLimit)
    mIdleThreadLimit = mThreadLimit;

  
  if (mIdleCount > mIdleThreadLimit) {
    mon.NotifyAll();  
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadTimeout(uint32_t *value)
{
  *value = mIdleThreadTimeout;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadTimeout(uint32_t value)
{
  ReentrantMonitorAutoEnter mon(mEvents.GetReentrantMonitor());
  uint32_t oldTimeout = mIdleThreadTimeout;
  mIdleThreadTimeout = value;

  
  if (mIdleThreadTimeout < oldTimeout && mIdleCount > 0) {
    mon.NotifyAll();  
  }
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
    if (mThreads.Length())
      return NS_ERROR_NOT_AVAILABLE;
  }

  mName = aName;
  return NS_OK;
}
