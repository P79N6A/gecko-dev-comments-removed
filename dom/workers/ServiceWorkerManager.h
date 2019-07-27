





#ifndef mozilla_dom_workers_serviceworkermanager_h
#define mozilla_dom_workers_serviceworkermanager_h

#include "nsIServiceWorkerManager.h"
#include "nsCOMPtr.h"

#include "ipc/IPCMessageUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Preferences.h"
#include "mozilla/TypedEnumBits.h"
#include "mozilla/WeakPtr.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerBinding.h" 
#include "mozilla/dom/ServiceWorkerCommon.h"
#include "mozilla/dom/ServiceWorkerRegistrar.h"
#include "mozilla/dom/ServiceWorkerRegistrarTypes.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsTArrayForwardDeclare.h"
#include "nsTObserverArray.h"

namespace mozilla {

class OriginAttributes;

namespace dom {

class ServiceWorkerRegistrationListener;

namespace workers {

class ServiceWorker;
class ServiceWorkerClientInfo;
class ServiceWorkerInfo;
class ServiceWorkerJob;
class ServiceWorkerJobQueue;
class ServiceWorkerManagerChild;



class ServiceWorkerRegistrationInfo final : public nsISupports
{
  uint32_t mControlledDocumentsCounter;

  virtual ~ServiceWorkerRegistrationInfo();

public:
  NS_DECL_ISUPPORTS

  nsCString mScope;
  
  
  nsCString mScriptSpec;

  nsCOMPtr<nsIPrincipal> mPrincipal;

  nsRefPtr<ServiceWorkerInfo> mActiveWorker;
  nsRefPtr<ServiceWorkerInfo> mWaitingWorker;
  nsRefPtr<ServiceWorkerInfo> mInstallingWorker;

  
  
  
  bool mPendingUninstall;

  ServiceWorkerRegistrationInfo(const nsACString& aScope,
                                nsIPrincipal* aPrincipal);

  already_AddRefed<ServiceWorkerInfo>
  Newest()
  {
    nsRefPtr<ServiceWorkerInfo> newest;
    if (mInstallingWorker) {
      newest = mInstallingWorker;
    } else if (mWaitingWorker) {
      newest = mWaitingWorker;
    } else {
      newest = mActiveWorker;
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
    return mActiveWorker && mControlledDocumentsCounter > 0;
  }

  void
  Clear();

  void
  PurgeActiveWorker();

  void
  TryToActivate();

  void
  Activate();

  void
  FinishActivate(bool aSuccess);

};







class ServiceWorkerInfo final
{
private:
  const ServiceWorkerRegistrationInfo* mRegistration;
  nsCString mScriptSpec;
  nsString mCacheName;
  ServiceWorkerState mState;

  
  
  uint64_t mServiceWorkerID;

  
  
  
  
  nsAutoTArray<ServiceWorker*, 1> mInstances;
  bool mSkipWaitingFlag;

  ~ServiceWorkerInfo()
  { }

  
  
  uint64_t
  GetNextID() const;

public:
  NS_INLINE_DECL_REFCOUNTING(ServiceWorkerInfo)

  const nsCString&
  ScriptSpec() const
  {
    return mScriptSpec;
  }

  const nsCString&
  Scope() const
  {
    return mRegistration->mScope;
  }

  void SetScriptSpec(const nsCString& aSpec)
  {
    MOZ_ASSERT(!aSpec.IsEmpty());
    mScriptSpec = aSpec;
  }

  bool SkipWaitingFlag() const
  {
    AssertIsOnMainThread();
    return mSkipWaitingFlag;
  }

  void SetSkipWaitingFlag()
  {
    AssertIsOnMainThread();
    mSkipWaitingFlag = true;
  }

  ServiceWorkerInfo(ServiceWorkerRegistrationInfo* aReg,
                    const nsACString& aScriptSpec,
                    const nsAString& aCacheName)
    : mRegistration(aReg)
    , mScriptSpec(aScriptSpec)
    , mCacheName(aCacheName)
    , mState(ServiceWorkerState::EndGuard_)
    , mServiceWorkerID(GetNextID())
    , mSkipWaitingFlag(false)
  {
    MOZ_ASSERT(mRegistration);
    MOZ_ASSERT(!aCacheName.IsEmpty());
  }

  ServiceWorkerState
  State() const
  {
    return mState;
  }

  const nsString&
  CacheName() const
  {
    return mCacheName;
  }

  uint64_t
  ID() const
  {
    return mServiceWorkerID;
  }

  void
  UpdateState(ServiceWorkerState aState);

  
  void
  SetActivateStateUncheckedWithoutEvent(ServiceWorkerState aState)
  {
    mState = aState;
  }

  void
  AppendWorker(ServiceWorker* aWorker);

