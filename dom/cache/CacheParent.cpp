





#include "mozilla/dom/cache/CacheParent.h"

#include "mozilla/dom/cache/CacheOpParent.h"
#include "mozilla/dom/cache/CachePushStreamParent.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
namespace cache {


void
DeallocPCacheParent(PCacheParent* aActor)
{
  delete aActor;
}

CacheParent::CacheParent(cache::Manager* aManager, CacheId aCacheId)
  : mManager(aManager)
  , mCacheId(aCacheId)
{
  MOZ_COUNT_CTOR(cache::CacheParent);
  MOZ_ASSERT(mManager);
  mManager->AddRefCacheId(mCacheId);
}

CacheParent::~CacheParent()
{
  MOZ_COUNT_DTOR(cache::CacheParent);
  MOZ_ASSERT(!mManager);
}

void
CacheParent::ActorDestroy(ActorDestroyReason aReason)
{
  MOZ_ASSERT(mManager);
  mManager->ReleaseCacheId(mCacheId);
  mManager = nullptr;
}

PCacheOpParent*
CacheParent::AllocPCacheOpParent(const CacheOpArgs& aOpArgs)
{
  if (aOpArgs.type() != CacheOpArgs::TCacheMatchArgs &&
      aOpArgs.type() != CacheOpArgs::TCacheMatchAllArgs &&
      aOpArgs.type() != CacheOpArgs::TCacheAddAllArgs &&
      aOpArgs.type() != CacheOpArgs::TCachePutAllArgs &&
      aOpArgs.type() != CacheOpArgs::TCacheDeleteArgs &&
      aOpArgs.type() != CacheOpArgs::TCacheKeysArgs)
  {
    MOZ_CRASH("Invalid operation sent to Cache actor!");
  }

  return new CacheOpParent(Manager(), mCacheId, aOpArgs);
}

bool
CacheParent::DeallocPCacheOpParent(PCacheOpParent* aActor)
{
  delete aActor;
  return true;
}

bool
CacheParent::RecvPCacheOpConstructor(PCacheOpParent* aActor,
                                     const CacheOpArgs& aOpArgs)
{
  auto actor = static_cast<CacheOpParent*>(aActor);
  actor->Execute(mManager);
  return true;
}

PCachePushStreamParent*
CacheParent::AllocPCachePushStreamParent()
{
  return CachePushStreamParent::Create();
}

bool
CacheParent::DeallocPCachePushStreamParent(PCachePushStreamParent* aActor)
{
  delete aActor;
  return true;
}

bool
CacheParent::RecvTeardown()
{
  if (!Send__delete__(this)) {
    
    NS_WARNING("Cache failed to send delete.");
  }
  return true;
}

} 
} 
} 
