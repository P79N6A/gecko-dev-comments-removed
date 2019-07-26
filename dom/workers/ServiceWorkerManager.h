



#ifndef mozilla_dom_workers_serviceworkermanager_h
#define mozilla_dom_workers_serviceworkermanager_h

#include "nsIServiceWorkerManager.h"
#include "nsCOMPtr.h"

#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Promise.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsTArrayForwardDeclare.h"
#include "nsTWeakRef.h"

namespace mozilla {
namespace dom {
namespace workers {

class ServiceWorker;







class ServiceWorkerInfo
{
  const nsCString mScriptSpec;
public:

  bool
  IsValid() const
  {
    return !mScriptSpec.IsVoid();
  }

  const nsCString&
  GetScriptSpec() const
  {
    MOZ_ASSERT(IsValid());
    return mScriptSpec;
  }

  ServiceWorkerInfo()
  { }

  explicit ServiceWorkerInfo(const nsACString& aScriptSpec)
    : mScriptSpec(aScriptSpec)
  { }
};

class ServiceWorkerRegistration
{
public:
  NS_INLINE_DECL_REFCOUNTING(ServiceWorkerRegistration)

  nsCString mScope;
  
  
  nsCString mScriptSpec;

  ServiceWorkerInfo mCurrentWorker;
  ServiceWorkerInfo mWaitingWorker;
  ServiceWorkerInfo mInstallingWorker;

  bool mHasUpdatePromise;

  void
  AddUpdatePromiseObserver(Promise* aPromise)
  {
    
  }

  bool
  HasUpdatePromise()
  {
    return mHasUpdatePromise;
  }

  
  
  
  bool mPendingUninstall;

  explicit ServiceWorkerRegistration(const nsACString& aScope)
    : mScope(aScope),
      mHasUpdatePromise(false),
      mPendingUninstall(false)
  { }

  ServiceWorkerInfo
  Newest() const
  {
    if (mInstallingWorker.IsValid()) {
      return mInstallingWorker;
    } else if (mWaitingWorker.IsValid()) {
      return mWaitingWorker;
    } else {
      return mCurrentWorker;
    }
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
  friend class RegisterRunnable;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERVICEWORKERMANAGER
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SERVICEWORKERMANAGER_IMPL_IID)

  static ServiceWorkerManager* FactoryCreate()
  {
    ServiceWorkerManager* res = new ServiceWorkerManager;
    NS_ADDREF(res);
    return res;
  }

  




  struct ServiceWorkerDomainInfo
  {
    
    nsRefPtrHashtable<nsCStringHashKey, ServiceWorkerRegistration> mServiceWorkerRegistrations;

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
      return registration;
    }
  };

  nsClassHashtable<nsCStringHashKey, ServiceWorkerDomainInfo> mDomainMap;

  
  
  
  
  
  

  static already_AddRefed<ServiceWorkerManager>
  GetInstance();

private:
  ServiceWorkerManager();
  ~ServiceWorkerManager();

  NS_IMETHOD
  Update(ServiceWorkerRegistration* aRegistration, nsPIDOMWindow* aWindow);

  NS_IMETHODIMP
  CreateServiceWorkerForWindow(nsPIDOMWindow* aWindow,
                               const nsACString& aScriptSpec,
                               const nsACString& aScope,
                               ServiceWorker** aServiceWorker);

  static PLDHashOperator
  CleanupServiceWorkerInformation(const nsACString& aDomain,
                                  ServiceWorkerDomainInfo* aDomainInfo,
                                  void *aUnused);
};

NS_DEFINE_STATIC_IID_ACCESSOR(ServiceWorkerManager,
                              NS_SERVICEWORKERMANAGER_IMPL_IID);

} 
} 
} 

#endif 
