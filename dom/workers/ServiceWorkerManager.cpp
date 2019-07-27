



#include "ServiceWorkerManager.h"

#include "nsIDOMEventTarget.h"
#include "nsIDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"

#include "jsapi.h"

#include "mozilla/Preferences.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/ErrorEvent.h"
#include "mozilla/dom/InstallEventBinding.h"
#include "mozilla/dom/PromiseNativeHandler.h"

#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsTArray.h"

#include "RuntimeService.h"
#include "ServiceWorker.h"
#include "ServiceWorkerEvents.h"
#include "WorkerInlines.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

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

  ~ServiceWorkerUpdateInstance() {}

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
    nsRefPtr<ServiceWorkerManager::ServiceWorkerDomainInfo> domainInfo;
    
    
    if (!swm->mDomainMap.Get(domain, getter_AddRefs(domainInfo))) {
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

  nsRefPtr<ServiceWorkerDomainInfo> domainInfo;
  if (!mDomainMap.Get(domain, getter_AddRefs(domainInfo))) {
    return;
  }

  nsCString scope;
  scope.Assign(aScope);
  nsRefPtr<ServiceWorkerRegistration> registration = domainInfo->GetRegistration(scope);
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
  nsMainThreadPtrHandle<ServiceWorkerRegistration> mRegistration;

public:
  explicit FinishInstallRunnable(
    const nsMainThreadPtrHandle<ServiceWorkerRegistration>& aRegistration)
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

class CancelServiceWorkerInstallationRunnable MOZ_FINAL : public nsRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistration> mRegistration;

public:
  explicit CancelServiceWorkerInstallationRunnable(
    const nsMainThreadPtrHandle<ServiceWorkerRegistration>& aRegistration)
    : mRegistration(aRegistration)
  {
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();
    
    
    mRegistration->mInstallingWorker.Invalidate();
    return NS_OK;
  }
};




class FinishInstallHandler MOZ_FINAL : public PromiseNativeHandler
{
  nsMainThreadPtrHandle<ServiceWorkerRegistration> mRegistration;

  virtual
  ~FinishInstallHandler()
  { }

public:
  explicit FinishInstallHandler(
    const nsMainThreadPtrHandle<ServiceWorkerRegistration>& aRegistration)
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







class InstallEventRunnable MOZ_FINAL : public WorkerRunnable
{
  nsMainThreadPtrHandle<ServiceWorkerRegistration> mRegistration;
  nsCString mScope;

public:
  InstallEventRunnable(
    WorkerPrivate* aWorkerPrivate,
    const nsMainThreadPtrHandle<ServiceWorkerRegistration>& aRegistration)
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

void
ServiceWorkerManager::Install(ServiceWorkerRegistration* aRegistration,
                              ServiceWorkerInfo aServiceWorkerInfo)
{
  AssertIsOnMainThread();
  aRegistration->mInstallingWorker = aServiceWorkerInfo;

  nsMainThreadPtrHandle<ServiceWorkerRegistration> handle =
    new nsMainThreadPtrHolder<ServiceWorkerRegistration>(aRegistration);

  nsRefPtr<ServiceWorker> serviceWorker;
  nsresult rv =
    CreateServiceWorker(aServiceWorkerInfo.GetScriptSpec(),
                        aRegistration->mScope,
                        getter_AddRefs(serviceWorker));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRegistration->mInstallingWorker.Invalidate();
    return;
  }

  nsRefPtr<InstallEventRunnable> r =
    new InstallEventRunnable(serviceWorker->GetWorkerPrivate(), handle);

  AutoSafeJSContext cx;
  r->Dispatch(cx);

  
  
  
  
  
  
  
  
  
  
  
  
  

  FireEventOnServiceWorkerContainers(aRegistration,
                                     NS_LITERAL_STRING("updatefound"));
}

class ActivationRunnable : public nsRunnable
{
public:
  explicit ActivationRunnable(ServiceWorkerRegistration* aRegistration)
  { }
};

void
ServiceWorkerManager::FinishInstall(ServiceWorkerRegistration* aRegistration)
{
  AssertIsOnMainThread();

  if (aRegistration->mWaitingWorker.IsValid()) {
    
  }

  aRegistration->mWaitingWorker = aRegistration->mInstallingWorker;
  aRegistration->mInstallingWorker.Invalidate();

  
  
  

  

  

  nsRefPtr<ActivationRunnable> r =
    new ActivationRunnable(aRegistration);

  nsresult rv = NS_DispatchToMainThread(r);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    
  }
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

already_AddRefed<ServiceWorkerRegistration>
ServiceWorkerManager::GetServiceWorkerRegistration(nsPIDOMWindow* aWindow)
{
  nsCOMPtr<nsIURI> documentURI = aWindow->GetDocumentURI();
  return GetServiceWorkerRegistration(documentURI);
}

already_AddRefed<ServiceWorkerRegistration>
ServiceWorkerManager::GetServiceWorkerRegistration(nsIDocument* aDoc)
{
  nsCOMPtr<nsIURI> documentURI = aDoc->GetDocumentURI();
  return GetServiceWorkerRegistration(documentURI);
}

already_AddRefed<ServiceWorkerRegistration>
ServiceWorkerManager::GetServiceWorkerRegistration(nsIURI* aURI)
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

