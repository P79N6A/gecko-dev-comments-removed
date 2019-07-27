





#ifndef mozilla_dom_cache_CacheStorage_h
#define mozilla_dom_cache_CacheStorage_h

#include "mozilla/dom/CacheBinding.h"
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

class CacheStorageChild;
class Feature;

class CacheStorage final : public nsIIPCBackgroundChildCreateCallback
                         , public nsWrapperCache
                         , public TypeUtils
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

  
  virtual nsIGlobalObject* GetGlobalObject() const override;
#ifdef DEBUG
  virtual void AssertOwningThread() const override;
#endif

  virtual CachePushStreamChild*
  CreatePushStream(nsIAsyncInputStream* aStream) override;

private:
  CacheStorage(Namespace aNamespace, nsIGlobalObject* aGlobal,
               const mozilla::ipc::PrincipalInfo& aPrincipalInfo, Feature* aFeature);
  ~CacheStorage();

  void MaybeRunPendingRequests();

  const Namespace mNamespace;
  nsCOMPtr<nsIGlobalObject> mGlobal;
  UniquePtr<mozilla::ipc::PrincipalInfo> mPrincipalInfo;
  nsRefPtr<Feature> mFeature;

  
  CacheStorageChild* mActor;

  struct Entry;
  nsTArray<nsAutoPtr<Entry>> mPendingRequests;

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
