




#ifndef mozilla_SyncRunnable_h
#define mozilla_SyncRunnable_h

#include "nsThreadUtils.h"
#include "mozilla/Monitor.h"

namespace mozilla {

















class SyncRunnable : public nsRunnable
{
public:
  SyncRunnable(nsIRunnable* r)
    : mRunnable(r)
    , mMonitor("SyncRunnable")
    , mDone(false)
  { }

  void DispatchToThread(nsIEventTarget* thread,
                        bool forceDispatch = false)
  {
    nsresult rv;
    bool on;

    if (!forceDispatch) {
      rv = thread->IsOnCurrentThread(&on);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
      if (NS_SUCCEEDED(rv) && on) {
        mRunnable->Run();
        return;
      }
    }

    rv = thread->Dispatch(this, NS_DISPATCH_NORMAL);
    if (NS_SUCCEEDED(rv)) {
      mozilla::MonitorAutoLock lock(mMonitor);
      while (!mDone) {
        lock.Wait();
      }
    }
  }

  static void DispatchToThread(nsIEventTarget* thread,
                               nsIRunnable* r,
                               bool forceDispatch = false)
  {
    nsRefPtr<SyncRunnable> s(new SyncRunnable(r));
    s->DispatchToThread(thread, forceDispatch);
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
