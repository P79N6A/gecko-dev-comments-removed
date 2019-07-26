





#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/DOMErrorBinding.h"
#include "nsContentUtils.h"
#include "nsDOMException.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(DOMError, mWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMError)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMError)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMError)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
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
  const char *name, *message;
  NS_GetNameAndMessageForDOMNSResult(aValue, &name, &message);

  mName = NS_ConvertASCIItoUTF16(name);
  mMessage = NS_ConvertASCIItoUTF16(message);

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
DOMError::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DOMErrorBinding::Wrap(aCx, aScope, this);
}

 already_AddRefed<DOMError>
DOMError::Constructor(const GlobalObject& aGlobal, const nsAString& aName,
                      const nsAString& aMessage, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.Get());

  

  nsRefPtr<DOMError> ret = new DOMError(window, aName, aMessage);
  return ret.forget();
}

} 
} 
