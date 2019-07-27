



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
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBackgroundTestParent*
  AllocPBackgroundTestParent(const nsCString& aTestArg) override;

  virtual bool
  RecvPBackgroundTestConstructor(PBackgroundTestParent* aActor,
                                 const nsCString& aTestArg) override;

  virtual bool
  DeallocPBackgroundTestParent(PBackgroundTestParent* aActor) override;

  virtual PBackgroundIDBFactoryParent*
  AllocPBackgroundIDBFactoryParent(const LoggingInfo& aLoggingInfo)
                                   override;

  virtual bool
  RecvPBackgroundIDBFactoryConstructor(PBackgroundIDBFactoryParent* aActor,
                                       const LoggingInfo& aLoggingInfo)
                                       override;

  virtual bool
  DeallocPBackgroundIDBFactoryParent(PBackgroundIDBFactoryParent* aActor)
                                     override;

  virtual PBlobParent*
  AllocPBlobParent(const BlobConstructorParams& aParams) override;

  virtual bool
  DeallocPBlobParent(PBlobParent* aActor) override;

  virtual PFileDescriptorSetParent*
  AllocPFileDescriptorSetParent(const FileDescriptor& aFileDescriptor)
                                override;

  virtual bool
  DeallocPFileDescriptorSetParent(PFileDescriptorSetParent* aActor)
                                  override;

  virtual PVsyncParent*
  AllocPVsyncParent() override;

  virtual bool
  DeallocPVsyncParent(PVsyncParent* aActor) override;

  virtual PBroadcastChannelParent*
  AllocPBroadcastChannelParent(const PrincipalInfo& aPrincipalInfo,
                               const nsCString& aOrigin,
                               const nsString& aChannel,
                               const bool& aPrivateBrowsing) override;

  virtual bool
  RecvPBroadcastChannelConstructor(PBroadcastChannelParent* actor,
                                   const PrincipalInfo& aPrincipalInfo,
                                   const nsCString& origin,
                                   const nsString& channel,
                                   const bool& aPrivateBrowsing) override;

  virtual bool
  DeallocPBroadcastChannelParent(PBroadcastChannelParent* aActor) override;

  virtual PServiceWorkerManagerParent*
  AllocPServiceWorkerManagerParent() override;

  virtual bool
  DeallocPServiceWorkerManagerParent(PServiceWorkerManagerParent* aActor) override;

  virtual bool
  RecvShutdownServiceWorkerRegistrar() override;

  virtual dom::cache::PCacheStorageParent*
  AllocPCacheStorageParent(const dom::cache::Namespace& aNamespace,
                           const PrincipalInfo& aPrincipalInfo) override;

  virtual bool
  DeallocPCacheStorageParent(dom::cache::PCacheStorageParent* aActor) override;

  virtual dom::cache::PCacheParent* AllocPCacheParent() override;

  virtual bool
  DeallocPCacheParent(dom::cache::PCacheParent* aActor) override;

  virtual dom::cache::PCacheStreamControlParent*
  AllocPCacheStreamControlParent() override;

  virtual bool
  DeallocPCacheStreamControlParent(dom::cache::PCacheStreamControlParent* aActor)
                                   override;

  virtual PUDPSocketParent*
  AllocPUDPSocketParent(const OptionalPrincipalInfo& pInfo,
                        const nsCString& aFilter) override;
  virtual bool
  RecvPUDPSocketConstructor(PUDPSocketParent*,
                            const OptionalPrincipalInfo& aPrincipalInfo,
                            const nsCString& aFilter) override;
  virtual bool
  DeallocPUDPSocketParent(PUDPSocketParent*) override;

  virtual PMessagePortParent*
  AllocPMessagePortParent(const nsID& aUUID,
                          const nsID& aDestinationUUID,
                          const uint32_t& aSequenceID) override;

  virtual bool
  RecvPMessagePortConstructor(PMessagePortParent* aActor,
                              const nsID& aUUID,
                              const nsID& aDestinationUUID,
                              const uint32_t& aSequenceID) override;

  virtual bool
  DeallocPMessagePortParent(PMessagePortParent* aActor) override;

  virtual bool
  RecvMessagePortForceClose(const nsID& aUUID,
                            const nsID& aDestinationUUID,
                            const uint32_t& aSequenceID) override;
};

} 
} 

#endif 
