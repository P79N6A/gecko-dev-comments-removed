





#include "AbstractThread.h"

#include "TaskQueue.h"
#include "nsThreadUtils.h"
#include "TaskDispatcher.h"

#include "nsContentUtils.h"
#include "nsServiceManagerUtils.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Maybe.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"

namespace mozilla {

StaticRefPtr<AbstractThread> sMainThread;
ThreadLocal<AbstractThread*> AbstractThread::sCurrentThreadTLS;

class XPCOMThreadWrapper : public AbstractThread
{
public:
  explicit XPCOMThreadWrapper(nsIThread* aTarget, bool aRequireTailDispatch)
    : AbstractThread(aRequireTailDispatch)
    , mTarget(aTarget)
  {
    
    
    
    
    
    
    
    
    MOZ_ASSERT_IF(aRequireTailDispatch,
                  NS_IsMainThread() && NS_GetCurrentThread() == aTarget);
  }

  virtual void Dispatch(already_AddRefed<nsIRunnable> aRunnable,
                        DispatchFailureHandling aFailureHandling = AssertDispatchSuccess,
                        DispatchReason aReason = NormalDispatch) override
  {
    nsCOMPtr<nsIRunnable> r = aRunnable;
    AbstractThread* currentThread;
    if (aReason != TailDispatch && (currentThread = GetCurrent()) && RequiresTailDispatch(currentThread)) {
      currentThread->TailDispatcher().AddTask(this, r.forget(), aFailureHandling);
      return;
    }

    nsresult rv = mTarget->Dispatch(r, NS_DISPATCH_NORMAL);
    MOZ_DIAGNOSTIC_ASSERT(aFailureHandling == DontAssertDispatchSuccess || NS_SUCCEEDED(rv));
    unused << rv;
  }

  virtual bool IsCurrentThreadIn() override
  {
    
    
    PRThread* thread = nullptr;
    mTarget->GetPRThread(&thread);
    bool in = PR_GetCurrentThread() == thread;
    MOZ_ASSERT(in == (GetCurrent() == this));
    return in;
  }

  void FireTailDispatcher()
  {
    MOZ_DIAGNOSTIC_ASSERT(mTailDispatcher.isSome());
    mTailDispatcher.ref().DrainDirectTasks();
    mTailDispatcher.reset();
  }

  virtual TaskDispatcher& TailDispatcher() override
  {
    MOZ_ASSERT(this == sMainThread); 
    MOZ_ASSERT(IsCurrentThreadIn());
    if (!mTailDispatcher.isSome()) {
      mTailDispatcher.emplace( true);

      nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &XPCOMThreadWrapper::FireTailDispatcher);
      nsContentUtils::RunInStableState(event.forget());
    }

    return mTailDispatcher.ref();
  }

  virtual nsIThread* AsXPCOMThread() override { return mTarget; }

private:
  nsRefPtr<nsIThread> mTarget;
  Maybe<AutoTaskDispatcher> mTailDispatcher;
};

bool
AbstractThread::RequiresTailDispatch(AbstractThread* aThread) const
{
  
  
  return SupportsTailDispatch() && aThread->SupportsTailDispatch();
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
  sMainThread = new XPCOMThreadWrapper(mainThread.get(),  true);
  ClearOnShutdown(&sMainThread);

  if (!sCurrentThreadTLS.init()) {
    MOZ_CRASH();
  }
  sCurrentThreadTLS.set(sMainThread);
}

void
AbstractThread::DispatchStateChange(already_AddRefed<nsIRunnable> aRunnable)
{
  GetCurrent()->TailDispatcher().AddStateChangeTask(this, Move(aRunnable));
}

 void
AbstractThread::DispatchDirectTask(already_AddRefed<nsIRunnable> aRunnable)
{
  GetCurrent()->TailDispatcher().AddDirectTask(Move(aRunnable));
}

} 
