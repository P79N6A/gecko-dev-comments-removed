



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
  ProcessingError(Result aCode, const char* aReason) override;

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBackgroundTestChild*
  AllocPBackgroundTestChild(const nsCString& aTestArg) override;

  virtual bool
  DeallocPBackgroundTestChild(PBackgroundTestChild* aActor) override;

  virtual PBackgroundIDBFactoryChild*
  AllocPBackgroundIDBFactoryChild(const LoggingInfo& aLoggingInfo) override;

  virtual bool
  DeallocPBackgroundIDBFactoryChild(PBackgroundIDBFactoryChild* aActor)
                                    override;

  virtual PBlobChild*
  AllocPBlobChild(const BlobConstructorParams& aParams) override;

  virtual bool
  DeallocPBlobChild(PBlobChild* aActor) override;

  virtual PFileDescriptorSetChild*
  AllocPFileDescriptorSetChild(const FileDescriptor& aFileDescriptor)
                               override;

  virtual bool
  DeallocPFileDescriptorSetChild(PFileDescriptorSetChild* aActor) override;

  virtual PMediaChild*
  AllocPMediaChild() override;

  virtual bool
  DeallocPMediaChild(PMediaChild* aActor) override;

  virtual PVsyncChild*
  AllocPVsyncChild() override;

  virtual bool
  DeallocPVsyncChild(PVsyncChild* aActor) override;

  virtual PBroadcastChannelChild*
  AllocPBroadcastChannelChild(const PrincipalInfo& aPrincipalInfo,
                              const nsString& aOrigin,
                              const nsString& aChannel) override;

  virtual bool
  DeallocPBroadcastChannelChild(PBroadcastChannelChild* aActor) override;

  virtual dom::cache::PCacheStorageChild*
  AllocPCacheStorageChild(const dom::cache::Namespace& aNamespace,
                          const PrincipalInfo& aPrincipalInfo) override;

  virtual bool
  DeallocPCacheStorageChild(dom::cache::PCacheStorageChild* aActor) override;

  virtual dom::cache::PCacheChild* AllocPCacheChild() override;

  virtual bool
  DeallocPCacheChild(dom::cache::PCacheChild* aActor) override;

  virtual dom::cache::PCacheStreamControlChild*
  AllocPCacheStreamControlChild() override;

  virtual bool
  DeallocPCacheStreamControlChild(dom::cache::PCacheStreamControlChild* aActor) override;
};

class BackgroundChildImpl::ThreadLocal final
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
