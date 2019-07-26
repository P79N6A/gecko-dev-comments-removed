



#include "ServiceWorkerManager.h"

#include "nsIDocument.h"
#include "nsPIDOMWindow.h"

#include "jsapi.h"

#include "mozilla/Preferences.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsNetUtil.h"
#include "nsTArray.h"

#include "RuntimeService.h"
#include "ServiceWorker.h"
#include "WorkerInlines.h"

using namespace mozilla;
using namespace mozilla::dom;

BEGIN_WORKERS_NAMESPACE





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

    nsRefPtr<ServiceWorkerRegistration> registration = domainInfo->GetRegistration(mScope);

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

    
    
    
    return NS_OK;
  }
};



NS_IMETHODIMP
ServiceWorkerManager::Register(nsIDOMWindow* aWindow, const nsAString& aScope,
                               const nsAString& aScriptURL, nsISupports** aPromise)
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
                                       true );
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
  rv = scopeURI->GetSpec(cleanedScope);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsIRunnable> registerRunnable =
    new RegisterRunnable(window, cleanedScope, scriptURI, promise);
  promise.forget(aPromise);
  return NS_DispatchToCurrentThread(registerRunnable);
}

NS_IMETHODIMP
ServiceWorkerManager::Update(ServiceWorkerRegistration* aRegistration,
                             nsPIDOMWindow* aWindow)
{
  
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
  nsCOMPtr<nsIServiceWorkerManager> swm = do_GetService(SERVICEWORKERMANAGER_CONTRACTID);
  nsRefPtr<ServiceWorkerManager> concrete = do_QueryObject(swm);
  return concrete.forget();
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
