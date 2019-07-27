





#include "mozilla/dom/cache/CacheStorage.h"

#include "mozilla/unused.h"
#include "mozilla/dom/CacheStorageBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/Response.h"
#include "mozilla/dom/cache/AutoUtils.h"
#include "mozilla/dom/cache/Cache.h"
#include "mozilla/dom/cache/CacheChild.h"
#include "mozilla/dom/cache/CacheStorageChild.h"
#include "mozilla/dom/cache/Feature.h"
#include "mozilla/dom/cache/PCacheChild.h"
#include "mozilla/dom/cache/ReadStream.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/ipc/PBackgroundSharedTypes.h"
#include "nsIDocument.h"
#include "nsIGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "WorkerPrivate.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::unused;
using mozilla::ErrorResult;
using mozilla::dom::workers::WorkerPrivate;
using mozilla::ipc::BackgroundChild;
using mozilla::ipc::PBackgroundChild;
using mozilla::ipc::IProtocol;
using mozilla::ipc::PrincipalInfo;
using mozilla::ipc::PrincipalToPrincipalInfo;

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozilla::dom::cache::CacheStorage);
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozilla::dom::cache::CacheStorage);
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(mozilla::dom::cache::CacheStorage,
                                      mGlobal);

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CacheStorage)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIIPCBackgroundChildCreateCallback)
NS_INTERFACE_MAP_END



struct CacheStorage::Entry final
{
  nsRefPtr<Promise> mPromise;
  CacheOpArgs mArgs;
  
  
  nsRefPtr<InternalRequest> mRequest;
};


already_AddRefed<CacheStorage>
CacheStorage::CreateOnMainThread(Namespace aNamespace, nsIGlobalObject* aGlobal,
                                 nsIPrincipal* aPrincipal, bool aPrivateBrowsing,
                                 ErrorResult& aRv)
{
  MOZ_ASSERT(aGlobal);
  MOZ_ASSERT(aPrincipal);
  MOZ_ASSERT(NS_IsMainThread());

  if (aPrivateBrowsing) {
    NS_WARNING("CacheStorage not supported during private browsing.");
    nsRefPtr<CacheStorage> ref = new CacheStorage(NS_ERROR_DOM_SECURITY_ERR);
    return ref.forget();
  }

  bool nullPrincipal;
  nsresult rv = aPrincipal->GetIsNullPrincipal(&nullPrincipal);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  if (nullPrincipal) {
    NS_WARNING("CacheStorage not supported on null principal.");
    nsRefPtr<CacheStorage> ref = new CacheStorage(NS_ERROR_DOM_SECURITY_ERR);
    return ref.forget();
  }

  
  
  
  
  bool unknownAppId = false;
  aPrincipal->GetUnknownAppId(&unknownAppId);
  if (unknownAppId) {
    NS_WARNING("CacheStorage not supported on principal with unknown appId.");
    nsRefPtr<CacheStorage> ref = new CacheStorage(NS_ERROR_DOM_SECURITY_ERR);
    return ref.forget();
  }

  PrincipalInfo principalInfo;
  rv = PrincipalToPrincipalInfo(aPrincipal, &principalInfo);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  nsRefPtr<CacheStorage> ref = new CacheStorage(aNamespace, aGlobal,
                                                principalInfo, nullptr);
  return ref.forget();
}


