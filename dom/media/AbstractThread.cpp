





#include "AbstractThread.h"

#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"

namespace mozilla {

template<>
nsresult
AbstractThreadImpl<MediaTaskQueue>::Dispatch(already_AddRefed<nsIRunnable> aRunnable)
{
  RefPtr<nsIRunnable> r(aRunnable);
  return mTarget->ForceDispatch(r);
}

template<>
nsresult
AbstractThreadImpl<nsIThread>::Dispatch(already_AddRefed<nsIRunnable> aRunnable)
{
  nsCOMPtr<nsIRunnable> r = aRunnable;
  return mTarget->Dispatch(r, NS_DISPATCH_NORMAL);
}

template<>
bool
AbstractThreadImpl<MediaTaskQueue>::IsCurrentThreadIn()
{
  return mTarget->IsCurrentThreadIn();
}

template<>
bool
AbstractThreadImpl<nsIThread>::IsCurrentThreadIn()
{
  return NS_GetCurrentThread() == mTarget;
}

} 
