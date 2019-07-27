





#include "ServiceWorkerRegistration.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerRegistrationBinding.h"
#include "mozilla/Services.h"
#include "nsCycleCollectionParticipant.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "ServiceWorker.h"

#include "nsIDocument.h"
#include "nsIServiceWorkerManager.h"
#include "nsISupportsPrimitives.h"
#include "nsPIDOMWindow.h"

#ifndef MOZ_SIMPLEPUSH
#include "mozilla/dom/PushManagerBinding.h"
#endif

using namespace mozilla::dom::workers;

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistration)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(ServiceWorkerRegistration, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(ServiceWorkerRegistration, DOMEventTargetHelper)

#ifdef MOZ_SIMPLEPUSH

NS_IMPL_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistration,
                                   DOMEventTargetHelper,
                                   mInstallingWorker,
                                   mWaitingWorker,
                                   mActiveWorker)

#else

NS_IMPL_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistration,
                                   DOMEventTargetHelper,
                                   mInstallingWorker,
                                   mWaitingWorker,
                                   mActiveWorker,
                                   mPushManager)
#endif

ServiceWorkerRegistration::ServiceWorkerRegistration(nsPIDOMWindow* aWindow,
                                                     const nsAString& aScope)
  : DOMEventTargetHelper(aWindow)
  , mScope(aScope)
  , mListeningForEvents(false)
{
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(aWindow->IsInnerWindow());

  StartListeningForEvents();
}

ServiceWorkerRegistration::~ServiceWorkerRegistration()
{
  StopListeningForEvents();
}

void
ServiceWorkerRegistration::DisconnectFromOwner()
{
  StopListeningForEvents();
  DOMEventTargetHelper::DisconnectFromOwner();
}

