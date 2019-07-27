





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
  virtual nsresult Dispatch(already_AddRefed<nsIRunnable> aRunnable) = 0;
  virtual bool IsCurrentThreadIn() = 0;

  
  
  void MaybeTailDispatch(already_AddRefed<nsIRunnable> aRunnable,
                         bool aAssertDispatchSuccess = true);

  
  static AbstractThread* MainThread();

  
  static void InitStatics();

protected:
  virtual ~AbstractThread() {}
};

template<typename TargetType>
class AbstractThreadImpl : public AbstractThread
{
public:
  explicit AbstractThreadImpl(TargetType* aTarget) : mTarget(aTarget) {}
  virtual nsresult Dispatch(already_AddRefed<nsIRunnable> aRunnable);
  virtual bool IsCurrentThreadIn();
private:
  nsRefPtr<TargetType> mTarget;
};

} 

#endif
