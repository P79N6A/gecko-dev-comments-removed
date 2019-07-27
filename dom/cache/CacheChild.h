





#ifndef mozilla_dom_cache_CacheChild_h
#define mozilla_dom_cache_CacheChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCacheChild.h"

class nsIAsyncInputStream;
class nsIGlobalObject;

namespace mozilla {
namespace dom {

class Promise;

namespace cache {

class Cache;
class CacheOpArgs;
class CachePushStreamChild;

class CacheChild final : public PCacheChild
                       , public ActorChild
{
public:
  CacheChild();
  ~CacheChild();

  void SetListener(Cache* aListener);

  
  
  
  void ClearListener();

  void
  ExecuteOp(nsIGlobalObject* aGlobal, Promise* aPromise,
            const CacheOpArgs& aArgs);

  CachePushStreamChild*
  CreatePushStream(nsIAsyncInputStream* aStream);

  

  
  
  virtual void StartDestroy() override;

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  virtual PCacheOpChild*
  AllocPCacheOpChild(const CacheOpArgs& aOpArgs) override;

  virtual bool
  DeallocPCacheOpChild(PCacheOpChild* aActor) override;

  virtual PCachePushStreamChild*
  AllocPCachePushStreamChild() override;

  virtual bool
  DeallocPCachePushStreamChild(PCachePushStreamChild* aActor) override;

  
  void
  NoteDeletedActor();

  
  
  
  Cache* MOZ_NON_OWNING_REF mListener;
  uint32_t mNumChildActors;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
