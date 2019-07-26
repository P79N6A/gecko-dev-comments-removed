





#include "DOMMMIError.h"
#include "mozilla/dom/DOMMMIErrorBinding.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMMMIError)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DOMMMIError, DOMError)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DOMMMIError, DOMError)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DOMMMIError)
NS_INTERFACE_MAP_END_INHERITING(DOMError)

NS_IMPL_ADDREF_INHERITED(DOMMMIError, DOMError)
NS_IMPL_RELEASE_INHERITED(DOMMMIError, DOMError)

DOMMMIError::DOMMMIError(nsPIDOMWindow* aWindow, const nsAString& aName,
                         const nsAString& aMessage, const nsAString& aServiceCode,
                         const Nullable<int16_t>& aInfo)
  : DOMError(aWindow, aName, aMessage)
  , mServiceCode(aServiceCode)
  , mInfo(aInfo)
{
}

JSObject*
DOMMMIError::WrapObject(JSContext* aCx)
{
  return DOMMMIErrorBinding::Wrap(aCx, this);
}



 already_AddRefed<DOMMMIError>
DOMMMIError::Constructor(const GlobalObject& aGlobal,
                         const nsAString& aServiceCode,
                         const nsAString& aName,
                         const nsAString& aMessage,
                         const Nullable<int16_t>& aInfo,
                         ErrorResult& aRv) {
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<DOMMMIError> error = new DOMMMIError(window, aName, aMessage,
                                                aServiceCode, aInfo);

  return error.forget();
}
