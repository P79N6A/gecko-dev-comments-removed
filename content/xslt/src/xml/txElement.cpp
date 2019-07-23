










































#include "txDOM.h"
#include "txAtoms.h"
#include "txXMLUtils.h"

Element::Element(nsIAtom *aPrefix, nsIAtom *aLocalName, PRInt32 aNamespaceID,
                 Document* aOwner) :
  NodeDefinition(Node::ELEMENT_NODE, aLocalName, EmptyString(), aOwner),
  mPrefix(aPrefix),
  mNamespaceID(aNamespaceID)
{
}





Element::~Element()
{
}

Node* Element::appendChild(Node* newChild)
{
  switch (newChild->getNodeType())
    {
      case Node::ELEMENT_NODE :
      case Node::TEXT_NODE :
      case Node::COMMENT_NODE :
      case Node::PROCESSING_INSTRUCTION_NODE :
        {
          
          NodeDefinition* pNewChild = (NodeDefinition*)newChild;
          if (pNewChild->getParentNode() == this)
            pNewChild = implRemoveChild(pNewChild);

          return implAppendChild(pNewChild);
        }

      default:
        break;
    }

  return nsnull;
}

nsresult
Element::getNodeName(nsAString& aName) const
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




MBool Element::getLocalName(nsIAtom** aLocalName)
{
  if (!aLocalName)
    return MB_FALSE;
  *aLocalName = mLocalName;
  NS_ADDREF(*aLocalName);
  return MB_TRUE;
}




PRInt32 Element::getNamespaceID()
{
  return mNamespaceID;
}

nsresult
Element::appendAttributeNS(nsIAtom *aPrefix, nsIAtom *aLocalName,
                           PRInt32 aNamespaceID, const nsAString& aValue)
{
  nsAutoPtr<Attr> newAttribute;
  newAttribute = new Attr(aPrefix, aLocalName, aNamespaceID, this, aValue);
  if (!newAttribute) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (mFirstAttribute) {
    Attr *lastAttribute = mFirstAttribute;
    while (lastAttribute->mNextAttribute) {
      lastAttribute = lastAttribute->mNextAttribute;
    }
    lastAttribute->mNextAttribute = newAttribute;
  }
  else {
    mFirstAttribute = newAttribute;
  }

  return NS_OK;
}






MBool Element::getAttr(nsIAtom* aLocalName, PRInt32 aNSID,
                       nsAString& aValue)
{
  aValue.Truncate();
  Attr *attribute = mFirstAttribute;
  while (attribute) {
    if (attribute->equals(aLocalName, aNSID)) {
      attribute->getNodeValue(aValue);

      return PR_TRUE;
    }

    attribute = attribute->mNextAttribute;
  }

  return PR_FALSE;
}





MBool Element::hasAttr(nsIAtom* aLocalName, PRInt32 aNSID)
{
  Attr *attribute = mFirstAttribute;
  while (attribute) {
    if (attribute->equals(aLocalName, aNSID)) {
      return PR_TRUE;
    }

    attribute = attribute->mNextAttribute;
  }

  return PR_FALSE;
}




PRBool
Element::getIDValue(nsAString& aValue)
{
  if (mIDValue.IsEmpty()) {
    return PR_FALSE;
  }
  aValue = mIDValue;
  return PR_TRUE;
}

void
Element::setIDValue(const nsAString& aValue)
{
  mIDValue = aValue;
}