JSObject*
ServiceWorkerRegistration::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return ServiceWorkerRegistrationBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistration::GetInstalling()
{
  if (!mInstallingWorker) {
    mInstallingWorker = GetWorkerReference(WhichServiceWorker::INSTALLING_WORKER);
  }

  nsRefPtr<ServiceWorker> ret = mInstallingWorker;
  return ret.forget();
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistration::GetWaiting()
{
  if (!mWaitingWorker) {
    mWaitingWorker = GetWorkerReference(WhichServiceWorker::WAITING_WORKER);
  }

  nsRefPtr<ServiceWorker> ret = mWaitingWorker;
  return ret.forget();
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistration::GetActive()
{
  if (!mActiveWorker) {
    mActiveWorker = GetWorkerReference(WhichServiceWorker::ACTIVE_WORKER);
  }

  nsRefPtr<ServiceWorker> ret = mActiveWorker;
  return ret.forget();
}

namespace {

class UnregisterCallback final : public nsIServiceWorkerUnregisterCallback
{
  nsRefPtr<Promise> mPromise;

public:
  NS_DECL_ISUPPORTS

  explicit UnregisterCallback(Promise* aPromise)
    : mPromise(aPromise)
  {
    MOZ_ASSERT(mPromise);
  }

  NS_IMETHODIMP
  UnregisterSucceeded(bool aState) override
  {
    AssertIsOnMainThread();
    mPromise->MaybeResolve(aState);
    return NS_OK;
  }

  NS_IMETHODIMP
  UnregisterFailed() override
  {
    AssertIsOnMainThread();

    AutoJSAPI api;
    api.Init(mPromise->GetParentObject());
    mPromise->MaybeReject(api.cx(), JS::UndefinedHandleValue);
    return NS_OK;
  }

private:
  ~UnregisterCallback()
  { }
};

NS_IMPL_ISUPPORTS(UnregisterCallback, nsIServiceWorkerUnregisterCallback)

} 

void
ServiceWorkerRegistration::Update()
{
  nsCOMPtr<nsIServiceWorkerManager> swm =
    mozilla::services::GetServiceWorkerManager();
  MOZ_ASSERT(swm);
  
  swm->SoftUpdate(mScope);
}

already_AddRefed<Promise>
ServiceWorkerRegistration::Unregister(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(GetOwner());
  if (!go) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> document = GetOwner()->GetExtantDoc();
  if (!document) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsIURI> scopeURI;
  nsCOMPtr<nsIURI> baseURI = document->GetBaseURI();
  
  nsresult rv = NS_NewURI(getter_AddRefs(scopeURI), mScope, nullptr, baseURI);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  nsCOMPtr<nsIPrincipal> documentPrincipal = document->NodePrincipal();
  rv = documentPrincipal->CheckMayLoad(scopeURI, true ,
                                       false );
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  nsAutoCString uriSpec;
  aRv = scopeURI->GetSpecIgnoringRef(uriSpec);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsCOMPtr<nsIServiceWorkerManager> swm =
    do_GetService(SERVICEWORKERMANAGER_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(go, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<UnregisterCallback> cb = new UnregisterCallback(promise);

  NS_ConvertUTF8toUTF16 scope(uriSpec);
  aRv = swm->Unregister(documentPrincipal, cb, scope);
  if (aRv.Failed()) {
    return nullptr;
  }

  return promise.forget();
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistration::GetWorkerReference(WhichServiceWorker aWhichOne)
{
  nsCOMPtr<nsPIDOMWindow> window = GetOwner();
  if (!window) {
    return nullptr;
  }

  nsresult rv;
  nsCOMPtr<nsIServiceWorkerManager> swm =
    do_GetService(SERVICEWORKERMANAGER_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  nsCOMPtr<nsISupports> serviceWorker;
  switch(aWhichOne) {
    case WhichServiceWorker::INSTALLING_WORKER:
      rv = swm->GetInstalling(window, mScope, getter_AddRefs(serviceWorker));
      break;
    case WhichServiceWorker::WAITING_WORKER:
      rv = swm->GetWaiting(window, mScope, getter_AddRefs(serviceWorker));
      break;
    case WhichServiceWorker::ACTIVE_WORKER:
      rv = swm->GetActive(window, mScope, getter_AddRefs(serviceWorker));
      break;
    default:
      MOZ_CRASH("Invalid enum value");
  }

  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv) || rv == NS_ERROR_DOM_NOT_FOUND_ERR,
                   "Unexpected error getting service worker instance from ServiceWorkerManager");
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  nsRefPtr<ServiceWorker> ref =
    static_cast<ServiceWorker*>(serviceWorker.get());
  return ref.forget();
}

void
ServiceWorkerRegistration::InvalidateWorkerReference(WhichServiceWorker aWhichOnes)
{
  if (aWhichOnes & WhichServiceWorker::INSTALLING_WORKER) {
    mInstallingWorker = nullptr;
  }

  if (aWhichOnes & WhichServiceWorker::WAITING_WORKER) {
    mWaitingWorker = nullptr;
  }

  if (aWhichOnes & WhichServiceWorker::ACTIVE_WORKER) {
    mActiveWorker = nullptr;
  }
}



void
ServiceWorkerRegistration::StartListeningForEvents()
{
  nsCOMPtr<nsIServiceWorkerManager> swm = do_GetService(SERVICEWORKERMANAGER_CONTRACTID);
  if (swm) {
    swm->AddRegistrationEventListener(mScope, this);
    mListeningForEvents = true;
  }
}

void
ServiceWorkerRegistration::StopListeningForEvents()
{
  if (!mListeningForEvents) {
    return;
  }

  nsCOMPtr<nsIServiceWorkerManager> swm = do_GetService(SERVICEWORKERMANAGER_CONTRACTID);
  if (swm) {
    swm->RemoveRegistrationEventListener(mScope, this);
    mListeningForEvents = false;
  }
}

already_AddRefed<PushManager>
ServiceWorkerRegistration::GetPushManager(ErrorResult& aRv)
{
  AssertIsOnMainThread();

#ifdef MOZ_SIMPLEPUSH
  return nullptr;
#else

  if (!mPushManager) {
    nsCOMPtr<nsIGlobalObject> globalObject = do_QueryInterface(GetOwner());

    if (!globalObject) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    AutoJSAPI jsapi;
    if (NS_WARN_IF(!jsapi.Init(globalObject))) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    JSContext* cx = jsapi.cx();

    JS::RootedObject globalJs(cx, globalObject->GetGlobalJSObject());
    GlobalObject global(cx, globalJs);

    
    JS::Rooted<JSObject*> jsImplObj(cx);
    nsCOMPtr<nsIGlobalObject> unused = ConstructJSImplementation(cx, "@mozilla.org/push/PushManager;1",
                              global, &jsImplObj, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
    mPushManager = new PushManager(jsImplObj, globalObject);

    mPushManager->SetScope(mScope, aRv);
    if (aRv.Failed()) {
      mPushManager = nullptr;
      return nullptr;
    }
  }

  nsRefPtr<PushManager> ret = mPushManager;
  return ret.forget();

  #endif 
}

} 
} 
