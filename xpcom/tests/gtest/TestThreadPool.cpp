





#include <stdio.h>
#include <stdlib.h>
#include "nsXPCOM.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIThreadPool.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "mozilla/Atomics.h"
#include "mozilla/Monitor.h"
#include "gtest/gtest.h"

class Task final : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  explicit Task(int i) : mIndex(i) {}

  NS_IMETHOD Run() override
  {
    printf("###(%d) running from thread: %p\n", mIndex, (void *) PR_GetCurrentThread());
    int r = (int) ((float) rand() * 200 / RAND_MAX);
    PR_Sleep(PR_MillisecondsToInterval(r));
    printf("###(%d) exiting from thread: %p\n", mIndex, (void *) PR_GetCurrentThread());
    ++sCount;
    return NS_OK;
  }

  static mozilla::Atomic<int> sCount;

private:
  ~Task() {}

  int mIndex;
};
NS_IMPL_ISUPPORTS(Task, nsIRunnable)

mozilla::Atomic<int> Task::sCount;

TEST(ThreadPool, Main)
{
  nsCOMPtr<nsIThreadPool> pool = do_CreateInstance(NS_THREADPOOL_CONTRACTID);
  EXPECT_TRUE(pool);

  for (int i = 0; i < 100; ++i) {
    nsCOMPtr<nsIRunnable> task = new Task(i);
    EXPECT_TRUE(task);

    pool->Dispatch(task, NS_DISPATCH_NORMAL);
  }

  pool->Shutdown();
  EXPECT_EQ(Task::sCount, 100);
}

TEST(ThreadPool, Parallelism)
{
  nsCOMPtr<nsIThreadPool> pool = do_CreateInstance(NS_THREADPOOL_CONTRACTID);
  EXPECT_TRUE(pool);

  
  nsCOMPtr<nsIRunnable> r0 = new nsRunnable();
  pool->Dispatch(r0, NS_DISPATCH_SYNC);
  PR_Sleep(PR_SecondsToInterval(2));

  class Runnable1 : public nsRunnable {
  public:
    Runnable1(Monitor& aMonitor, bool& aDone)
      : mMonitor(aMonitor), mDone(aDone) {}

    NS_IMETHOD Run() override {
      MonitorAutoLock mon(mMonitor);
      if (!mDone) {
        
        
        mon.Wait(PR_SecondsToInterval(300));
      }
      EXPECT_TRUE(mDone);
      return NS_OK;
    }
  private:
    Monitor& mMonitor;
    bool& mDone;
  };

  class Runnable2 : public nsRunnable {
  public:
    Runnable2(Monitor& aMonitor, bool& aDone)
      : mMonitor(aMonitor), mDone(aDone) {}

    NS_IMETHOD Run() override {
      MonitorAutoLock mon(mMonitor);
      mDone = true;
      mon.NotifyAll();
      return NS_OK;
    }
  private:
    Monitor& mMonitor;
    bool& mDone;
  };

  
  
  
  
  Monitor mon("ThreadPool::Parallelism");
  bool done = false;
  nsCOMPtr<nsIRunnable> r1 = new Runnable1(mon, done);
  nsCOMPtr<nsIRunnable> r2 = new Runnable2(mon, done);
  pool->Dispatch(r1, NS_DISPATCH_NORMAL);
  pool->Dispatch(r2, NS_DISPATCH_NORMAL);

  pool->Shutdown();
}
