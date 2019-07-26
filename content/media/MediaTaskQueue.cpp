





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
MediaTaskQueue::Dispatch(nsIRunnable* aRunnable)
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

nsresult
MediaTaskQueue::Runner::Run()
{
  RefPtr<nsIRunnable> event;
  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    MOZ_ASSERT(mQueue->mIsRunning);
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

  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    if (mQueue->mTasks.size() == 0) {
      
      mQueue->mIsRunning = false;
      mon.NotifyAll();
      return NS_OK;
    }
  }

  
  
  
  
  
  nsresult rv = mQueue->mPool->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    mQueue->mIsRunning = false;
    mQueue->mIsShutdown = true;
    mon.NotifyAll();
  }

  return NS_OK;
}

} 
