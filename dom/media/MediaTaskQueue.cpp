





#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"
#include "SharedThreadPool.h"

namespace mozilla {

MediaTaskQueue::MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool,
                               bool aRequireTailDispatch)
  : AbstractThread(aRequireTailDispatch)
  , mPool(aPool)
  , mQueueMonitor("MediaTaskQueue::Queue")
  , mTailDispatcher(nullptr)
  , mIsRunning(false)
  , mIsShutdown(false)
  , mIsFlushing(false)
{
  MOZ_COUNT_CTOR(MediaTaskQueue);
}

MediaTaskQueue::~MediaTaskQueue()
{
  MonitorAutoLock mon(mQueueMonitor);
  MOZ_ASSERT(mIsShutdown);
  MOZ_COUNT_DTOR(MediaTaskQueue);
}

TaskDispatcher&
MediaTaskQueue::TailDispatcher()
{
  MOZ_ASSERT(IsCurrentThreadIn());
  MOZ_ASSERT(mTailDispatcher);
  return *mTailDispatcher;
}

nsresult
MediaTaskQueue::DispatchLocked(already_AddRefed<nsIRunnable> aRunnable,
                               DispatchMode aMode, DispatchFailureHandling aFailureHandling,
                               DispatchReason aReason)
{
  nsCOMPtr<nsIRunnable> r = aRunnable;
  AbstractThread* currentThread;
  if (aReason != TailDispatch && (currentThread = GetCurrent()) && currentThread->RequiresTailDispatch()) {
    currentThread->TailDispatcher().AddTask(this, r.forget(), aFailureHandling);
    return NS_OK;
  }

  mQueueMonitor.AssertCurrentThreadOwns();
  if (mIsFlushing && aMode == AbortIfFlushing) {
    return NS_ERROR_ABORT;
  }
  if (mIsShutdown) {
    return NS_ERROR_FAILURE;
  }
  mTasks.push(r.forget());
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

  void WaitUntilDone() {
    MonitorAutoLock mon(mMonitor);
    while (!mDone) {
      mon.Wait();
    }
  }
private:
  RefPtr<nsIRunnable> mRunnable;
  Monitor mMonitor;
  bool mDone;
};

void
MediaTaskQueue::SyncDispatch(TemporaryRef<nsIRunnable> aRunnable) {
  NS_WARNING("MediaTaskQueue::SyncDispatch is dangerous and deprecated. Stop using this!");
  nsRefPtr<MediaTaskQueueSyncRunnable> task(new MediaTaskQueueSyncRunnable(aRunnable));

  
  
  
  MOZ_ASSERT_IF(AbstractThread::GetCurrent(),
                !AbstractThread::GetCurrent()->TailDispatcher().HasTasksFor(this));
  nsRefPtr<MediaTaskQueueSyncRunnable> taskRef = task;
  Dispatch(taskRef.forget(), AssertDispatchSuccess, TailDispatch);

  task->WaitUntilDone();
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
  
  
  MOZ_ASSERT_IF(AbstractThread::GetCurrent(),
                !AbstractThread::GetCurrent()->TailDispatcher().HasTasksFor(this));

  mQueueMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(mIsRunning || mTasks.empty());
  while (mIsRunning) {
    mQueueMonitor.Wait();
  }
}

void
MediaTaskQueue::AwaitShutdownAndIdle()
{
  
  
  MOZ_ASSERT_IF(AbstractThread::GetCurrent(),
                !AbstractThread::GetCurrent()->TailDispatcher().HasTasksFor(this));

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
  MonitorAutoLock mon(mQueueMonitor);
  AutoSetFlushing autoFlush(this);
  FlushLocked();
  nsCOMPtr<nsIRunnable> r = dont_AddRef(aRunnable.take());
  nsresult rv = DispatchLocked(r.forget(), IgnoreFlushing, AssertDispatchSuccess);
  NS_ENSURE_SUCCESS(rv, rv);
  AwaitIdleLocked();
  return NS_OK;
}

void
FlushableMediaTaskQueue::FlushLocked()
{
  
  
  MOZ_ASSERT_IF(AbstractThread::GetCurrent(),
                !AbstractThread::GetCurrent()->TailDispatcher().HasTasksFor(this));

  mQueueMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(mIsFlushing);

  
  
  while (!mTasks.empty()) {
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
  bool in = NS_GetCurrentThread() == mRunningThread;
  MOZ_ASSERT(in == (GetCurrent() == this));
  return in;
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
      mQueue->mShutdownPromise.ResolveIfExists(true, __func__);
      mon.NotifyAll();
      return NS_OK;
    }
    event = mQueue->mTasks.front();
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
