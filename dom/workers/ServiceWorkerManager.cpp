



#include "ServiceWorkerManager.h"

#include "nsIDOMEventTarget.h"
#include "nsIDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"

#include "jsapi.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/ErrorEvent.h"
#include "mozilla/dom/InstallEventBinding.h"
#include "mozilla/dom/PromiseNativeHandler.h"

#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsTArray.h"

#include "RuntimeService.h"
#include "ServiceWorker.h"
#include "ServiceWorkerRegistration.h"
#include "ServiceWorkerEvents.h"
#include "WorkerInlines.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

using namespace mozilla;
using namespace mozilla::dom;

BEGIN_WORKERS_NAMESPACE

NS_IMPL_ISUPPORTS0(ServiceWorkerRegistrationInfo)

UpdatePromise::UpdatePromise()
  : mState(Pending)
{
  MOZ_COUNT_CTOR(UpdatePromise);
}

UpdatePromise::~UpdatePromise()
{
  MOZ_COUNT_DTOR(UpdatePromise);
}

void
UpdatePromise::AddPromise(Promise* aPromise)
{
  MOZ_ASSERT(mState == Pending);
  mPromises.AppendElement(aPromise);
}

void
UpdatePromise::ResolveAllPromises(const nsACString& aScriptSpec, const nsACString& aScope)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mState == Pending);
  mState = Resolved;
  RuntimeService* rs = RuntimeService::GetOrCreateService();
  MOZ_ASSERT(rs);

  nsTArray<WeakPtr<Promise>> array;
  array.SwapElements(mPromises);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    WeakPtr<Promise>& pendingPromise = array.ElementAt(i);
    if (pendingPromise) {
      nsCOMPtr<nsIGlobalObject> go =
        do_QueryInterface(pendingPromise->GetParentObject());
      MOZ_ASSERT(go);

      AutoSafeJSContext cx;
      JS::Rooted<JSObject*> global(cx, go->GetGlobalJSObject());
      JSAutoCompartment ac(cx, global);

      GlobalObject domGlobal(cx, global);

      
      nsRefPtr<ServiceWorker> serviceWorker;
      nsresult rv = rs->CreateServiceWorker(domGlobal,
                                            NS_ConvertUTF8toUTF16(aScriptSpec),
                                            aScope,
                                            getter_AddRefs(serviceWorker));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        pendingPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
        continue;
      }

      
      
      nsCOMPtr<nsPIDOMWindow> window =
        do_QueryInterface(pendingPromise->GetParentObject());
      nsRefPtr<ServiceWorkerRegistration> swr =
        new ServiceWorkerRegistration(window, NS_ConvertUTF8toUTF16(aScope));

      pendingPromise->MaybeResolve(swr);
    }
  }
}

void
UpdatePromise::RejectAllPromises(nsresult aRv)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mState == Pending);
  mState = Rejected;

  nsTArray<WeakPtr<Promise>> array;
  array.SwapElements(mPromises);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    WeakPtr<Promise>& pendingPromise = array.ElementAt(i);
    if (pendingPromise) {
      
      
      nsCOMPtr<nsPIDOMWindow> window =
        do_QueryInterface(pendingPromise->GetParentObject());
      MOZ_ASSERT(window);
      nsRefPtr<DOMError> domError = new DOMError(window, aRv);
      pendingPromise->MaybeRejectBrokenly(domError);
    }
  }
}

void
UpdatePromise::RejectAllPromises(const ErrorEventInit& aErrorDesc)
{
  MOZ_ASSERT(mState == Pending);
  mState = Rejected;

  nsTArray<WeakPtr<Promise>> array;
  array.SwapElements(mPromises);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    WeakPtr<Promise>& pendingPromise = array.ElementAt(i);
    if (pendingPromise) {
      
      
      nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(pendingPromise->GetParentObject());
      MOZ_ASSERT(go);

      AutoJSAPI jsapi;
      jsapi.Init(go);

      JSContext* cx = jsapi.cx();

      JS::Rooted<JSString*> stack(cx, JS_GetEmptyString(JS_GetRuntime(cx)));

      JS::Rooted<JS::Value> fnval(cx);
      ToJSValue(cx, aErrorDesc.mFilename, &fnval);
      JS::Rooted<JSString*> fn(cx, fnval.toString());

      JS::Rooted<JS::Value> msgval(cx);
      ToJSValue(cx, aErrorDesc.mMessage, &msgval);
      JS::Rooted<JSString*> msg(cx, msgval.toString());

      JS::Rooted<JS::Value> error(cx);
      if (!JS::CreateError(cx, JSEXN_ERR, stack, fn, aErrorDesc.mLineno,
                           aErrorDesc.mColno, nullptr, msg, &error)) {
        pendingPromise->MaybeReject(NS_ERROR_FAILURE);
        continue;
      }

      pendingPromise->MaybeReject(cx, error);
    }
  }
}

void
ServiceWorkerRegistrationInfo::Clear()
{
  if (mInstallingWorker) {
    
    
    
    mInstallingWorker = nullptr;
    
  }

  if (mWaitingWorker) {
    
    
    mWaitingWorker = nullptr;
  }

  if (mCurrentWorker) {
    
    mCurrentWorker = nullptr;
  }

  nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  MOZ_ASSERT(swm);
  swm->InvalidateServiceWorkerRegistrationWorker(this,
                                                 WhichServiceWorker::INSTALLING_WORKER |
                                                 WhichServiceWorker::WAITING_WORKER |
                                                 WhichServiceWorker::ACTIVE_WORKER);
}

class FinishFetchOnMainThreadRunnable : public nsRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerUpdateInstance> mUpdateInstance;
public:
  explicit FinishFetchOnMainThreadRunnable
    (const nsMainThreadPtrHandle<ServiceWorkerUpdateInstance>& aUpdateInstance)
    : mUpdateInstance(aUpdateInstance)
  { }

  NS_IMETHOD
  Run() MOZ_OVERRIDE;
};

class FinishSuccessfulFetchWorkerRunnable : public WorkerRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerUpdateInstance> mUpdateInstance;
public:
  FinishSuccessfulFetchWorkerRunnable(WorkerPrivate* aWorkerPrivate,
                                      const nsMainThreadPtrHandle<ServiceWorkerUpdateInstance>& aUpdateInstance)
    : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount),
      mUpdateInstance(aUpdateInstance)
  {
    AssertIsOnMainThread();
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->AssertIsOnWorkerThread();
    if (!aWorkerPrivate->WorkerScriptExecutedSuccessfully()) {
      return true;
    }

    nsRefPtr<FinishFetchOnMainThreadRunnable> r =
      new FinishFetchOnMainThreadRunnable(mUpdateInstance);
    NS_DispatchToMainThread(r);
    return true;
  }
};





class ServiceWorkerUpdateInstance MOZ_FINAL : public nsISupports
{
  
  ServiceWorkerRegistrationInfo* mRegistration;
  nsCString mScriptSpec;
  nsCOMPtr<nsPIDOMWindow> mWindow;

  bool mAborted;

