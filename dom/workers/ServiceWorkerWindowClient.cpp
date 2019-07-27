





#include "ServiceWorkerWindowClient.h"

#include "mozilla/dom/ClientBinding.h"

using namespace mozilla::dom;
using namespace mozilla::dom::workers;

JSObject*
ServiceWorkerWindowClient::WrapObject(JSContext* aCx)
{
  return WindowClientBinding::Wrap(aCx, this);
}

already_AddRefed<Promise>
ServiceWorkerWindowClient::Focus() const
{
  ErrorResult result;
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  MOZ_ASSERT(global);

  nsRefPtr<Promise> promise = Promise::Create(global, result);
  if (NS_WARN_IF(result.Failed())) {
    return nullptr;
  }

  promise->MaybeReject(NS_ERROR_NOT_AVAILABLE);
  return promise.forget();
}
