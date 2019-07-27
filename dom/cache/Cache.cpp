





#include "mozilla/dom/cache/Cache.h"

#include "mozilla/dom/Headers.h"
#include "mozilla/dom/InternalResponse.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseNativeHandler.h"
#include "mozilla/dom/Response.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/CacheBinding.h"
#include "mozilla/dom/cache/AutoUtils.h"
#include "mozilla/dom/cache/CacheChild.h"
#include "mozilla/dom/cache/CachePushStreamChild.h"
#include "mozilla/dom/cache/Feature.h"
#include "mozilla/dom/cache/ReadStream.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "nsIGlobalObject.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::dom::workers::GetCurrentThreadWorkerPrivate;
using mozilla::dom::workers::WorkerPrivate;

namespace {

bool
IsValidPutRequestURL(const nsAString& aUrl, ErrorResult& aRv)
{
  bool validScheme = false;

  
  nsAutoString url(aUrl);

  TypeUtils::ProcessURL(url, &validScheme, nullptr, aRv);
  if (aRv.Failed()) {
    return false;
  }

  if (!validScheme) {
    NS_NAMED_LITERAL_STRING(label, "Request");
    aRv.ThrowTypeError(MSG_INVALID_URL_SCHEME, &label, &url);
    return false;
  }

  return true;
}

static bool
IsValidPutRequestMethod(const Request& aRequest, ErrorResult& aRv)
{
  nsAutoCString method;
  aRequest.GetMethod(method);
  if (!method.LowerCaseEqualsLiteral("get")) {
    NS_ConvertASCIItoUTF16 label(method);
    aRv.ThrowTypeError(MSG_INVALID_REQUEST_METHOD, &label);
    return false;
  }

  return true;
}

static bool
IsValidPutRequestMethod(const RequestOrUSVString& aRequest, ErrorResult& aRv)
{
  
  
  if (!aRequest.IsRequest()) {
    return true;
  }
  return IsValidPutRequestMethod(aRequest.GetAsRequest(), aRv);
}

} 






class Cache::FetchHandler final : public PromiseNativeHandler
{
public:
  FetchHandler(Feature* aFeature, Cache* aCache,
               nsTArray<nsRefPtr<Request>>&& aRequestList, Promise* aPromise)
    : mFeature(aFeature)
    , mCache(aCache)
    , mRequestList(Move(aRequestList))
    , mPromise(aPromise)
  {
    MOZ_ASSERT_IF(!NS_IsMainThread(), mFeature);
    MOZ_ASSERT(mCache);
    MOZ_ASSERT(mPromise);
  }

  virtual void
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override
  {
    NS_ASSERT_OWNINGTHREAD(FetchHandler);

    
    nsRefPtr<Feature> feature;
    feature.swap(mFeature);

    
    
    

    nsAutoTArray<nsRefPtr<Response>, 256> responseList;
    responseList.SetCapacity(mRequestList.Length());

    if (NS_WARN_IF(!JS_IsArrayObject(aCx, aValue))) {
      Fail();
      return;
    }

    JS::Rooted<JSObject*> obj(aCx, &aValue.toObject());

    uint32_t length;
    if (NS_WARN_IF(!JS_GetArrayLength(aCx, obj, &length))) {
      Fail();
      return;
    }

    for (uint32_t i = 0; i < length; ++i) {
      JS::Rooted<JS::Value> value(aCx);

      if (NS_WARN_IF(!JS_GetElement(aCx, obj, i, &value))) {
        Fail();
        return;
      }

      if (NS_WARN_IF(!value.isObject())) {
        Fail();
        return;
      }

      JS::Rooted<JSObject*> responseObj(aCx, &value.toObject());

      nsRefPtr<Response> response;
      nsresult rv = UNWRAP_OBJECT(Response, responseObj, response);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        Fail();
        return;
      }

      if (NS_WARN_IF(response->Type() == ResponseType::Error)) {
        Fail();
        return;
      }

      responseList.AppendElement(Move(response));
    }

