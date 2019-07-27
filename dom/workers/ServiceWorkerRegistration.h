





#ifndef mozilla_dom_ServiceWorkerRegistration_h
#define mozilla_dom_ServiceWorkerRegistration_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/ServiceWorkerBinding.h"
#include "mozilla/dom/ServiceWorkerCommon.h"
#include "mozilla/dom/workers/bindings/WorkerFeature.h"


#include "mozilla/dom/NotificationBinding.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;
class PushManager;
class WorkerListener;

namespace workers {
class ServiceWorker;
class WorkerPrivate;
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

  
  already_AddRefed<Promise>
  ShowNotification(JSContext* aCx,
                   const nsAString& aTitle,
                   const NotificationOptions& aOptions,
                   ErrorResult& aRv);

  already_AddRefed<Promise>
  GetNotifications(const GetNotificationOptions& aOptions, ErrorResult& aRv);

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
                                                  , public workers::WorkerFeature
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistrationWorkerThread,
                                           ServiceWorkerRegistrationBase)

  ServiceWorkerRegistrationWorkerThread(workers::WorkerPrivate* aWorkerPrivate,
                                        const nsAString& aScope);

  void
  Update();

  already_AddRefed<Promise>
  Unregister(ErrorResult& aRv);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  already_AddRefed<Promise>
  ShowNotification(JSContext* aCx,
                   const nsAString& aTitle,
                   const NotificationOptions& aOptions,
                   ErrorResult& aRv);

  already_AddRefed<Promise>
  GetNotifications(const GetNotificationOptions& aOptions, ErrorResult& aRv);

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

  bool
  Notify(JSContext* aCx, workers::Status aStatus) override;

private:
  enum Reason
  {
    RegistrationIsGoingAway = 0,
    WorkerIsGoingAway,
  };

  ~ServiceWorkerRegistrationWorkerThread();

  void
  InitListener();

  void
  ReleaseListener(Reason aReason);

  workers::WorkerPrivate* mWorkerPrivate;
  nsRefPtr<WorkerListener> mListener;
};

} 
} 

#endif
