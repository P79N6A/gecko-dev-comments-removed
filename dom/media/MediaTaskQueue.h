





#ifndef MediaTaskQueue_h_
#define MediaTaskQueue_h_

#include <queue>
#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"
#include "SharedThreadPool.h"
#include "nsThreadUtils.h"
#include "MediaPromise.h"

class nsIRunnable;

namespace mozilla {

class SharedThreadPool;

typedef MediaPromise<bool, bool> ShutdownPromise;






class MediaTaskQueue MOZ_FINAL {
  ~MediaTaskQueue();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaTaskQueue)

  explicit MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool);

  nsresult Dispatch(TemporaryRef<nsIRunnable> aRunnable);

  
  
  nsresult ForceDispatch(TemporaryRef<nsIRunnable> aRunnable);

  nsresult SyncDispatch(TemporaryRef<nsIRunnable> aRunnable);

  nsresult FlushAndDispatch(TemporaryRef<nsIRunnable> aRunnable);

  
  
  void Flush();

  
  
  
  
  
  
  nsRefPtr<ShutdownPromise> BeginShutdown();

  
  void AwaitIdle();

  
  
  void AwaitShutdownAndIdle();

  bool IsEmpty();

  
  
  bool IsCurrentThreadIn();

private:

  
  
  
  void AwaitIdleLocked();

  enum DispatchMode { AbortIfFlushing, IgnoreFlushing, Forced };

  nsresult DispatchLocked(TemporaryRef<nsIRunnable> aRunnable,
                          DispatchMode aMode);
  void FlushLocked();

  RefPtr<SharedThreadPool> mPool;

  
  Monitor mQueueMonitor;

  struct TaskQueueEntry {
    RefPtr<nsIRunnable> mRunnable;
    bool mForceDispatch;

    TaskQueueEntry(TemporaryRef<nsIRunnable> aRunnable, bool aForceDispatch = false)
      : mRunnable(aRunnable), mForceDispatch(aForceDispatch) {}
  };

  
  std::queue<TaskQueueEntry> mTasks;

  
  
  
  RefPtr<nsIThread> mRunningThread;

  
  
  bool mIsRunning;

  
  bool mIsShutdown;
  MediaPromiseHolder<ShutdownPromise> mShutdownPromise;

  class MOZ_STACK_CLASS AutoSetFlushing
  {
  public:
    explicit AutoSetFlushing(MediaTaskQueue* aTaskQueue) : mTaskQueue(aTaskQueue)
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
    MediaTaskQueue* mTaskQueue;
  };

  
  bool mIsFlushing;

  class Runner : public nsRunnable {
  public:
    explicit Runner(MediaTaskQueue* aQueue)
      : mQueue(aQueue)
    {
    }
    NS_METHOD Run() MOZ_OVERRIDE;
  private:
    RefPtr<MediaTaskQueue> mQueue;
  };
};

} 

#endif 
