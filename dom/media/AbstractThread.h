





#if !defined(AbstractThread_h_)
#define AbstractThread_h_

#include "nscore.h"
#include "nsIRunnable.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsRefPtr.h"

#include "mozilla/ThreadLocal.h"

namespace mozilla {

class MediaTaskQueue;
class TaskDispatcher;














class AbstractThread
{
public:
  
  
  static AbstractThread* GetCurrent() { return sCurrentThreadTLS.get(); }

  AbstractThread(bool aSupportsTailDispatch) : mSupportsTailDispatch(aSupportsTailDispatch) {}

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AbstractThread);

  enum DispatchFailureHandling { AssertDispatchSuccess, DontAssertDispatchSuccess };
  enum DispatchReason { NormalDispatch, TailDispatch };
  virtual void Dispatch(already_AddRefed<nsIRunnable> aRunnable,
                        DispatchFailureHandling aHandling = AssertDispatchSuccess,
                        DispatchReason aReason = NormalDispatch) = 0;

  virtual bool IsCurrentThreadIn() = 0;

  
  
  virtual bool IsDispatchReliable() { return true; }

  
  
  
  
  
  virtual TaskDispatcher& TailDispatcher() = 0;

  
  bool SupportsTailDispatch() const { return mSupportsTailDispatch; }

  
  
  bool RequiresTailDispatch(AbstractThread* aThread) const;

  virtual MediaTaskQueue* AsTaskQueue() { MOZ_CRASH("Not a task queue!"); }
  virtual nsIThread* AsXPCOMThread() { MOZ_CRASH("Not an XPCOM thread!"); }

  
  static AbstractThread* MainThread();

  
  static void InitStatics();

  void DispatchStateChange(already_AddRefed<nsIRunnable> aRunnable);

  static void DispatchDirectTask(already_AddRefed<nsIRunnable> aRunnable);

protected:
  virtual ~AbstractThread() {}
  static ThreadLocal<AbstractThread*> sCurrentThreadTLS;

  
  
  const bool mSupportsTailDispatch;
};

} 

#endif
