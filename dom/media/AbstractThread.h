





#if !defined(AbstractThread_h_)
#define AbstractThread_h_

#include "nscore.h"
#include "nsIRunnable.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsRefPtr.h"

namespace mozilla {














class AbstractThread
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AbstractThread);

  enum DispatchFailureHandling { AssertDispatchSuccess, DontAssertDispatchSuccess };
  virtual void Dispatch(already_AddRefed<nsIRunnable> aRunnable,
                        DispatchFailureHandling aHandling = AssertDispatchSuccess) = 0;
  virtual bool IsCurrentThreadIn() = 0;

  
  
  void MaybeTailDispatch(already_AddRefed<nsIRunnable> aRunnable,
                         DispatchFailureHandling aFailureHandling = AssertDispatchSuccess);

  
  
  virtual bool IsDispatchReliable() { return true; }

  
  static AbstractThread* MainThread();

  
  static void InitStatics();

protected:
  virtual ~AbstractThread() {}
};

} 

#endif
