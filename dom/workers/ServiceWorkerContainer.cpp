





#include "ServiceWorkerContainer.h"

#include "nsIDocument.h"
#include "nsIServiceWorkerManager.h"
#include "nsPIDOMWindow.h"

#include "nsCycleCollectionParticipant.h"
#include "nsServiceManagerUtils.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerContainerBinding.h"
#include "mozilla/dom/workers/bindings/ServiceWorker.h"

namespace mozilla {
namespace dom {
namespace workers {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ServiceWorkerContainer)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(ServiceWorkerContainer, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(ServiceWorkerContainer, DOMEventTargetHelper)

NS_IMPL_CYCLE_COLLECTION_INHERITED(ServiceWorkerContainer, DOMEventTargetHelper, mWindow)

JSObject*
ServiceWorkerContainer::WrapObject(JSContext* aCx)
{
  return ServiceWorkerContainerBinding::Wrap(aCx, this);
}

already_AddRefed<Promise>
ServiceWorkerContainer::Register(const nsAString& aScriptURL,
                                 const RegistrationOptionList& aOptions,
                                 ErrorResult& aRv)
{
  nsCOMPtr<nsISupports> promise;

  nsresult rv;
  nsCOMPtr<nsIServiceWorkerManager> swm = do_GetService(SERVICEWORKERMANAGER_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  aRv = swm->Register(mWindow, aOptions.mScope, aScriptURL, getter_AddRefs(promise));
  if (aRv.Failed()) {
    return nullptr;
  }

  nsRefPtr<Promise> ret = static_cast<Promise*>(promise.get());
  MOZ_ASSERT(ret);
  return ret.forget();
}

already_AddRefed<Promise>
ServiceWorkerContainer::Unregister(const nsAString& aScope,
                                   ErrorResult& aRv)
{
  nsCOMPtr<nsISupports> promise;

  nsresult rv;
  nsCOMPtr<nsIServiceWorkerManager> swm = do_GetService(SERVICEWORKERMANAGER_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  aRv = swm->Unregister(mWindow, aScope, getter_AddRefs(promise));
  if (aRv.Failed()) {
    return nullptr;
  }

  nsRefPtr<Promise> ret = static_cast<Promise*>(promise.get());
  MOZ_ASSERT(ret);
  return ret.forget();
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerContainer::GetInstalling()
{
  
  return nullptr;
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerContainer::GetWaiting()
{
  
  return nullptr;
}

already_AddRefed<workers::ServiceWorker>
ServiceWorkerContainer::GetCurrent()
{
  
  return nullptr;
}

already_AddRefed<Promise>
ServiceWorkerContainer::GetAll(ErrorResult& aRv)
{
  
  aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
  return nullptr;
}

already_AddRefed<Promise>
ServiceWorkerContainer::WhenReady(ErrorResult& aRv)
{
  
  aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
  return nullptr;
}


already_AddRefed<Promise>
ServiceWorkerContainer::ClearAllServiceWorkerData(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
  return nullptr;
}


void
ServiceWorkerContainer::GetControllingWorkerScriptURLForPath(
                                                        const nsAString& aPath,
                                                        nsString& aScriptURL,
                                                        ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
}
} 
} 
} 
