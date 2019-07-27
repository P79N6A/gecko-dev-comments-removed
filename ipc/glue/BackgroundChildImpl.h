



#ifndef mozilla_ipc_backgroundchildimpl_h__
#define mozilla_ipc_backgroundchildimpl_h__

#include "mozilla/Attributes.h"
#include "mozilla/ipc/PBackgroundChild.h"

template <class> class nsAutoPtr;

namespace mozilla {
namespace ipc {



class BackgroundChildImpl : public PBackgroundChild
{
public:
  class ThreadLocal;

  
  
  
  
  
  static ThreadLocal*
  GetThreadLocalForCurrentThread();

protected:
  BackgroundChildImpl();
  virtual ~BackgroundChildImpl();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBackgroundTestChild*
  AllocPBackgroundTestChild(const nsCString& aTestArg) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundTestChild(PBackgroundTestChild* aActor) MOZ_OVERRIDE;
};

class BackgroundChildImpl::ThreadLocal
{
  friend class nsAutoPtr<ThreadLocal>;

  

public:
  ThreadLocal();

private:
  
  ~ThreadLocal();
};

} 
} 

#endif 
