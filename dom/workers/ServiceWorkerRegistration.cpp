





#include "ServiceWorkerRegistration.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
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

#include "Workers.h"

#ifndef MOZ_SIMPLEPUSH
#include "mozilla/dom/PushManagerBinding.h"
#endif

using namespace mozilla::dom::workers;

namespace mozilla {
namespace dom {

bool
ServiceWorkerRegistrationVisible(JSContext* aCx, JSObject* aObj)
{
  if (NS_IsMainThread()) {
    return Preferences::GetBool("dom.serviceWorkers.enabled", false);
  }

  
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  if (!workerPrivate) {
    return false;
  }

  return workerPrivate->ServiceWorkersEnabled();
}

NS_IMPL_ADDREF_INHERITED(ServiceWorkerRegistrationBase, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(ServiceWorkerRegistrationBase, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistrationBase)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

ServiceWorkerRegistrationBase::ServiceWorkerRegistrationBase(nsPIDOMWindow* aWindow,
                                                             const nsAString& aScope)
  : DOMEventTargetHelper(aWindow)
  , mScope(aScope)
{}



NS_IMPL_ADDREF_INHERITED(ServiceWorkerRegistrationMainThread, ServiceWorkerRegistrationBase)
NS_IMPL_RELEASE_INHERITED(ServiceWorkerRegistrationMainThread, ServiceWorkerRegistrationBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistrationMainThread)
NS_INTERFACE_MAP_END_INHERITING(ServiceWorkerRegistrationBase)

#ifndef MOZ_SIMPLEPUSH
NS_IMPL_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistrationMainThread, ServiceWorkerRegistrationBase,
                                   mPushManager,
                                   mInstallingWorker, mWaitingWorker, mActiveWorker);
#else
NS_IMPL_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistrationMainThread, ServiceWorkerRegistrationBase,
                                   mInstallingWorker, mWaitingWorker, mActiveWorker);
#endif

ServiceWorkerRegistrationMainThread::ServiceWorkerRegistrationMainThread(nsPIDOMWindow* aWindow,
                                                                         const nsAString& aScope)
  : ServiceWorkerRegistrationBase(aWindow, aScope)
  , mListeningForEvents(false)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(aWindow->IsInnerWindow());
  StartListeningForEvents();
}

ServiceWorkerRegistrationMainThread::~ServiceWorkerRegistrationMainThread()
{
  StopListeningForEvents();
  MOZ_ASSERT(!mListeningForEvents);
}


already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationMainThread::GetWorkerReference(WhichServiceWorker aWhichOne)
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
ServiceWorkerRegistrationMainThread::StartListeningForEvents()
{
  AssertIsOnMainThread();
  MOZ_ASSERT(!mListeningForEvents);
  nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  if (swm) {
    swm->AddRegistrationEventListener(mScope, this);
    mListeningForEvents = true;
  }
}

void
ServiceWorkerRegistrationMainThread::StopListeningForEvents()
{
  AssertIsOnMainThread();
  if (!mListeningForEvents) {
    return;
  }

  nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  if (swm) {
    swm->RemoveRegistrationEventListener(mScope, this);
  }
  mListeningForEvents = false;
}

