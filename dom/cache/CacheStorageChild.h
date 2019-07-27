





#ifndef mozilla_dom_cache_CacheStorageChild_h
#define mozilla_dom_cache_CacheStorageChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/dom/cache/PCacheStorageChild.h"

namespace mozilla {
namespace dom {
namespace cache {

class CacheStorage;
class PCacheChild;
class Feature;

class CacheStorageChild MOZ_FINAL : public PCacheStorageChild
                                  , public ActorChild
{
public:
  CacheStorageChild(CacheStorage* aListener, Feature* aFeature);
  ~CacheStorageChild();

  
  
  
  
  void ClearListener();

  

  
  
  virtual void StartDestroy() MOZ_OVERRIDE;

private:
  
  virtual void ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE;
  virtual bool RecvMatchResponse(const RequestId& aRequestId,
                                 const nsresult& aRv,
                                 const PCacheResponseOrVoid& response) MOZ_OVERRIDE;
  virtual bool RecvHasResponse(const cache::RequestId& aRequestId,
                               const nsresult& aRv,
                               const bool& aSuccess) MOZ_OVERRIDE;
  virtual bool RecvOpenResponse(const cache::RequestId& aRequestId,
                                const nsresult& aRv,
                                PCacheChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvDeleteResponse(const cache::RequestId& aRequestId,
                                  const nsresult& aRv,
                                  const bool& aResult) MOZ_OVERRIDE;
  virtual bool RecvKeysResponse(const cache::RequestId& aRequestId,
                                const nsresult& aRv,
                                nsTArray<nsString>&& aKeys) MOZ_OVERRIDE;

  
  
  
  CacheStorage* MOZ_NON_OWNING_REF mListener;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
