



#ifndef mozilla_ipc_backgroundchildimpl_h__
#define mozilla_ipc_backgroundchildimpl_h__

#include "mozilla/Attributes.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {
namespace indexedDB {

class ThreadLocal;

} 
} 

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
  ProcessingError(Result aCode, const char* aReason) MOZ_OVERRIDE;

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBackgroundTestChild*
  AllocPBackgroundTestChild(const nsCString& aTestArg) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundTestChild(PBackgroundTestChild* aActor) MOZ_OVERRIDE;

  virtual PBackgroundIDBFactoryChild*
  AllocPBackgroundIDBFactoryChild(const LoggingInfo& aLoggingInfo) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBFactoryChild(PBackgroundIDBFactoryChild* aActor)
                                    MOZ_OVERRIDE;

  virtual PBlobChild*
  AllocPBlobChild(const BlobConstructorParams& aParams) MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobChild(PBlobChild* aActor) MOZ_OVERRIDE;

  virtual PFileDescriptorSetChild*
  AllocPFileDescriptorSetChild(const FileDescriptor& aFileDescriptor)
                               MOZ_OVERRIDE;

  virtual bool
  DeallocPFileDescriptorSetChild(PFileDescriptorSetChild* aActor) MOZ_OVERRIDE;

  virtual PVsyncChild*
  AllocPVsyncChild() MOZ_OVERRIDE;

  virtual bool
  DeallocPVsyncChild(PVsyncChild* aActor) MOZ_OVERRIDE;

  virtual PBroadcastChannelChild*
  AllocPBroadcastChannelChild(const PrincipalInfo& aPrincipalInfo,
                              const nsString& aOrigin,
                              const nsString& aChannel) MOZ_OVERRIDE;

  virtual bool
  DeallocPBroadcastChannelChild(PBroadcastChannelChild* aActor) MOZ_OVERRIDE;

  virtual dom::cache::PCacheStorageChild*
  AllocPCacheStorageChild(const dom::cache::Namespace& aNamespace,
                          const PrincipalInfo& aPrincipalInfo) MOZ_OVERRIDE;

  virtual bool
  DeallocPCacheStorageChild(dom::cache::PCacheStorageChild* aActor) MOZ_OVERRIDE;

  virtual dom::cache::PCacheChild* AllocPCacheChild() MOZ_OVERRIDE;

  virtual bool
  DeallocPCacheChild(dom::cache::PCacheChild* aActor) MOZ_OVERRIDE;

  virtual dom::cache::PCacheStreamControlChild*
  AllocPCacheStreamControlChild() MOZ_OVERRIDE;

  virtual bool
  DeallocPCacheStreamControlChild(dom::cache::PCacheStreamControlChild* aActor) MOZ_OVERRIDE;
};

class BackgroundChildImpl::ThreadLocal MOZ_FINAL
{
  friend class nsAutoPtr<ThreadLocal>;

public:
  nsAutoPtr<mozilla::dom::indexedDB::ThreadLocal> mIndexedDBThreadLocal;

public:
  ThreadLocal();

private:
  
  ~ThreadLocal();
};

} 
} 

#endif 
