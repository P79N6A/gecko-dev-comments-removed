





#ifndef TaskQueue_h_
#define TaskQueue_h_

#include <queue>
#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"
#include "mozilla/unused.h"
#include "SharedThreadPool.h"
#include "nsThreadUtils.h"
#include "MozPromise.h"
#include "TaskDispatcher.h"

class nsIRunnable;

namespace mozilla {

class SharedThreadPool;

typedef MozPromise<bool, bool, false> ShutdownPromise;






class TaskQueue : public AbstractThread {
public:
  explicit TaskQueue(already_AddRefed<SharedThreadPool> aPool, bool aSupportsTailDispatch = false);

  TaskDispatcher& TailDispatcher() override;

  TaskQueue* AsTaskQueue() override { return this; }

  void Dispatch(already_AddRefed<nsIRunnable> aRunnable,
                DispatchFailureHandling aFailureHandling = AssertDispatchSuccess,
                DispatchReason aReason = NormalDispatch) override
  {
    MonitorAutoLock mon(mQueueMonitor);
    nsresult rv = DispatchLocked(Move(aRunnable), AbortIfFlushing, aFailureHandling, aReason);
    MOZ_DIAGNOSTIC_ASSERT(aFailureHandling == DontAssertDispatchSuccess || NS_SUCCEEDED(rv));
    unused << rv;
  }

  
  
  void SyncDispatch(already_AddRefed<nsIRunnable> aRunnable);

  
  
  
  
  
  
  nsRefPtr<ShutdownPromise> BeginShutdown();

  
  void AwaitIdle();

  
  
  void AwaitShutdownAndIdle();

  bool IsEmpty();

  
  
  bool IsCurrentThreadIn() override;

protected:
  virtual ~TaskQueue();


  
  
  
  void AwaitIdleLocked();

  enum DispatchMode { AbortIfFlushing, IgnoreFlushing };

  nsresult DispatchLocked(already_AddRefed<nsIRunnable> aRunnable, DispatchMode aMode,
                          DispatchFailureHandling aFailureHandling,
                          DispatchReason aReason = NormalDispatch);

  void MaybeResolveShutdown()
  {
    mQueueMonitor.AssertCurrentThreadOwns();
    if (mIsShutdown && !mIsRunning) {
      mShutdownPromise.ResolveIfExists(true, __func__);
      mPool = nullptr;
    }
  }

  RefPtr<SharedThreadPool> mPool;

  
  Monitor mQueueMonitor;

  
  std::queue<nsCOMPtr<nsIRunnable>> mTasks;

  
  
  
  
  
  
  
  
  Atomic<nsIThread*> mRunningThread;

  
  class AutoTaskGuard : public AutoTaskDispatcher
  {
  public:
    explicit AutoTaskGuard(TaskQueue* aQueue)
      : AutoTaskDispatcher( true), mQueue(aQueue)
    {
      
      
      MOZ_ASSERT(!mQueue->mTailDispatcher);
      mQueue->mTailDispatcher = this;

      MOZ_ASSERT(sCurrentThreadTLS.get() == nullptr);
      sCurrentThreadTLS.set(aQueue);

      MOZ_ASSERT(mQueue->mRunningThread == nullptr);
      mQueue->mRunningThread = NS_GetCurrentThread();
    }

    ~AutoTaskGuard()
    {
      DrainDirectTasks();

      MOZ_ASSERT(mQueue->mRunningThread == NS_GetCurrentThread());
      mQueue->mRunningThread = nullptr;

      sCurrentThreadTLS.set(nullptr);
      mQueue->mTailDispatcher = nullptr;
    }

  private:
  TaskQueue* mQueue;
  };

  TaskDispatcher* mTailDispatcher;

  
  
  bool mIsRunning;

  
  bool mIsShutdown;
  MozPromiseHolder<ShutdownPromise> mShutdownPromise;

  
  bool mIsFlushing;

  class Runner : public nsRunnable {
  public:
    explicit Runner(TaskQueue* aQueue)
      : mQueue(aQueue)
    {
    }
    NS_METHOD Run() override;
  private:
    RefPtr<TaskQueue> mQueue;
  };
};

class FlushableTaskQueue : public TaskQueue
{
public:
  explicit FlushableTaskQueue(already_AddRefed<SharedThreadPool> aPool) : TaskQueue(Move(aPool)) {}
  nsresult FlushAndDispatch(already_AddRefed<nsIRunnable> aRunnable);
  void Flush();

  bool IsDispatchReliable() override { return false; }

private:

  class MOZ_STACK_CLASS AutoSetFlushing
  {
  public:
    explicit AutoSetFlushing(FlushableTaskQueue* aTaskQueue) : mTaskQueue(aTaskQueue)
    {
      mTaskQueue->mQueueMonitor.AssertCurrentThreadOwns();
      mTaskQueue->mIsFlushing = true;
    }
    ~AutoSetFlushing()
    {
      mTaskQueue->mQueueMonitor.AssertCurrentThreadOwns();
      mTaskQueue->mIsFlushing = false;
    }

  private:
    FlushableTaskQueue* mTaskQueue;
  };

  void FlushLocked();

};

} 

#endif 