  void
  RemoveWorker(ServiceWorker* aWorker);
};

#define NS_SERVICEWORKERMANAGER_IMPL_IID                 \
{ /* f4f8755a-69ca-46e8-a65d-775745535990 */             \
  0xf4f8755a,                                            \
  0x69ca,                                                \
  0x46e8,                                                \
  { 0xa6, 0x5d, 0x77, 0x57, 0x45, 0x53, 0x59, 0x90 }     \
}






class ServiceWorkerManager final
  : public nsIServiceWorkerManager
  , public nsIIPCBackgroundChildCreateCallback
  , public nsIObserver
{
  friend class GetReadyPromiseRunnable;
  friend class GetRegistrationsRunnable;
  friend class GetRegistrationRunnable;
  friend class ServiceWorkerJobQueue;
  friend class ServiceWorkerRegisterJob;
  friend class ServiceWorkerRegistrationInfo;
  friend class ServiceWorkerUnregisterJob;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERVICEWORKERMANAGER
  NS_DECL_NSIIPCBACKGROUNDCHILDCREATECALLBACK
  NS_DECL_NSIOBSERVER

  struct RegistrationDataPerPrincipal;
  nsClassHashtable<nsCStringHashKey, RegistrationDataPerPrincipal> mRegistrationInfos;

  nsTObserverArray<ServiceWorkerRegistrationListener*> mServiceWorkerRegistrationListeners;

  nsRefPtrHashtable<nsISupportsHashKey, ServiceWorkerRegistrationInfo> mControlledDocuments;

  
  nsTHashtable<nsISupportsHashKey> mAllDocuments;

  bool
  IsAvailable(const OriginAttributes& aOriginAttributes, nsIURI* aURI);

  bool
  IsControlled(nsIDocument* aDocument, ErrorResult& aRv);

  void
  DispatchFetchEvent(const OriginAttributes& aOriginAttributes,
                     nsIDocument* aDoc,
                     nsIInterceptedChannel* aChannel,
                     bool aIsReload,
                     ErrorResult& aRv);

  void
  SoftUpdate(nsIPrincipal* aPrincipal, const nsACString& aScope);

  void
  SoftUpdate(const OriginAttributes& aOriginAttributes, const nsACString& aScope);

  void
  PropagateSoftUpdate(const OriginAttributes& aOriginAttributes,
                      const nsAString& aScope);

  void
  PropagateRemove(const nsACString& aHost);

  void
  Remove(const nsACString& aHost);

  void
  PropagateRemoveAll();

  void
  RemoveAll();

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetRegistration(nsIPrincipal* aPrincipal, const nsACString& aScope) const;

  ServiceWorkerRegistrationInfo*
  CreateNewRegistration(const nsCString& aScope, nsIPrincipal* aPrincipal);

  void
  RemoveRegistration(ServiceWorkerRegistrationInfo* aRegistration);

  void StoreRegistration(nsIPrincipal* aPrincipal,
                         ServiceWorkerRegistrationInfo* aRegistration);

  void
  FinishFetch(ServiceWorkerRegistrationInfo* aRegistration);

  
  
  bool
  HandleError(JSContext* aCx,
              nsIPrincipal* aPrincipal,
              const nsCString& aScope,
              const nsString& aWorkerURL,
              nsString aMessage,
              nsString aFilename,
              nsString aLine,
              uint32_t aLineNumber,
              uint32_t aColumnNumber,
              uint32_t aFlags);

  void
  GetAllClients(nsIPrincipal* aPrincipal,
                const nsCString& aScope,
                nsTArray<ServiceWorkerClientInfo>& aControlledDocuments);

  void
  MaybeClaimClient(nsIDocument* aDocument,
                   ServiceWorkerRegistrationInfo* aWorkerRegistration);

  nsresult
  ClaimClients(nsIPrincipal* aPrincipal, const nsCString& aScope, uint64_t aId);

  nsresult
  SetSkipWaitingFlag(nsIPrincipal* aPrincipal, const nsCString& aScope,
                     uint64_t aServiceWorkerID);

  static already_AddRefed<ServiceWorkerManager>
  GetInstance();

 void
 LoadRegistration(const ServiceWorkerRegistrationData& aRegistration);

  void
  LoadRegistrations(const nsTArray<ServiceWorkerRegistrationData>& aRegistrations);

  
  
  void
  ForceUnregister(RegistrationDataPerPrincipal* aRegistrationData,
                  ServiceWorkerRegistrationInfo* aRegistration);

  NS_IMETHOD
  AddRegistrationEventListener(const nsAString& aScope,
                               ServiceWorkerRegistrationListener* aListener);

  NS_IMETHOD
  RemoveRegistrationEventListener(const nsAString& aScope,
                                  ServiceWorkerRegistrationListener* aListener);
private:
  ServiceWorkerManager();
  ~ServiceWorkerManager();

  void
  Init();

  ServiceWorkerJobQueue*
  GetOrCreateJobQueue(const nsACString& aOriginSuffix,
                      const nsACString& aScope);

  void
  MaybeRemoveRegistrationInfo(const nsACString& aScopeKey);

  void
  SoftUpdate(const nsACString& aScopeKey, const nsACString& aScope);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetRegistration(const nsACString& aScopeKey,
                  const nsACString& aScope) const;

  void
  AbortCurrentUpdate(ServiceWorkerRegistrationInfo* aRegistration);

  nsresult
  Update(ServiceWorkerRegistrationInfo* aRegistration);

  nsresult
  GetDocumentRegistration(nsIDocument* aDoc, ServiceWorkerRegistrationInfo** aRegistrationInfo);

  NS_IMETHOD
  CreateServiceWorkerForWindow(nsPIDOMWindow* aWindow,
                               ServiceWorkerInfo* aInfo,
                               ServiceWorker** aServiceWorker);

  NS_IMETHOD
  CreateServiceWorker(nsIPrincipal* aPrincipal,
                      ServiceWorkerInfo* aInfo,
                      ServiceWorker** aServiceWorker);

  NS_IMETHODIMP
  GetServiceWorkerForScope(nsIDOMWindow* aWindow,
                           const nsAString& aScope,
                           WhichServiceWorker aWhichWorker,
                           nsISupports** aServiceWorker);

  already_AddRefed<ServiceWorker>
  CreateServiceWorkerForScope(const OriginAttributes& aOriginAttributes,
                              const nsACString& aScope);

  void
  InvalidateServiceWorkerRegistrationWorker(ServiceWorkerRegistrationInfo* aRegistration,
                                            WhichServiceWorker aWhichOnes);

  void
  StartControllingADocument(ServiceWorkerRegistrationInfo* aRegistration,
                            nsIDocument* aDoc);

  void
  StopControllingADocument(ServiceWorkerRegistrationInfo* aRegistration);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(nsPIDOMWindow* aWindow);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(nsIDocument* aDoc);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(nsIPrincipal* aPrincipal, nsIURI* aURI);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(const OriginAttributes& aOriginAttributes,
                                   nsIURI* aURI);

  already_AddRefed<ServiceWorkerRegistrationInfo>
  GetServiceWorkerRegistrationInfo(const nsACString& aScopeKey,
                                   nsIURI* aURI);

  
  
  
  static nsresult
  PrincipalToScopeKey(nsIPrincipal* aPrincipal, nsACString& aKey);

  static void
  AddScopeAndRegistration(const nsACString& aScope,
                          ServiceWorkerRegistrationInfo* aRegistation);

  static bool
  FindScopeForPath(const nsACString& aScopeKey,
                   const nsACString& aPath,
                   RegistrationDataPerPrincipal** aData, nsACString& aMatch);

#ifdef DEBUG
  static bool
  HasScope(nsIPrincipal* aPrincipal, const nsACString& aScope);
#endif

  static void
  RemoveScopeAndRegistration(ServiceWorkerRegistrationInfo* aRegistration);

  void
  QueueFireEventOnServiceWorkerRegistrations(ServiceWorkerRegistrationInfo* aRegistration,
                                             const nsAString& aName);

  void
  FireUpdateFoundOnServiceWorkerRegistrations(ServiceWorkerRegistrationInfo* aRegistration);

  void
  FireControllerChange(ServiceWorkerRegistrationInfo* aRegistration);

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

  void AppendPendingOperation(nsIRunnable* aRunnable);
  void AppendPendingOperation(ServiceWorkerJobQueue* aQueue,
                              ServiceWorkerJob* aJob);

  bool HasBackgroundActor() const
  {
    return !!mActor;
  }

  static PLDHashOperator
  CheckPendingReadyPromisesEnumerator(nsISupports* aSupports,
                                      nsAutoPtr<PendingReadyPromise>& aData,
                                      void* aUnused);

  nsClassHashtable<nsISupportsHashKey, PendingReadyPromise> mPendingReadyPromises;

  void
  MaybeRemoveRegistration(ServiceWorkerRegistrationInfo* aRegistration);

  
  
  
  
  
  void
  RemoveRegistrationInternal(ServiceWorkerRegistrationInfo* aRegistration);

  
  void
  RemoveAllRegistrations(nsIPrincipal* aPrincipal);

  nsRefPtr<ServiceWorkerManagerChild> mActor;

  struct PendingOperation;
  nsTArray<PendingOperation> mPendingOperations;
};

} 
} 
} 

#endif 
