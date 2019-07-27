



#include "ServiceWorkerManager.h"

#include "nsIDocument.h"
#include "nsPIDOMWindow.h"

#include "jsapi.h"

#include "mozilla/Preferences.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/ErrorEvent.h"

#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsTArray.h"

#include "RuntimeService.h"
#include "ServiceWorker.h"
#include "WorkerInlines.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"

using namespace mozilla;
using namespace mozilla::dom;

BEGIN_WORKERS_NAMESPACE

NS_IMPL_ISUPPORTS0(ServiceWorkerRegistration)

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

  nsTArray<nsTWeakRef<Promise>> array;
  array.SwapElements(mPromises);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    nsTWeakRef<Promise>& pendingPromise = array.ElementAt(i);
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

      pendingPromise->MaybeResolve(serviceWorker);
    }
  }
}

void
UpdatePromise::RejectAllPromises(nsresult aRv)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mState == Pending);
  mState = Rejected;

  nsTArray<nsTWeakRef<Promise>> array;
  array.SwapElements(mPromises);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    nsTWeakRef<Promise>& pendingPromise = array.ElementAt(i);
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

  nsTArray<nsTWeakRef<Promise>> array;
  array.SwapElements(mPromises);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    nsTWeakRef<Promise>& pendingPromise = array.ElementAt(i);
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

class FinishFetchOnMainThreadRunnable : public nsRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerUpdateInstance> mUpdateInstance;
public:
  FinishFetchOnMainThreadRunnable
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
  
  ServiceWorkerRegistration* mRegistration;
  nsCString mScriptSpec;
  nsCOMPtr<nsPIDOMWindow> mWindow;

  bool mAborted;
public:
  NS_DECL_ISUPPORTS

  ServiceWorkerUpdateInstance(ServiceWorkerRegistration *aRegistration,
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

    nsMainThreadPtrHandle<ServiceWorkerUpdateInstance> handle =
      new nsMainThreadPtrHolder<ServiceWorkerUpdateInstance>(this);
    
    
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

ServiceWorkerRegistration::ServiceWorkerRegistration(const nsACString& aScope)
  : mScope(aScope),
    mPendingUninstall(false)
{ }

ServiceWorkerRegistration::~ServiceWorkerRegistration()
{ }





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
  aDomainInfo->mServiceWorkerRegistrations.Clear();
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
    nsCString domain;
    nsresult rv = mScriptURI->GetHost(domain);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mPromise->MaybeReject(rv);
      return NS_OK;
    }

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    ServiceWorkerManager::ServiceWorkerDomainInfo* domainInfo =
      swm->mDomainMap.Get(domain);
    
    
    if (!swm->mDomainMap.Get(domain, &domainInfo)) {
      domainInfo = new ServiceWorkerManager::ServiceWorkerDomainInfo;
      swm->mDomainMap.Put(domain, domainInfo);
    }

    nsRefPtr<ServiceWorkerRegistration> registration =
      domainInfo->GetRegistration(mScope);

    nsCString spec;
    rv = mScriptURI->GetSpec(spec);
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

        
        
        
        ServiceWorkerInfo info = registration->Newest();
        if (info.IsValid()) {
          nsRefPtr<ServiceWorker> serviceWorker;
          nsresult rv =
            swm->CreateServiceWorkerForWindow(mWindow,
                                              info.GetScriptSpec(),
                                              registration->mScope,
                                              getter_AddRefs(serviceWorker));

          if (NS_WARN_IF(NS_FAILED(rv))) {
            return NS_ERROR_FAILURE;
          }

          mPromise->MaybeResolve(serviceWorker);
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



NS_IMETHODIMP
ServiceWorkerManager::Register(nsIDOMWindow* aWindow, const nsAString& aScope,
                               const nsAString& aScriptURL,
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
  nsRefPtr<Promise> promise = new Promise(sgo);

  nsCOMPtr<nsIURI> documentURI = window->GetDocumentURI();
  if (!documentURI) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  

  nsresult rv;
  
  if (!Preferences::GetBool("dom.serviceWorkers.testing.enabled")) {
    bool isHttps;
    rv = documentURI->SchemeIs("https", &isHttps);
    if (NS_FAILED(rv) || !isHttps) {
      NS_WARNING("ServiceWorker registration from insecure websites is not allowed.");
      return NS_ERROR_DOM_SECURITY_ERR;
    }
  }

  nsCOMPtr<nsIPrincipal> documentPrincipal;
  if (window->GetExtantDoc()) {
    documentPrincipal = window->GetExtantDoc()->NodePrincipal();
  } else {
    documentPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1");
  }

  nsCOMPtr<nsIURI> scriptURI;
  rv = NS_NewURI(getter_AddRefs(scriptURI), aScriptURL, nullptr, documentURI);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = documentPrincipal->CheckMayLoad(scriptURI, true ,
                                       false );
  if (NS_FAILED(rv)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIURI> scopeURI;
  rv = NS_NewURI(getter_AddRefs(scopeURI), aScope, nullptr, documentURI);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  rv = documentPrincipal->CheckMayLoad(scopeURI, true ,
                                       false );
  if (NS_FAILED(rv)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCString cleanedScope;
  rv = scopeURI->GetSpecIgnoringRef(cleanedScope);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsIRunnable> registerRunnable =
    new RegisterRunnable(window, cleanedScope, scriptURI, promise);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(registerRunnable);
}

void
ServiceWorkerManager::RejectUpdatePromiseObservers(ServiceWorkerRegistration* aRegistration,
                                                   nsresult aRv)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  aRegistration->mUpdatePromise->RejectAllPromises(aRv);
  aRegistration->mUpdatePromise = nullptr;
}

void
ServiceWorkerManager::RejectUpdatePromiseObservers(ServiceWorkerRegistration* aRegistration,
                                                   const ErrorEventInit& aErrorDesc)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aRegistration->HasUpdatePromise());
  aRegistration->mUpdatePromise->RejectAllPromises(aErrorDesc);
  aRegistration->mUpdatePromise = nullptr;
}





