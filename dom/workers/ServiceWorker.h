




#ifndef mozilla_dom_workers_serviceworker_h__
#define mozilla_dom_workers_serviceworker_h__

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ServiceWorkerBinding.h" 

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;

namespace workers {

class SharedWorker;

class ServiceWorker MOZ_FINAL : public DOMEventTargetHelper
{
  friend class RuntimeService;
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ServiceWorker, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(statechange)
  IMPL_EVENT_HANDLER(error)

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  ServiceWorkerState
  State() const
  {
    return mState;
  }

  void
  GetScope(nsString& aScope) const
  {
    aScope = mScope;
  }

  void
  GetUrl(nsString& aURL) const
  {
    aURL = mURL;
  }

  WorkerPrivate*
  GetWorkerPrivate() const;

private:
  
  ServiceWorker(nsPIDOMWindow* aWindow, SharedWorker* aSharedWorker);

  
  ~ServiceWorker();

  ServiceWorkerState mState;
  nsString mScope;
  nsString mURL;

  
  
  
  
  nsRefPtr<SharedWorker> mSharedWorker;
};

} 
} 
} 

#endif 
