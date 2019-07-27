





#ifndef mozilla_dom_cache_CacheChild_h
#define mozilla_dom_cache_CacheChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCacheChild.h"

namespace mozilla {
namespace dom {
namespace cache {

class Cache;

class CacheChild final : public PCacheChild
                       , public ActorChild
{
public:
  CacheChild();
  ~CacheChild();

  void SetListener(Cache* aListener);

  
  
  
  void ClearListener();

  

  
  
  virtual void StartDestroy() override;

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  virtual PCachePushStreamChild*
  AllocPCachePushStreamChild() override;

  virtual bool
  DeallocPCachePushStreamChild(PCachePushStreamChild* aActor) override;

  virtual bool
  RecvMatchResponse(const RequestId& requestId, const nsresult& aRv,
                    const PCacheResponseOrVoid& aResponse) override;
  virtual bool
  RecvMatchAllResponse(const RequestId& requestId, const nsresult& aRv,
                       nsTArray<PCacheResponse>&& responses) override;
  virtual bool
  RecvAddAllResponse(const RequestId& requestId,
                     const nsresult& aRv) override;
  virtual bool
  RecvPutResponse(const RequestId& aRequestId,
                  const nsresult& aRv) override;
  virtual bool
  RecvDeleteResponse(const RequestId& requestId, const nsresult& aRv,
                     const bool& result) override;
  virtual bool
  RecvKeysResponse(const RequestId& requestId, const nsresult& aRv,
                   nsTArray<PCacheRequest>&& requests) override;

  
  
  
  Cache* MOZ_NON_OWNING_REF mListener;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
