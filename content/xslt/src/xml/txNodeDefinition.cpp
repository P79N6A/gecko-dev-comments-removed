














































#include "txDOM.h"
#include "nsTArray.h"
#include "txURIUtils.h"
#include "txAtoms.h"
#include <string.h>

NodeDefinition::NodeDefinition(NodeType type, nsIAtom *aLocalName,
                               const nsAString& value, Document* owner) :
    mLocalName(aLocalName),
    nodeValue(value),
    nodeType(type),
    parentNode(nsnull),
    previousSibling(nsnull),
    nextSibling(nsnull),
    ownerDocument(owner),
    length(0),
    firstChild(nsnull),
    lastChild(nsnull),
    mOrderInfo(nsnull)
{
}




NodeDefinition::~NodeDefinition()
{
  NodeDefinition* pCurrent = firstChild;
  NodeDefinition* pDestroyer;

  while (pCurrent)
    {
      pDestroyer = pCurrent;
      pCurrent = pCurrent->nextSibling;
      delete pDestroyer;
    }

  delete mOrderInfo;
}

nsresult NodeDefinition::getNodeName(nsAString& aName) const
{
  mLocalName->ToString(aName);
  return NS_OK;
}

nsresult NodeDefinition::getNodeValue(nsAString& aValue)
{
  aValue = nodeValue;
  return NS_OK;
}

unsigned short NodeDefinition::getNodeType() const
{
  return nodeType;
}

Node* NodeDefinition::getParentNode() const
{
  return parentNode;
}

Node* NodeDefinition::getFirstChild() const
{
  return firstChild;
}

Node* NodeDefinition::getLastChild() const
{
  return lastChild;
}

Node* NodeDefinition::getPreviousSibling() const
{
  return previousSibling;
}

Node* NodeDefinition::getNextSibling() const
{
  return nextSibling;
}

Document* NodeDefinition::getOwnerDocument() const
{
  return ownerDocument;
}

Node* NodeDefinition::appendChild(Node* newChild)
{
  return nsnull;
}

NodeDefinition* NodeDefinition::implAppendChild(NodeDefinition* newChild)
{
  
  if (!newChild->previousSibling && !newChild->nextSibling &&
      !newChild->parentNode)
    {
      newChild->previousSibling = lastChild;

      if (lastChild)
        lastChild->nextSibling = newChild;

      lastChild = newChild;

      newChild->parentNode = this;

      if (!newChild->previousSibling)
        firstChild = newChild;

      ++length;

      return newChild;
    }

  return nsnull;
}

NodeDefinition* NodeDefinition::implRemoveChild(NodeDefinition* oldChild)
{
  if (oldChild != firstChild)
    oldChild->previousSibling->nextSibling = oldChild->nextSibling;
  else
    firstChild = oldChild->nextSibling;

  if (oldChild != lastChild)
    oldChild->nextSibling->previousSibling = oldChild->previousSibling;
  else
    lastChild = oldChild->previousSibling;

  oldChild->nextSibling = nsnull;
  oldChild->previousSibling = nsnull;
  oldChild->parentNode = nsnull;

  --length;

  return oldChild;
}

MBool NodeDefinition::hasChildNodes() const
{
  if (firstChild)
    return MB_TRUE;
  else
    return MB_FALSE;
}

MBool NodeDefinition::getLocalName(nsIAtom** aLocalName)
{
  if (!aLocalName)
    return MB_FALSE;
  *aLocalName = 0;
  return MB_TRUE;
}

nsresult NodeDefinition::getNamespaceURI(nsAString& aNSURI)
{
  return txStandaloneNamespaceManager::getNamespaceURI(getNamespaceID(),
                                                       aNSURI);
}

PRInt32 NodeDefinition::getNamespaceID()
{
  return kNameSpaceID_None;
}

Node* NodeDefinition::getXPathParent()
{
  return parentNode;
}