    MOZ_ASSERT(mRequestList.Length() == responseList.Length());

    
    ErrorResult result;
    nsRefPtr<Promise> put = mCache->PutAll(mRequestList, responseList, result);
    if (NS_WARN_IF(result.Failed())) {
      
      mPromise->MaybeReject(result);
      return;
    }

    
    
    mPromise->MaybeResolve(put);
  }

  virtual void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override
  {
    NS_ASSERT_OWNINGTHREAD(FetchHandler);
    Fail();
  }

private:
  ~FetchHandler()
  {
  }

  void
  Fail()
  {
    ErrorResult rv;
    rv.ThrowTypeError(MSG_FETCH_FAILED);
    mPromise->MaybeReject(rv);
  }

  nsRefPtr<Feature> mFeature;
  nsRefPtr<Cache> mCache;
  nsTArray<nsRefPtr<Request>> mRequestList;
  nsRefPtr<Promise> mPromise;

  NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS0(Cache::FetchHandler)

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
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, IgnoreBody, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  CacheQueryParams params;
  ToCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheMatchArgs(CacheRequest(), params));

  args.Add(ir, IgnoreBody, IgnoreInvalidScheme, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::MatchAll(const Optional<RequestOrUSVString>& aRequest,
                const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  CacheQueryParams params;
  ToCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheMatchAllArgs(void_t(), params));

  if (aRequest.WasPassed()) {
    nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest.Value(),
                                                     IgnoreBody, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    args.Add(ir, IgnoreBody, IgnoreInvalidScheme, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Add(JSContext* aContext, const RequestOrUSVString& aRequest,
           ErrorResult& aRv)
{
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  if (!IsValidPutRequestMethod(aRequest, aRv)) {
    return nullptr;
  }

  GlobalObject global(aContext, mGlobal->GetGlobalJSObject());
  MOZ_ASSERT(!global.Failed());

  nsTArray<nsRefPtr<Request>> requestList(1);
  nsRefPtr<Request> request = Request::Constructor(global, aRequest,
                                                   RequestInit(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsAutoString url;
  request->GetUrl(url);
  if (NS_WARN_IF(!IsValidPutRequestURL(url, aRv))) {
    return nullptr;
  }

  requestList.AppendElement(Move(request));
  return AddAll(global, Move(requestList), aRv);
}

already_AddRefed<Promise>
Cache::AddAll(JSContext* aContext,
              const Sequence<OwningRequestOrUSVString>& aRequestList,
              ErrorResult& aRv)
{
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  GlobalObject global(aContext, mGlobal->GetGlobalJSObject());
  MOZ_ASSERT(!global.Failed());

  nsTArray<nsRefPtr<Request>> requestList(aRequestList.Length());
  for (uint32_t i = 0; i < aRequestList.Length(); ++i) {
    RequestOrUSVString requestOrString;

    if (aRequestList[i].IsRequest()) {
      requestOrString.SetAsRequest() = aRequestList[i].GetAsRequest();
      if (NS_WARN_IF(!IsValidPutRequestMethod(requestOrString.GetAsRequest(),
                     aRv))) {
        return nullptr;
      }
    } else {
      requestOrString.SetAsUSVString().Rebind(
        aRequestList[i].GetAsUSVString().Data(),
        aRequestList[i].GetAsUSVString().Length());
    }

    nsRefPtr<Request> request = Request::Constructor(global, requestOrString,
                                                     RequestInit(), aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }

    nsAutoString url;
    request->GetUrl(url);
    if (NS_WARN_IF(!IsValidPutRequestURL(url, aRv))) {
      return nullptr;
    }

    requestList.AppendElement(Move(request));
  }

  return AddAll(global, Move(requestList), aRv);
}

already_AddRefed<Promise>
Cache::Put(const RequestOrUSVString& aRequest, Response& aResponse,
           ErrorResult& aRv)
{
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  if (NS_WARN_IF(!IsValidPutRequestMethod(aRequest, aRv))) {
    return nullptr;
  }

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, ReadBody, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  AutoChildOpArgs args(this, CachePutAllArgs());

  args.Add(ir, ReadBody, TypeErrorOnInvalidScheme,
           aResponse, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Delete(const RequestOrUSVString& aRequest,
              const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest, IgnoreBody, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  CacheQueryParams params;
  ToCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheDeleteArgs(CacheRequest(), params));

  args.Add(ir, IgnoreBody, IgnoreInvalidScheme, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return ExecuteOp(args, aRv);
}

already_AddRefed<Promise>
Cache::Keys(const Optional<RequestOrUSVString>& aRequest,
            const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  CacheQueryParams params;
  ToCacheQueryParams(params, aOptions);

  AutoChildOpArgs args(this, CacheKeysArgs(void_t(), params));

  if (aRequest.WasPassed()) {
    nsRefPtr<InternalRequest> ir = ToInternalRequest(aRequest.Value(),
                                                     IgnoreBody, aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }

    args.Add(ir, IgnoreBody, IgnoreInvalidScheme, aRv);
    if (NS_WARN_IF(aRv.Failed())) {
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
  return mActor->CreatePushStream(this, aStream);
}

Cache::~Cache()
{
  NS_ASSERT_OWNINGTHREAD(Cache);
  if (mActor) {
    mActor->StartDestroyFromListener();
    
    
    MOZ_ASSERT(!mActor);
  }
}

already_AddRefed<Promise>
Cache::ExecuteOp(AutoChildOpArgs& aOpArgs, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(!promise)) {
    return nullptr;
  }

  mActor->ExecuteOp(mGlobal, promise, this, aOpArgs.SendAsOpArgs());
  return promise.forget();
}

already_AddRefed<Promise>
Cache::AddAll(const GlobalObject& aGlobal,
              nsTArray<nsRefPtr<Request>>&& aRequestList, ErrorResult& aRv)
{
  MOZ_ASSERT(mActor);

  
  if (aRequestList.IsEmpty()) {
    nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
    if (NS_WARN_IF(!promise)) {
      return nullptr;
    }

    promise->MaybeResolve(JS::UndefinedHandleValue);
    return promise.forget();
  }

  nsAutoTArray<nsRefPtr<Promise>, 256> fetchList;
  fetchList.SetCapacity(aRequestList.Length());

  
  
  

  for (uint32_t i = 0; i < aRequestList.Length(); ++i) {
    RequestOrUSVString requestOrString;
    requestOrString.SetAsRequest() = aRequestList[i];
    nsRefPtr<Promise> fetch = FetchRequest(mGlobal, requestOrString,
                                           RequestInit(), aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }

    fetchList.AppendElement(Move(fetch));
  }

  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<FetchHandler> handler = new FetchHandler(mActor->GetFeature(), this,
                                                    Move(aRequestList), promise);

  nsRefPtr<Promise> fetchPromise = Promise::All(aGlobal, fetchList, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  fetchPromise->AppendNativeHandler(handler);

  return promise.forget();
}

already_AddRefed<Promise>
Cache::PutAll(const nsTArray<nsRefPtr<Request>>& aRequestList,
              const nsTArray<nsRefPtr<Response>>& aResponseList,
              ErrorResult& aRv)
{
  MOZ_ASSERT(aRequestList.Length() == aResponseList.Length());

  if (NS_WARN_IF(!mActor)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  AutoChildOpArgs args(this, CachePutAllArgs());

  for (uint32_t i = 0; i < aRequestList.Length(); ++i) {
    nsRefPtr<InternalRequest> ir = aRequestList[i]->GetInternalRequest();
    args.Add(ir, ReadBody, TypeErrorOnInvalidScheme, *aResponseList[i], aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }
  }

  return ExecuteOp(args, aRv);
}

} 
} 
} 