  ~ServiceWorkerUpdateInstance() {}

public:
  NS_DECL_ISUPPORTS

  ServiceWorkerUpdateInstance(ServiceWorkerRegistrationInfo *aRegistration,
                              nsPIDOMWindow* aWindow)
    : mRegistration(aRegistration),
      
      mScriptSpec(aRegistration->mScriptSpec),
      mWindow(aWindow),
      mAborted(false)
  {
    AssertIsOnMainThread();
  }

  const nsCString&
  GetScriptSpec() const
  {
    return mScriptSpec;
  }

  void
  Abort()
  {
    MOZ_ASSERT(!mAborted);
    mAborted = true;
  }

  void
  Update()
  {
    AssertIsOnMainThread();
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    MOZ_ASSERT(swm);

    nsRefPtr<ServiceWorker> serviceWorker;
    nsresult rv = swm->CreateServiceWorkerForWindow(mWindow,
                                                    mScriptSpec,
                                                    mRegistration->mScope,
                                                    getter_AddRefs(serviceWorker));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      swm->RejectUpdatePromiseObservers(mRegistration, rv);
      return;
    }

    nsMainThreadPtrHandle<ServiceWorkerUpdateInstance> handle(
      new nsMainThreadPtrHolder<ServiceWorkerUpdateInstance>(this));
    
    
    nsRefPtr<FinishSuccessfulFetchWorkerRunnable> r =
      new FinishSuccessfulFetchWorkerRunnable(serviceWorker->GetWorkerPrivate(), handle);

    AutoSafeJSContext cx;
    if (!r->Dispatch(cx)) {
      swm->RejectUpdatePromiseObservers(mRegistration, NS_ERROR_FAILURE);
    }
  }

  void
  FetchDone()
  {
    AssertIsOnMainThread();
    if (mAborted) {
      return;
    }

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    MOZ_ASSERT(swm);
    swm->FinishFetch(mRegistration, mWindow);
  }
};

NS_IMPL_ISUPPORTS0(ServiceWorkerUpdateInstance)

NS_IMETHODIMP
FinishFetchOnMainThreadRunnable::Run()
{
  AssertIsOnMainThread();
  mUpdateInstance->FetchDone();
  return NS_OK;
}

ServiceWorkerRegistrationInfo::ServiceWorkerRegistrationInfo(const nsACString& aScope)
  : mControlledDocumentsCounter(0),
    mScope(aScope),
    mPendingUninstall(false)
{ }

ServiceWorkerRegistrationInfo::~ServiceWorkerRegistrationInfo()
{
  MOZ_ASSERT(!IsControllingDocuments());
}





NS_IMPL_ADDREF(ServiceWorkerManager)
NS_IMPL_RELEASE(ServiceWorkerManager)

NS_INTERFACE_MAP_BEGIN(ServiceWorkerManager)
  NS_INTERFACE_MAP_ENTRY(nsIServiceWorkerManager)
  if (aIID.Equals(NS_GET_IID(ServiceWorkerManager)))
    foundInterface = static_cast<nsIServiceWorkerManager*>(this);
  else
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIServiceWorkerManager)
NS_INTERFACE_MAP_END

ServiceWorkerManager::ServiceWorkerManager()
{
}

ServiceWorkerManager::~ServiceWorkerManager()
{
  
  mDomainMap.EnumerateRead(CleanupServiceWorkerInformation, nullptr);
  mDomainMap.Clear();
}

 PLDHashOperator
ServiceWorkerManager::CleanupServiceWorkerInformation(const nsACString& aDomain,
                                                      ServiceWorkerDomainInfo* aDomainInfo,
                                                      void *aUnused)
{
  aDomainInfo->mServiceWorkerRegistrationInfos.Clear();
  return PL_DHASH_NEXT;
}




class RegisterRunnable : public nsRunnable
{
  nsCOMPtr<nsPIDOMWindow> mWindow;
  const nsCString mScope;
  nsCOMPtr<nsIURI> mScriptURI;
  nsRefPtr<Promise> mPromise;
public:
  RegisterRunnable(nsPIDOMWindow* aWindow, const nsCString aScope,
                   nsIURI* aScriptURI, Promise* aPromise)
    : mWindow(aWindow), mScope(aScope), mScriptURI(aScriptURI),
      mPromise(aPromise)
  { }

  NS_IMETHODIMP
  Run()
  {
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    nsRefPtr<ServiceWorkerManager::ServiceWorkerDomainInfo> domainInfo = swm->GetDomainInfo(mScriptURI);
    if (!domainInfo) {
      nsCString domain;
      nsresult rv = mScriptURI->GetHost(domain);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        mPromise->MaybeReject(rv);
        return NS_OK;
      }

      domainInfo = new ServiceWorkerManager::ServiceWorkerDomainInfo;
      swm->mDomainMap.Put(domain, domainInfo);
    }

    nsRefPtr<ServiceWorkerRegistrationInfo> registration =
      domainInfo->GetRegistration(mScope);

    nsCString spec;
    nsresult rv = mScriptURI->GetSpec(spec);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mPromise->MaybeReject(rv);
      return NS_OK;
    }

    if (registration) {
      registration->mPendingUninstall = false;
      if (spec.Equals(registration->mScriptSpec)) {
        
        
        if (registration->HasUpdatePromise()) {
          registration->AddUpdatePromiseObserver(mPromise);
          return NS_OK;
        }

        
        
        
        nsRefPtr<ServiceWorkerInfo> info = registration->Newest();
        if (info) {
          nsRefPtr<ServiceWorker> serviceWorker;
          nsresult rv =
            swm->CreateServiceWorkerForWindow(mWindow,
                                              info->GetScriptSpec(),
                                              registration->mScope,
                                              getter_AddRefs(serviceWorker));

          if (NS_WARN_IF(NS_FAILED(rv))) {
            return NS_ERROR_FAILURE;
          }

          nsRefPtr<ServiceWorkerRegistration> swr =
            new ServiceWorkerRegistration(mWindow,
                                          NS_ConvertUTF8toUTF16(registration->mScope));

          mPromise->MaybeResolve(swr);
          return NS_OK;
        }
      }
    } else {
      registration = domainInfo->CreateNewRegistration(mScope);
    }

    registration->mScriptSpec = spec;

    rv = swm->Update(registration, mWindow);
    MOZ_ASSERT(registration->HasUpdatePromise());

    
    
    
    
    registration->mUpdatePromise->AddPromise(mPromise);

    return rv;
  }
};




class UnregisterRunnable : public nsRunnable
{
  nsCOMPtr<nsIGlobalObject> mGlobal;
  nsCOMPtr<nsIURI> mScopeURI;
  nsRefPtr<Promise> mPromise;
public:
  UnregisterRunnable(nsIGlobalObject* aGlobal, nsIURI* aScopeURI,
                     Promise* aPromise)
    : mGlobal(aGlobal), mScopeURI(aScopeURI), mPromise(aPromise)
  {
    AssertIsOnMainThread();
  }

