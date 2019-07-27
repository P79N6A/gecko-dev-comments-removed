





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




class ServiceWorkerRegistration final
{
public:
  
  
  static bool
  WebPushMethodHider(JSContext* unusedContext, JSObject* unusedObject) {
    return false;
  }

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

  void
  GetScope(nsAString& aScope) const
  {
    aScope = mScope;
  }

  
  virtual void
  InvalidateWorkerReference(WhichServiceWorker aWhichOnes) = 0;

  
  virtual void DisconnectFromOwner() override;

protected:
  virtual ~ServiceWorkerRegistrationBase();

  const nsString mScope;
private:
  void
  StartListeningForEvents();

  void
  StopListeningForEvents();

  bool mListeningForEvents;

  nsCOMPtr<nsISupports> mCCDummy;
};

class ServiceWorkerRegistrationMainThread final : public ServiceWorkerRegistrationBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistrationMainThread,
                                           ServiceWorkerRegistrationBase)

  ServiceWorkerRegistrationMainThread(nsPIDOMWindow* aWindow,
                                      const nsAString& aScope)
    : ServiceWorkerRegistrationBase(aWindow, aScope)
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
  
  already_AddRefed<PushManager>
  GetPushManager(ErrorResult& aRv);

  void
  InvalidateWorkerReference(WhichServiceWorker aWhichOnes) override;

private:
  ~ServiceWorkerRegistrationMainThread()
  {}

  already_AddRefed<workers::ServiceWorker>
  GetWorkerReference(WhichServiceWorker aWhichOne);

  
  
  
  
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

  ServiceWorkerRegistrationWorkerThread(nsPIDOMWindow* aWindow,
                                        const nsAString& aScope)
    : ServiceWorkerRegistrationBase(aWindow, aScope)
  {}

  void
  Update()
  {
    MOZ_CRASH("FIXME");
  }

  already_AddRefed<Promise>
  Unregister(ErrorResult& aRv)
  {
    MOZ_CRASH("FIXME");
    return nullptr;
  }

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<workers::ServiceWorker>
  GetInstalling() override;

  already_AddRefed<workers::ServiceWorker>
  GetWaiting() override;

  already_AddRefed<workers::ServiceWorker>
  GetActive() override;

  void
  InvalidateWorkerReference(WhichServiceWorker aWhichOnes) override;

private:
  ~ServiceWorkerRegistrationWorkerThread()
  {}

  nsCOMPtr<nsISupports> mCCDummyWorkerThread;
};

} 
} 

#endif 