JSObject*
ServiceWorkerRegistrationMainThread::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  AssertIsOnMainThread();
  return ServiceWorkerRegistrationBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationMainThread::GetInstalling()
{
  AssertIsOnMainThread();
  if (!mInstallingWorker) {
    mInstallingWorker = GetWorkerReference(WhichServiceWorker::INSTALLING_WORKER);
  }

  nsRefPtr<ServiceWorker> ret = mInstallingWorker;
  return ret.forget();
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationMainThread::GetWaiting()
{
  AssertIsOnMainThread();
  if (!mWaitingWorker) {
    mWaitingWorker = GetWorkerReference(WhichServiceWorker::WAITING_WORKER);
  }

  nsRefPtr<ServiceWorker> ret = mWaitingWorker;
  return ret.forget();
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationMainThread::GetActive()
{
  AssertIsOnMainThread();
  if (!mActiveWorker) {
    mActiveWorker = GetWorkerReference(WhichServiceWorker::ACTIVE_WORKER);
  }

  nsRefPtr<ServiceWorker> ret = mActiveWorker;
  return ret.forget();
}

void
ServiceWorkerRegistrationMainThread::UpdateFound()
{
  DispatchTrustedEvent(NS_LITERAL_STRING("updatefound"));
}

void
ServiceWorkerRegistrationMainThread::InvalidateWorkers(WhichServiceWorker aWhichOnes)
{
  AssertIsOnMainThread();
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

namespace {

void
UpdateInternal(const nsAString& aScope)
{
  AssertIsOnMainThread();
  nsCOMPtr<nsIServiceWorkerManager> swm =
    mozilla::services::GetServiceWorkerManager();
  MOZ_ASSERT(swm);
  
  swm->SoftUpdate(aScope);
}

class UpdateRunnable final : public nsRunnable
{
public:
  explicit UpdateRunnable(const nsAString& aScope)
    : mScope(aScope)
  {}

  NS_IMETHOD
  Run() override
  {
    AssertIsOnMainThread();
    UpdateInternal(mScope);
    return NS_OK;
  }

private:
  ~UpdateRunnable()
  {}

  const nsString mScope;
};

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

    mPromise->MaybeReject(NS_ERROR_DOM_SECURITY_ERR);
    return NS_OK;
  }

private:
  ~UnregisterCallback()
  { }
};

NS_IMPL_ISUPPORTS(UnregisterCallback, nsIServiceWorkerUnregisterCallback)

class FulfillUnregisterPromiseRunnable final : public WorkerRunnable
{
  nsRefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  Maybe<bool> mState;
public:
  FulfillUnregisterPromiseRunnable(WorkerPrivate* aWorkerPrivate,
                                   PromiseWorkerProxy* aProxy,
                                   Maybe<bool> aState)
    : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount)
    , mPromiseWorkerProxy(aProxy)
    , mState(aState)
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(mPromiseWorkerProxy);
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    Promise* promise = mPromiseWorkerProxy->GetWorkerPromise();
    MOZ_ASSERT(promise);
    if (mState.isSome()) {
      promise->MaybeResolve(mState.value());
    } else {
      promise->MaybeReject(NS_ERROR_DOM_SECURITY_ERR);
    }

    mPromiseWorkerProxy->CleanUp(aCx);
    return true;
  }
};

class WorkerUnregisterCallback final : public nsIServiceWorkerUnregisterCallback
{
  nsRefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
public:
  NS_DECL_ISUPPORTS

  explicit WorkerUnregisterCallback(PromiseWorkerProxy* aProxy)
    : mPromiseWorkerProxy(aProxy)
  {
  }

  NS_IMETHODIMP
  UnregisterSucceeded(bool aState) override
  {
    AssertIsOnMainThread();
    Finish(Some(aState));
    return NS_OK;
  }

  NS_IMETHODIMP
  UnregisterFailed() override
  {
    AssertIsOnMainThread();
    Finish(Nothing());
    return NS_OK;
  }

private:
  ~WorkerUnregisterCallback()
  { }

  void
  Finish(Maybe<bool> aState)
  {
    AssertIsOnMainThread();
    if (!mPromiseWorkerProxy) {
      return;
    }

    MutexAutoLock lock(mPromiseWorkerProxy->GetCleanUpLock());
    if (mPromiseWorkerProxy->IsClean()) {
      return;
    }

    nsRefPtr<WorkerRunnable> r =
      new FulfillUnregisterPromiseRunnable(mPromiseWorkerProxy->GetWorkerPrivate(),
                                           mPromiseWorkerProxy, aState);

    AutoJSAPI jsapi;
    jsapi.Init();
    if (!r->Dispatch(jsapi.cx())) {
      nsRefPtr<WorkerControlRunnable> cr =
        new PromiseWorkerProxyControlRunnable(
          mPromiseWorkerProxy->GetWorkerPrivate(),
          mPromiseWorkerProxy);
      cr->Dispatch(jsapi.cx());
    }
  }
};

NS_IMPL_ISUPPORTS(WorkerUnregisterCallback, nsIServiceWorkerUnregisterCallback);





class StartUnregisterRunnable final : public nsRunnable
{
  nsRefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  const nsString mScope;

public:
  StartUnregisterRunnable(WorkerPrivate* aWorker, Promise* aPromise,
                          const nsAString& aScope)
    : mPromiseWorkerProxy(PromiseWorkerProxy::Create(aWorker, aPromise))
    , mScope(aScope)
  {
    
  }

  NS_IMETHOD
  Run() override
  {
    AssertIsOnMainThread();

    nsRefPtr<WorkerUnregisterCallback> cb = new WorkerUnregisterCallback(mPromiseWorkerProxy);

    
    
    
    
    
    nsCOMPtr<nsIPrincipal> principal;
    {
      MutexAutoLock lock(mPromiseWorkerProxy->GetCleanUpLock());
      if (mPromiseWorkerProxy->IsClean()) {
        return NS_OK;
      }

      WorkerPrivate* worker = mPromiseWorkerProxy->GetWorkerPrivate();
      MOZ_ASSERT(worker);
      principal = worker->GetPrincipal();
    }
    MOZ_ASSERT(principal);

    nsCOMPtr<nsIServiceWorkerManager> swm =
      mozilla::services::GetServiceWorkerManager();
    nsresult rv = swm->Unregister(principal, cb, mScope);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      cb->UnregisterFailed();
    }

