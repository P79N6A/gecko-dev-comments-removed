



#ifndef mozilla_ipc_backgroundparentimpl_h__
#define mozilla_ipc_backgroundparentimpl_h__

#include "mozilla/Attributes.h"
#include "mozilla/ipc/PBackgroundParent.h"

namespace mozilla {

namespace layout {
class VsyncParent;
}

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
  AllocPBackgroundIDBFactoryParent(const LoggingInfo& aLoggingInfo)
                                   MOZ_OVERRIDE;

  virtual bool
  RecvPBackgroundIDBFactoryConstructor(PBackgroundIDBFactoryParent* aActor,
                                       const LoggingInfo& aLoggingInfo)
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

  virtual PVsyncParent*
  AllocPVsyncParent() MOZ_OVERRIDE;

  virtual bool
  DeallocPVsyncParent(PVsyncParent* aActor) MOZ_OVERRIDE;

  virtual PBroadcastChannelParent*
  AllocPBroadcastChannelParent(const PrincipalInfo& aPrincipalInfo,
                               const nsString& aOrigin,
                               const nsString& aChannel) MOZ_OVERRIDE;

  virtual bool
  RecvPBroadcastChannelConstructor(PBroadcastChannelParent* actor,
                                   const PrincipalInfo& aPrincipalInfo,
                                   const nsString& origin,
                                   const nsString& channel) MOZ_OVERRIDE;

  virtual bool
  DeallocPBroadcastChannelParent(PBroadcastChannelParent* aActor) MOZ_OVERRIDE;
};

} 
} 

#endif 
