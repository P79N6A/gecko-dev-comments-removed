





#include "MediaPromise.h"

#include "MediaDecoderStateMachineScheduler.h"
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

nsresult
DispatchMediaPromiseRunnable(MediaDecoderStateMachineScheduler* aScheduler, nsIRunnable* aRunnable)
{
  return aScheduler->GetStateMachineThread()->Dispatch(aRunnable, NS_DISPATCH_NORMAL);
}

void
AssertOnThread(MediaTaskQueue* aQueue)
{
  MOZ_ASSERT(aQueue->IsCurrentThreadIn());
}

void AssertOnThread(nsIEventTarget* aTarget)
{
  nsCOMPtr<nsIThread> targetThread = do_QueryInterface(aTarget);
  MOZ_ASSERT(targetThread, "Don't know how to deal with threadpools etc here");
  MOZ_ASSERT(NS_GetCurrentThread() == targetThread);
}

void
AssertOnThread(MediaDecoderStateMachineScheduler* aScheduler)
{
  MOZ_ASSERT(aScheduler->OnStateMachineThread());
}

}
} 