  NS_IMETHODIMP
  Run()
  {
    AssertIsOnMainThread();

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();

    nsRefPtr<ServiceWorkerManager::ServiceWorkerDomainInfo> domainInfo =
      swm->GetDomainInfo(mScopeURI);
    MOZ_ASSERT(domainInfo);

    nsCString spec;
    nsresult rv = mScopeURI->GetSpecIgnoringRef(spec);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      AutoJSAPI api;
      api.Init(mGlobal);
      mPromise->MaybeReject(api.cx(), JS::UndefinedHandleValue);
      return NS_OK;
    }

    nsRefPtr<ServiceWorkerRegistrationInfo> registration;
    if (!domainInfo->mServiceWorkerRegistrationInfos.Get(spec,
                                                         getter_AddRefs(registration))) {
      mPromise->MaybeResolve(JS::FalseHandleValue);
      return NS_OK;
    }

    MOZ_ASSERT(registration);

    registration->mPendingUninstall = true;
    mPromise->MaybeResolve(JS::TrueHandleValue);

    
    
    
    if (!registration->IsControllingDocuments()) {
      if (!registration->mPendingUninstall) {
        return NS_OK;
      }

      registration->Clear();
      domainInfo->RemoveRegistration(registration);
    }

    return NS_OK;
  }
};



NS_IMETHODIMP
ServiceWorkerManager::Register(const nsAString& aScope,
                               const nsAString& aScriptURL,
                               nsISupports** aPromise)
{
  AssertIsOnMainThread();

  
  
  MOZ_ASSERT(!nsContentUtils::IsCallerChrome());

  nsCOMPtr<nsIGlobalObject> sgo = GetEntryGlobal();
  MOZ_ASSERT(sgo, "Register() should only be called from a valid entry settings object!");

  ErrorResult result;
  nsRefPtr<Promise> promise = Promise::Create(sgo, result);
  if (result.Failed()) {
    return result.ErrorCode();
  }

  nsCOMPtr<nsIDocument> doc = GetEntryDocument();
  if (!doc) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  

  nsCOMPtr<nsIURI> documentURI = doc->GetBaseURI();

  bool httpsNeeded = true;

  
  if (Preferences::GetBool("dom.serviceWorkers.testing.enabled")) {
    httpsNeeded = false;
  }

  
  if (httpsNeeded) {
    nsAutoCString host;
    result = documentURI->GetHost(host);
    if (NS_WARN_IF(result.Failed())) {
      return result.ErrorCode();
    }

    if (host.Equals("127.0.0.1") ||
        host.Equals("localhost") ||
        host.Equals("::1")) {
      httpsNeeded = false;
    }
  }

  if (httpsNeeded) {
    bool isHttps;
    result = documentURI->SchemeIs("https", &isHttps);
    if (result.Failed() || !isHttps) {
      NS_WARNING("ServiceWorker registration from insecure websites is not allowed.");
      return NS_ERROR_DOM_SECURITY_ERR;
    }
  }

  nsCOMPtr<nsIURI> scriptURI;
  result = NS_NewURI(getter_AddRefs(scriptURI), aScriptURL, nullptr, documentURI);
  if (NS_WARN_IF(result.Failed())) {
    return result.ErrorCode();
  }

  
  nsCOMPtr<nsIPrincipal> documentPrincipal = doc->NodePrincipal();

  result = documentPrincipal->CheckMayLoad(scriptURI, true ,
                                           false );
  if (result.Failed()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIURI> scopeURI;
  result = NS_NewURI(getter_AddRefs(scopeURI), aScope, nullptr, documentURI);
  if (NS_WARN_IF(result.Failed())) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  result = documentPrincipal->CheckMayLoad(scopeURI, true ,
                                           false );
  if (result.Failed()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCString cleanedScope;
  result = scopeURI->GetSpecIgnoringRef(cleanedScope);
  if (NS_WARN_IF(result.Failed())) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsPIDOMWindow> window = do_QueryObject(sgo);
  if (!window) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsIRunnable> registerRunnable =
    new RegisterRunnable(window, cleanedScope, scriptURI, promise);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(registerRunnable);
}




class GetRegistrationsRunnable : public nsRunnable
{
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<Promise> mPromise;
public:
  GetRegistrationsRunnable(nsPIDOMWindow* aWindow, Promise* aPromise)
    : mWindow(aWindow), mPromise(aPromise)
  { }

  NS_IMETHODIMP
  Run()
  {
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();

    nsIDocument* doc = mWindow->GetExtantDoc();
    if (!doc) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    nsCOMPtr<nsIURI> docURI = doc->GetDocumentURI();
    if (!docURI) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    nsCOMPtr<nsIPrincipal> principal = doc->NodePrincipal();
    if (!principal) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    nsTArray<nsRefPtr<ServiceWorkerRegistration>> array;

    nsRefPtr<ServiceWorkerManager::ServiceWorkerDomainInfo> domainInfo =
      swm->GetDomainInfo(docURI);

    if (!domainInfo) {
      mPromise->MaybeResolve(array);
      return NS_OK;
    }

    for (uint32_t i = 0; i < domainInfo->mOrderedScopes.Length(); ++i) {
      NS_ConvertUTF8toUTF16 scope(domainInfo->mOrderedScopes[i]);
      nsRefPtr<ServiceWorkerRegistration> swr =
        new ServiceWorkerRegistration(mWindow, scope);

      array.AppendElement(swr);
    }

    mPromise->MaybeResolve(array);
    return NS_OK;
  }
};



NS_IMETHODIMP
ServiceWorkerManager::GetRegistrations(nsIDOMWindow* aWindow,
                                       nsISupports** aPromise)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  
  
  MOZ_ASSERT(!nsContentUtils::IsCallerChrome());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  if (!window) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIGlobalObject> sgo = do_QueryInterface(window);
  ErrorResult result;
  nsRefPtr<Promise> promise = Promise::Create(sgo, result);
  if (result.Failed()) {
    return result.ErrorCode();
  }

  nsRefPtr<nsIRunnable> runnable =
    new GetRegistrationsRunnable(window, promise);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(runnable);
}




class GetRegistrationRunnable : public nsRunnable
{
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<Promise> mPromise;
  nsString mDocumentURL;

public:
  GetRegistrationRunnable(nsPIDOMWindow* aWindow, Promise* aPromise,
                          const nsAString& aDocumentURL)
    : mWindow(aWindow), mPromise(aPromise), mDocumentURL(aDocumentURL)
  { }

  NS_IMETHODIMP
  Run()
  {
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();

    nsIDocument* doc = mWindow->GetExtantDoc();
    if (!doc) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    nsCOMPtr<nsIURI> docURI = doc->GetDocumentURI();
    if (!docURI) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), mDocumentURL, nullptr, docURI);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mPromise->MaybeReject(rv);
      return NS_OK;
    }

    nsCOMPtr<nsIPrincipal> principal = doc->NodePrincipal();
    if (!principal) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    rv = principal->CheckMayLoad(uri, true ,
                                 false );
    if (NS_FAILED(rv)) {
      mPromise->MaybeReject(NS_ERROR_DOM_SECURITY_ERR);
      return NS_OK;
    }

    nsRefPtr<ServiceWorkerRegistrationInfo> registration =
      swm->GetServiceWorkerRegistrationInfo(uri);

    if (!registration) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
      return NS_OK;
    }

    NS_ConvertUTF8toUTF16 scope(registration->mScope);
    nsRefPtr<ServiceWorkerRegistration> swr =
      new ServiceWorkerRegistration(mWindow, scope);
    mPromise->MaybeResolve(swr);

    return NS_OK;
  }
};



