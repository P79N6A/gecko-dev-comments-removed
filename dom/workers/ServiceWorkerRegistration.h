





#ifndef mozilla_dom_ServiceWorkerRegistration_h
#define mozilla_dom_ServiceWorkerRegistration_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/ServiceWorkerCommon.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;

namespace workers {
class ServiceWorker;
}

class ServiceWorkerRegistration MOZ_FINAL : public DOMEventTargetHelper
                                          , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerRegistration,
                                           DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(updatefound)

  ServiceWorkerRegistration(nsPIDOMWindow* aWindow,
                            const nsAString& aScope);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  JSObject*
  WrapObject(JSContext* aCx);

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

  

  nsIURI*
  GetDocumentURI() const;

  void
  InvalidateWorkerReference(WhichServiceWorker aWhichOnes);

private:
  ~ServiceWorkerRegistration();

  already_AddRefed<workers::ServiceWorker>
  GetWorkerReference(WhichServiceWorker aWhichOne);

  void
  StartListeningForEvents();

  void
  StopListeningForEvents();

  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  
  
  
  nsRefPtr<workers::ServiceWorker> mInstallingWorker;
  nsRefPtr<workers::ServiceWorker> mWaitingWorker;
  nsRefPtr<workers::ServiceWorker> mActiveWorker;

  const nsString mScope;

  uint64_t mInnerID;
  bool mIsListeningForEvents;
};

} 
} 

#endif 