nsresult NodeDefinition::getBaseURI(nsAString& aURI)
{
  Node* node = this;
  nsTArray<nsString> baseUrls;
  nsAutoString url;

  while (node) {
    switch (node->getNodeType()) {
      case Node::ELEMENT_NODE :
        if (((Element*)node)->getAttr(txXMLAtoms::base, kNameSpaceID_XML,
                                      url))
          baseUrls.AppendElement(url);
        break;

      case Node::DOCUMENT_NODE :
        node->getBaseURI(url);
        baseUrls.AppendElement(url);
        break;
    
      default:
        break;
    }
    node = node->getXPathParent();
  }

  PRUint32 count = baseUrls.Length();
  if (count) {
    aURI = baseUrls[--count];

    while (count > 0) {
      nsAutoString dest;
      URIUtils::resolveHref(baseUrls[--count], aURI, dest);
      aURI = dest;
    }
  }
  
  return NS_OK;
} 




PRInt32 NodeDefinition::compareDocumentPosition(Node* aOther)
{
  OrderInfo* myOrder = getOrderInfo();
  OrderInfo* otherOrder = ((NodeDefinition*)aOther)->getOrderInfo();
  if (!myOrder || !otherOrder)
      return -1;

  if (myOrder->mRoot == otherOrder->mRoot) {
    int c = 0;
    while (c < myOrder->mSize && c < otherOrder->mSize) {
      if (myOrder->mOrder[c] < otherOrder->mOrder[c])
        return -1;
      if (myOrder->mOrder[c] > otherOrder->mOrder[c])
        return 1;
      ++c;
    }
    if (c < myOrder->mSize)
      return 1;
    if (c < otherOrder->mSize)
      return -1;
    return 0;
  }

  if (myOrder->mRoot < otherOrder->mRoot)
    return -1;

  return 1;
}




NodeDefinition::OrderInfo* NodeDefinition::getOrderInfo()
{
  if (mOrderInfo)
    return mOrderInfo;

  mOrderInfo = new OrderInfo;
  if (!mOrderInfo)
    return 0;

  Node* parent = getXPathParent();
  if (!parent) {
    mOrderInfo->mOrder = 0;
    mOrderInfo->mSize = 0;
    mOrderInfo->mRoot = this;
    return mOrderInfo;
  }

  OrderInfo* parentOrder = ((NodeDefinition*)parent)->getOrderInfo();
  mOrderInfo->mSize = parentOrder->mSize + 1;
  mOrderInfo->mRoot = parentOrder->mRoot;
  mOrderInfo->mOrder = new PRUint32[mOrderInfo->mSize];
  if (!mOrderInfo->mOrder) {
    delete mOrderInfo;
    mOrderInfo = 0;
    return 0;
  }
  memcpy(mOrderInfo->mOrder,
         parentOrder->mOrder,
         parentOrder->mSize * sizeof(PRUint32*));

  
  int lastElem = parentOrder->mSize;
  switch (getNodeType()) {
    case Node::ATTRIBUTE_NODE:
    {
      NS_ASSERTION(parent->getNodeType() == Node::ELEMENT_NODE,
                   "parent to attribute is not an element");

      Element* elem = (Element*)parent;
      Attr *attribute = elem->getFirstAttribute();
      PRUint32 i = 0;
      while (attribute) {
        if (attribute == this) {
          mOrderInfo->mOrder[lastElem] = i + kTxAttrIndexOffset;
          return mOrderInfo;
        }
        attribute = attribute->getNextAttribute();
        ++i;
      }
      break;
    }
    
    default:
    {
      PRUint32 i = 0;
      Node * child = parent->getFirstChild();
      while (child) {
        if (child == this) {
          mOrderInfo->mOrder[lastElem] = i + kTxChildIndexOffset;
          return mOrderInfo;
        }
        ++i;
        child = child->getNextSibling();
      }
      break;
    }
  }

  NS_ERROR("unable to get childnumber");
  mOrderInfo->mOrder[lastElem] = 0;
  return mOrderInfo;
}




NodeDefinition::OrderInfo::~OrderInfo()
{
    delete [] mOrder;
}
