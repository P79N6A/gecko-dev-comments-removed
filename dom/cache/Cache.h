





#ifndef mozilla_dom_cache_Cache_h
#define mozilla_dom_cache_Cache_h

#include "mozilla/dom/PromiseNativeHandler.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsWrapperCache.h"

class nsIGlobalObject;

namespace mozilla {

class ErrorResult;

namespace dom {

class OwningRequestOrUSVString;
class Promise;
struct CacheQueryOptions;
class RequestOrUSVString;
class Response;
template<typename T> class Optional;
template<typename T> class Sequence;

namespace cache {

class CacheChild;
class PCacheRequest;
class PCacheResponse;
class PCacheResponseOrVoid;

class Cache final : public PromiseNativeHandler
                      , public nsWrapperCache
                      , public TypeUtils
{
public:
  Cache(nsIGlobalObject* aGlobal, CacheChild* aActor);

  
  already_AddRefed<Promise>
  Match(const RequestOrUSVString& aRequest, const CacheQueryOptions& aOptions,
        ErrorResult& aRv);
  already_AddRefed<Promise>
  MatchAll(const Optional<RequestOrUSVString>& aRequest,
           const CacheQueryOptions& aOptions, ErrorResult& aRv);
  already_AddRefed<Promise>
  Add(const RequestOrUSVString& aRequest, ErrorResult& aRv);
  already_AddRefed<Promise>
  AddAll(const Sequence<OwningRequestOrUSVString>& aRequests,
         ErrorResult& aRv);
  already_AddRefed<Promise>
  Put(const RequestOrUSVString& aRequest, Response& aResponse,
      ErrorResult& aRv);
  already_AddRefed<Promise>
  Delete(const RequestOrUSVString& aRequest, const CacheQueryOptions& aOptions,
         ErrorResult& aRv);
  already_AddRefed<Promise>
  Keys(const Optional<RequestOrUSVString>& aRequest,
       const CacheQueryOptions& aParams, ErrorResult& aRv);

  
  static bool PrefEnabled(JSContext* aCx, JSObject* aObj);

  nsISupports* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aContext, JS::Handle<JSObject*> aGivenProto) override;

  
  void DestroyInternal(CacheChild* aActor);

  
  void RecvMatchResponse(RequestId aRequestId, nsresult aRv,
                         const PCacheResponseOrVoid& aResponse);
  void RecvMatchAllResponse(RequestId aRequestId, nsresult aRv,
                            const nsTArray<PCacheResponse>& aResponses);
  void RecvAddAllResponse(RequestId aRequestId, nsresult aRv);
  void RecvPutResponse(RequestId aRequestId, nsresult aRv);

  void RecvDeleteResponse(RequestId aRequestId, nsresult aRv,
                          bool aSuccess);
  void RecvKeysResponse(RequestId aRequestId, nsresult aRv,
                        const nsTArray<PCacheRequest>& aRequests);

  
  virtual nsIGlobalObject*
  GetGlobalObject() const override;

#ifdef DEBUG
  virtual void AssertOwningThread() const override;
#endif

  
  virtual void
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override;

  virtual void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override;

private:
  ~Cache();

  
  void DisconnectFromActor();

  
  RequestId AddRequestPromise(Promise* aPromise, ErrorResult& aRv);
  already_AddRefed<Promise> RemoveRequestPromise(RequestId aRequestId);

  nsCOMPtr<nsIGlobalObject> mGlobal;
  CacheChild* mActor;
  nsTArray<nsRefPtr<Promise>> mRequestPromises;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Cache)
};

} 
} 
} 

#endif
