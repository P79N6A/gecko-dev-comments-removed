





#ifndef MediaTaskQueue_h_
#define MediaTaskQueue_h_

#include <queue>
#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"
#include "mozilla/ThreadLocal.h"
#include "SharedThreadPool.h"
#include "nsThreadUtils.h"
#include "MediaPromise.h"
#include "TaskDispatcher.h"

class nsIRunnable;

namespace mozilla {

class SharedThreadPool;

typedef MediaPromise<bool, bool, false> ShutdownPromise;






class MediaTaskQueue : public AbstractThread {
  static ThreadLocal<MediaTaskQueue*> sCurrentQueueTLS;
public:
  static void InitStatics();

  
  
  static MediaTaskQueue* GetCurrentQueue() { return sCurrentQueueTLS.get(); }

  explicit MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool, bool aRequireTailDispatch = false);

  nsresult Dispatch(TemporaryRef<nsIRunnable> aRunnable);

  
  
  
  
  TaskDispatcher& TailDispatcher();

  
  
  bool RequiresTailDispatch() { return mRequireTailDispatch; }

#ifdef DEBUG
  static void AssertInTailDispatchIfNeeded()
  {
    
    
    MediaTaskQueue* currentQueue = MediaTaskQueue::GetCurrentQueue();
    if (!currentQueue || !currentQueue->RequiresTailDispatch()) {
      return;
    }

    
    
    
    
    MOZ_ASSERT(!currentQueue->mTailDispatcher,
               "Not allowed to dispatch tasks directly from this task queue - use TailDispatcher()");
  }
#else
  static void AssertInTailDispatchIfNeeded() {}
#endif

  
  nsresult Dispatch(already_AddRefed<nsIRunnable> aRunnable) override
  {
    RefPtr<nsIRunnable> r(aRunnable);
    return ForceDispatch(r);
  }

  
  
  nsresult ForceDispatch(TemporaryRef<nsIRunnable> aRunnable);

  nsresult SyncDispatch(TemporaryRef<nsIRunnable> aRunnable);

  
  
  
  
  
  
  nsRefPtr<ShutdownPromise> BeginShutdown();

  
  void AwaitIdle();

  
  
  void AwaitShutdownAndIdle();

  bool IsEmpty();

  
  
  bool IsCurrentThreadIn() override;

protected:
  virtual ~MediaTaskQueue();


  
  
  
  void AwaitIdleLocked();

  enum DispatchMode { AbortIfFlushing, IgnoreFlushing, Forced };

  nsresult DispatchLocked(TemporaryRef<nsIRunnable> aRunnable,
                          DispatchMode aMode);

  RefPtr<SharedThreadPool> mPool;

  
  Monitor mQueueMonitor;

  struct TaskQueueEntry {
    RefPtr<nsIRunnable> mRunnable;
    bool mForceDispatch;

    explicit TaskQueueEntry(TemporaryRef<nsIRunnable> aRunnable,
                            bool aForceDispatch = false)
      : mRunnable(aRunnable), mForceDispatch(aForceDispatch) {}
  };

  
  std::queue<TaskQueueEntry> mTasks;

  
  
  
  
  
  
  
  
  Atomic<nsIThread*> mRunningThread;

  
  class AutoTaskGuard : public AutoTaskDispatcher
  {
  public:
    explicit AutoTaskGuard(MediaTaskQueue* aQueue) : mQueue(aQueue)
    {
      
      
      MOZ_ASSERT(!mQueue->mTailDispatcher);
      mQueue->mTailDispatcher = this;

      MOZ_ASSERT(sCurrentQueueTLS.get() == nullptr);
      sCurrentQueueTLS.set(aQueue);

      MOZ_ASSERT(mQueue->mRunningThread == nullptr);
      mQueue->mRunningThread = NS_GetCurrentThread();
    }

    ~AutoTaskGuard()
    {
      MOZ_ASSERT(mQueue->mRunningThread == NS_GetCurrentThread());
      mQueue->mRunningThread = nullptr;

      sCurrentQueueTLS.set(nullptr);
      mQueue->mTailDispatcher = nullptr;
    }

  private:
  MediaTaskQueue* mQueue;
  };

  TaskDispatcher* mTailDispatcher;

  
  
  bool mIsRunning;

  
  bool mIsShutdown;
  MediaPromiseHolder<ShutdownPromise> mShutdownPromise;

  
  bool mIsFlushing;

  
  
  bool mRequireTailDispatch;

  class Runner : public nsRunnable {
  public:
    explicit Runner(MediaTaskQueue* aQueue)
      : mQueue(aQueue)
    {
    }
    NS_METHOD Run() override;
  private:
    RefPtr<MediaTaskQueue> mQueue;
  };
};

class FlushableMediaTaskQueue : public MediaTaskQueue
{
public:
  explicit FlushableMediaTaskQueue(TemporaryRef<SharedThreadPool> aPool) : MediaTaskQueue(aPool) {}
  nsresult FlushAndDispatch(TemporaryRef<nsIRunnable> aRunnable);
  void Flush();

private:

  class MOZ_STACK_CLASS AutoSetFlushing
  {
  public:
    explicit AutoSetFlushing(FlushableMediaTaskQueue* aTaskQueue) : mTaskQueue(aTaskQueue)
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
    FlushableMediaTaskQueue* mTaskQueue;
  };

  void FlushLocked();

};

} 

#endif 
