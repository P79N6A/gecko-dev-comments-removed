





#include "AbstractThread.h"

#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {

StaticRefPtr<AbstractThread> sMainThread;

template<>
nsresult
AbstractThreadImpl<nsIThread>::Dispatch(already_AddRefed<nsIRunnable> aRunnable)
{
  MediaTaskQueue::AssertInTailDispatchIfNeeded();
  nsCOMPtr<nsIRunnable> r = aRunnable;
  return mTarget->Dispatch(r, NS_DISPATCH_NORMAL);
}

template<>
bool
AbstractThreadImpl<nsIThread>::IsCurrentThreadIn()
{
  bool in = NS_GetCurrentThread() == mTarget;
  MOZ_ASSERT_IF(in, MediaTaskQueue::GetCurrentQueue() == nullptr);
  return in;
}

AbstractThread*
AbstractThread::MainThread()
{
  MOZ_ASSERT(sMainThread);
  return sMainThread;
}

void
AbstractThread::InitStatics()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sMainThread);
  nsCOMPtr<nsIThread> mainThread;
  NS_GetMainThread(getter_AddRefs(mainThread));
  MOZ_DIAGNOSTIC_ASSERT(mainThread);
  sMainThread = AbstractThread::Create(mainThread.get());
  ClearOnShutdown(&sMainThread);
}

} 
