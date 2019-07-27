





#include "mozilla/dom/cache/CacheChild.h"

#include "mozilla/unused.h"
#include "mozilla/dom/cache/ActorUtils.h"
#include "mozilla/dom/cache/Cache.h"
#include "mozilla/dom/cache/CacheOpChild.h"
#include "mozilla/dom/cache/CachePushStreamChild.h"

namespace mozilla {
namespace dom {
namespace cache {


PCacheChild*
AllocPCacheChild()
{
  return new CacheChild();
}


void
DeallocPCacheChild(PCacheChild* aActor)
{
  delete aActor;
}

CacheChild::CacheChild()
  : mListener(nullptr)
  , mNumChildActors(0)
  , mDelayedDestroy(false)
{
  MOZ_COUNT_CTOR(cache::CacheChild);
}

CacheChild::~CacheChild()
{
  MOZ_COUNT_DTOR(cache::CacheChild);
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  MOZ_ASSERT(!mListener);
  MOZ_ASSERT(!mNumChildActors);
}

void
CacheChild::SetListener(Cache* aListener)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  MOZ_ASSERT(!mListener);
  mListener = aListener;
  MOZ_ASSERT(mListener);
}

void
CacheChild::ClearListener()
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  MOZ_ASSERT(mListener);
  mListener = nullptr;
}

void
CacheChild::ExecuteOp(nsIGlobalObject* aGlobal, Promise* aPromise,
                      nsISupports* aParent, const CacheOpArgs& aArgs)
{
  mNumChildActors += 1;
  MOZ_ALWAYS_TRUE(SendPCacheOpConstructor(
    new CacheOpChild(GetFeature(), aGlobal, aParent, aPromise), aArgs));
}

CachePushStreamChild*
CacheChild::CreatePushStream(nsIAsyncInputStream* aStream)
{
  mNumChildActors += 1;
  auto actor = SendPCachePushStreamConstructor(
    new CachePushStreamChild(GetFeature(), aStream));
  MOZ_ASSERT(actor);
  return static_cast<CachePushStreamChild*>(actor);
}

void
CacheChild::StartDestroyFromListener()
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);

  
  
  
  MOZ_ASSERT(!mNumChildActors);

  StartDestroy();
}

void
CacheChild::StartDestroy()
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);

  
  
  
  
  if (mNumChildActors) {
    mDelayedDestroy = true;
    return;
  }

  nsRefPtr<Cache> listener = mListener;

  
  
  
  if (!listener) {
    return;
  }

  listener->DestroyInternal(this);

  
  MOZ_ASSERT(!mListener);

  
  unused << SendTeardown();
}

void
CacheChild::ActorDestroy(ActorDestroyReason aReason)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  nsRefPtr<Cache> listener = mListener;
  if (listener) {
    listener->DestroyInternal(this);
    
    MOZ_ASSERT(!mListener);
  }

  RemoveFeature();
}

PCacheOpChild*
CacheChild::AllocPCacheOpChild(const CacheOpArgs& aOpArgs)
{
  MOZ_CRASH("CacheOpChild should be manually constructed.");
  return nullptr;
}

bool
CacheChild::DeallocPCacheOpChild(PCacheOpChild* aActor)
{
  delete aActor;
  NoteDeletedActor();
  return true;
}

PCachePushStreamChild*
CacheChild::AllocPCachePushStreamChild()
{
  MOZ_CRASH("CachePushStreamChild should be manually constructed.");
  return nullptr;
}

bool
CacheChild::DeallocPCachePushStreamChild(PCachePushStreamChild* aActor)
{
  delete aActor;
  NoteDeletedActor();
  return true;
}

void
CacheChild::NoteDeletedActor()
{
  mNumChildActors -= 1;
  if (!mNumChildActors && mDelayedDestroy) {
    StartDestroy();
  }
}

} 
} 
} 
