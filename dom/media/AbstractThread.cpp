





#include "AbstractThread.h"

#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {

StaticRefPtr<AbstractThread> sMainThread;

class XPCOMThreadWrapper : public AbstractThread
{
public:
  explicit XPCOMThreadWrapper(nsIThread* aTarget) : mTarget(aTarget) {}

  virtual nsresult Dispatch(already_AddRefed<nsIRunnable> aRunnable) override
  {
    MediaTaskQueue::AssertInTailDispatchIfNeeded();
    nsCOMPtr<nsIRunnable> r = aRunnable;
    return mTarget->Dispatch(r, NS_DISPATCH_NORMAL);
  }

  virtual bool IsCurrentThreadIn() override
  {
    bool in = NS_GetCurrentThread() == mTarget;
    MOZ_ASSERT_IF(in, MediaTaskQueue::GetCurrentQueue() == nullptr);
    return in;
  }

private:
  nsRefPtr<nsIThread> mTarget;
};

void
AbstractThread::MaybeTailDispatch(already_AddRefed<nsIRunnable> aRunnable,
                                  bool aAssertDispatchSuccess)
{
  MediaTaskQueue* currentQueue = MediaTaskQueue::GetCurrentQueue();
  if (currentQueue && currentQueue->RequiresTailDispatch()) {
    currentQueue->TailDispatcher().AddTask(this, Move(aRunnable), aAssertDispatchSuccess);
  } else {
    nsresult rv = Dispatch(Move(aRunnable));
    MOZ_DIAGNOSTIC_ASSERT(!aAssertDispatchSuccess || NS_SUCCEEDED(rv));
    unused << rv;
  }
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
  sMainThread = new XPCOMThreadWrapper(mainThread.get());
  ClearOnShutdown(&sMainThread);
}

} 
