




#include "mozilla/dom/DOMStringList.h"
#include "nsError.h"
#include "nsDOMClassInfoID.h"
#include "nsINode.h"

DOMCI_DATA(DOMStringList, mozilla::dom::DOMStringList)

namespace mozilla {
namespace dom {

DOMStringList::DOMStringList()
{
}

DOMStringList::~DOMStringList()
{
}

NS_IMPL_ADDREF(DOMStringList)
NS_IMPL_RELEASE(DOMStringList)
NS_INTERFACE_TABLE_HEAD(DOMStringList)
  NS_INTERFACE_TABLE1(DOMStringList, nsIDOMDOMStringList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMStringList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
DOMStringList::Item(uint32_t aIndex, nsAString& aResult)
{
  if (aIndex >= mNames.Length()) {
    SetDOMStringToNull(aResult);
  } else {
    aResult = mNames[aIndex];
  }

  return NS_OK;
}

NS_IMETHODIMP
DOMStringList::GetLength(uint32_t *aLength)
{
  *aLength = mNames.Length();

  return NS_OK;
}

NS_IMETHODIMP
DOMStringList::Contains(const nsAString& aString, bool *aResult)
{
  *aResult = mNames.Contains(aString);

  return NS_OK;
}

} 
} 
