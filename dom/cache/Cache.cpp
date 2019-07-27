





#include "mozilla/dom/cache/Cache.h"

#include "mozilla/dom/Headers.h"
#include "mozilla/dom/InternalResponse.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/Response.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/CacheBinding.h"
#include "mozilla/dom/cache/AutoUtils.h"
#include "mozilla/dom/cache/CacheChild.h"
#include "mozilla/dom/cache/CacheOpChild.h"
#include "mozilla/dom/cache/CachePushStreamChild.h"
#include "mozilla/dom/cache/ReadStream.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "nsIGlobalObject.h"
#include "nsNetUtil.h"

namespace {

using mozilla::ErrorResult;
using mozilla::dom::MSG_INVALID_REQUEST_METHOD;
using mozilla::dom::OwningRequestOrUSVString;
using mozilla::dom::Request;
using mozilla::dom::RequestOrUSVString;

static bool
IsValidPutRequestMethod(const Request& aRequest, ErrorResult& aRv)
{
  nsAutoCString method;
  aRequest.GetMethod(method);
  bool valid = method.LowerCaseEqualsLiteral("get");
  if (!valid) {
    NS_ConvertASCIItoUTF16 label(method);
    aRv.ThrowTypeError(MSG_INVALID_REQUEST_METHOD, &label);
  }
  return valid;
}

static bool
IsValidPutRequestMethod(const RequestOrUSVString& aRequest,
                        ErrorResult& aRv)
{
  
  
  if (!aRequest.IsRequest()) {
    return true;
  }
  return IsValidPutRequestMethod(aRequest.GetAsRequest(), aRv);
}

static bool
IsValidPutRequestMethod(const OwningRequestOrUSVString& aRequest,
                        ErrorResult& aRv)
{
  if (!aRequest.IsRequest()) {
    return true;
  }
  return IsValidPutRequestMethod(*aRequest.GetAsRequest().get(), aRv);
}

} 

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::ErrorResult;
using mozilla::unused;
using mozilla::dom::workers::GetCurrentThreadWorkerPrivate;
using mozilla::dom::workers::WorkerPrivate;

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozilla::dom::cache::Cache);
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozilla::dom::cache::Cache);
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(mozilla::dom::cache::Cache, mGlobal);

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Cache)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Cache::Cache(nsIGlobalObject* aGlobal, CacheChild* aActor)
  : mGlobal(aGlobal)
  , mActor(aActor)
{
  MOZ_ASSERT(mGlobal);
  MOZ_ASSERT(mActor);
  mActor->SetListener(this);
}

