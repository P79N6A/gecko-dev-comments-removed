





#ifndef mozilla_dom_cache_CacheOpParent_h
#define mozilla_dom_cache_CacheOpParent_h

#include "mozilla/dom/cache/FetchPut.h"
#include "mozilla/dom/cache/Manager.h"
#include "mozilla/dom/cache/PCacheOpParent.h"
#include "mozilla/dom/cache/PrincipalVerifier.h"
#include "nsTArray.h"

namespace mozilla {
namespace ipc {
class PBackgroundParent;
}
namespace dom {
namespace cache {

class CacheOpParent final : public PCacheOpParent
                          , public PrincipalVerifier::Listener
                          , public Manager::Listener
                          , public FetchPut::Listener
{
  
  using Manager::Listener::OnOpComplete;

public:
  CacheOpParent(mozilla::ipc::PBackgroundParent* aIpcManager, CacheId aCacheId,
                const CacheOpArgs& aOpArgs);
  CacheOpParent(mozilla::ipc::PBackgroundParent* aIpcManager,
                Namespace aNamespace, const CacheOpArgs& aOpArgs);
  ~CacheOpParent();

  void
  Execute(ManagerId* aManagerId);

  void
  Execute(Manager* aManager);

  void
  WaitForVerification(PrincipalVerifier* aVerifier);

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  
  virtual void
  OnPrincipalVerified(nsresult aRv, ManagerId* aManagerId) override;

  
  virtual void
  OnOpComplete(ErrorResult&& aRv, const CacheOpResult& aResult,
               CacheId aOpenedCacheId,
               const nsTArray<SavedResponse>& aSavedResponseList,
               const nsTArray<SavedRequest>& aSavedRequestList,
               StreamList* aStreamList) override;

  
  virtual void
  OnFetchPut(FetchPut* aFetchPut, ErrorResult&& aRv) override;

  
  already_AddRefed<nsIInputStream>
  DeserializeCacheStream(const PCacheReadStreamOrVoid& aStreamOrVoid);

  mozilla::ipc::PBackgroundParent* mIpcManager;
  const CacheId mCacheId;
  const Namespace mNamespace;
  const CacheOpArgs mOpArgs;
  nsRefPtr<Manager> mManager;
  nsRefPtr<PrincipalVerifier> mVerifier;
  nsTArray<nsRefPtr<FetchPut>> mFetchPutList;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
