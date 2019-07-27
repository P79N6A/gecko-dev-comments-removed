





#include "mozilla/dom/cache/CacheChild.h"

#include "mozilla/unused.h"
#include "mozilla/dom/cache/ActorUtils.h"
#include "mozilla/dom/cache/Cache.h"
#include "mozilla/dom/cache/PCachePushStreamChild.h"
#include "mozilla/dom/cache/StreamUtils.h"

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
{
  MOZ_COUNT_CTOR(cache::CacheChild);
}

CacheChild::~CacheChild()
{
  MOZ_COUNT_DTOR(cache::CacheChild);
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  MOZ_ASSERT(!mListener);
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
CacheChild::StartDestroy()
{
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
  return true;
}

bool
CacheChild::RecvMatchResponse(const RequestId& requestId, const nsresult& aRv,
                              const PCacheResponseOrVoid& aResponse)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);

  AddFeatureToStreamChild(aResponse, GetFeature());

  nsRefPtr<Cache> listener = mListener;
  if (!listener) {
    StartDestroyStreamChild(aResponse);
    return true;
  }

  listener->RecvMatchResponse(requestId, aRv, aResponse);
  return true;
}

bool
CacheChild::RecvMatchAllResponse(const RequestId& requestId, const nsresult& aRv,
                                 nsTArray<PCacheResponse>&& aResponses)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);

  AddFeatureToStreamChild(aResponses, GetFeature());

  nsRefPtr<Cache> listener = mListener;
  if (!listener) {
    StartDestroyStreamChild(aResponses);
    return true;
  }

  listener->RecvMatchAllResponse(requestId, aRv, aResponses);
  return true;
}

bool
CacheChild::RecvAddAllResponse(const RequestId& requestId,
                               const mozilla::ErrorResult& aError)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  nsRefPtr<Cache> listener = mListener;
  if (listener) {
    listener->RecvAddAllResponse(requestId, aError);
  }
  return true;
}

bool
CacheChild::RecvPutResponse(const RequestId& aRequestId, const nsresult& aRv)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  nsRefPtr<Cache> listener = mListener;
  if (listener) {
    listener->RecvPutResponse(aRequestId, aRv);
  }
  return true;
}

bool
CacheChild::RecvDeleteResponse(const RequestId& requestId, const nsresult& aRv,
                               const bool& result)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);
  nsRefPtr<Cache> listener = mListener;
  if (listener) {
    listener->RecvDeleteResponse(requestId, aRv, result);
  }
  return true;
}

bool
CacheChild::RecvKeysResponse(const RequestId& requestId, const nsresult& aRv,
                             nsTArray<PCacheRequest>&& aRequests)
{
  NS_ASSERT_OWNINGTHREAD(CacheChild);

  AddFeatureToStreamChild(aRequests, GetFeature());

  nsRefPtr<Cache> listener = mListener;
  if (!listener) {
    StartDestroyStreamChild(aRequests);
    return true;
  }

  listener->RecvKeysResponse(requestId, aRv, aRequests);
  return true;
}

} 
} 
} 