NS_IMETHODIMP
ServiceWorkerManager::GetRegistration(nsIDOMWindow* aWindow,
                                      const nsAString& aDocumentURL,
                                      nsISupports** aPromise)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  
  
  MOZ_ASSERT(!nsContentUtils::IsCallerChrome());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  if (!window) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIGlobalObject> sgo = do_QueryInterface(window);
  ErrorResult result;
  nsRefPtr<Promise> promise = Promise::Create(sgo, result);
  if (result.Failed()) {
    return result.ErrorCode();
  }

  nsRefPtr<nsIRunnable> runnable =
    new GetRegistrationRunnable(window, promise, aDocumentURL);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(runnable);
}

class GetReadyPromiseRunnable : public nsRunnable
{
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<Promise> mPromise;

public:
  GetReadyPromiseRunnable(nsPIDOMWindow* aWindow, Promise* aPromise)
    : mWindow(aWindow), mPromise(aPromise)
  { }

  NS_IMETHODIMP
  Run()
  {
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();

    nsIDocument* doc = mWindow->GetExtantDoc();
    if (!doc) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    nsCOMPtr<nsIURI> docURI = doc->GetDocumentURI();
    if (!docURI) {
      mPromise->MaybeReject(NS_ERROR_UNEXPECTED);
      return NS_OK;
    }

    if (!swm->CheckReadyPromise(mWindow, docURI, mPromise)) {
      swm->StorePendingReadyPromise(mWindow, docURI, mPromise);
    }

    return NS_OK;
  }
};

NS_IMETHODIMP
ServiceWorkerManager::GetReadyPromise(nsIDOMWindow* aWindow,
                                      nsISupports** aPromise)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  
  
  MOZ_ASSERT(!nsContentUtils::IsCallerChrome());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  if (!window) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(!mPendingReadyPromises.Contains(window));

  nsCOMPtr<nsIGlobalObject> sgo = do_QueryInterface(window);
  ErrorResult result;
  nsRefPtr<Promise> promise = Promise::Create(sgo, result);
  if (result.Failed()) {
    return result.ErrorCode();
  }

  nsRefPtr<nsIRunnable> runnable =
    new GetReadyPromiseRunnable(window, promise);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(runnable);
}

NS_IMETHODIMP
ServiceWorkerManager::RemoveReadyPromise(nsIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  if (!window) {
    return NS_ERROR_FAILURE;
  }

  mPendingReadyPromises.Remove(aWindow);
  return NS_OK;
}

void
ServiceWorkerManager::StorePendingReadyPromise(nsPIDOMWindow* aWindow,
                                               nsIURI* aURI,
                                               Promise* aPromise)
{
  PendingReadyPromise* data;

  
  MOZ_ASSERT(!mPendingReadyPromises.Get(aWindow, &data));

  data = new PendingReadyPromise(aURI, aPromise);
  mPendingReadyPromises.Put(aWindow, data);
}

void
ServiceWorkerManager::CheckPendingReadyPromises()
{
  mPendingReadyPromises.Enumerate(CheckPendingReadyPromisesEnumerator, this);
}

PLDHashOperator
ServiceWorkerManager::CheckPendingReadyPromisesEnumerator(
                                          nsISupports* aSupports,
                                          nsAutoPtr<PendingReadyPromise>& aData,
                                          void* aPtr)
{
  ServiceWorkerManager* aSwm = static_cast<ServiceWorkerManager*>(aPtr);

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aSupports);

  if (aSwm->CheckReadyPromise(window, aData->mURI, aData->mPromise)) {
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

bool
ServiceWorkerManager::CheckReadyPromise(nsPIDOMWindow* aWindow,
                                        nsIURI* aURI, Promise* aPromise)
{
  nsRefPtr<ServiceWorkerRegistrationInfo> registration =
    GetServiceWorkerRegistrationInfo(aURI);

  if (registration && registration->mCurrentWorker) {
    NS_ConvertUTF8toUTF16 scope(registration->mScope);
    nsRefPtr<ServiceWorkerRegistration> swr =
      new ServiceWorkerRegistration(aWindow, scope);
    aPromise->MaybeResolve(swr);
    return true;
  }

  return false;
}

void
ServiceWorkerManager::RejectUpdatePromiseObservers(ServiceWorkerRegistrationInfo* aRegistration,
                                                   nsresult aRv)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  aRegistration->mUpdatePromise->RejectAllPromises(aRv);
  aRegistration->mUpdatePromise = nullptr;
}

void
ServiceWorkerManager::RejectUpdatePromiseObservers(ServiceWorkerRegistrationInfo* aRegistration,
                                                   const ErrorEventInit& aErrorDesc)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  aRegistration->mUpdatePromise->RejectAllPromises(aErrorDesc);
  aRegistration->mUpdatePromise = nullptr;
}





NS_IMETHODIMP
ServiceWorkerManager::Update(ServiceWorkerRegistrationInfo* aRegistration,
                             nsPIDOMWindow* aWindow)
{
  if (aRegistration->HasUpdatePromise()) {
    NS_WARNING("Already had a UpdatePromise. Aborting that one!");
    AbortCurrentUpdate(aRegistration);
  }

  if (aRegistration->mInstallingWorker) {
    
    
    
    
    
    
    aRegistration->mInstallingWorker = nullptr;
    InvalidateServiceWorkerRegistrationWorker(aRegistration,
                                              WhichServiceWorker::INSTALLING_WORKER);
  }

  aRegistration->mUpdatePromise = new UpdatePromise();
  
  
  

  aRegistration->mUpdateInstance =
    new ServiceWorkerUpdateInstance(aRegistration, aWindow);
  aRegistration->mUpdateInstance->Update();

  return NS_OK;
}

void
ServiceWorkerManager::AbortCurrentUpdate(ServiceWorkerRegistrationInfo* aRegistration)
{
  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  RejectUpdatePromiseObservers(aRegistration, NS_ERROR_DOM_ABORT_ERR);
  MOZ_ASSERT(aRegistration->mUpdateInstance);
  aRegistration->mUpdateInstance->Abort();
  aRegistration->mUpdateInstance = nullptr;
}


