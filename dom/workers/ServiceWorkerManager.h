



#ifndef mozilla_dom_workers_serviceworkermanager_h
#define mozilla_dom_workers_serviceworkermanager_h

#include "nsIServiceWorkerManager.h"
#include "nsCOMPtr.h"

#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerContainer.h"
#include "nsRefPtrHashtable.h"
#include "nsTArrayForwardDeclare.h"
#include "nsTObserverArray.h"
#include "nsTWeakRef.h"

class nsIScriptError;

namespace mozilla {
namespace dom {
namespace workers {

class ServiceWorker;
class ServiceWorkerContainer;
class ServiceWorkerUpdateInstance;










class UpdatePromise MOZ_FINAL
{
public:
  UpdatePromise();
  ~UpdatePromise();

  void AddPromise(Promise* aPromise);
  void ResolveAllPromises(const nsACString& aScriptSpec, const nsACString& aScope);
  void RejectAllPromises(nsresult aRv);
  void RejectAllPromises(const ErrorEventInit& aErrorDesc);

  bool
  IsRejected() const
  {
    return mState == Rejected;
  }

private:
  enum {
    Pending,
    Resolved,
    Rejected
  } mState;

  
  
  nsTArray<nsTWeakRef<Promise>> mPromises;
};







class ServiceWorkerInfo MOZ_FINAL
{
  nsCString mScriptSpec;

  ~ServiceWorkerInfo()
  { }

public:
  NS_INLINE_DECL_REFCOUNTING(ServiceWorkerInfo)

  const nsCString&
  GetScriptSpec() const
  {
    return mScriptSpec;
  }

  explicit ServiceWorkerInfo(const nsACString& aScriptSpec)
    : mScriptSpec(aScriptSpec)
  { }
};



class ServiceWorkerRegistration MOZ_FINAL : public nsISupports
{
  uint32_t mControlledDocumentsCounter;

  virtual ~ServiceWorkerRegistration();

public:
  NS_DECL_ISUPPORTS

  nsCString mScope;
  
  
  nsCString mScriptSpec;

  nsRefPtr<ServiceWorkerInfo> mCurrentWorker;
  nsRefPtr<ServiceWorkerInfo> mWaitingWorker;
  nsRefPtr<ServiceWorkerInfo> mInstallingWorker;

  nsAutoPtr<UpdatePromise> mUpdatePromise;
  nsRefPtr<ServiceWorkerUpdateInstance> mUpdateInstance;

  void
  AddUpdatePromiseObserver(Promise* aPromise)
  {
    MOZ_ASSERT(HasUpdatePromise());
    mUpdatePromise->AddPromise(aPromise);
  }

  bool
  HasUpdatePromise()
  {
    return mUpdatePromise;
  }

  
  
  
  bool mPendingUninstall;

  explicit ServiceWorkerRegistration(const nsACString& aScope);

  already_AddRefed<ServiceWorkerInfo>
  Newest()
  {
    nsRefPtr<ServiceWorkerInfo> newest;
    if (mInstallingWorker) {
      newest = mInstallingWorker;
    } else if (mWaitingWorker) {
      newest = mWaitingWorker;
    } else {
      newest = mCurrentWorker;
    }

    return newest.forget();
  }

  void
  StartControllingADocument()
  {
    ++mControlledDocumentsCounter;
  }

  void
  StopControllingADocument()
  {
    --mControlledDocumentsCounter;
  }

  bool
  IsControllingDocuments() const
  {
    return mControlledDocumentsCounter > 0;
  }
};

#define NS_SERVICEWORKERMANAGER_IMPL_IID                 \
{ /* f4f8755a-69ca-46e8-a65d-775745535990 */             \
  0xf4f8755a,                                            \
  0x69ca,                                                \
  0x46e8,                                                \
  { 0xa6, 0x5d, 0x77, 0x57, 0x45, 0x53, 0x59, 0x90 }     \
}






class ServiceWorkerManager MOZ_FINAL : public nsIServiceWorkerManager
{
  friend class ActivationRunnable;
  friend class RegisterRunnable;
  friend class CallInstallRunnable;
  friend class ServiceWorkerUpdateInstance;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERVICEWORKERMANAGER
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SERVICEWORKERMANAGER_IMPL_IID)

  static ServiceWorkerManager* FactoryCreate()
  {
    AssertIsOnMainThread();
    if (!Preferences::GetBool("dom.serviceWorkers.enabled")) {
      return nullptr;
    }

    ServiceWorkerManager* res = new ServiceWorkerManager;
    NS_ADDREF(res);
    return res;
  }

  




  struct ServiceWorkerDomainInfo
  {
    
    
    
    
    
    
    
