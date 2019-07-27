



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

  virtual PBackgroundIDBFactoryParent*
  AllocPBackgroundIDBFactoryParent(const OptionalWindowId& aOptionalWindowId)
                                   MOZ_OVERRIDE;

  virtual bool
  RecvPBackgroundIDBFactoryConstructor(
                                      PBackgroundIDBFactoryParent* aActor,
                                      const OptionalWindowId& aOptionalWindowId)
                                      MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBFactoryParent(PBackgroundIDBFactoryParent* aActor)
                                     MOZ_OVERRIDE;

  virtual PBlobParent*
  AllocPBlobParent(const BlobConstructorParams& aParams) MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobParent(PBlobParent* aActor) MOZ_OVERRIDE;

  virtual PFileDescriptorSetParent*
  AllocPFileDescriptorSetParent(const FileDescriptor& aFileDescriptor)
                                MOZ_OVERRIDE;

  virtual bool
  DeallocPFileDescriptorSetParent(PFileDescriptorSetParent* aActor)
                                  MOZ_OVERRIDE;
};

} 
} 

#endif 
