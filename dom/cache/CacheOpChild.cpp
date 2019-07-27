





#include "mozilla/dom/cache/CacheOpChild.h"

#include "mozilla/dom/cache/Cache.h"
#include "mozilla/dom/cache/CacheChild.h"

namespace mozilla {
namespace dom {
namespace cache {

CacheOpChild::CacheOpChild(Feature* aFeature, nsIGlobalObject* aGlobal,
                           nsISupports* aParent, Promise* aPromise)
  : mGlobal(aGlobal)
  , mParent(aParent)
  , mPromise(aPromise)
{
  MOZ_ASSERT(mGlobal);
  MOZ_ASSERT(mParent);
  MOZ_ASSERT(mPromise);

  MOZ_ASSERT_IF(!NS_IsMainThread(), aFeature);
  SetFeature(aFeature);
}

CacheOpChild::~CacheOpChild()
{
  NS_ASSERT_OWNINGTHREAD(CacheOpChild);
  MOZ_ASSERT(!mPromise);
}

void
CacheOpChild::ActorDestroy(ActorDestroyReason aReason)
{
  NS_ASSERT_OWNINGTHREAD(CacheOpChild);

  
  
  if (mPromise) {
    mPromise->MaybeReject(NS_ERROR_FAILURE);
    mPromise = nullptr;
  }

  RemoveFeature();
}

bool
CacheOpChild::Recv__delete__(const ErrorResult& aRv,
                             const CacheOpResult& aResult)
{
  NS_ASSERT_OWNINGTHREAD(CacheOpChild);

  if (aRv.Failed()) {
    
    
    
    mPromise->MaybeReject(const_cast<ErrorResult&>(aRv));
    mPromise = nullptr;
    return true;
  }

  switch (aResult.type()) {
    case CacheOpResult::TCacheMatchResult:
    {
      HandleResponse(aResult.get_CacheMatchResult().responseOrVoid());
      break;
    }
    case CacheOpResult::TCacheMatchAllResult:
    {
      HandleResponseList(aResult.get_CacheMatchAllResult().responseList());
      break;
    }
    case CacheOpResult::TCacheAddAllResult:
    case CacheOpResult::TCachePutAllResult:
    {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
      break;
    }
    case CacheOpResult::TCacheDeleteResult:
    {
      mPromise->MaybeResolve(aResult.get_CacheDeleteResult().success());
      break;
    }
    case CacheOpResult::TCacheKeysResult:
    {
      HandleRequestList(aResult.get_CacheKeysResult().requestList());
      break;
    }
    case CacheOpResult::TStorageMatchResult:
    {
      HandleResponse(aResult.get_StorageMatchResult().responseOrVoid());
      break;
    }
    case CacheOpResult::TStorageHasResult:
    {
      mPromise->MaybeResolve(aResult.get_StorageHasResult().success());
      break;
    }
    case CacheOpResult::TStorageOpenResult:
    {
      auto actor = static_cast<CacheChild*>(
        aResult.get_StorageOpenResult().actorChild());
      actor->SetFeature(GetFeature());
      nsRefPtr<Cache> cache = new Cache(mGlobal, actor);
      mPromise->MaybeResolve(cache);
      break;
    }
    case CacheOpResult::TStorageDeleteResult:
    {
      mPromise->MaybeResolve(aResult.get_StorageDeleteResult().success());
      break;
    }
    case CacheOpResult::TStorageKeysResult:
    {
      mPromise->MaybeResolve(aResult.get_StorageKeysResult().keyList());
      break;
    }
    default:
      MOZ_CRASH("Unknown Cache op result type!");
  }

  mPromise = nullptr;

  return true;
}

void
CacheOpChild::StartDestroy()
{
  NS_ASSERT_OWNINGTHREAD(CacheOpChild);

  
  
}

nsIGlobalObject*
CacheOpChild::GetGlobalObject() const
{
  return mGlobal;
}

#ifdef DEBUG
void
CacheOpChild::AssertOwningThread() const
{
  NS_ASSERT_OWNINGTHREAD(CacheOpChild);
}
#endif

CachePushStreamChild*
CacheOpChild::CreatePushStream(nsIAsyncInputStream* aStream)
{
  MOZ_CRASH("CacheOpChild should never create a push stream actor!");
}

void
CacheOpChild::HandleResponse(const CacheResponseOrVoid& aResponseOrVoid)
{
  nsRefPtr<Response> response;
  if (aResponseOrVoid.type() == CacheResponseOrVoid::TCacheResponse) {
    response = ToResponse(aResponseOrVoid);
  }

  if (!response) {
    mPromise->MaybeResolve(JS::UndefinedHandleValue);
    return;
  }

  mPromise->MaybeResolve(response);
}

void
CacheOpChild::HandleResponseList(const nsTArray<CacheResponse>& aResponseList)
{
  nsAutoTArray<nsRefPtr<Response>, 256> responses;
  responses.SetCapacity(aResponseList.Length());

  for (uint32_t i = 0; i < aResponseList.Length(); ++i) {
    responses.AppendElement(ToResponse(aResponseList[i]));
  }

  mPromise->MaybeResolve(responses);
}

void
CacheOpChild::HandleRequestList(const nsTArray<CacheRequest>& aRequestList)
{
  nsAutoTArray<nsRefPtr<Request>, 256> requests;
  requests.SetCapacity(aRequestList.Length());

  for (uint32_t i = 0; i < aRequestList.Length(); ++i) {
    requests.AppendElement(ToRequest(aRequestList[i]));
  }

  mPromise->MaybeResolve(requests);
}

} 
} 
} 
