





#ifndef mozilla_dom_cache_CacheStorage_h
#define mozilla_dom_cache_CacheStorage_h

#include "mozilla/dom/CacheBinding.h"
#include "mozilla/dom/PromiseNativeHandler.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"
#include "nsIIPCBackgroundChildCreateCallback.h"

class nsIGlobalObject;

namespace mozilla {

class ErrorResult;

namespace ipc {
  class PrincipalInfo;
}

namespace dom {

class Promise;

namespace workers {
  class WorkerPrivate;
}

namespace cache {

class CacheChild;
class CacheStorageChild;
class Feature;
class PCacheResponseOrVoid;

class CacheStorage final : public nsIIPCBackgroundChildCreateCallback
                         , public nsWrapperCache
                         , public TypeUtils
                         , public PromiseNativeHandler
{
  typedef mozilla::ipc::PBackgroundChild PBackgroundChild;

public:
  static already_AddRefed<CacheStorage>
  CreateOnMainThread(Namespace aNamespace, nsIGlobalObject* aGlobal,
                     nsIPrincipal* aPrincipal, ErrorResult& aRv);

  static already_AddRefed<CacheStorage>
  CreateOnWorker(Namespace aNamespace, nsIGlobalObject* aGlobal,
                 workers::WorkerPrivate* aWorkerPrivate, ErrorResult& aRv);

  
  already_AddRefed<Promise> Match(const RequestOrUSVString& aRequest,
                                  const CacheQueryOptions& aOptions,
                                  ErrorResult& aRv);
  already_AddRefed<Promise> Has(const nsAString& aKey, ErrorResult& aRv);
  already_AddRefed<Promise> Open(const nsAString& aKey, ErrorResult& aRv);
  already_AddRefed<Promise> Delete(const nsAString& aKey, ErrorResult& aRv);
  already_AddRefed<Promise> Keys(ErrorResult& aRv);

  
  static bool PrefEnabled(JSContext* aCx, JSObject* aObj);

  nsISupports* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aContext, JS::Handle<JSObject*> aGivenProto) override;

  
  virtual void ActorCreated(PBackgroundChild* aActor) override;
  virtual void ActorFailed() override;

  
  void DestroyInternal(CacheStorageChild* aActor);

  
  void RecvMatchResponse(RequestId aRequestId, nsresult aRv,
                         const PCacheResponseOrVoid& aResponse);
  void RecvHasResponse(RequestId aRequestId, nsresult aRv, bool aSuccess);
  void RecvOpenResponse(RequestId aRequestId, nsresult aRv,
                        CacheChild* aActor);
  void RecvDeleteResponse(RequestId aRequestId, nsresult aRv, bool aSuccess);
  void RecvKeysResponse(RequestId aRequestId, nsresult aRv,
                        const nsTArray<nsString>& aKeys);

  
  virtual nsIGlobalObject* GetGlobalObject() const override;
#ifdef DEBUG
  virtual void AssertOwningThread() const override;
#endif

  virtual CachePushStreamChild*
  CreatePushStream(nsIAsyncInputStream* aStream) override;

  
  virtual void
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override;

  virtual void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) override;

private:
  CacheStorage(Namespace aNamespace, nsIGlobalObject* aGlobal,
               const mozilla::ipc::PrincipalInfo& aPrincipalInfo, Feature* aFeature);
  ~CacheStorage();

  
  void DisconnectFromActor();

  void MaybeRunPendingRequests();

  RequestId AddRequestPromise(Promise* aPromise, ErrorResult& aRv);
  already_AddRefed<Promise> RemoveRequestPromise(RequestId aRequestId);

  
  
  const Namespace mNamespace;
  nsCOMPtr<nsIGlobalObject> mGlobal;
  UniquePtr<mozilla::ipc::PrincipalInfo> mPrincipalInfo;
  nsRefPtr<Feature> mFeature;
  CacheStorageChild* mActor;
  nsTArray<nsRefPtr<Promise>> mRequestPromises;

  enum Op
  {
    OP_MATCH,
    OP_HAS,
    OP_OPEN,
    OP_DELETE,
    OP_KEYS
  };

  struct Entry
  {
    RequestId mRequestId;
    Op mOp;
    
    
    
    nsRefPtr<InternalRequest> mRequest;
    CacheQueryOptions mOptions;
    
    
    
    nsString mKey;
  };

  nsTArray<Entry> mPendingRequests;
  bool mFailedActor;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(CacheStorage,
                                           nsIIPCBackgroundChildCreateCallback)
};

} 
} 
} 

#endif
