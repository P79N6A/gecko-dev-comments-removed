





#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/DOMErrorBinding.h"
#include "mozilla/dom/DOMException.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(DOMError, mWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMError)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMError)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMError)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(DOMError)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DOMError::DOMError(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
  SetIsDOMBinding();
}

DOMError::DOMError(nsPIDOMWindow* aWindow, nsresult aValue)
  : mWindow(aWindow)
{
  nsCString name, message;
  NS_GetNameAndMessageForDOMNSResult(aValue, name, message);

  CopyUTF8toUTF16(name, mName);
  CopyUTF8toUTF16(message, mMessage);

  SetIsDOMBinding();
}

DOMError::DOMError(nsPIDOMWindow* aWindow, const nsAString& aName)
  : mWindow(aWindow)
  , mName(aName)
{
  SetIsDOMBinding();
}

DOMError::DOMError(nsPIDOMWindow* aWindow, const nsAString& aName,
                   const nsAString& aMessage)
  : mWindow(aWindow)
  , mName(aName)
  , mMessage(aMessage)
{
  SetIsDOMBinding();
}

DOMError::~DOMError()
{
}

JSObject*
DOMError::WrapObject(JSContext* aCx)
{
  return DOMErrorBinding::Wrap(aCx, this);
}

 already_AddRefed<DOMError>
DOMError::Constructor(const GlobalObject& aGlobal,
                      const nsAString& aName, const nsAString& aMessage,
                      ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());

  

  nsRefPtr<DOMError> ret = new DOMError(window, aName, aMessage);
  return ret.forget();
}

} 
} 
