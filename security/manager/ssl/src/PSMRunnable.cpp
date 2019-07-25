


































 
#include "PSMRunnable.h"
 
namespace mozilla { namespace psm {

SyncRunnableBase::SyncRunnableBase()
  : monitor("SyncRunnableBase::monitor")
{
}

nsresult
SyncRunnableBase::DispatchToMainThreadAndWait()
{
  NS_ASSERTION(!NS_IsMainThread(),
               "DispatchToMainThreadAndWait called on the main thread.");

  mozilla::MonitorAutoLock lock(monitor);
  nsresult rv = NS_DispatchToMainThread(this);
  if (NS_SUCCEEDED(rv)) {
    lock.Wait();
  }
  return rv;
}

NS_IMETHODIMP
SyncRunnableBase::Run()
{
  RunOnTargetThread();
  mozilla::MonitorAutoLock(monitor).Notify();
  return NS_OK;
}

} } 
