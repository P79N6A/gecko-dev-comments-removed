





#include "DOMError.h"

#include "nsDOMClassInfo.h"
#include "nsDOMException.h"

using mozilla::dom::DOMError;

namespace {

struct NameMap
{
  PRUint16 code;
  const char* name;
};

} 


already_AddRefed<nsIDOMDOMError>
DOMError::CreateForNSResult(nsresult aRv)
{
  const char* name;
  const char* message;
  aRv = NS_GetNameAndMessageForDOMNSResult(aRv, &name, &message);
  if (NS_FAILED(aRv) || !name) {
    return nullptr;
  }
  return CreateWithName(NS_ConvertASCIItoUTF16(name));
}

NS_IMPL_ADDREF(DOMError)
NS_IMPL_RELEASE(DOMError)

NS_INTERFACE_MAP_BEGIN(DOMError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMError)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMError)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DOMCI_DATA(DOMError, DOMError)

NS_IMETHODIMP
DOMError::GetName(nsAString& aName)
{
  aName = mName;
  return NS_OK;
}
