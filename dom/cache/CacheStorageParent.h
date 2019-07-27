





#ifndef mozilla_dom_cache_CacheStorageParent_h
#define mozilla_dom_cache_CacheStorageParent_h

#include "mozilla/dom/cache/PCacheStorageParent.h"
#include "mozilla/dom/cache/PrincipalVerifier.h"
#include "mozilla/dom/cache/Types.h"

namespace mozilla {
namespace dom {
namespace cache {

class ManagerId;

class CacheStorageParent final : public PCacheStorageParent
                               , public PrincipalVerifier::Listener
{
public:
  CacheStorageParent(PBackgroundParent* aManagingActor, Namespace aNamespace,
                     const mozilla::ipc::PrincipalInfo& aPrincipalInfo);
  virtual ~CacheStorageParent();

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  virtual PCacheOpParent*
  AllocPCacheOpParent(const CacheOpArgs& aOpArgs) override;

  virtual bool
  DeallocPCacheOpParent(PCacheOpParent* aActor) override;

  virtual bool
  RecvPCacheOpConstructor(PCacheOpParent* actor,
                          const CacheOpArgs& aOpArgs) override;

  virtual bool
  RecvTeardown() override;

  
  virtual void OnPrincipalVerified(nsresult aRv,
                                   ManagerId* aManagerId) override;

  const Namespace mNamespace;
  nsRefPtr<PrincipalVerifier> mVerifier;
  nsresult mVerifiedStatus;
  nsRefPtr<ManagerId> mManagerId;
};

} 
} 
} 

#endif 