already_AddRefed<Promise>
Cache::Match(const RequestOrUSVString& aRequest,
             const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, IgnoreBody, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  PCacheQueryParams params;
  ToPCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheMatchArgs(PCacheRequest(), params));

  args.Add(ir, IgnoreBody, PassThroughReferrer, IgnoreInvalidScheme, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::MatchAll(const Optional<RequestOrUSVString>& aRequest,
                const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  PCacheQueryParams params;
  ToPCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheMatchAllArgs(void_t(), params));

  if (aRequest.WasPassed()) {
    nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest.Value(),
                                                     IgnoreBody, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    args.Add(ir, IgnoreBody, PassThroughReferrer, IgnoreInvalidScheme, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Add(const RequestOrUSVString& aRequest, ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  if (!IsValidPutRequestMethod(aRequest, aRv)) {
    return nullptr;
  }

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, ReadBody, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  AutoChildOpArgs args(this, CacheAddAllArgs());

  args.Add(ir, ReadBody, ExpandReferrer, NetworkErrorOnInvalidScheme, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::AddAll(const Sequence<OwningRequestOrUSVString>& aRequests,
              ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  
  if (aRequests.IsEmpty()) {
    nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
    if (!promise) {
      return nullptr;
    }

    promise->MaybeResolve(JS::UndefinedHandleValue);
    return promise.forget();
  }

  AutoChildOpArgs args(this, CacheAddAllArgs());

  for (uint32_t i = 0; i < aRequests.Length(); ++i) {
    if (!IsValidPutRequestMethod(aRequests[i], aRv)) {
      return nullptr;
    }

    nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequests[i], ReadBody,
                                                     aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    args.Add(ir, ReadBody, ExpandReferrer, NetworkErrorOnInvalidScheme, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Put(const RequestOrUSVString& aRequest, Response& aResponse,
           ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  if (!IsValidPutRequestMethod(aRequest, aRv)) {
    return nullptr;
  }

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, ReadBody, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  AutoChildOpArgs args(this, CachePutAllArgs());

  args.Add(ir, ReadBody, PassThroughReferrer, TypeErrorOnInvalidScheme,
           aResponse, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Delete(const RequestOrUSVString& aRequest,
              const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, IgnoreBody, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  PCacheQueryParams params;
  ToPCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheDeleteArgs(PCacheRequest(), params));

  args.Add(ir, IgnoreBody, PassThroughReferrer, IgnoreInvalidScheme, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Keys(const Optional<RequestOrUSVString>& aRequest,
            const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  PCacheQueryParams params;
  ToPCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheKeysArgs(void_t(), params));

  if (aRequest.WasPassed()) {
    nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest.Value(),
                                                     IgnoreBody, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    args.Add(ir, IgnoreBody, PassThroughReferrer, IgnoreInvalidScheme, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }

  return ExecuteOp(args, aRv);
}


bool
Cache::PrefEnabled(JSContext* aCx, JSObject* aObj)
{
  using mozilla::dom::workers::WorkerPrivate;
  using mozilla::dom::workers::GetWorkerPrivateFromContext;

  
  if (NS_IsMainThread()) {
    bool enabled = false;
    Preferences::GetBool("dom.caches.enabled", &enabled);
    return enabled;
  }

  
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  if (!workerPrivate) {
    return false;
  }

  return workerPrivate->DOMCachesEnabled();
}

nsISupports*
Cache::GetParentObject() const
{
  return mGlobal;
}

JSObject*
Cache::WrapObject(JSContext* aContext, JS::Handle<JSObject*> aGivenProto)
{
  return CacheBinding::Wrap(aContext, this, aGivenProto);
}

void
Cache::DestroyInternal(CacheChild* aActor)
{
  MOZ_ASSERT(mActor);
  MOZ_ASSERT(mActor == aActor);
  mActor->ClearListener();
  mActor = nullptr;
}

nsIGlobalObject*
Cache::GetGlobalObject() const
{
  return mGlobal;
}

#ifdef DEBUG
void
Cache::AssertOwningThread() const
{
  NS_ASSERT_OWNINGTHREAD(Cache);
}
#endif

CachePushStreamChild*
Cache::CreatePushStream(nsIAsyncInputStream* aStream)
{
  NS_ASSERT_OWNINGTHREAD(Cache);
  MOZ_ASSERT(mActor);
  MOZ_ASSERT(aStream);
  auto actor = mActor->SendPCachePushStreamConstructor(
    new CachePushStreamChild(mActor->GetFeature(), aStream));
  MOZ_ASSERT(actor);
  return static_cast<CachePushStreamChild*>(actor);
}

Cache::~Cache()
{
  NS_ASSERT_OWNINGTHREAD(Cache);
  if (mActor) {
    mActor->StartDestroy();
    
    
    MOZ_ASSERT(!mActor);
  }
}

already_AddRefed<Promise>
Cache::ExecuteOp(AutoChildOpArgs& aOpArgs, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (!promise) {
    return nullptr;
  }

  unused << mActor->SendPCacheOpConstructor(
    new CacheOpChild(mActor->GetFeature(), mGlobal, this, promise),
    aOpArgs.SendAsOpArgs());

  return promise.forget();
}

} 
} 
} 
