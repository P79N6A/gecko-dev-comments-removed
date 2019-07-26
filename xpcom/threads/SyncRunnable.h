




#ifndef mozilla_SyncRunnable_h

#include "nsThreadUtils.h"
#include "mozilla/Monitor.h"

namespace mozilla {

















class SyncRunnable : public nsRunnable
{
public:
  SyncRunnable(nsIRunnable* r)
    : mRunnable(r)
    , mMonitor("SyncRunnable")
  { }

  void DispatchToThread(nsIEventTarget* thread)
  {
    nsresult rv;
    bool on;

    rv = thread->IsOnCurrentThread(&on);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    if (NS_SUCCEEDED(rv) && on) {
      mRunnable->Run();
      return;
    }

    mozilla::MonitorAutoLock lock(mMonitor);
    rv = thread->Dispatch(this, NS_DISPATCH_NORMAL);
    if (NS_SUCCEEDED(rv)) {
      lock.Wait();
    }
  }

  static void DispatchToThread(nsIEventTarget* thread,
                               nsIRunnable* r)
  {
    nsRefPtr<SyncRunnable> s(new SyncRunnable(r));
    s->DispatchToThread(thread);
  }

protected:
  NS_IMETHODIMP Run()
  {
    mRunnable->Run();
    mozilla::MonitorAutoLock(mMonitor).Notify();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIRunnable> mRunnable;
  mozilla::Monitor mMonitor;
};

} 

#endif 
