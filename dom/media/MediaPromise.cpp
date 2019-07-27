





#include "MediaPromise.h"
#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace detail {

nsresult
DispatchMediaPromiseRunnable(MediaTaskQueue* aTaskQueue, nsIRunnable* aRunnable)
{
  return aTaskQueue->ForceDispatch(aRunnable);
}

nsresult
DispatchMediaPromiseRunnable(nsIEventTarget* aEventTarget, nsIRunnable* aRunnable)
{
  return aEventTarget->Dispatch(aRunnable, NS_DISPATCH_NORMAL);
}

}
} 