NS_IMETHODIMP
ServiceWorkerManager::Unregister(const nsAString& aScope, nsISupports** aPromise)
{
  AssertIsOnMainThread();

  
  MOZ_ASSERT(!nsContentUtils::IsCallerChrome());

  nsCOMPtr<nsIGlobalObject> sgo = GetEntryGlobal();
  if (!sgo) {
    return NS_ERROR_FAILURE;
  }

  ErrorResult result;
  nsRefPtr<Promise> promise = Promise::Create(sgo, result);
  if (result.Failed()) {
    return result.ErrorCode();
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> document = GetEntryDocument();
  if (!document) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> scopeURI;
  nsCOMPtr<nsIURI> baseURI = document->GetBaseURI();
  nsresult rv = NS_NewURI(getter_AddRefs(scopeURI), aScope, nullptr, baseURI);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIPrincipal> documentPrincipal = document->NodePrincipal();
  rv = documentPrincipal->CheckMayLoad(scopeURI, true ,
                                       false );
  if (NS_FAILED(rv)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsRefPtr<nsIRunnable> unregisterRunnable =
    new UnregisterRunnable(sgo, scopeURI, promise);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(unregisterRunnable);
}


already_AddRefed<ServiceWorkerManager>
ServiceWorkerManager::GetInstance()
{
  nsCOMPtr<nsIServiceWorkerManager> swm = mozilla::services::GetServiceWorkerManager();
  nsRefPtr<ServiceWorkerManager> concrete = do_QueryObject(swm);
  return concrete.forget();
}

void
ServiceWorkerManager::ResolveRegisterPromises(ServiceWorkerRegistrationInfo* aRegistration,
                                              const nsACString& aWorkerScriptSpec)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  if (aRegistration->mUpdatePromise->IsRejected()) {
    aRegistration->mUpdatePromise = nullptr;
    return;
  }

  aRegistration->mUpdatePromise->ResolveAllPromises(aWorkerScriptSpec,
                                                    aRegistration->mScope);
  aRegistration->mUpdatePromise = nullptr;
}


void
ServiceWorkerManager::FinishFetch(ServiceWorkerRegistrationInfo* aRegistration,
                                  nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();

  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  MOZ_ASSERT(aRegistration->mUpdateInstance);
  aRegistration->mUpdateInstance = nullptr;
  if (aRegistration->mUpdatePromise->IsRejected()) {
    aRegistration->mUpdatePromise = nullptr;
    return;
  }

  

  nsRefPtr<ServiceWorker> worker;
  nsresult rv = CreateServiceWorkerForWindow(aWindow,
                                             aRegistration->mScriptSpec,
                                             aRegistration->mScope,
                                             getter_AddRefs(worker));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    RejectUpdatePromiseObservers(aRegistration, rv);
    return;
  }

  ResolveRegisterPromises(aRegistration, aRegistration->mScriptSpec);

  nsRefPtr<ServiceWorkerInfo> info = new ServiceWorkerInfo(aRegistration->mScriptSpec);
  Install(aRegistration, info);
}

void
ServiceWorkerManager::HandleError(JSContext* aCx,
                                  const nsACString& aScope,
                                  const nsAString& aWorkerURL,
                                  nsString aMessage,
                                  nsString aFilename,
                                  nsString aLine,
                                  uint32_t aLineNumber,
                                  uint32_t aColumnNumber,
                                  uint32_t aFlags)
{
  AssertIsOnMainThread();

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aScope, nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(uri);
  if (!domainInfo) {
    return;
  }

  nsCString scope;
  scope.Assign(aScope);
  nsRefPtr<ServiceWorkerRegistrationInfo> registration = domainInfo->GetRegistration(scope);
  MOZ_ASSERT(registration);

  RootedDictionary<ErrorEventInit> init(aCx);
  init.mMessage = aMessage;
  init.mFilename = aFilename;
  init.mLineno = aLineNumber;
  init.mColno = aColumnNumber;

  
  

  
  
  
  
  
  if (registration->mUpdateInstance &&
      registration->mUpdateInstance->GetScriptSpec().Equals(NS_ConvertUTF16toUTF8(aWorkerURL))) {
    RejectUpdatePromiseObservers(registration, init);
    
    registration->mUpdateInstance = nullptr;
  } else {
    
  }
}

class FinishInstallRunnable MOZ_FINAL : public nsRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;

public:
  explicit FinishInstallRunnable(
    const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
    : mRegistration(aRegistration)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    swm->FinishInstall(mRegistration.get());
    return NS_OK;
  }
};

class FinishActivationRunnable : public nsRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;

public:
  explicit FinishActivationRunnable(const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
    : mRegistration(aRegistration)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHODIMP
  Run()
  {
    AssertIsOnMainThread();

    
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    swm->FinishActivate(mRegistration.get());
    return NS_OK;
  }
};

class CancelServiceWorkerInstallationRunnable MOZ_FINAL : public nsRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;

public:
  explicit CancelServiceWorkerInstallationRunnable(
    const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
    : mRegistration(aRegistration)
  {
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
    
    
    mRegistration->mInstallingWorker = nullptr;
    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    swm->InvalidateServiceWorkerRegistrationWorker(mRegistration,
                                                   WhichServiceWorker::INSTALLING_WORKER);
    return NS_OK;
  }
};




class FinishInstallHandler MOZ_FINAL : public PromiseNativeHandler
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;

  virtual
  ~FinishInstallHandler()
  { }

public:
  explicit FinishInstallHandler(
    const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
    : mRegistration(aRegistration)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  void
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) MOZ_OVERRIDE
  {
    WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(workerPrivate);
    workerPrivate->AssertIsOnWorkerThread();

    nsRefPtr<FinishInstallRunnable> r = new FinishInstallRunnable(mRegistration);
    NS_DispatchToMainThread(r);
  }

  void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) MOZ_OVERRIDE
  {
    nsRefPtr<CancelServiceWorkerInstallationRunnable> r =
      new CancelServiceWorkerInstallationRunnable(mRegistration);
    NS_DispatchToMainThread(r);
  }
};

class FinishActivateHandler : public PromiseNativeHandler
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;

public:
  explicit FinishActivateHandler(const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
    : mRegistration(aRegistration)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  virtual
  ~FinishActivateHandler()
  { }

  void
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) MOZ_OVERRIDE
  {
    WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(workerPrivate);
    workerPrivate->AssertIsOnWorkerThread();

    nsRefPtr<FinishActivationRunnable> r = new FinishActivationRunnable(mRegistration);
    NS_DispatchToMainThread(r);
  }

  void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) MOZ_OVERRIDE
  {
    
  }
};







class InstallEventRunnable MOZ_FINAL : public WorkerRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;
  nsCString mScope;

public:
  InstallEventRunnable(
    WorkerPrivate* aWorkerPrivate,
    const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
      : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount),
        mRegistration(aRegistration),
        mScope(aRegistration.get()->mScope) 
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aWorkerPrivate);
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate);
    return DispatchInstallEvent(aCx, aWorkerPrivate);
  }

