





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

  virtual PCacheOpChild*
  AllocPCacheOpChild(const CacheOpArgs& aOpArgs) override;

  virtual bool
  DeallocPCacheOpChild(PCacheOpChild* aActor) override;

  
  
  
  CacheStorage* MOZ_NON_OWNING_REF mListener;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
