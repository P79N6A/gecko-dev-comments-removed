







































#include "nsDOMSettableTokenList.h"


nsDOMSettableTokenList::nsDOMSettableTokenList(nsGenericElement *aElement, nsIAtom* aAttrAtom)
  : nsDOMTokenList(aElement, aAttrAtom)
{
}

nsDOMSettableTokenList::~nsDOMSettableTokenList()
{
}

DOMCI_DATA(DOMSettableTokenList, nsDOMSettableTokenList)

NS_INTERFACE_TABLE_HEAD(nsDOMSettableTokenList)
  NS_INTERFACE_TABLE1(nsDOMSettableTokenList,
                      nsIDOMDOMSettableTokenList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMSettableTokenList)
NS_INTERFACE_MAP_END_INHERITING(nsDOMTokenList)

NS_IMPL_ADDREF_INHERITED(nsDOMSettableTokenList, nsDOMTokenList)
NS_IMPL_RELEASE_INHERITED(nsDOMSettableTokenList, nsDOMTokenList)

NS_IMETHODIMP
nsDOMSettableTokenList::GetValue(nsAString& aResult)
{
  return ToString(aResult);
}

NS_IMETHODIMP
nsDOMSettableTokenList::SetValue(const nsAString& aValue)
{
  if (!mElement) {
    return NS_OK;
  }

  return mElement->SetAttr(kNameSpaceID_None, mAttrAtom, aValue, true);
}

