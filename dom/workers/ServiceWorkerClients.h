





#ifndef mozilla_dom_workers_serviceworkerclients_h
#define mozilla_dom_workers_serviceworkerclients_h

#include "nsAutoPtr.h"
#include "nsWrapperCache.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class Promise;

namespace workers {

class ServiceWorkerGlobalScope;

class ServiceWorkerClients MOZ_FINAL : public nsISupports,
                                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ServiceWorkerClients)

  ServiceWorkerClients(ServiceWorkerGlobalScope* aWorkerScope);

  already_AddRefed<Promise> GetServiced(ErrorResult& aRv);
  already_AddRefed<Promise> ReloadAll(ErrorResult& aRv);

  JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  ServiceWorkerGlobalScope* GetParentObject() const
  {
    return mWorkerScope;
  }

private:
  ~ServiceWorkerClients()
  {
  }

  nsRefPtr<ServiceWorkerGlobalScope> mWorkerScope;
};

} 
} 
} 

#endif 