NS_IMETHODIMP
ServiceWorkerManager::Update(ServiceWorkerRegistration* aRegistration,
                             nsPIDOMWindow* aWindow)
{
  if (aRegistration->HasUpdatePromise()) {
    NS_WARNING("Already had a UpdatePromise. Aborting that one!");
    RejectUpdatePromiseObservers(aRegistration, NS_ERROR_DOM_ABORT_ERR);
    MOZ_ASSERT(aRegistration->mUpdateInstance);
    aRegistration->mUpdateInstance->Abort();
    aRegistration->mUpdateInstance = nullptr;
  }

  if (aRegistration->mInstallingWorker.IsValid()) {
    
    
    
    
    
    
    aRegistration->mInstallingWorker.Invalidate();
  }

  aRegistration->mUpdatePromise = new UpdatePromise();
  
  
  

  aRegistration->mUpdateInstance =
    new ServiceWorkerUpdateInstance(aRegistration, aWindow);
  aRegistration->mUpdateInstance->Update();

  return NS_OK;
}


NS_IMETHODIMP
ServiceWorkerManager::Unregister(nsIDOMWindow* aWindow, const nsAString& aScope,
                                 nsISupports** aPromise)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  
  MOZ_ASSERT(!nsContentUtils::IsCallerChrome());

  

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}


already_AddRefed<ServiceWorkerManager>
ServiceWorkerManager::GetInstance()
{
  nsCOMPtr<nsIServiceWorkerManager> swm =
    do_GetService(SERVICEWORKERMANAGER_CONTRACTID);
  nsRefPtr<ServiceWorkerManager> concrete = do_QueryObject(swm);
  return concrete.forget();
}

void
ServiceWorkerManager::ResolveRegisterPromises(ServiceWorkerRegistration* aRegistration,
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
ServiceWorkerManager::FinishFetch(ServiceWorkerRegistration* aRegistration,
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

  ServiceWorkerInfo info(aRegistration->mScriptSpec);
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

  nsCString domain;
  rv = uri->GetHost(domain);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  ServiceWorkerDomainInfo* domainInfo;
  if (!mDomainMap.Get(domain, &domainInfo)) {
    return;
  }

  nsCString scope;
  scope.Assign(aScope);
  nsRefPtr<ServiceWorkerRegistration> registration = domainInfo->GetRegistration(scope);
  MOZ_ASSERT(registration);

  ErrorEventInit init;
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

void
ServiceWorkerManager::Install(ServiceWorkerRegistration* aRegistration,
                              ServiceWorkerInfo aServiceWorkerInfo)
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

END_WORKERS_NAMESPACE