    return NS_OK;
  }
};
} 

void
ServiceWorkerRegistrationMainThread::Update()
{
  UpdateInternal(mScope);
}

already_AddRefed<Promise>
ServiceWorkerRegistrationMainThread::Unregister(ErrorResult& aRv)
{
  AssertIsOnMainThread();
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
    mozilla::services::GetServiceWorkerManager();

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

already_AddRefed<PushManager>
ServiceWorkerRegistrationMainThread::GetPushManager(ErrorResult& aRv)
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



class WorkerListener final : public ServiceWorkerRegistrationListener
{
  
  WorkerPrivate* mWorkerPrivate;
  nsString mScope;
  bool mListeningForEvents;

  
  ServiceWorkerRegistrationWorkerThread* mRegistration;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WorkerListener, override)

  WorkerListener(WorkerPrivate* aWorkerPrivate,
                 ServiceWorkerRegistrationWorkerThread* aReg)
    : mWorkerPrivate(aWorkerPrivate)
    , mListeningForEvents(false)
    , mRegistration(aReg)
  {
    MOZ_ASSERT(mWorkerPrivate);
    mWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(mRegistration);
    
    mRegistration->GetScope(mScope);
  }

  void
  StartListeningForEvents()
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(!mListeningForEvents);
    MOZ_ASSERT(mWorkerPrivate);
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    if (swm) {
      
      swm->AddRegistrationEventListener(mScope, this);
      mListeningForEvents = true;
    }
  }

  void
  StopListeningForEvents()
  {
    AssertIsOnMainThread();

    MOZ_ASSERT(mListeningForEvents);

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();

    
    
    mWorkerPrivate = nullptr;

    if (swm) {
      
      swm->RemoveRegistrationEventListener(mScope, this);
      mListeningForEvents = false;
    }
  }

  
  void
  UpdateFound() override;

  void
  InvalidateWorkers(WhichServiceWorker aWhichOnes) override
  {
    AssertIsOnMainThread();
    
  }

  void
  GetScope(nsAString& aScope) const override
  {
    aScope = mScope;
  }

  ServiceWorkerRegistrationWorkerThread*
  GetRegistration() const
  {
    if (mWorkerPrivate) {
      mWorkerPrivate->AssertIsOnWorkerThread();
    }
    return mRegistration;
  }

  void
  ClearRegistration()
  {
    if (mWorkerPrivate) {
      mWorkerPrivate->AssertIsOnWorkerThread();
    }
    mRegistration = nullptr;
  }

private:
  ~WorkerListener()
  {
    MOZ_ASSERT(!mListeningForEvents);
  }
};

NS_IMPL_ADDREF_INHERITED(ServiceWorkerRegistrationWorkerThread, ServiceWorkerRegistrationBase)
NS_IMPL_RELEASE_INHERITED(ServiceWorkerRegistrationWorkerThread, ServiceWorkerRegistrationBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ServiceWorkerRegistrationWorkerThread)
NS_INTERFACE_MAP_END_INHERITING(ServiceWorkerRegistrationBase)


NS_IMPL_CYCLE_COLLECTION_CLASS(ServiceWorkerRegistrationWorkerThread)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ServiceWorkerRegistrationWorkerThread,
                                                  ServiceWorkerRegistrationBase)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ServiceWorkerRegistrationWorkerThread,
                                                ServiceWorkerRegistrationBase)
  tmp->ReleaseListener(RegistrationIsGoingAway);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

ServiceWorkerRegistrationWorkerThread::ServiceWorkerRegistrationWorkerThread(WorkerPrivate* aWorkerPrivate,
                                                                             const nsAString& aScope)
  : ServiceWorkerRegistrationBase(nullptr, aScope)
  , mWorkerPrivate(aWorkerPrivate)
{
  InitListener();
}

ServiceWorkerRegistrationWorkerThread::~ServiceWorkerRegistrationWorkerThread()
{
  ReleaseListener(RegistrationIsGoingAway);
  MOZ_ASSERT(!mListener);
}

