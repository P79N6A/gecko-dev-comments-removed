



#ifndef mozilla_dom_workers_serviceworkermanager_h
#define mozilla_dom_workers_serviceworkermanager_h

#include "nsIServiceWorkerManager.h"
#include "nsCOMPtr.h"

#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Preferences.h"
#include "mozilla/TypedEnum.h"
#include "mozilla/TypedEnumBits.h"
#include "mozilla/WeakPtr.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerCommon.h"
#include "nsRefPtrHashtable.h"
#include "nsTArrayForwardDeclare.h"
#include "nsTObserverArray.h"
#include "nsClassHashtable.h"

class nsIScriptError;

namespace mozilla {
namespace dom {

class ServiceWorkerRegistration;

namespace workers {

class ServiceWorker;
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

  
  
  nsTArray<WeakPtr<Promise>> mPromises;
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



class ServiceWorkerRegistrationInfo MOZ_FINAL : public nsISupports
{
  uint32_t mControlledDocumentsCounter;

  virtual ~ServiceWorkerRegistrationInfo();

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

  explicit ServiceWorkerRegistrationInfo(const nsACString& aScope);

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

  void
  Clear();
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
  friend class CancelServiceWorkerInstallationRunnable;
  friend class ServiceWorkerRegistrationInfo;
  friend class ServiceWorkerUpdateInstance;
  friend class GetReadyPromiseRunnable;
  friend class GetRegistrationsRunnable;
  friend class GetRegistrationRunnable;

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

    
    nsRefPtrHashtable<nsCStringHashKey, ServiceWorkerRegistrationInfo> mServiceWorkerRegistrationInfos;

    nsTObserverArray<ServiceWorkerRegistration*> mServiceWorkerRegistrations;

    nsRefPtrHashtable<nsISupportsHashKey, ServiceWorkerRegistrationInfo> mControlledDocuments;

    ServiceWorkerDomainInfo()
    { }

    already_AddRefed<ServiceWorkerRegistrationInfo>
    GetRegistration(const nsCString& aScope) const
    {
      nsRefPtr<ServiceWorkerRegistrationInfo> reg;
      mServiceWorkerRegistrationInfos.Get(aScope, getter_AddRefs(reg));
      return reg.forget();
    }

    ServiceWorkerRegistrationInfo*
    CreateNewRegistration(const nsCString& aScope)
    {
      ServiceWorkerRegistrationInfo* registration =
        new ServiceWorkerRegistrationInfo(aScope);
      
      
      mServiceWorkerRegistrationInfos.Put(aScope, registration);
      ServiceWorkerManager::AddScope(mOrderedScopes, aScope);
      return registration;
    }

    void
    RemoveRegistration(ServiceWorkerRegistrationInfo* aRegistration)
    {
      MOZ_ASSERT(mServiceWorkerRegistrationInfos.Contains(aRegistration->mScope));
      ServiceWorkerManager::RemoveScope(mOrderedScopes, aRegistration->mScope);
      mServiceWorkerRegistrationInfos.Remove(aRegistration->mScope);
    }

    NS_INLINE_DECL_REFCOUNTING(ServiceWorkerDomainInfo)

  private:
    ~ServiceWorkerDomainInfo()
    { }
  };

  nsRefPtrHashtable<nsCStringHashKey, ServiceWorkerDomainInfo> mDomainMap;

  void
  ResolveRegisterPromises(ServiceWorkerRegistrationInfo* aRegistration,
                          const nsACString& aWorkerScriptSpec);

  void
  RejectUpdatePromiseObservers(ServiceWorkerRegistrationInfo* aRegistration,
                               nsresult aResult);

  void
  RejectUpdatePromiseObservers(ServiceWorkerRegistrationInfo* aRegistration,
                               const ErrorEventInit& aErrorDesc);

  void
  FinishFetch(ServiceWorkerRegistrationInfo* aRegistration,
              nsPIDOMWindow* aWindow);

  void
  FinishInstall(ServiceWorkerRegistrationInfo* aRegistration);

  void
  FinishActivate(ServiceWorkerRegistrationInfo* aRegistration);

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

  void
  AbortCurrentUpdate(ServiceWorkerRegistrationInfo* aRegistration);

  NS_IMETHOD
  Update(ServiceWorkerRegistrationInfo* aRegistration, nsPIDOMWindow* aWindow);

  void
  Install(ServiceWorkerRegistrationInfo* aRegistration,
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

  NS_IMETHODIMP
  GetServiceWorkerForScope(nsIDOMWindow* aWindow,
                           const nsAString& aScope,
                           WhichServiceWorker aWhichWorker,
                           nsISupports** aServiceWorker);

  void
  InvalidateServiceWorkerRegistrationWorker(ServiceWorkerRegistrationInfo* aRegistration,
                                            WhichServiceWorker aWhichOnes);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(nsPIDOMWindow* aWindow);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(nsIDocument* aDoc);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(nsIURI* aURI);

  static void
  AddScope(nsTArray<nsCString>& aList, const nsACString& aScope);

  static nsCString
  FindScopeForPath(nsTArray<nsCString>& aList, const nsACString& aPath);

  static void
  RemoveScope(nsTArray<nsCString>& aList, const nsACString& aScope);

  void
  FireEventOnServiceWorkerRegistrations(ServiceWorkerRegistrationInfo* aRegistration,
                                        const nsAString& aName);

  void
  StorePendingReadyPromise(nsPIDOMWindow* aWindow, nsIURI* aURI, Promise* aPromise);

  void
  CheckPendingReadyPromises();

  bool
  CheckReadyPromise(nsPIDOMWindow* aWindow, nsIURI* aURI, Promise* aPromise);

  struct PendingReadyPromise
  {
    PendingReadyPromise(nsIURI* aURI, Promise* aPromise)
      : mURI(aURI), mPromise(aPromise)
    { }

    nsCOMPtr<nsIURI> mURI;
    nsRefPtr<Promise> mPromise;
  };

  static PLDHashOperator
  CheckPendingReadyPromisesEnumerator(nsISupports* aSupports,
                                      nsAutoPtr<PendingReadyPromise>& aData,
                                      void* aUnused);

  nsClassHashtable<nsISupportsHashKey, PendingReadyPromise> mPendingReadyPromises;
};

NS_DEFINE_STATIC_IID_ACCESSOR(ServiceWorkerManager,
                              NS_SERVICEWORKERMANAGER_IMPL_IID);

} 
} 
} 

#endif 
