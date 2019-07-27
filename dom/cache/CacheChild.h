





#ifndef mozilla_dom_cache_CacheChild_h
#define mozilla_dom_cache_CacheChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCacheChild.h"

namespace mozilla {
namespace dom {
namespace cache {

class Cache;

class CacheChild MOZ_FINAL : public PCacheChild
                           , public ActorChild
{
public:
  CacheChild();
  ~CacheChild();

  void SetListener(Cache* aListener);

  
  
  
  void ClearListener();

  

  
  
  virtual void StartDestroy() MOZ_OVERRIDE;

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE;

  virtual bool
  RecvMatchResponse(const RequestId& requestId, const nsresult& aRv,
                    const PCacheResponseOrVoid& aResponse) MOZ_OVERRIDE;
  virtual bool
  RecvMatchAllResponse(const RequestId& requestId, const nsresult& aRv,
                       nsTArray<PCacheResponse>&& responses) MOZ_OVERRIDE;
  virtual bool
  RecvAddAllResponse(const RequestId& requestId,
                     const nsresult& aRv) MOZ_OVERRIDE;
  virtual bool
  RecvPutResponse(const RequestId& aRequestId,
                  const nsresult& aRv) MOZ_OVERRIDE;
  virtual bool
  RecvDeleteResponse(const RequestId& requestId, const nsresult& aRv,
                     const bool& result) MOZ_OVERRIDE;
  virtual bool
  RecvKeysResponse(const RequestId& requestId, const nsresult& aRv,
                   nsTArray<PCacheRequest>&& requests) MOZ_OVERRIDE;

  
  
  
  Cache* MOZ_NON_OWNING_REF mListener;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
