



#ifndef mozilla_ipc_backgroundparentimpl_h__
#define mozilla_ipc_backgroundparentimpl_h__

#include "mozilla/Attributes.h"
#include "mozilla/ipc/PBackgroundParent.h"

namespace mozilla {
namespace ipc {



class BackgroundParentImpl : public PBackgroundParent
{
protected:
  BackgroundParentImpl();
  virtual ~BackgroundParentImpl();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBackgroundTestParent*
  AllocPBackgroundTestParent(const nsCString& aTestArg) MOZ_OVERRIDE;

  virtual bool
  RecvPBackgroundTestConstructor(PBackgroundTestParent* aActor,
                                 const nsCString& aTestArg) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundTestParent(PBackgroundTestParent* aActor) MOZ_OVERRIDE;
};

} 
} 

#endif 
