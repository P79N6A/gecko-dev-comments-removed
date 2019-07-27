





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

class CacheStorageChild final : public PCacheStorageChild
                              , public ActorChild
{
public:
  CacheStorageChild(CacheStorage* aListener, Feature* aFeature);
  ~CacheStorageChild();

  
  
  
  
  void ClearListener();

  

  
  
  virtual void StartDestroy() override;

private:
  
  virtual void ActorDestroy(ActorDestroyReason aReason) override;

  virtual bool RecvMatchResponse(const RequestId& aRequestId,
                                 const nsresult& aRv,
                                 const PCacheResponseOrVoid& response) override;
  virtual bool RecvHasResponse(const cache::RequestId& aRequestId,
                               const nsresult& aRv,
                               const bool& aSuccess) override;
  virtual bool RecvOpenResponse(const cache::RequestId& aRequestId,
                                const nsresult& aRv,
                                PCacheChild* aActor) override;
  virtual bool RecvDeleteResponse(const cache::RequestId& aRequestId,
                                  const nsresult& aRv,
                                  const bool& aResult) override;
  virtual bool RecvKeysResponse(const cache::RequestId& aRequestId,
                                const nsresult& aRv,
                                nsTArray<nsString>&& aKeys) override;

  
  
  
  CacheStorage* MOZ_NON_OWNING_REF mListener;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
