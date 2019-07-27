



#ifndef mozilla_ipc_backgroundchildimpl_h__
#define mozilla_ipc_backgroundchildimpl_h__

#include "mozilla/Attributes.h"
#include "mozilla/ipc/PBackgroundChild.h"

template <class> class nsAutoPtr;

namespace mozilla {
namespace dom {
namespace indexedDB {

class IDBTransaction;

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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBackgroundTestChild*
  AllocPBackgroundTestChild(const nsCString& aTestArg) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundTestChild(PBackgroundTestChild* aActor) MOZ_OVERRIDE;

  virtual PBackgroundIDBFactoryChild*
  AllocPBackgroundIDBFactoryChild(const OptionalWindowId& aOptionalWindowId)
                                  MOZ_OVERRIDE;

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
};

class BackgroundChildImpl::ThreadLocal MOZ_FINAL
{
  friend class nsAutoPtr<ThreadLocal>;

public:
  mozilla::dom::indexedDB::IDBTransaction* mCurrentTransaction;

#ifdef MOZ_ENABLE_PROFILER_SPS
  uint64_t mNextTransactionSerialNumber;
  uint64_t mNextRequestSerialNumber;
#endif

public:
  ThreadLocal();

private:
  
  ~ThreadLocal();
};

} 
} 

#endif 