private:
  bool
  DispatchInstallEvent(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(aWorkerPrivate->IsServiceWorker());
    InstallEventInit init;
    init.mBubbles = false;
    init.mCancelable = true;

    

    nsRefPtr<EventTarget> target = aWorkerPrivate->GlobalScope();
    nsRefPtr<InstallEvent> event =
      InstallEvent::Constructor(target, NS_LITERAL_STRING("install"), init);

    event->SetTrusted(true);

    nsRefPtr<Promise> waitUntilPromise;

    nsresult rv = target->DispatchDOMEvent(nullptr, event, nullptr, nullptr);

    nsCOMPtr<nsIGlobalObject> sgo = aWorkerPrivate->GlobalScope();
    if (NS_SUCCEEDED(rv)) {
      waitUntilPromise = event->GetPromise();
      if (!waitUntilPromise) {
        ErrorResult rv;
        waitUntilPromise =
          Promise::Resolve(sgo,
                           aCx, JS::UndefinedHandleValue, rv);
      }
    } else {
      ErrorResult rv;
      
      waitUntilPromise = Promise::Reject(sgo, aCx,
                                         JS::UndefinedHandleValue, rv);
    }

    nsRefPtr<FinishInstallHandler> handler =
      new FinishInstallHandler(mRegistration);
    waitUntilPromise->AppendNativeHandler(handler);
    return true;
  }
};

class ActivateEventRunnable : public WorkerRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> mRegistration;

public:
  ActivateEventRunnable(WorkerPrivate* aWorkerPrivate,
                        const nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo>& aRegistration)
      : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount),
        mRegistration(aRegistration)
  {
    MOZ_ASSERT(aWorkerPrivate);
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate);
    return DispatchActivateEvent(aCx, aWorkerPrivate);
  }

private:
  bool
  DispatchActivateEvent(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate->IsServiceWorker());
    nsRefPtr<EventTarget> target = do_QueryObject(aWorkerPrivate->GlobalScope());

    
    EventInit init;
    init.mBubbles = false;
    init.mCancelable = true;
    nsRefPtr<InstallPhaseEvent> event =
      InstallPhaseEvent::Constructor(target, NS_LITERAL_STRING("activate"), init);

    event->SetTrusted(true);

    nsRefPtr<Promise> waitUntilPromise;

    nsresult rv = target->DispatchDOMEvent(nullptr, event, nullptr, nullptr);
    if (NS_SUCCEEDED(rv)) {
      waitUntilPromise = event->GetPromise();
      if (!waitUntilPromise) {
        ErrorResult rv;
        nsCOMPtr<nsIGlobalObject> global =
          do_QueryObject(aWorkerPrivate->GlobalScope());
        waitUntilPromise =
          Promise::Resolve(global,
                           aCx, JS::UndefinedHandleValue, rv);
      }
    } else {
      ErrorResult rv;
      nsCOMPtr<nsIGlobalObject> global =
        do_QueryObject(aWorkerPrivate->GlobalScope());
      
      waitUntilPromise = Promise::Reject(global, aCx,
                                         JS::UndefinedHandleValue, rv);
    }

    nsRefPtr<FinishActivateHandler> handler = new FinishActivateHandler(mRegistration);
    waitUntilPromise->AppendNativeHandler(handler);
    return true;
  }
};

void
ServiceWorkerManager::Install(ServiceWorkerRegistrationInfo* aRegistration,
                              ServiceWorkerInfo* aServiceWorkerInfo)
{
  AssertIsOnMainThread();
  aRegistration->mInstallingWorker = aServiceWorkerInfo;
  MOZ_ASSERT(aRegistration->mInstallingWorker);
  InvalidateServiceWorkerRegistrationWorker(aRegistration,
                                            WhichServiceWorker::INSTALLING_WORKER);

  nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> handle(
    new nsMainThreadPtrHolder<ServiceWorkerRegistrationInfo>(aRegistration));

  nsRefPtr<ServiceWorker> serviceWorker;
  nsresult rv =
    CreateServiceWorker(aServiceWorkerInfo->GetScriptSpec(),
                        aRegistration->mScope,
                        getter_AddRefs(serviceWorker));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRegistration->mInstallingWorker = nullptr;
    
    return;
  }

  nsRefPtr<InstallEventRunnable> r =
    new InstallEventRunnable(serviceWorker->GetWorkerPrivate(), handle);

  AutoSafeJSContext cx;
  r->Dispatch(cx);

  
  
  
  
  
  
  
  
  
  
  
  
  

  FireEventOnServiceWorkerRegistrations(aRegistration,
                                        NS_LITERAL_STRING("updatefound"));
}

class ActivationRunnable : public nsRunnable
{
  nsRefPtr<ServiceWorkerRegistrationInfo> mRegistration;
public:
  explicit ActivationRunnable(ServiceWorkerRegistrationInfo* aRegistration)
    : mRegistration(aRegistration)
  {
  }

  NS_IMETHODIMP
  Run() MOZ_OVERRIDE
  {
    if (mRegistration->mCurrentWorker) {
      
    }

    mRegistration->mCurrentWorker = mRegistration->mWaitingWorker.forget();

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    swm->InvalidateServiceWorkerRegistrationWorker(mRegistration,
                                                   WhichServiceWorker::ACTIVE_WORKER | WhichServiceWorker::WAITING_WORKER);
    if (!mRegistration->mCurrentWorker) {
      
      return NS_OK;
    }

    swm->CheckPendingReadyPromises();

    

    swm->FireEventOnServiceWorkerRegistrations(mRegistration,
                                               NS_LITERAL_STRING("controllerchange"));

    MOZ_ASSERT(mRegistration->mCurrentWorker);
    nsRefPtr<ServiceWorker> serviceWorker;
    nsresult rv =
      swm->CreateServiceWorker(mRegistration->mCurrentWorker->GetScriptSpec(),
                               mRegistration->mScope,
                               getter_AddRefs(serviceWorker));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsMainThreadPtrHandle<ServiceWorkerRegistrationInfo> handle(
      new nsMainThreadPtrHolder<ServiceWorkerRegistrationInfo>(mRegistration));

    nsRefPtr<ActivateEventRunnable> r =
      new ActivateEventRunnable(serviceWorker->GetWorkerPrivate(), handle);

    AutoSafeJSContext cx;
    if (!r->Dispatch(cx)) {
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }
};

void
ServiceWorkerManager::FinishInstall(ServiceWorkerRegistrationInfo* aRegistration)
{
  AssertIsOnMainThread();

  if (aRegistration->mWaitingWorker) {
    
  }

  if (!aRegistration->mInstallingWorker) {
    
    
    
    
    
    return;
  }

  aRegistration->mWaitingWorker = aRegistration->mInstallingWorker.forget();
  MOZ_ASSERT(aRegistration->mWaitingWorker);
  InvalidateServiceWorkerRegistrationWorker(aRegistration,
                                            WhichServiceWorker::WAITING_WORKER | WhichServiceWorker::INSTALLING_WORKER);

  
  
  

  

  if (!aRegistration->IsControllingDocuments()) {
    nsRefPtr<ActivationRunnable> r =
      new ActivationRunnable(aRegistration);

    nsresult rv = NS_DispatchToMainThread(r);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      
    }
  }
}

void
ServiceWorkerManager::FinishActivate(ServiceWorkerRegistrationInfo* aRegistration)
{
  
  
}

