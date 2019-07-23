









































#include "nsGenericDOMNodeList.h"
#include "nsGenericElement.h"

nsGenericDOMNodeList::nsGenericDOMNodeList() 
{
}

nsGenericDOMNodeList::~nsGenericDOMNodeList()
{
}


NS_IMPL_ADDREF(nsGenericDOMNodeList)
NS_IMPL_RELEASE(nsGenericDOMNodeList)



NS_INTERFACE_MAP_BEGIN(nsGenericDOMNodeList)
  NS_INTERFACE_MAP_ENTRY(nsINodeList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNodeList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNodeList)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NodeList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsGenericDOMNodeList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsINode* node = GetNodeAt(aIndex);
  if (!node) {
    *aReturn = nsnull;
    return NS_OK;
  }

  return CallQueryInterface(node, aReturn);
}
