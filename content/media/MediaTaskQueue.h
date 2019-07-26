





#ifndef MediaTaskQueue_h_
#define MediaTaskQueue_h_

#include <queue>
#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"
#include "nsThreadUtils.h"

class nsIRunnable;

namespace mozilla {

class SharedThreadPool;






class MediaTaskQueue : public AtomicRefCounted<MediaTaskQueue> {
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(MediaTaskQueue)
  MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool);
  ~MediaTaskQueue();

  nsresult Dispatch(nsIRunnable* aRunnable);

  
  
  void Flush();

  
  
  void Shutdown();

  
  void AwaitIdle();

  bool IsEmpty();

  
  
  bool IsCurrentThreadIn();

private:

  
  
  
  void AwaitIdleLocked();

  RefPtr<SharedThreadPool> mPool;

  
  Monitor mQueueMonitor;

  
  std::queue<RefPtr<nsIRunnable>> mTasks;

  
  
  
  RefPtr<nsIThread> mRunningThread;

  
  
  bool mIsRunning;

  
  bool mIsShutdown;

  class Runner : public nsRunnable {
  public:
    Runner(MediaTaskQueue* aQueue)
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
