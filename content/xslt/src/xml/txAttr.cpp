










































#include "txDOM.h"
#include "txAtoms.h"
#include "txXMLUtils.h"

Attr::Attr(nsIAtom *aPrefix, nsIAtom *aLocalName, PRInt32 aNamespaceID,
           Element *aOwnerElement, const nsAString &aValue) :
    NodeDefinition(Node::ATTRIBUTE_NODE, aLocalName, aValue,
                   aOwnerElement->getOwnerDocument()),
    mPrefix(aPrefix),
    mNamespaceID(aNamespaceID),
    mOwnerElement(aOwnerElement)
{
}




Node* Attr::appendChild(Node* newChild)
{
  NS_ERROR("not implemented");
  return nsnull;
}

nsresult
Attr::getNodeName(nsAString& aName) const
{
  if (mPrefix) {
    mPrefix->ToString(aName);
    aName.Append(PRUnichar(':'));
  }
  else {
    aName.Truncate();
  }

  const char* ASCIIAtom;
  mLocalName->GetUTF8String(&ASCIIAtom);
  AppendUTF8toUTF16(ASCIIAtom, aName);

  return NS_OK;
}




MBool Attr::getLocalName(nsIAtom** aLocalName)
{
  if (!aLocalName)
    return MB_FALSE;
  *aLocalName = mLocalName;
  NS_ADDREF(*aLocalName);
  return MB_TRUE;
}






PRInt32 Attr::getNamespaceID()
{
  return mNamespaceID;
}




Node* Attr::getXPathParent()
{
  return mOwnerElement;
}
