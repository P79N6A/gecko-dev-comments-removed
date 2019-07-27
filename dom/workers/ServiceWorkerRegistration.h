





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

class ServiceWorkerRegistration final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistration,
                                           DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(updatefound)

  ServiceWorkerRegistration(nsPIDOMWindow* aWindow,
                            const nsAString& aScope);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<workers::ServiceWorker>
  GetInstalling();

  already_AddRefed<workers::ServiceWorker>
  GetWaiting();

  already_AddRefed<workers::ServiceWorker>
  GetActive();

  void
  GetScope(nsAString& aScope) const
  {
    aScope = mScope;
  }

  already_AddRefed<Promise>
  Unregister(ErrorResult& aRv);

  
  void
  InvalidateWorkerReference(WhichServiceWorker aWhichOnes);

  
  virtual void DisconnectFromOwner() override;

  already_AddRefed<PushManager>
  GetPushManager(ErrorResult& aRv);

  
  
  static bool
  WebPushMethodHider(JSContext* unusedContext, JSObject* unusedObject) {
    return false;
  }

private:
  ~ServiceWorkerRegistration();

  already_AddRefed<workers::ServiceWorker>
  GetWorkerReference(WhichServiceWorker aWhichOne);

  void
  StartListeningForEvents();

  void
  StopListeningForEvents();

  
  
  
  
  nsRefPtr<workers::ServiceWorker> mInstallingWorker;
  nsRefPtr<workers::ServiceWorker> mWaitingWorker;
  nsRefPtr<workers::ServiceWorker> mActiveWorker;

#ifndef MOZ_SIMPLEPUSH
  nsRefPtr<PushManager> mPushManager;
#endif

  const nsString mScope;
  bool mListeningForEvents;
};

} 
} 

#endif 