already_AddRefed<CacheStorage>
CacheStorage::CreateOnWorker(Namespace aNamespace, nsIGlobalObject* aGlobal,
                             WorkerPrivate* aWorkerPrivate, ErrorResult& aRv)
{
  MOZ_ASSERT(aGlobal);
  MOZ_ASSERT(aWorkerPrivate);
  aWorkerPrivate->AssertIsOnWorkerThread();

  if (aWorkerPrivate->IsInPrivateBrowsing()) {
    NS_WARNING("CacheStorage not supported during private browsing.");
    nsRefPtr<CacheStorage> ref = new CacheStorage(NS_ERROR_DOM_SECURITY_ERR);
    return ref.forget();
  }

  nsRefPtr<Feature> feature = Feature::Create(aWorkerPrivate);
  if (!feature) {
    NS_WARNING("Worker thread is shutting down.");
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  const PrincipalInfo& principalInfo = aWorkerPrivate->GetPrincipalInfo();
  if (principalInfo.type() == PrincipalInfo::TNullPrincipalInfo) {
    NS_WARNING("CacheStorage not supported on null principal.");
    nsRefPtr<CacheStorage> ref = new CacheStorage(NS_ERROR_DOM_SECURITY_ERR);
    return ref.forget();
  }

  if (principalInfo.type() == PrincipalInfo::TContentPrincipalInfo &&
      principalInfo.get_ContentPrincipalInfo().appId() ==
      nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    NS_WARNING("CacheStorage not supported on principal with unknown appId.");
    nsRefPtr<CacheStorage> ref = new CacheStorage(NS_ERROR_DOM_SECURITY_ERR);
    return ref.forget();
  }

  nsRefPtr<CacheStorage> ref = new CacheStorage(aNamespace, aGlobal,
                                                principalInfo, feature);
  return ref.forget();
}

CacheStorage::CacheStorage(Namespace aNamespace, nsIGlobalObject* aGlobal,
                           const PrincipalInfo& aPrincipalInfo, Feature* aFeature)
  : mNamespace(aNamespace)
  , mGlobal(aGlobal)
  , mPrincipalInfo(MakeUnique<PrincipalInfo>(aPrincipalInfo))
  , mFeature(aFeature)
  , mActor(nullptr)
  , mStatus(NS_OK)
{
  MOZ_ASSERT(mGlobal);

  
  
  PBackgroundChild* actor = BackgroundChild::GetForCurrentThread();
  if (actor) {
    ActorCreated(actor);
    return;
  }

  
  
  MOZ_ASSERT(NS_IsMainThread());
  bool ok = BackgroundChild::GetOrCreateForCurrentThread(this);
  if (NS_WARN_IF(!ok)) {
    ActorFailed();
  }
}

CacheStorage::CacheStorage(nsresult aFailureResult)
  : mNamespace(INVALID_NAMESPACE)
  , mActor(nullptr)
  , mStatus(aFailureResult)
{
  MOZ_ASSERT(NS_FAILED(mStatus));
}

already_AddRefed<Promise>
CacheStorage::Match(const RequestOrUSVString& aRequest,
                    const CacheQueryOptions& aOptions, ErrorResult& aRv)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);

  if (NS_WARN_IF(NS_FAILED(mStatus))) {
    aRv.Throw(mStatus);
    return nullptr;
  }

  nsRefPtr<InternalRequest> request = ToInternalRequest(aRequest, IgnoreBody,
                                                        aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(!promise)) {
    return nullptr;
  }

  CacheQueryParams params;
  ToCacheQueryParams(params, aOptions);

  nsAutoPtr<Entry> entry(new Entry());
  entry->mPromise = promise;
  entry->mArgs = StorageMatchArgs(CacheRequest(), params);
  entry->mRequest = request;

  mPendingRequests.AppendElement(entry.forget());
  MaybeRunPendingRequests();

  return promise.forget();
}

already_AddRefed<Promise>
CacheStorage::Has(const nsAString& aKey, ErrorResult& aRv)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);

  if (NS_WARN_IF(NS_FAILED(mStatus))) {
    aRv.Throw(mStatus);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(!promise)) {
    return nullptr;
  }

  nsAutoPtr<Entry> entry(new Entry());
  entry->mPromise = promise;
  entry->mArgs = StorageHasArgs(nsString(aKey));

  mPendingRequests.AppendElement(entry.forget());
  MaybeRunPendingRequests();

  return promise.forget();
}

already_AddRefed<Promise>
CacheStorage::Open(const nsAString& aKey, ErrorResult& aRv)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);

  if (NS_WARN_IF(NS_FAILED(mStatus))) {
    aRv.Throw(mStatus);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(!promise)) {
    return nullptr;
  }

  nsAutoPtr<Entry> entry(new Entry());
  entry->mPromise = promise;
  entry->mArgs = StorageOpenArgs(nsString(aKey));

  mPendingRequests.AppendElement(entry.forget());
  MaybeRunPendingRequests();

  return promise.forget();
}

already_AddRefed<Promise>
CacheStorage::Delete(const nsAString& aKey, ErrorResult& aRv)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);

  if (NS_WARN_IF(NS_FAILED(mStatus))) {
    aRv.Throw(mStatus);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(!promise)) {
    return nullptr;
  }

  nsAutoPtr<Entry> entry(new Entry());
  entry->mPromise = promise;
  entry->mArgs = StorageDeleteArgs(nsString(aKey));

  mPendingRequests.AppendElement(entry.forget());
  MaybeRunPendingRequests();

  return promise.forget();
}

already_AddRefed<Promise>
CacheStorage::Keys(ErrorResult& aRv)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);

  if (NS_WARN_IF(NS_FAILED(mStatus))) {
    aRv.Throw(mStatus);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(!promise)) {
    return nullptr;
  }

  nsAutoPtr<Entry> entry(new Entry());
  entry->mPromise = promise;
  entry->mArgs = StorageKeysArgs();

  mPendingRequests.AppendElement(entry.forget());
  MaybeRunPendingRequests();

  return promise.forget();
}


