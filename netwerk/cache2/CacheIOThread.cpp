



#include "CacheIOThread.h"
#include "CacheFileIOManager.h"

#include "nsIRunnable.h"
#include "nsISupportsImpl.h"
#include "nsPrintfCString.h"
#include "nsThreadUtils.h"
#include "mozilla/VisualEventTracer.h"

namespace mozilla {
namespace net {

CacheIOThread* CacheIOThread::sSelf = nullptr;

NS_IMPL_ISUPPORTS1(CacheIOThread, nsIThreadObserver)

CacheIOThread::CacheIOThread()
: mMonitor("CacheIOThread")
, mThread(nullptr)
, mLowestLevelWaiting(LAST_LEVEL)
, mCurrentlyExecutingLevel(0)
, mHasXPCOMEvents(false)
, mRerunCurrentEvent(false)
, mShutdown(false)
{
  sSelf = this;
}

CacheIOThread::~CacheIOThread()
{
  sSelf = nullptr;
#ifdef DEBUG
  for (uint32_t level = 0; level < LAST_LEVEL; ++level) {
    MOZ_ASSERT(!mEventQueue[level].Length());
  }
#endif
}

nsresult CacheIOThread::Init()
{
  mThread = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD, 128 * 1024);
  if (!mThread)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult CacheIOThread::Dispatch(nsIRunnable* aRunnable, uint32_t aLevel)
{
  NS_ENSURE_ARG(aLevel < LAST_LEVEL);

  MonitorAutoLock lock(mMonitor);

  if (mShutdown && (PR_GetCurrentThread() != mThread))
    return NS_ERROR_UNEXPECTED;

  return DispatchInternal(aRunnable, aLevel);
}

nsresult CacheIOThread::DispatchAfterPendingOpens(nsIRunnable* aRunnable)
{
  MonitorAutoLock lock(mMonitor);

  if (mShutdown && (PR_GetCurrentThread() != mThread))
    return NS_ERROR_UNEXPECTED;

  
  
  mEventQueue[OPEN_PRIORITY].AppendElements(mEventQueue[OPEN]);
  mEventQueue[OPEN].Clear();

  return DispatchInternal(aRunnable, OPEN_PRIORITY);
}

nsresult CacheIOThread::DispatchInternal(nsIRunnable* aRunnable, uint32_t aLevel)
{
  mMonitor.AssertCurrentThreadOwns();

  mEventQueue[aLevel].AppendElement(aRunnable);
  if (mLowestLevelWaiting > aLevel)
    mLowestLevelWaiting = aLevel;

  mMonitor.NotifyAll();

  return NS_OK;
}

bool CacheIOThread::IsCurrentThread()
{
  return mThread == PR_GetCurrentThread();
}

bool CacheIOThread::YieldInternal()
{
  if (!IsCurrentThread()) {
    NS_WARNING("Trying to yield to priority events on non-cache2 I/O thread? "
               "You probably do something wrong.");
    return false;
  }

  if (mCurrentlyExecutingLevel == XPCOM_LEVEL) {
    
    
    return false;
  }

  if (!EventsPending(mCurrentlyExecutingLevel))
    return false;

  mRerunCurrentEvent = true;
  return true;
}

nsresult CacheIOThread::Shutdown()
{
  {
    MonitorAutoLock lock(mMonitor);
    mShutdown = true;
    mMonitor.NotifyAll();
  }

  PR_JoinThread(mThread);
  mThread = nullptr;

  return NS_OK;
}

already_AddRefed<nsIEventTarget> CacheIOThread::Target()
{
  nsCOMPtr<nsIEventTarget> target;

  target = mXPCOMThread;
  if (!target && mThread)
  {
    MonitorAutoLock lock(mMonitor);
    if (!mXPCOMThread)
      lock.Wait();

    target = mXPCOMThread;
  }

  return target.forget();
}


void CacheIOThread::ThreadFunc(void* aClosure)
{
  PR_SetCurrentThreadName("Cache2 I/O");
  CacheIOThread* thread = static_cast<CacheIOThread*>(aClosure);
  thread->ThreadFunc();
}

void CacheIOThread::ThreadFunc()
{
  nsCOMPtr<nsIThreadInternal> threadInternal;

  {
    MonitorAutoLock lock(mMonitor);

    
    nsCOMPtr<nsIThread> xpcomThread = NS_GetCurrentThread();

    threadInternal = do_QueryInterface(xpcomThread);
    if (threadInternal)
      threadInternal->SetObserver(this);

    mXPCOMThread.swap(xpcomThread);

    lock.NotifyAll();

    do {
loopStart:
      
      
      
      mLowestLevelWaiting = LAST_LEVEL;

      
      while (mHasXPCOMEvents) {
        eventtracer::AutoEventTracer tracer(this, eventtracer::eExec, eventtracer::eDone,
          "net::cache::io::level(xpcom)");

        mHasXPCOMEvents = false;
        mCurrentlyExecutingLevel = XPCOM_LEVEL;

        MonitorAutoUnlock unlock(mMonitor);

        bool processedEvent;
        nsresult rv;
        do {
          rv = mXPCOMThread->ProcessNextEvent(false, &processedEvent);
        } while (NS_SUCCEEDED(rv) && processedEvent);
      }

      uint32_t level;
      for (level = 0; level < LAST_LEVEL; ++level) {
        if (!mEventQueue[level].Length()) {
          
          continue;
        }

        LoopOneLevel(level);

        
        goto loopStart;
      }

      if (EventsPending())
        continue;

      if (mShutdown)
        break;

      lock.Wait(PR_INTERVAL_NO_TIMEOUT);

      if (EventsPending())
        continue;

    } while (true);

    MOZ_ASSERT(!EventsPending());
  } 

  if (threadInternal)
    threadInternal->SetObserver(nullptr);
}

static const char* const sLevelTraceName[] = {
  "net::cache::io::level(0)",
  "net::cache::io::level(1)",
  "net::cache::io::level(2)",
  "net::cache::io::level(3)",
  "net::cache::io::level(4)",
  "net::cache::io::level(5)",
  "net::cache::io::level(6)",
  "net::cache::io::level(7)",
  "net::cache::io::level(8)",
  "net::cache::io::level(9)",
  "net::cache::io::level(10)",
  "net::cache::io::level(11)",
  "net::cache::io::level(12)"
};

void CacheIOThread::LoopOneLevel(uint32_t aLevel)
{
  eventtracer::AutoEventTracer tracer(this, eventtracer::eExec, eventtracer::eDone,
    sLevelTraceName[aLevel]);

  nsTArray<nsRefPtr<nsIRunnable> > events;
  events.SwapElements(mEventQueue[aLevel]);
  uint32_t length = events.Length();

  mCurrentlyExecutingLevel = aLevel;

  bool returnEvents = false;
  uint32_t index;
  {
    MonitorAutoUnlock unlock(mMonitor);

    for (index = 0; index < length; ++index) {
      if (EventsPending(aLevel)) {
        
        
        returnEvents = true;
        break;
      }

      
      
      mRerunCurrentEvent = false;

      events[index]->Run();

      if (mRerunCurrentEvent) {
        
        returnEvents = true;
        break;
      }

      
      events[index] = nullptr;
    }
  }

  if (returnEvents)
    mEventQueue[aLevel].InsertElementsAt(0, events.Elements() + index, length - index);
}

bool CacheIOThread::EventsPending(uint32_t aLastLevel)
{
  return mLowestLevelWaiting < aLastLevel || mHasXPCOMEvents;
}

NS_IMETHODIMP CacheIOThread::OnDispatchedEvent(nsIThreadInternal *thread)
{
  MonitorAutoLock lock(mMonitor);
  mHasXPCOMEvents = true;
  MOZ_ASSERT(!mShutdown || (PR_GetCurrentThread() == mThread));
  lock.Notify();
  return NS_OK;
}

NS_IMETHODIMP CacheIOThread::OnProcessNextEvent(nsIThreadInternal *thread, bool mayWait, uint32_t recursionDepth)
{
  return NS_OK;
}

NS_IMETHODIMP CacheIOThread::AfterProcessNextEvent(nsIThreadInternal *thread, uint32_t recursionDepth,
                                                   bool eventWasProcessed)
{
  return NS_OK;
}



size_t CacheIOThread::SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  MonitorAutoLock lock(const_cast<CacheIOThread*>(this)->mMonitor);

  size_t n = 0;
  n += mallocSizeOf(mThread);
  for (uint32_t level = 0; level < LAST_LEVEL; ++level) {
    n += mEventQueue[level].SizeOfExcludingThis(mallocSizeOf);
    
    
    
  }

  return n;
}

size_t CacheIOThread::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  return mallocSizeOf(this) + SizeOfExcludingThis(mallocSizeOf);
}

} 
} 
