





#include "mozilla/dom/cache/CacheStorageChild.h"

#include "mozilla/unused.h"
#include "mozilla/dom/cache/CacheChild.h"
#include "mozilla/dom/cache/CacheOpChild.h"
#include "mozilla/dom/cache/CacheStorage.h"
#include "mozilla/dom/cache/StreamUtils.h"

namespace mozilla {
namespace dom {
namespace cache {


void
DeallocPCacheStorageChild(PCacheStorageChild* aActor)
{
  delete aActor;
}

CacheStorageChild::CacheStorageChild(CacheStorage* aListener, Feature* aFeature)
  : mListener(aListener)
  , mNumChildActors(0)
{
  MOZ_COUNT_CTOR(cache::CacheStorageChild);
  MOZ_ASSERT(mListener);

  SetFeature(aFeature);
}

CacheStorageChild::~CacheStorageChild()
{
  MOZ_COUNT_DTOR(cache::CacheStorageChild);
  NS_ASSERT_OWNINGTHREAD(CacheStorageChild);
  MOZ_ASSERT(!mListener);
}

void
CacheStorageChild::ClearListener()
{
  NS_ASSERT_OWNINGTHREAD(CacheStorageChild);
  MOZ_ASSERT(mListener);
  mListener = nullptr;
}

void
CacheStorageChild::ExecuteOp(nsIGlobalObject* aGlobal, Promise* aPromise,
                             const CacheOpArgs& aArgs)
{
  mNumChildActors += 1;
  unused << SendPCacheOpConstructor(
    new CacheOpChild(GetFeature(), aGlobal, aPromise), aArgs);
}

void
CacheStorageChild::StartDestroy()
{
  NS_ASSERT_OWNINGTHREAD(CacheStorageChild);

  nsRefPtr<CacheStorage> listener = mListener;

  
  
  
  if (!listener) {
    return;
  }

  listener->DestroyInternal(this);

  
  MOZ_ASSERT(!mListener);

  
  
  
  
  if (mNumChildActors) {
    return;
  }

  
  unused << SendTeardown();
}

void
CacheStorageChild::ActorDestroy(ActorDestroyReason aReason)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorageChild);
  nsRefPtr<CacheStorage> listener = mListener;
  if (listener) {
    listener->DestroyInternal(this);
    
    MOZ_ASSERT(!mListener);
  }

  RemoveFeature();
}

PCacheOpChild*
CacheStorageChild::AllocPCacheOpChild(const CacheOpArgs& aOpArgs)
{
  MOZ_CRASH("CacheOpChild should be manually constructed.");
  return nullptr;
}

bool
CacheStorageChild::DeallocPCacheOpChild(PCacheOpChild* aActor)
{
  delete aActor;
  NoteDeletedActor();
  return true;
}

void
CacheStorageChild::NoteDeletedActor()
{
  MOZ_ASSERT(mNumChildActors);
  mNumChildActors -= 1;
  if (!mNumChildActors && !mListener) {
    unused << SendTeardown();
  }
}

} 
} 
} 
