




#ifndef mozilla_dom_workers_serviceworker_h__
#define mozilla_dom_workers_serviceworker_h__

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ServiceWorkerBinding.h" 

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

namespace workers {

class ServiceWorkerInfo;
class ServiceWorkerManager;
class SharedWorker;

bool
ServiceWorkerVisible(JSContext* aCx, JSObject* aObj);

class ServiceWorker final : public DOMEventTargetHelper
{
  friend class ServiceWorkerManager;
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorker, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(statechange)
  IMPL_EVENT_HANDLER(error)

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  ServiceWorkerState
  State() const
  {
    return mState;
  }

  void
  SetState(ServiceWorkerState aState)
  {
    mState = aState;
  }

  void
  GetScriptURL(nsString& aURL) const;

  void
  DispatchStateChange(ServiceWorkerState aState)
  {
    SetState(aState);
    DOMEventTargetHelper::DispatchTrustedEvent(NS_LITERAL_STRING("statechange"));
  }

  void
  QueueStateChangeEvent(ServiceWorkerState aState);

#ifdef XP_WIN
#undef PostMessage
#endif

  void
  PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
              const Optional<Sequence<JS::Value>>& aTransferable,
              ErrorResult& aRv);

  WorkerPrivate*
  GetWorkerPrivate() const;

private:
  
  ServiceWorker(nsPIDOMWindow* aWindow, ServiceWorkerInfo* aInfo,
                SharedWorker* aSharedWorker);

  
  ~ServiceWorker();

  ServiceWorkerState mState;
  const nsRefPtr<ServiceWorkerInfo> mInfo;

  
  
  
  
  nsRefPtr<SharedWorker> mSharedWorker;
};

} 
} 
} 

#endif 