JSObject*
ServiceWorkerRegistrationWorkerThread::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return ServiceWorkerRegistrationBinding_workers::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationWorkerThread::GetInstalling()
{
  
  return nullptr;
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationWorkerThread::GetWaiting()
{
  
  return nullptr;
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerRegistrationWorkerThread::GetActive()
{
  
  return nullptr;
}

void
ServiceWorkerRegistrationWorkerThread::Update()
{
#ifdef DEBUG
  WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(worker);
  worker->AssertIsOnWorkerThread();
#endif
  nsCOMPtr<nsIRunnable> r = new UpdateRunnable(mScope);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(r)));
}

already_AddRefed<Promise>
ServiceWorkerRegistrationWorkerThread::Unregister(ErrorResult& aRv)
{
  WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(worker);
  worker->AssertIsOnWorkerThread();

  if (!worker->IsServiceWorker()) {
    
    
    
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(worker->GlobalScope(), aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  nsRefPtr<StartUnregisterRunnable> r = new StartUnregisterRunnable(worker, promise, mScope);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(r)));

  return promise.forget();
}

class StartListeningRunnable final : public nsRunnable
{
  nsRefPtr<WorkerListener> mListener;
public:
  explicit StartListeningRunnable(WorkerListener* aListener)
    : mListener(aListener)
  {}

  NS_IMETHOD
  Run() override
  {
    mListener->StartListeningForEvents();
    return NS_OK;
  }
};

void
ServiceWorkerRegistrationWorkerThread::InitListener()
{
  MOZ_ASSERT(!mListener);
  WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(worker);
  worker->AssertIsOnWorkerThread();

  mListener = new WorkerListener(worker, this);
  if (!worker->AddFeature(worker->GetJSContext(), this)) {
    mListener = nullptr;
    NS_WARNING("Could not add feature");
    return;
  }

  nsRefPtr<StartListeningRunnable> r =
    new StartListeningRunnable(mListener);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(r)));
}

class AsyncStopListeningRunnable final : public nsRunnable
{
  nsRefPtr<WorkerListener> mListener;
public:
  explicit AsyncStopListeningRunnable(WorkerListener* aListener)
    : mListener(aListener)
  {}

  NS_IMETHOD
  Run() override
  {
    mListener->StopListeningForEvents();
    return NS_OK;
  }
};

class SyncStopListeningRunnable final : public WorkerMainThreadRunnable
{
  nsRefPtr<WorkerListener> mListener;
public:
  SyncStopListeningRunnable(WorkerPrivate* aWorkerPrivate,
                            WorkerListener* aListener)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mListener(aListener)
  {}

  bool
  MainThreadRun() override
  {
    mListener->StopListeningForEvents();
    return true;
  }
};

void
ServiceWorkerRegistrationWorkerThread::ReleaseListener(Reason aReason)
{
  if (!mListener) {
    return;
  }

  
  
  
  
  
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->RemoveFeature(mWorkerPrivate->GetJSContext(), this);

  mListener->ClearRegistration();

  if (aReason == RegistrationIsGoingAway) {
    nsRefPtr<AsyncStopListeningRunnable> r =
      new AsyncStopListeningRunnable(mListener);
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(r)));
  } else if (aReason == WorkerIsGoingAway) {
    nsRefPtr<SyncStopListeningRunnable> r =
      new SyncStopListeningRunnable(mWorkerPrivate, mListener);
    if (!r->Dispatch(nullptr)) {
      NS_ERROR("Failed to dispatch stop listening runnable!");
    }
  } else {
    MOZ_CRASH("Bad reason");
  }
  mListener = nullptr;
  mWorkerPrivate = nullptr;
}

bool
ServiceWorkerRegistrationWorkerThread::Notify(JSContext* aCx, workers::Status aStatus)
{
  ReleaseListener(WorkerIsGoingAway);
  return true;
}

class FireUpdateFoundRunnable final : public WorkerRunnable
{
  nsRefPtr<WorkerListener> mListener;
public:
  FireUpdateFoundRunnable(WorkerPrivate* aWorkerPrivate,
                          WorkerListener* aListener)
    : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount)
    , mListener(aListener)
  {
    
    
    
    MOZ_ASSERT(aWorkerPrivate->IsServiceWorker());
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();

    ServiceWorkerRegistrationWorkerThread* reg = mListener->GetRegistration();
    if (reg) {
      reg->DispatchTrustedEvent(NS_LITERAL_STRING("updatefound"));
    }
    return true;
  }
};

void
WorkerListener::UpdateFound()
{
  AssertIsOnMainThread();
  if (mWorkerPrivate) {
    nsRefPtr<FireUpdateFoundRunnable> r =
      new FireUpdateFoundRunnable(mWorkerPrivate, this);
    AutoJSAPI jsapi;
    jsapi.Init();
    if (NS_WARN_IF(!r->Dispatch(jsapi.cx()))) {
    }
  }
}
} 
} 
