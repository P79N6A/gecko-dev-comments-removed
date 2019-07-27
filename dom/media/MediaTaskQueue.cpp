





#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"
#include "SharedThreadPool.h"

namespace mozilla {

ThreadLocal<MediaTaskQueue*> MediaTaskQueue::sCurrentQueueTLS;

 void
MediaTaskQueue::InitStatics()
{
  if (!sCurrentQueueTLS.init()) {
    MOZ_CRASH();
  }
}

MediaTaskQueue::MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool,
                               bool aRequireTailDispatch)
  : mPool(aPool)
  , mQueueMonitor("MediaTaskQueue::Queue")
  , mTailDispatcher(nullptr)
  , mIsRunning(false)
  , mIsShutdown(false)
  , mIsFlushing(false)
  , mRequireTailDispatch(aRequireTailDispatch)
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
  AssertInTailDispatchIfNeeded(); 
  MonitorAutoLock mon(mQueueMonitor);
  return DispatchLocked(aRunnable, AbortIfFlushing);
}

TaskDispatcher&
MediaTaskQueue::TailDispatcher()
{
  MOZ_ASSERT(IsCurrentThreadIn());
  MOZ_ASSERT(mTailDispatcher);
  return *mTailDispatcher;
}

nsresult
MediaTaskQueue::ForceDispatch(TemporaryRef<nsIRunnable> aRunnable)
{
  AssertInTailDispatchIfNeeded(); 
  MonitorAutoLock mon(mQueueMonitor);
  return DispatchLocked(aRunnable, Forced);
}

nsresult
MediaTaskQueue::DispatchLocked(TemporaryRef<nsIRunnable> aRunnable,
                               DispatchMode aMode)
{
  mQueueMonitor.AssertCurrentThreadOwns();
  if (mIsFlushing && aMode == AbortIfFlushing) {
    return NS_ERROR_ABORT;
  }
  if (mIsShutdown) {
    return NS_ERROR_FAILURE;
  }
  mTasks.push(TaskQueueEntry(aRunnable, aMode == Forced));
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
  explicit MediaTaskQueueSyncRunnable(TemporaryRef<nsIRunnable> aRunnable)
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
MediaTaskQueue::AwaitShutdownAndIdle()
{
  MonitorAutoLock mon(mQueueMonitor);
  while (!mIsShutdown) {
    mQueueMonitor.Wait();
  }
  AwaitIdleLocked();
}

nsRefPtr<ShutdownPromise>
MediaTaskQueue::BeginShutdown()
{
  MonitorAutoLock mon(mQueueMonitor);
  mIsShutdown = true;
  nsRefPtr<ShutdownPromise> p = mShutdownPromise.Ensure(__func__);
  if (!mIsRunning) {
    mShutdownPromise.Resolve(true, __func__);
  }
  mon.NotifyAll();
  return p;
}

void
FlushableMediaTaskQueue::Flush()
{
  MonitorAutoLock mon(mQueueMonitor);
  AutoSetFlushing autoFlush(this);
  FlushLocked();
  AwaitIdleLocked();
}

nsresult
FlushableMediaTaskQueue::FlushAndDispatch(TemporaryRef<nsIRunnable> aRunnable)
{
  AssertInTailDispatchIfNeeded(); 
  MonitorAutoLock mon(mQueueMonitor);
  AutoSetFlushing autoFlush(this);
  FlushLocked();
  nsresult rv = DispatchLocked(aRunnable, IgnoreFlushing);
  NS_ENSURE_SUCCESS(rv, rv);
  AwaitIdleLocked();
  return NS_OK;
}

void
FlushableMediaTaskQueue::FlushLocked()
{
  mQueueMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(mIsFlushing);

  
  
  size_t numTasks = mTasks.size();
  for (size_t i = 0; i < numTasks; ++i) {
    if (mTasks.front().mForceDispatch) {
      mTasks.push(mTasks.front());
    }
    mTasks.pop();
  }
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
  MonitorAutoLock mon(mQueueMonitor);
  bool in = NS_GetCurrentThread() == mRunningThread;
  MOZ_ASSERT_IF(in, GetCurrentQueue() == this);
  return in;
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
      mQueue->mShutdownPromise.ResolveIfExists(true, __func__);
      mon.NotifyAll();
      return NS_OK;
    }
    event = mQueue->mTasks.front().mRunnable;
    mQueue->mTasks.pop();
  }
  MOZ_ASSERT(event);

  
  
  
  
  
  {
    AutoTaskGuard g(mQueue);
    event->Run();
  }

  
  
  
  
  
  event = nullptr;

  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    if (mQueue->mTasks.size() == 0) {
      
      mQueue->mIsRunning = false;
      mQueue->mShutdownPromise.ResolveIfExists(true, __func__);
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

#ifdef DEBUG
void
TaskDispatcher::AssertIsTailDispatcherIfRequired()
{
  MediaTaskQueue* currentQueue = MediaTaskQueue::GetCurrentQueue();

  
  
  
  
  MOZ_ASSERT_IF(currentQueue && currentQueue->RequiresTailDispatch(),
                this == currentQueue->mTailDispatcher);
}
#endif

} 
