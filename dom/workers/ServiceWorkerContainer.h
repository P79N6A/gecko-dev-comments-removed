





#ifndef mozilla_dom_serviceworkercontainer_h__
#define mozilla_dom_serviceworkercontainer_h__

#include "mozilla/DOMEventTargetHelper.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;
struct RegistrationOptions;

namespace workers {
class ServiceWorker;
}


class ServiceWorkerContainer final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorkerContainer, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(controllerchange)
  IMPL_EVENT_HANDLER(reloadpage)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(message)

  static bool IsEnabled(JSContext* aCx, JSObject* aGlobal);

  explicit ServiceWorkerContainer(nsPIDOMWindow* aWindow);

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<Promise>
  Register(const nsAString& aScriptURL,
           const RegistrationOptions& aOptions,
           ErrorResult& aRv);

  already_AddRefed<workers::ServiceWorker>
  GetController();

  already_AddRefed<Promise>
  GetRegistration(const nsAString& aDocumentURL,
                  ErrorResult& aRv);

  already_AddRefed<Promise>
  GetRegistrations(ErrorResult& aRv);

  Promise*
  GetReady(ErrorResult& aRv);

  
  void
  GetScopeForUrl(const nsAString& aUrl, nsString& aScope, ErrorResult& aRv);

  
  void DisconnectFromOwner() override;

  
  
  void
  ControllerChanged(ErrorResult& aRv);

private:
  ~ServiceWorkerContainer();

  void RemoveReadyPromise();

  
  
  nsRefPtr<workers::ServiceWorker> mControllerWorker;

  nsRefPtr<Promise> mReadyPromise;
};

} 
} 

#endif 