bool
CacheStorage::PrefEnabled(JSContext* aCx, JSObject* aObj)
{
  return Cache::PrefEnabled(aCx, aObj);
}


already_AddRefed<CacheStorage>
CacheStorage::Constructor(const GlobalObject& aGlobal,
                          CacheStorageNamespace aNamespace,
                          nsIPrincipal* aPrincipal, ErrorResult& aRv)
{
  if (NS_WARN_IF(!NS_IsMainThread())) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  
  static_assert(DEFAULT_NAMESPACE == (uint32_t)CacheStorageNamespace::Content,
                "Default namespace should match webidl Content enum");
  static_assert(CHROME_ONLY_NAMESPACE == (uint32_t)CacheStorageNamespace::Chrome,
                "Chrome namespace should match webidl Chrome enum");
  static_assert(NUMBER_OF_NAMESPACES == (uint32_t)CacheStorageNamespace::EndGuard_,
                "Number of namespace should match webidl endguard enum");

  Namespace ns = static_cast<Namespace>(aNamespace);
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());

  bool privateBrowsing = false;
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(global);
  if (window) {
    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    if (doc) {
      nsCOMPtr<nsILoadContext> loadContext = doc->GetLoadContext();
      privateBrowsing = loadContext && loadContext->UsePrivateBrowsing();
    }
  }

  return CreateOnMainThread(ns, global, aPrincipal, privateBrowsing, aRv);
}

nsISupports*
CacheStorage::GetParentObject() const
{
  return mGlobal;
}

JSObject*
CacheStorage::WrapObject(JSContext* aContext, JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::CacheStorageBinding::Wrap(aContext, this, aGivenProto);
}

void
CacheStorage::ActorCreated(PBackgroundChild* aActor)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);
  MOZ_ASSERT(aActor);

  if (NS_WARN_IF(mFeature && mFeature->Notified())) {
    ActorFailed();
    return;
  }

  
  
  
  CacheStorageChild* newActor = new CacheStorageChild(this, mFeature);
  PCacheStorageChild* constructedActor =
    aActor->SendPCacheStorageConstructor(newActor, mNamespace, *mPrincipalInfo);

  if (NS_WARN_IF(!constructedActor)) {
    ActorFailed();
    return;
  }

  mFeature = nullptr;

  MOZ_ASSERT(constructedActor == newActor);
  mActor = newActor;

  MaybeRunPendingRequests();
  MOZ_ASSERT(mPendingRequests.IsEmpty());
}

void
CacheStorage::ActorFailed()
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);
  MOZ_ASSERT(!NS_FAILED(mStatus));

  mStatus = NS_ERROR_UNEXPECTED;
  mFeature = nullptr;

  for (uint32_t i = 0; i < mPendingRequests.Length(); ++i) {
    nsAutoPtr<Entry> entry(mPendingRequests[i].forget());
    entry->mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
  }
  mPendingRequests.Clear();
}

void
CacheStorage::DestroyInternal(CacheStorageChild* aActor)
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);
  MOZ_ASSERT(mActor);
  MOZ_ASSERT(mActor == aActor);
  mActor->ClearListener();
  mActor = nullptr;

  
  
  ActorFailed();
}

nsIGlobalObject*
CacheStorage::GetGlobalObject() const
{
  return mGlobal;
}

#ifdef DEBUG
void
CacheStorage::AssertOwningThread() const
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);
}
#endif

CachePushStreamChild*
CacheStorage::CreatePushStream(nsIAsyncInputStream* aStream)
{
  
  MOZ_CRASH("CacheStorage should never create a push stream.");
}

CacheStorage::~CacheStorage()
{
  NS_ASSERT_OWNINGTHREAD(CacheStorage);
  if (mActor) {
    mActor->StartDestroyFromListener();
    
    
    MOZ_ASSERT(!mActor);
  }
}

void
CacheStorage::MaybeRunPendingRequests()
{
  if (!mActor) {
    return;
  }

  for (uint32_t i = 0; i < mPendingRequests.Length(); ++i) {
    ErrorResult rv;
    nsAutoPtr<Entry> entry(mPendingRequests[i].forget());
    AutoChildOpArgs args(this, entry->mArgs);
    if (entry->mRequest) {
      args.Add(entry->mRequest, IgnoreBody, IgnoreInvalidScheme, rv);
    }
    if (NS_WARN_IF(rv.Failed())) {
      entry->mPromise->MaybeReject(rv);
      continue;
    }
    mActor->ExecuteOp(mGlobal, entry->mPromise, this, args.SendAsOpArgs());
  }
  mPendingRequests.Clear();
}

} 
} 
} 
