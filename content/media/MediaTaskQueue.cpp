





#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"
#include "SharedThreadPool.h"

namespace mozilla {

MediaTaskQueue::MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool)
  : mPool(aPool)
  , mQueueMonitor("MediaTaskQueue::Queue")
  , mIsRunning(false)
  , mIsShutdown(false)
{
  MOZ_COUNT_CTOR(MediaTaskQueue);
}

MediaTaskQueue::~MediaTaskQueue()
{
  MonitorAutoLock mon(mQueueMonitor);
  MOZ_ASSERT(mIsShutdown);
  MOZ_COUNT_DTOR(MediaTaskQueue);
}

nsresult
MediaTaskQueue::Dispatch(TemporaryRef<nsIRunnable> aRunnable)
{
  MonitorAutoLock mon(mQueueMonitor);
  if (mIsShutdown) {
    return NS_ERROR_FAILURE;
  }
  mTasks.push(aRunnable);
  if (mIsRunning) {
    return NS_OK;
  }
  RefPtr<nsIRunnable> runner(new Runner(this));
  nsresult rv = mPool->Dispatch(runner, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch runnable to run MediaTaskQueue");
    return rv;
  }
  mIsRunning = true;

  return NS_OK;
}

class MediaTaskQueueSyncRunnable : public nsRunnable {
public:
  MediaTaskQueueSyncRunnable(TemporaryRef<nsIRunnable> aRunnable)
    : mRunnable(aRunnable)
    , mMonitor("MediaTaskQueueSyncRunnable")
    , mDone(false)
  {
  }

  NS_IMETHOD Run() {
    nsresult rv = mRunnable->Run();
    {
      MonitorAutoLock mon(mMonitor);
      mDone = true;
      mon.NotifyAll();
    }
    return rv;
  }

  nsresult WaitUntilDone() {
    MonitorAutoLock mon(mMonitor);
    while (!mDone) {
      mon.Wait();
    }
    return NS_OK;
  }
private:
  RefPtr<nsIRunnable> mRunnable;
  Monitor mMonitor;
  bool mDone;
};

nsresult
MediaTaskQueue::SyncDispatch(TemporaryRef<nsIRunnable> aRunnable) {
  RefPtr<MediaTaskQueueSyncRunnable> task(new MediaTaskQueueSyncRunnable(aRunnable));
  nsresult rv = Dispatch(task);
  NS_ENSURE_SUCCESS(rv, rv);
  return task->WaitUntilDone();
}

void
MediaTaskQueue::AwaitIdle()
{
  MonitorAutoLock mon(mQueueMonitor);
  AwaitIdleLocked();
}

void
MediaTaskQueue::AwaitIdleLocked()
{
  mQueueMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(mIsRunning || mTasks.empty());
  while (mIsRunning) {
    mQueueMonitor.Wait();
  }
}

void
MediaTaskQueue::Shutdown()
{
  MonitorAutoLock mon(mQueueMonitor);
  mIsShutdown = true;
  AwaitIdleLocked();
}

void
MediaTaskQueue::Flush()
{
  MonitorAutoLock mon(mQueueMonitor);
  while (!mTasks.empty()) {
    mTasks.pop();
  }
  AwaitIdleLocked();
}

bool
MediaTaskQueue::IsEmpty()
{
  MonitorAutoLock mon(mQueueMonitor);
  return mTasks.empty();
}

bool
MediaTaskQueue::IsCurrentThreadIn()
{
#ifdef DEBUG
  MonitorAutoLock mon(mQueueMonitor);
  return NS_GetCurrentThread() == mRunningThread;
#else
  return false;
#endif
}

nsresult
MediaTaskQueue::Runner::Run()
{
  RefPtr<nsIRunnable> event;
  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    MOZ_ASSERT(mQueue->mIsRunning);
    mQueue->mRunningThread = NS_GetCurrentThread();
    if (mQueue->mTasks.size() == 0) {
      mQueue->mIsRunning = false;
      mon.NotifyAll();
      return NS_OK;
    }
    event = mQueue->mTasks.front();
    mQueue->mTasks.pop();
  }
  MOZ_ASSERT(event);

  
  
  
  
  
  event->Run();

  
  
  
  
  
  event = nullptr;

  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    if (mQueue->mTasks.size() == 0) {
      
      mQueue->mIsRunning = false;
      mon.NotifyAll();
      mQueue->mRunningThread = nullptr;
      return NS_OK;
    }
  }

  
  
  
  
  
  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    
    
    
    
    nsresult rv = mQueue->mPool->Dispatch(this, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      
      mQueue->mIsRunning = false;
      mQueue->mIsShutdown = true;
      mon.NotifyAll();
    }
    mQueue->mRunningThread = nullptr;
  }

  return NS_OK;
}

} 