    nsTArray<nsCString> mOrderedScopes;

    
    nsRefPtrHashtable<nsCStringHashKey, ServiceWorkerRegistration> mServiceWorkerRegistrations;

    
    
    
    
    nsTObserverArray<ServiceWorkerContainer*> mServiceWorkerContainers;

    nsRefPtrHashtable<nsISupportsHashKey, ServiceWorkerRegistration> mControlledDocuments;

    ServiceWorkerDomainInfo()
    { }

    already_AddRefed<ServiceWorkerRegistration>
    GetRegistration(const nsCString& aScope) const
    {
      nsRefPtr<ServiceWorkerRegistration> reg;
      mServiceWorkerRegistrations.Get(aScope, getter_AddRefs(reg));
      return reg.forget();
    }

    ServiceWorkerRegistration*
    CreateNewRegistration(const nsCString& aScope)
    {
      ServiceWorkerRegistration* registration =
        new ServiceWorkerRegistration(aScope);
      
      
      mServiceWorkerRegistrations.Put(aScope, registration);
      ServiceWorkerManager::AddScope(mOrderedScopes, aScope);
      return registration;
    }

    NS_INLINE_DECL_REFCOUNTING(ServiceWorkerDomainInfo)

  private:
    ~ServiceWorkerDomainInfo()
    { }
  };

  nsRefPtrHashtable<nsCStringHashKey, ServiceWorkerDomainInfo> mDomainMap;

  void
  ResolveRegisterPromises(ServiceWorkerRegistration* aRegistration,
                          const nsACString& aWorkerScriptSpec);

  void
  RejectUpdatePromiseObservers(ServiceWorkerRegistration* aRegistration,
                               nsresult aResult);

  void
  RejectUpdatePromiseObservers(ServiceWorkerRegistration* aRegistration,
                               const ErrorEventInit& aErrorDesc);

  void
  FinishFetch(ServiceWorkerRegistration* aRegistration,
              nsPIDOMWindow* aWindow);

  void
  FinishInstall(ServiceWorkerRegistration* aRegistration);

  void
  FinishActivate(ServiceWorkerRegistration* aRegistration);

  void
  HandleError(JSContext* aCx,
              const nsACString& aScope,
              const nsAString& aWorkerURL,
              nsString aMessage,
              nsString aFilename,
              nsString aLine,
              uint32_t aLineNumber,
              uint32_t aColumnNumber,
              uint32_t aFlags);

  static already_AddRefed<ServiceWorkerManager>
  GetInstance();

private:
  ServiceWorkerManager();
  ~ServiceWorkerManager();

  NS_IMETHOD
  Update(ServiceWorkerRegistration* aRegistration, nsPIDOMWindow* aWindow);

  void
  Install(ServiceWorkerRegistration* aRegistration,
          ServiceWorkerInfo* aServiceWorkerInfo);

  NS_IMETHOD
  CreateServiceWorkerForWindow(nsPIDOMWindow* aWindow,
                               const nsACString& aScriptSpec,
                               const nsACString& aScope,
                               ServiceWorker** aServiceWorker);

  NS_IMETHOD
  CreateServiceWorker(const nsACString& aScriptSpec,
                      const nsACString& aScope,
                      ServiceWorker** aServiceWorker);

  static PLDHashOperator
  CleanupServiceWorkerInformation(const nsACString& aDomain,
                                  ServiceWorkerDomainInfo* aDomainInfo,
                                  void *aUnused);

  already_AddRefed<ServiceWorkerDomainInfo>
  GetDomainInfo(nsIDocument* aDoc);

  already_AddRefed<ServiceWorkerDomainInfo>
  GetDomainInfo(nsIURI* aURI);

  already_AddRefed<ServiceWorkerDomainInfo>
  GetDomainInfo(const nsCString& aURL);

  already_AddRefed<ServiceWorkerRegistration>
  GetServiceWorkerRegistration(nsPIDOMWindow* aWindow);

  already_AddRefed<ServiceWorkerRegistration>
  GetServiceWorkerRegistration(nsIDocument* aDoc);

  already_AddRefed<ServiceWorkerRegistration>
  GetServiceWorkerRegistration(nsIURI* aURI);

  static void
  AddScope(nsTArray<nsCString>& aList, const nsACString& aScope);

  static nsCString
  FindScopeForPath(nsTArray<nsCString>& aList, const nsACString& aPath);

  static void
  RemoveScope(nsTArray<nsCString>& aList, const nsACString& aScope);

  void
  FireEventOnServiceWorkerContainers(ServiceWorkerRegistration* aRegistration,
                                     const nsAString& aName);

};

NS_DEFINE_STATIC_IID_ACCESSOR(ServiceWorkerManager,
                              NS_SERVICEWORKERMANAGER_IMPL_IID);

} 
} 
} 

#endif 
