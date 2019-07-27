



#include "mozilla/dom/IccCardLockError.h"
#include "mozilla/dom/IccCardLockErrorBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(IccCardLockError, DOMError)

 already_AddRefed<IccCardLockError>
IccCardLockError::Constructor(const GlobalObject& aGlobal,
                              const nsAString& aName,
                              int16_t aRetryCount,
                              ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<IccCardLockError> result =
    new IccCardLockError(window, aName, aRetryCount);
  return result.forget();
}

IccCardLockError::IccCardLockError(nsPIDOMWindow* aWindow,
                                   const nsAString& aName,
                                   int16_t aRetryCount)
  : DOMError(aWindow, aName)
  , mRetryCount(aRetryCount)
{
}

JSObject*
IccCardLockError::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return IccCardLockErrorBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
