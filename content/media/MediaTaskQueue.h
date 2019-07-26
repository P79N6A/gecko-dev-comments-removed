





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
  MediaTaskQueue(TemporaryRef<SharedThreadPool> aPool);
  ~MediaTaskQueue();

  nsresult Dispatch(nsIRunnable* aRunnable);

  
  
  void Flush();

  
  
  void Shutdown();

  
  void AwaitIdle();

  bool IsEmpty();

private:

  
  
  
  void AwaitIdleLocked();

  RefPtr<SharedThreadPool> mPool;

  
  Monitor mQueueMonitor;

  
  std::queue<RefPtr<nsIRunnable>> mTasks;

  
  
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