  ServiceWorkerRegistration* registration;
  domainInfo->mServiceWorkerRegistrations.Get(scope, &registration);
  
  MOZ_ASSERT(registration);

  return registration;
}

namespace {



void ScopeWithoutStar(const nsACString& aScope, nsACString& out)
{
  if (aScope.Last() == '*') {
    out.Assign(StringHead(aScope, aScope.Length() - 1));
    return;
  }

  out.Assign(aScope);
}
}; 

 void
ServiceWorkerManager::AddScope(nsTArray<nsCString>& aList, const nsACString& aScope)
{
  for (uint32_t i = 0; i < aList.Length(); ++i) {
    const nsCString& current = aList[i];

    
    if (aScope.Equals(current)) {
      return;
    }

    nsCString withoutStar;
    ScopeWithoutStar(current, withoutStar);
    
    
    if (aScope.Equals(withoutStar)) {
      aList.InsertElementAt(i, aScope);
      return;
    }

    
    
    
    if (StringBeginsWith(aScope, withoutStar)) {
      
      
      
      if (aScope.Last() == '*' &&
          withoutStar.Equals(current)) {
        aList.InsertElementAt(i+1, aScope);
      } else {
        aList.InsertElementAt(i, aScope);
      }
      return;
    }
  }

  aList.AppendElement(aScope);
}


 nsCString
ServiceWorkerManager::FindScopeForPath(nsTArray<nsCString>& aList, const nsACString& aPath)
{
  MOZ_ASSERT(aPath.FindChar('*') == -1);

  nsCString match;

  for (uint32_t i = 0; i < aList.Length(); ++i) {
    const nsCString& current = aList[i];
    nsCString withoutStar;
    ScopeWithoutStar(current, withoutStar);
    if (StringBeginsWith(aPath, withoutStar)) {
      
      if (current.Last() == '*' ||
          aPath.Equals(current)) {
        match = current;
        break;
      }
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

NS_IMETHODIMP
ServiceWorkerManager::GetScopeForUrl(const nsAString& aUrl, nsAString& aScope)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUrl, nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ServiceWorkerRegistration> r = GetServiceWorkerRegistration(uri);
  if (!r) {
      return NS_ERROR_FAILURE;
  }

  aScope = NS_ConvertUTF8toUTF16(r->mScope);
  return NS_OK;
}
NS_IMETHODIMP
ServiceWorkerManager::AddContainerEventListener(nsIURI* aDocumentURI, nsIDOMEventTarget* aListener)
{
  MOZ_ASSERT(aDocumentURI);
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

  ServiceWorkerContainer* container = static_cast<ServiceWorkerContainer*>(aListener);
  domainInfo->mServiceWorkerContainers.AppendElement(container);
  return NS_OK;
}

NS_IMETHODIMP
ServiceWorkerManager::RemoveContainerEventListener(nsIURI* aDocumentURI, nsIDOMEventTarget* aListener)
{
  MOZ_ASSERT(aDocumentURI);
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo = GetDomainInfo(aDocumentURI);
  if (!domainInfo) {
    return NS_OK;
  }

  ServiceWorkerContainer* container = static_cast<ServiceWorkerContainer*>(aListener);
  domainInfo->mServiceWorkerContainers.RemoveElement(container);
  return NS_OK;
}

void
ServiceWorkerManager::FireEventOnServiceWorkerContainers(
  ServiceWorkerRegistration* aRegistration,
  const nsAString& aName)
{
  AssertIsOnMainThread();
  nsRefPtr<ServiceWorkerDomainInfo> domainInfo =
    GetDomainInfo(aRegistration->mScriptSpec);

  if (domainInfo) {
    nsTObserverArray<ServiceWorkerContainer*>::ForwardIterator it(domainInfo->mServiceWorkerContainers);
    while (it.HasMore()) {
      nsRefPtr<ServiceWorkerContainer> target = it.GetNext();
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

END_WORKERS_NAMESPACE