NS_IMETHODIMP
ServiceWorkerManager::CreateServiceWorkerForWindow(nsPIDOMWindow* aWindow,
                                                   const nsACString& aScriptSpec,
                                                   const nsACString& aScope,
                                                   ServiceWorker** aServiceWorker)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  RuntimeService* rs = RuntimeService::GetOrCreateService();
  nsRefPtr<ServiceWorker> serviceWorker;

  nsCOMPtr<nsIGlobalObject> sgo = do_QueryInterface(aWindow);

  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> jsGlobal(cx, sgo->GetGlobalJSObject());
  JSAutoCompartment ac(cx, jsGlobal);

  GlobalObject global(cx, jsGlobal);
  nsresult rv = rs->CreateServiceWorker(global,
                                        NS_ConvertUTF8toUTF16(aScriptSpec),
                                        aScope,
                                        getter_AddRefs(serviceWorker));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  serviceWorker.forget(aServiceWorker);
  return rv;
}

already_AddRefed<ServiceWorkerRegistrationInfo>
ServiceWorkerManager::GetServiceWorkerRegistrationInfo(nsPIDOMWindow* aWindow)
{
  nsCOMPtr<nsIDocument> document = aWindow->GetExtantDoc();
  return GetServiceWorkerRegistrationInfo(document);
}

already_AddRefed<ServiceWorkerRegistrationInfo>
ServiceWorkerManager::GetServiceWorkerRegistrationInfo(nsIDocument* aDoc)
{
  nsCOMPtr<nsIURI> documentURI = aDoc->GetDocumentURI();
  return GetServiceWorkerRegistrationInfo(documentURI);
}

already_AddRefed<ServiceWorkerRegistrationInfo>
ServiceWorkerManager::GetServiceWorkerRegistrationInfo(nsIURI* aURI)
{
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(aURI);
  if (!domainInfo) {
    return nullptr;
  }

  nsCString spec;
  nsresult rv = aURI->GetSpec(spec);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  nsCString scope = FindScopeForPath(domainInfo->mOrderedScopes, spec);
  if (scope.IsEmpty()) {
    return nullptr;
  }

  nsRefPtr<ServiceWorkerRegistrationInfo> registration;
  domainInfo->mServiceWorkerRegistrationInfos.Get(scope, getter_AddRefs(registration));
  
  MOZ_ASSERT(registration);

  if (registration->mPendingUninstall) {
    return nullptr;
  }
  return registration.forget();
}

 void
ServiceWorkerManager::AddScope(nsTArray<nsCString>& aList, const nsACString& aScope)
{
  for (uint32_t i = 0; i < aList.Length(); ++i) {
    const nsCString& current = aList[i];

    
    if (aScope.Equals(current)) {
      return;
    }

    
    
    
    if (StringBeginsWith(aScope, current)) {
      aList.InsertElementAt(i, aScope);
      return;
    }
  }

  aList.AppendElement(aScope);
}

 nsCString
ServiceWorkerManager::FindScopeForPath(nsTArray<nsCString>& aList, const nsACString& aPath)
{
  nsCString match;

  for (uint32_t i = 0; i < aList.Length(); ++i) {
    const nsCString& current = aList[i];
    if (StringBeginsWith(aPath, current)) {
      match = current;
      break;
    }
  }

  return match;
}

 void
ServiceWorkerManager::RemoveScope(nsTArray<nsCString>& aList, const nsACString& aScope)
{
  aList.RemoveElement(aScope);
}

already_AddRefed<ServiceWorkerManager::ServiceWorkerDomainInfo>
ServiceWorkerManager::GetDomainInfo(nsIDocument* aDoc)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aDoc);
  nsCOMPtr<nsIURI> documentURI = aDoc->GetDocumentURI();
  return GetDomainInfo(documentURI);
}

already_AddRefed<ServiceWorkerManager::ServiceWorkerDomainInfo>
ServiceWorkerManager::GetDomainInfo(nsIURI* aURI)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aURI);

  nsCString domain;
  nsresult rv = aURI->GetHost(domain);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  nsRefPtr<ServiceWorkerDomainInfo> domainInfo;
  mDomainMap.Get(domain, getter_AddRefs(domainInfo));
  return domainInfo.forget();
}

already_AddRefed<ServiceWorkerManager::ServiceWorkerDomainInfo>
ServiceWorkerManager::GetDomainInfo(const nsCString& aURL)
{
  AssertIsOnMainThread();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL, nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  return GetDomainInfo(uri);
}

void
ServiceWorkerManager::MaybeStartControlling(nsIDocument* aDoc)
{
  AssertIsOnMainThread();
  if (!Preferences::GetBool("dom.serviceWorkers.enabled")) {
    return;
  }

  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(aDoc);
  if (!domainInfo) {
    return;
  }

  nsRefPtr<ServiceWorkerRegistrationInfo> registration =
    GetServiceWorkerRegistrationInfo(aDoc);
  if (registration && registration->mCurrentWorker) {
    MOZ_ASSERT(!domainInfo->mControlledDocuments.Contains(aDoc));
    registration->StartControllingADocument();
    
    
    domainInfo->mControlledDocuments.Put(aDoc, registration.forget());
  }
}

void
ServiceWorkerManager::MaybeStopControlling(nsIDocument* aDoc)
{
  MOZ_ASSERT(aDoc);
  if (!Preferences::GetBool("dom.serviceWorkers.enabled")) {
    return;
  }

  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(aDoc);
  if (!domainInfo) {
    return;
  }

  nsRefPtr<ServiceWorkerRegistrationInfo> registration;
  domainInfo->mControlledDocuments.Remove(aDoc, getter_AddRefs(registration));
  
  
  
  if (registration) {
    registration->StopControllingADocument();
  }
}

NS_IMETHODIMP
ServiceWorkerManager::GetScopeForUrl(const nsAString& aUrl, nsAString& aScope)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUrl, nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ServiceWorkerRegistrationInfo> r = GetServiceWorkerRegistrationInfo(uri);
  if (!r) {
      return NS_ERROR_FAILURE;
  }

  aScope = NS_ConvertUTF8toUTF16(r->mScope);
  return NS_OK;
}

NS_IMETHODIMP
ServiceWorkerManager::AddRegistrationEventListener(nsIURI* aDocumentURI, nsIDOMEventTarget* aListener)
{
  MOZ_ASSERT(aDocumentURI);
  AssertIsOnMainThread();
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(aDocumentURI);
  if (!domainInfo) {
    nsCString domain;
    nsresult rv = aDocumentURI->GetHost(domain);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    domainInfo = new ServiceWorkerDomainInfo;
    mDomainMap.Put(domain, domainInfo);
  }

  MOZ_ASSERT(domainInfo);

  
  ServiceWorkerRegistration* registration = static_cast<ServiceWorkerRegistration*>(aListener);
  MOZ_ASSERT(!domainInfo->mServiceWorkerRegistrations.Contains(registration));
  domainInfo->mServiceWorkerRegistrations.AppendElement(registration);
  return NS_OK;
}

