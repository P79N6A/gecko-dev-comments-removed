



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

  virtual bool
  RecvRegisterServiceWorker(const ServiceWorkerRegistrationData& aData)
                            MOZ_OVERRIDE;

  virtual bool
  RecvUnregisterServiceWorker(const PrincipalInfo& aPrincipalInfo,
                              const nsString& aScope) MOZ_OVERRIDE;

  virtual bool
  RecvShutdownServiceWorkerRegistrar() MOZ_OVERRIDE;

  virtual dom::cache::PCacheStorageParent*
  AllocPCacheStorageParent(const dom::cache::Namespace& aNamespace,
                           const PrincipalInfo& aPrincipalInfo) MOZ_OVERRIDE;

  virtual bool
  DeallocPCacheStorageParent(dom::cache::PCacheStorageParent* aActor) MOZ_OVERRIDE;

  virtual dom::cache::PCacheParent* AllocPCacheParent() MOZ_OVERRIDE;

  virtual bool
  DeallocPCacheParent(dom::cache::PCacheParent* aActor) MOZ_OVERRIDE;

  virtual dom::cache::PCacheStreamControlParent*
  AllocPCacheStreamControlParent() MOZ_OVERRIDE;

  virtual bool
  DeallocPCacheStreamControlParent(dom::cache::PCacheStreamControlParent* aActor) MOZ_OVERRIDE;
};

} 
} 

#endif 
