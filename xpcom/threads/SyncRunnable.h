





#ifndef mozilla_SyncRunnable_h
#define mozilla_SyncRunnable_h

#include "nsThreadUtils.h"
#include "mozilla/Monitor.h"

namespace mozilla {

















class SyncRunnable : public nsRunnable
{
public:
  explicit SyncRunnable(nsIRunnable* aRunnable)
    : mRunnable(aRunnable)
    , mMonitor("SyncRunnable")
    , mDone(false)
  {
  }

  void DispatchToThread(nsIEventTarget* aThread, bool aForceDispatch = false)
  {
    nsresult rv;
    bool on;

    if (!aForceDispatch) {
      rv = aThread->IsOnCurrentThread(&on);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
      if (NS_SUCCEEDED(rv) && on) {
        mRunnable->Run();
        return;
      }
    }

    rv = aThread->Dispatch(this, NS_DISPATCH_NORMAL);
    if (NS_SUCCEEDED(rv)) {
      mozilla::MonitorAutoLock lock(mMonitor);
      while (!mDone) {
        lock.Wait();
      }
    }
  }

  static void DispatchToThread(nsIEventTarget* aThread,
                               nsIRunnable* aRunnable,
                               bool aForceDispatch = false)
  {
    nsRefPtr<SyncRunnable> s(new SyncRunnable(aRunnable));
    s->DispatchToThread(aThread, aForceDispatch);
  }

protected:
  NS_IMETHODIMP Run()
  {
    mRunnable->Run();

    mozilla::MonitorAutoLock lock(mMonitor);
    MOZ_ASSERT(!mDone);

    mDone = true;
    mMonitor.Notify();

    return NS_OK;
  }

private:
  nsCOMPtr<nsIRunnable> mRunnable;
  mozilla::Monitor mMonitor;
  bool mDone;
};

} 

#endif 
