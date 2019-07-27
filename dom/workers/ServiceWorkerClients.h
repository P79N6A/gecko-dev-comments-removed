






#ifndef mozilla_dom_workers_serviceworkerclients_h
#define mozilla_dom_workers_serviceworkerclients_h

#include "nsAutoPtr.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/WorkerScope.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ClientsBinding.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {
namespace workers {

class ServiceWorkerClients final : public nsISupports,
                                   public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ServiceWorkerClients)

  explicit ServiceWorkerClients(ServiceWorkerGlobalScope* aWorkerScope);

  already_AddRefed<Promise>
  MatchAll(const ClientQueryOptions& aOptions, ErrorResult& aRv);

  already_AddRefed<Promise>
  OpenWindow(const nsAString& aUrl);

  already_AddRefed<Promise>
  Claim(ErrorResult& aRv);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  ServiceWorkerGlobalScope*
  GetParentObject() const
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
