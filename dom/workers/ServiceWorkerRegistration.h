





#ifndef mozilla_dom_ServiceWorkerRegistration_h
#define mozilla_dom_ServiceWorkerRegistration_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/ServiceWorkerBinding.h"
#include "mozilla/dom/ServiceWorkerCommon.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;
class PushManager;

namespace workers {
class ServiceWorker;
}

bool
ServiceWorkerRegistrationVisible(JSContext* aCx, JSObject* aObj);




class ServiceWorkerRegistration final
{
public:
  
  
  static bool
  WebPushMethodHider(JSContext* unusedContext, JSObject* unusedObject) {
    return false;
  }

};



class ServiceWorkerRegistrationListener
{
public:
  NS_IMETHOD_(MozExternalRefCountType) AddRef() = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release() = 0;

  virtual void
  UpdateFound() = 0;

  virtual void
  InvalidateWorkers(WhichServiceWorker aWhichOnes) = 0;

  virtual void
  GetScope(nsAString& aScope) const = 0;
};

class ServiceWorkerRegistrationBase : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistrationBase,
                                           DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(updatefound)

  ServiceWorkerRegistrationBase(nsPIDOMWindow* aWindow,
                                const nsAString& aScope);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override = 0;

  virtual already_AddRefed<workers::ServiceWorker>
  GetInstalling() = 0;

  virtual already_AddRefed<workers::ServiceWorker>
  GetWaiting() = 0;

  virtual already_AddRefed<workers::ServiceWorker>
  GetActive() = 0;

protected:
  virtual ~ServiceWorkerRegistrationBase()
  { }

  const nsString mScope;
private:
  nsCOMPtr<nsISupports> mCCDummy;
};

class ServiceWorkerRegistrationMainThread final : public ServiceWorkerRegistrationBase,
                                                  public ServiceWorkerRegistrationListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistrationMainThread,
                                           ServiceWorkerRegistrationBase)

  ServiceWorkerRegistrationMainThread(nsPIDOMWindow* aWindow,
                                      const nsAString& aScope);

  void
  Update();

  already_AddRefed<Promise>
  Unregister(ErrorResult& aRv);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<workers::ServiceWorker>
  GetInstalling() override;

  already_AddRefed<workers::ServiceWorker>
  GetWaiting() override;
  
  already_AddRefed<workers::ServiceWorker>
  GetActive() override;
  
  already_AddRefed<PushManager>
  GetPushManager(ErrorResult& aRv);

  
  void DisconnectFromOwner() override
  {
    StopListeningForEvents();
    ServiceWorkerRegistrationBase::DisconnectFromOwner();
  }

  
  void
  UpdateFound() override;

  void
  InvalidateWorkers(WhichServiceWorker aWhichOnes) override;

  void
  GetScope(nsAString& aScope) const override
  {
    aScope = mScope;
  }

private:
  ~ServiceWorkerRegistrationMainThread();

  already_AddRefed<workers::ServiceWorker>
  GetWorkerReference(WhichServiceWorker aWhichOne);

  void
  StartListeningForEvents();

  void
  StopListeningForEvents();

  bool mListeningForEvents;

  
  
  
  
  nsRefPtr<workers::ServiceWorker> mInstallingWorker;
  nsRefPtr<workers::ServiceWorker> mWaitingWorker;
  nsRefPtr<workers::ServiceWorker> mActiveWorker;

#ifndef MOZ_SIMPLEPUSH
  nsRefPtr<PushManager> mPushManager;
#endif
};

class ServiceWorkerRegistrationWorkerThread final : public ServiceWorkerRegistrationBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistrationWorkerThread,
                                           ServiceWorkerRegistrationBase)

  ServiceWorkerRegistrationWorkerThread(const nsAString& aScope)
    : ServiceWorkerRegistrationBase(nullptr, aScope)
  {}

  void
  Update();

  already_AddRefed<Promise>
  Unregister(ErrorResult& aRv);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<workers::ServiceWorker>
  GetInstalling() override;

  already_AddRefed<workers::ServiceWorker>
  GetWaiting() override;

  already_AddRefed<workers::ServiceWorker>
  GetActive() override;

  void
  GetScope(nsAString& aScope) const
  {
    aScope = mScope;
  }

private:
  ~ServiceWorkerRegistrationWorkerThread()
  {}

  nsCOMPtr<nsISupports> mCCDummyWorkerThread;
};

} 
} 

#endif 