NS_IMETHODIMP
ServiceWorkerManager::RemoveRegistrationEventListener(nsIURI* aDocumentURI, nsIDOMEventTarget* aListener)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aDocumentURI);
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(aDocumentURI);
  if (!domainInfo) {
    return NS_OK;
  }

  ServiceWorkerRegistration* registration = static_cast<ServiceWorkerRegistration*>(aListener);
  MOZ_ASSERT(domainInfo->mServiceWorkerRegistrations.Contains(registration));
  domainInfo->mServiceWorkerRegistrations.RemoveElement(registration);
  return NS_OK;
}

void
ServiceWorkerManager::FireEventOnServiceWorkerRegistrations(
  ServiceWorkerRegistrationInfo* aRegistration,
  const nsAString& aName)
{
  AssertIsOnMainThread();
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo =
    GetDomainInfo(aRegistration->mScriptSpec);

  if (domainInfo) {
    nsTObserverArray<ServiceWorkerRegistration*>::ForwardIterator it(domainInfo->mServiceWorkerRegistrations);
    while (it.HasMore()) {
      nsRefPtr<ServiceWorkerRegistration> target = it.GetNext();
      nsIURI* targetURI = target->GetDocumentURI();
      if (!targetURI) {
        NS_WARNING("Controlled domain cannot have page with null URI!");
        continue;
      }

      nsCString path;
      nsresult rv = targetURI->GetSpec(path);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        continue;
      }

      nsCString scope = FindScopeForPath(domainInfo->mOrderedScopes, path);
      if (scope.IsEmpty() ||
          !scope.Equals(aRegistration->mScope)) {
        continue;
      }

      target->DispatchTrustedEvent(aName);
    }
  }
}





NS_IMETHODIMP
ServiceWorkerManager::GetServiceWorkerForWindow(nsIDOMWindow* aWindow,
                                                WhichServiceWorker aWhichWorker,
                                                nsISupports** aServiceWorker)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  MOZ_ASSERT(window);

  nsRefPtr<ServiceWorkerRegistrationInfo> registration =
    GetServiceWorkerRegistrationInfo(window);

  if (!registration) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ServiceWorkerInfo> info;
  if (aWhichWorker == WhichServiceWorker::INSTALLING_WORKER) {
    info = registration->mInstallingWorker;
  } else if (aWhichWorker == WhichServiceWorker::WAITING_WORKER) {
    info = registration->mWaitingWorker;
  } else if (aWhichWorker == WhichServiceWorker::ACTIVE_WORKER) {
    info = registration->mCurrentWorker;
  } else {
    MOZ_CRASH("Invalid worker type");
  }

  if (!info) {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  nsRefPtr<ServiceWorker> serviceWorker;
  nsresult rv = CreateServiceWorkerForWindow(window,
                                             info->GetScriptSpec(),
                                             registration->mScope,
                                             getter_AddRefs(serviceWorker));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  serviceWorker.forget(aServiceWorker);
  return NS_OK;
}





NS_IMETHODIMP
ServiceWorkerManager::GetDocumentController(nsIDOMWindow* aWindow, nsISupports** aServiceWorker)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  MOZ_ASSERT(window);
  if (!window || !window->GetExtantDoc()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();

  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(doc);
  if (!domainInfo) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ServiceWorkerRegistrationInfo> registration;
  if (!domainInfo->mControlledDocuments.Get(doc, getter_AddRefs(registration))) {
    return NS_ERROR_FAILURE;
  }

  
  MOZ_ASSERT(registration->mCurrentWorker);

  nsRefPtr<ServiceWorker> serviceWorker;
  nsresult rv = CreateServiceWorkerForWindow(window,
                                             registration->mCurrentWorker->GetScriptSpec(),
                                             registration->mScope,
                                             getter_AddRefs(serviceWorker));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  serviceWorker.forget(aServiceWorker);
  return NS_OK;
}

NS_IMETHODIMP
ServiceWorkerManager::GetInstalling(nsIDOMWindow* aWindow,
                                    nsISupports** aServiceWorker)
{
  return GetServiceWorkerForWindow(aWindow, WhichServiceWorker::INSTALLING_WORKER,
                                   aServiceWorker);
}

NS_IMETHODIMP
ServiceWorkerManager::GetWaiting(nsIDOMWindow* aWindow,
                                 nsISupports** aServiceWorker)
{
  return GetServiceWorkerForWindow(aWindow, WhichServiceWorker::WAITING_WORKER,
                                   aServiceWorker);
}

NS_IMETHODIMP
ServiceWorkerManager::GetActive(nsIDOMWindow* aWindow, nsISupports** aServiceWorker)
{
  return GetServiceWorkerForWindow(aWindow, WhichServiceWorker::ACTIVE_WORKER,
                                   aServiceWorker);
}

NS_IMETHODIMP
ServiceWorkerManager::CreateServiceWorker(const nsACString& aScriptSpec,
                                          const nsACString& aScope,
                                          ServiceWorker** aServiceWorker)
{
  AssertIsOnMainThread();

  WorkerPrivate::LoadInfo info;
  nsresult rv = NS_NewURI(getter_AddRefs(info.mBaseURI), aScriptSpec, nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  info.mResolvedScriptURI = info.mBaseURI;

  rv = info.mBaseURI->GetHost(info.mDomain);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  rv = ssm->GetNoAppCodebasePrincipal(info.mBaseURI, getter_AddRefs(info.mPrincipal));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  AutoSafeJSContext cx;

  nsRefPtr<ServiceWorker> serviceWorker;
  RuntimeService* rs = RuntimeService::GetService();
  if (!rs) {
    return NS_ERROR_FAILURE;
  }

  rv = rs->CreateServiceWorkerFromLoadInfo(cx, info, NS_ConvertUTF8toUTF16(aScriptSpec), aScope,
                                           getter_AddRefs(serviceWorker));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  serviceWorker.forget(aServiceWorker);
  return NS_OK;
}

void
ServiceWorkerManager::InvalidateServiceWorkerRegistrationWorker(ServiceWorkerRegistrationInfo* aRegistration,
                                                                WhichServiceWorker aWhichOnes)
{
  AssertIsOnMainThread();
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo =
    GetDomainInfo(aRegistration->mScriptSpec);

  if (domainInfo) {
    nsTObserverArray<ServiceWorkerRegistration*>::ForwardIterator it(domainInfo->mServiceWorkerRegistrations);
    while (it.HasMore()) {
      nsRefPtr<ServiceWorkerRegistration> target = it.GetNext();

      nsIURI* targetURI = target->GetDocumentURI();
      nsCString path;
      nsresult rv = targetURI->GetSpec(path);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        continue;
      }

      nsCString scope = FindScopeForPath(domainInfo->mOrderedScopes, path);
      if (scope.IsEmpty() ||
          !scope.Equals(aRegistration->mScope)) {
        continue;
      }

      target->InvalidateWorkerReference(aWhichOnes);
    }
  }
}

END_WORKERS_NAMESPACE
