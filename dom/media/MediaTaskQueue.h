





#ifndef MediaTaskQueue_h_
#define MediaTaskQueue_h_

#include <queue>
#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"
#include "nsThreadUtils.h"

class nsIRunnable;

namespace mozilla {

class SharedThreadPool;






class MediaTaskQueue MOZ_FINAL {
  ~MediaTaskQueue();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaTaskQueue)

  explicit MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool);

  nsresult Dispatch(TemporaryRef<nsIRunnable> aRunnable);

  nsresult SyncDispatch(TemporaryRef<nsIRunnable> aRunnable);

  nsresult FlushAndDispatch(TemporaryRef<nsIRunnable> aRunnable);

  
  
  void Flush();

  
  
  void Shutdown();

  
  void AwaitIdle();

  bool IsEmpty();

  
  
  bool IsCurrentThreadIn();

private:

  
  
  
  void AwaitIdleLocked();

  enum DispatchMode { AbortIfFlushing, IgnoreFlushing };

  nsresult DispatchLocked(TemporaryRef<nsIRunnable> aRunnable,
                          DispatchMode aMode);

  RefPtr<SharedThreadPool> mPool;

  
  Monitor mQueueMonitor;

  
  std::queue<RefPtr<nsIRunnable>> mTasks;

  
  
  
  RefPtr<nsIThread> mRunningThread;

  
  
  bool mIsRunning;

  
  bool mIsShutdown;

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
