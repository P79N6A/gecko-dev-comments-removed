





#include "mozilla/TaskQueue.h"

#include "nsThreadUtils.h"
#include "SharedThreadPool.h"

namespace mozilla {

TaskQueue::TaskQueue(already_AddRefed<SharedThreadPool> aPool,
                               bool aRequireTailDispatch)
  : AbstractThread(aRequireTailDispatch)
  , mPool(aPool)
  , mQueueMonitor("TaskQueue::Queue")
  , mTailDispatcher(nullptr)
  , mIsRunning(false)
  , mIsShutdown(false)
  , mIsFlushing(false)
{
  MOZ_COUNT_CTOR(TaskQueue);
}

TaskQueue::~TaskQueue()
{
  MonitorAutoLock mon(mQueueMonitor);
  MOZ_ASSERT(mIsShutdown);
  MOZ_COUNT_DTOR(TaskQueue);
}

TaskDispatcher&
TaskQueue::TailDispatcher()
{
  MOZ_ASSERT(IsCurrentThreadIn());
  MOZ_ASSERT(mTailDispatcher);
  return *mTailDispatcher;
}

nsresult
TaskQueue::DispatchLocked(already_AddRefed<nsIRunnable> aRunnable,
                               DispatchMode aMode, DispatchFailureHandling aFailureHandling,
                               DispatchReason aReason)
{
  nsCOMPtr<nsIRunnable> r = aRunnable;
  AbstractThread* currentThread;
  if (aReason != TailDispatch && (currentThread = GetCurrent()) && RequiresTailDispatch(currentThread)) {
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
    NS_WARNING("Failed to dispatch runnable to run TaskQueue");
    return rv;
  }
  mIsRunning = true;

  return NS_OK;
}

class TaskQueueSyncRunnable : public nsRunnable {
public:
  explicit TaskQueueSyncRunnable(already_AddRefed<nsIRunnable> aRunnable)
    : mRunnable(aRunnable)
    , mMonitor("TaskQueueSyncRunnable")
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
TaskQueue::SyncDispatch(already_AddRefed<nsIRunnable> aRunnable) {
  NS_WARNING("TaskQueue::SyncDispatch is dangerous and deprecated. Stop using this!");
  nsRefPtr<TaskQueueSyncRunnable> task(new TaskQueueSyncRunnable(Move(aRunnable)));

  
  
  
  MOZ_ASSERT_IF(AbstractThread::GetCurrent(),
                !AbstractThread::GetCurrent()->TailDispatcher().HasTasksFor(this));
  nsRefPtr<TaskQueueSyncRunnable> taskRef = task;
  Dispatch(taskRef.forget(), AssertDispatchSuccess, TailDispatch);

  task->WaitUntilDone();
}

void
TaskQueue::AwaitIdle()
{
  MonitorAutoLock mon(mQueueMonitor);
  AwaitIdleLocked();
}

void
TaskQueue::AwaitIdleLocked()
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
TaskQueue::AwaitShutdownAndIdle()
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
TaskQueue::BeginShutdown()
{
  
  
  if (AbstractThread* currentThread = AbstractThread::GetCurrent()) {
    currentThread->TailDispatcher().DispatchTasksFor(this);
  }

  MonitorAutoLock mon(mQueueMonitor);
  mIsShutdown = true;
  nsRefPtr<ShutdownPromise> p = mShutdownPromise.Ensure(__func__);
  MaybeResolveShutdown();
  mon.NotifyAll();
  return p;
}

void
FlushableTaskQueue::Flush()
{
  MonitorAutoLock mon(mQueueMonitor);
  AutoSetFlushing autoFlush(this);
  FlushLocked();
  AwaitIdleLocked();
}

nsresult
FlushableTaskQueue::FlushAndDispatch(already_AddRefed<nsIRunnable> aRunnable)
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
FlushableTaskQueue::FlushLocked()
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
TaskQueue::IsEmpty()
{
  MonitorAutoLock mon(mQueueMonitor);
  return mTasks.empty();
}

bool
TaskQueue::IsCurrentThreadIn()
{
  bool in = NS_GetCurrentThread() == mRunningThread;
  MOZ_ASSERT(in == (GetCurrent() == this));
  return in;
}

nsresult
TaskQueue::Runner::Run()
{
  RefPtr<nsIRunnable> event;
  {
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    MOZ_ASSERT(mQueue->mIsRunning);
    if (mQueue->mTasks.size() == 0) {
      mQueue->mIsRunning = false;
      mQueue->MaybeResolveShutdown();
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
      mQueue->MaybeResolveShutdown();
      mon.NotifyAll();
      return NS_OK;
    }
  }

  
  
  
  
  
  nsresult rv = mQueue->mPool->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    
    MonitorAutoLock mon(mQueue->mQueueMonitor);
    mQueue->mIsRunning = false;
    mQueue->mIsShutdown = true;
    mQueue->MaybeResolveShutdown();
    mon.NotifyAll();
  }

  return NS_OK;
}

} 
