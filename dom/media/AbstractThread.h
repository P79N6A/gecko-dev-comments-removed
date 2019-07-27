





#if !defined(AbstractThread_h_)
#define AbstractThread_h_

#include "nscore.h"
#include "nsIRunnable.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsRefPtr.h"

#include "mozilla/ThreadLocal.h"

namespace mozilla {

class TaskDispatcher;














class AbstractThread
{
public:
  
  
  static AbstractThread* GetCurrent() { return sCurrentThreadTLS.get(); }

  AbstractThread(bool aRequireTailDispatch) : mRequireTailDispatch(aRequireTailDispatch) {}

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AbstractThread);

  enum DispatchFailureHandling { AssertDispatchSuccess, DontAssertDispatchSuccess };
  virtual void Dispatch(already_AddRefed<nsIRunnable> aRunnable,
                        DispatchFailureHandling aHandling = AssertDispatchSuccess) = 0;
  virtual bool IsCurrentThreadIn() = 0;

  
  
  void MaybeTailDispatch(already_AddRefed<nsIRunnable> aRunnable,
                         DispatchFailureHandling aFailureHandling = AssertDispatchSuccess);

  
  
  virtual bool IsDispatchReliable() { return true; }

  
  
  
  
  
  virtual TaskDispatcher& TailDispatcher() = 0;

  
  virtual bool InTailDispatch() = 0;

  
  
  bool RequiresTailDispatch() const { return mRequireTailDispatch; }

  
  static AbstractThread* MainThread();

  
  static void InitStatics();

#ifdef DEBUG
  static void AssertInTailDispatchIfNeeded()
  {
    
    
    AbstractThread* currentThread = GetCurrent();
    if (!currentThread || !currentThread->RequiresTailDispatch()) {
      return;
    }

    MOZ_ASSERT(currentThread->InTailDispatch(),
               "Not allowed to dispatch tasks directly from this task queue - use TailDispatcher()");
  }
#else
  static void AssertInTailDispatchIfNeeded() {}
#endif


protected:
  virtual ~AbstractThread() {}
  static ThreadLocal<AbstractThread*> sCurrentThreadTLS;

  
  
  const bool mRequireTailDispatch;
};

} 

#endif
