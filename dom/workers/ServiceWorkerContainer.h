





#ifndef mozilla_dom_workers_serviceworkercontainer_h__
#define mozilla_dom_workers_serviceworkercontainer_h__

#include "mozilla/DOMEventTargetHelper.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;
struct RegistrationOptionList;

namespace workers {

class ServiceWorker;


class ServiceWorkerContainer MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerContainer, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(updatefound)
  IMPL_EVENT_HANDLER(controllerchange)
  IMPL_EVENT_HANDLER(reloadpage)
  IMPL_EVENT_HANDLER(error)

  explicit ServiceWorkerContainer(nsPIDOMWindow* aWindow)
    : mWindow(aWindow)
  {
    SetIsDOMBinding();
    StartListeningForEvents();
  }

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  JSObject*
  WrapObject(JSContext* aCx);

  already_AddRefed<Promise>
  Register(const nsAString& aScriptURL,
           const RegistrationOptionList& aOptions,
           ErrorResult& aRv);

  already_AddRefed<Promise>
  Unregister(const nsAString& scope, ErrorResult& aRv);

  already_AddRefed<ServiceWorker>
  GetInstalling();

  already_AddRefed<ServiceWorker>
  GetWaiting();

  already_AddRefed<ServiceWorker>
  GetActive();

  already_AddRefed<ServiceWorker>
  GetController();

  already_AddRefed<Promise>
  GetAll(ErrorResult& aRv);

  already_AddRefed<Promise>
  Ready();

  nsIURI*
  GetDocumentURI() const
  {
    return mWindow->GetDocumentURI();
  }

  
  already_AddRefed<Promise>
  ClearAllServiceWorkerData(ErrorResult& aRv);

  
  void
  GetScopeForUrl(const nsAString& aUrl, nsString& aScope, ErrorResult& aRv);

  
  void
  GetControllingWorkerScriptURLForPath(const nsAString& aPath,
                                       nsString& aScriptURL,
                                       ErrorResult& aRv);
private:
  ~ServiceWorkerContainer()
  {
    StopListeningForEvents();
  }

  void
  StartListeningForEvents();

  void
  StopListeningForEvents();

  nsCOMPtr<nsPIDOMWindow> mWindow;
};

} 
} 
} 

#endif 
