





#ifndef mozilla_dom_cache_CacheParent_h
#define mozilla_dom_cache_CacheParent_h

#include "mozilla/dom/cache/PCacheParent.h"
#include "mozilla/dom/cache/Types.h"

namespace mozilla {
namespace dom {
namespace cache {

class Manager;

class CacheParent final : public PCacheParent
{
public:
  CacheParent(cache::Manager* aManager, CacheId aCacheId);
  virtual ~CacheParent();

private:
  
  virtual void ActorDestroy(ActorDestroyReason aReason) override;

  virtual PCacheOpParent*
  AllocPCacheOpParent(const CacheOpArgs& aOpArgs) override;

  virtual bool
  DeallocPCacheOpParent(PCacheOpParent* aActor) override;

  virtual bool
  RecvPCacheOpConstructor(PCacheOpParent* actor,
                          const CacheOpArgs& aOpArgs) override;

  virtual PCachePushStreamParent*
  AllocPCachePushStreamParent() override;

  virtual bool
  DeallocPCachePushStreamParent(PCachePushStreamParent* aActor) override;

  virtual bool
  RecvTeardown() override;

  nsRefPtr<cache::Manager> mManager;
  const CacheId mCacheId;
};

} 
} 
} 

#endif 
