





#include "AbstractThread.h"

#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {

StaticRefPtr<AbstractThread> sMainThread;

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

AbstractThread*
AbstractThread::MainThread()
{
  MOZ_ASSERT(sMainThread);
  return sMainThread;
}

void
AbstractThread::EnsureMainThreadSingleton()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!sMainThread) {
    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));
    MOZ_DIAGNOSTIC_ASSERT(mainThread);
    sMainThread = AbstractThread::Create(mainThread.get());
    ClearOnShutdown(&sMainThread);
  }
}

} 
