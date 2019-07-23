






































#include "txXPathTreeWalker.h"
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "txXMLUtils.h"

txXPathTreeWalker::txXPathTreeWalker(const txXPathTreeWalker& aOther)
    : mPosition(aOther.mPosition)
{
}

txXPathTreeWalker::txXPathTreeWalker(const txXPathNode& aNode)
    : mPosition(aNode)
{
}

txXPathTreeWalker::~txXPathTreeWalker()
{
}

#define INNER mPosition.mInner

void
txXPathTreeWalker::moveToRoot()
{
    if (INNER->nodeType != Node::DOCUMENT_NODE) {
        INNER = INNER->ownerDocument;
    }
}

PRBool
txXPathTreeWalker::moveToElementById(const nsAString& aID)
{
    Document* document;
    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        document = NS_STATIC_CAST(Document*, INNER);
    }
    else {
        document = INNER->ownerDocument;
    }

    Element* element =
        document->getElementById(aID);
    if (!element) {
        return PR_FALSE;
    }

    INNER = element;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstAttribute()
{
    if (INNER->nodeType != Node::ELEMENT_NODE) {
        return PR_FALSE;
    }

    Element* element = NS_STATIC_CAST(Element*, INNER);
    Attr *attribute = element->getFirstAttribute();
    while (attribute) {
        if (attribute->getNamespaceID() != kNameSpaceID_XMLNS) {
            INNER = attribute;

            return PR_TRUE;
        }

        attribute = attribute->getNextAttribute();
    }

    return PR_FALSE;
}

PRBool
txXPathTreeWalker::moveToNextAttribute()
{
    
    if (INNER->nodeType != Node::ATTRIBUTE_NODE) {
        return PR_FALSE;
    }

    Element* element = NS_STATIC_CAST(Element*, INNER->getXPathParent());
    Attr *attribute = element->getFirstAttribute();
    while (attribute != INNER) {
        attribute = attribute->getNextAttribute();
    }
    NS_ASSERTION(attribute, "Attr not attribute of it's owner?");

    attribute = attribute->getNextAttribute();

    while (attribute) {
        if (attribute->getNamespaceID() != kNameSpaceID_XMLNS) {
            INNER = attribute;

            return PR_TRUE;
        }

        attribute = attribute->getNextAttribute();
    }

    return PR_FALSE;
}

PRBool
txXPathTreeWalker::moveToNamedAttribute(nsIAtom* aLocalName, PRInt32 aNSID)
{
    if (INNER->nodeType != Node::ELEMENT_NODE) {
        return PR_FALSE;
    }

    Element* element = NS_STATIC_CAST(Element*, INNER);
    NamedNodeMap* attrs = element->getAttributes();
    NodeListDefinition::ListItem* item = attrs->firstItem;
    
    nsCOMPtr<nsIAtom> localName;
    while (item && (item->node->getNamespaceID() != aNSID ||
                    !item->node->getLocalName(getter_AddRefs(localName)) || 
                    localName != aLocalName) {
        item = item->next;
    }
    if (!item) {
        return PR_FALSE;
    }

    INNER = NS_STATIC_CAST(NodeDefinition*, item->node);
    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstChild()
{
    if (!INNER->firstChild) {
        return PR_FALSE;
    }

    INNER = INNER->firstChild;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToLastChild()
{
    if (!INNER->lastChild) {
        return PR_FALSE;
    }

    INNER = INNER->lastChild;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextSibling()
{
    if (!INNER->nextSibling) {
        return PR_FALSE;
    }

    INNER = INNER->nextSibling;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToPreviousSibling()
{
    if (!INNER->previousSibling) {
        return PR_FALSE;
    }

    INNER = INNER->previousSibling;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToParent()
{
    if (INNER->nodeType == Node::ATTRIBUTE_NODE) {
        INNER = NS_STATIC_CAST(NodeDefinition*, INNER->getXPathParent());
        return PR_TRUE;
    }

    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        return PR_FALSE;
    }

    NS_ASSERTION(INNER->parentNode, "orphaned node shouldn't happen");

    INNER = INNER->parentNode;

    return PR_TRUE;
}

txXPathNode::txXPathNode(const txXPathNode& aNode)
    : mInner(aNode.mInner)
{
}

PRBool
txXPathNode::operator==(const txXPathNode& aNode) const
{
    return (mInner == aNode.mInner);
}


PRBool
txXPathNodeUtils::getAttr(const txXPathNode& aNode, nsIAtom* aLocalName,
                          PRInt32 aNSID, nsAString& aValue)
{
    if (aNode.mInner->getNodeType() != Node::ELEMENT_NODE) {
        return PR_FALSE;
    }

    Element* elem = NS_STATIC_CAST(Element*, aNode.mInner);
    return elem->getAttr(aLocalName, aNSID, aValue);
}


already_AddRefed<nsIAtom>
txXPathNodeUtils::getLocalName(const txXPathNode& aNode)
{
    nsIAtom* localName;
    return aNode.mInner->getLocalName(&localName) ? localName : nsnull;
}


void
txXPathNodeUtils::getLocalName(const txXPathNode& aNode, nsAString& aLocalName)
{
    nsCOMPtr<nsIAtom> localName;
    PRBool hasName = aNode.mInner->getLocalName(getter_AddRefs(localName));
    if (hasName && localName) {
        localName->ToString(aLocalName);
    }
}


void
txXPathNodeUtils::getNodeName(const txXPathNode& aNode, nsAString& aName)
{
    aNode.mInner->getNodeName(aName);
}


PRInt32
txXPathNodeUtils::getNamespaceID(const txXPathNode& aNode)
{
    return aNode.mInner->getNamespaceID();
}


void
txXPathNodeUtils::getNamespaceURI(const txXPathNode& aNode, nsAString& aURI)
{
    aNode.mInner->getNamespaceURI(aURI);
}


PRUint16
txXPathNodeUtils::getNodeType(const txXPathNode& aNode)
{
    return aNode.mInner->getNodeType();
}


void
txXPathNodeUtils::appendNodeValueHelper(NodeDefinition* aNode,
                                        nsAString& aResult)
{

    NodeDefinition* child = NS_STATIC_CAST(NodeDefinition*,
                                           aNode->getFirstChild());
    while (child) {
        switch (child->getNodeType()) {
            case Node::TEXT_NODE:
            {
                aResult.Append(child->nodeValue);
            }
            case Node::ELEMENT_NODE:
            {
                appendNodeValueHelper(child, aResult);
            }
        }
        child = NS_STATIC_CAST(NodeDefinition*, child->getNextSibling());
    }
}


void
txXPathNodeUtils::appendNodeValue(const txXPathNode& aNode, nsAString& aResult)
{
    unsigned short type = aNode.mInner->getNodeType();
    if (type == Node::ATTRIBUTE_NODE ||
        type == Node::COMMENT_NODE ||
        type == Node::PROCESSING_INSTRUCTION_NODE ||
        type == Node::TEXT_NODE) {
        aResult.Append(aNode.mInner->nodeValue);

        return;
    }

    NS_ASSERTION(type == Node::ELEMENT_NODE || type == Node::DOCUMENT_NODE,
                 "Element or Document expected");

    appendNodeValueHelper(aNode.mInner, aResult);
}


PRBool
txXPathNodeUtils::isWhitespace(const txXPathNode& aNode)
{
    NS_ASSERTION(aNode.mInner->nodeType == Node::TEXT_NODE, "Wrong type!");

    return XMLUtils::isWhitespace(aNode.mInner->nodeValue);
}


txXPathNode*
txXPathNodeUtils::getDocument(const txXPathNode& aNode)
{
    if (aNode.mInner->nodeType == Node::DOCUMENT_NODE) {
        return new txXPathNode(aNode);
    }

    return new txXPathNode(aNode.mInner->ownerDocument);
}


txXPathNode*
txXPathNodeUtils::getOwnerDocument(const txXPathNode& aNode)
{
    return getDocument(aNode);
}

#ifndef HAVE_64BIT_OS
#define kFmtSize 13
const char gPrintfFmt[] = "id0x%08p";
#else
#define kFmtSize 21
const char gPrintfFmt[] = "id0x%016p";
#endif


nsresult
txXPathNodeUtils::getXSLTId(const txXPathNode& aNode,
                            nsAString& aResult)
{
    CopyASCIItoUTF16(nsPrintfCString(kFmtSize, gPrintfFmt, aNode.mInner),
                    aResult);

    return NS_OK;
}


void
txXPathNodeUtils::getBaseURI(const txXPathNode& aNode, nsAString& aURI)
{
    aNode.mInner->getBaseURI(aURI);
}


PRIntn
txXPathNodeUtils::comparePosition(const txXPathNode& aNode,
                                  const txXPathNode& aOtherNode)
{
    
    if (aNode == aOtherNode) {
        return 0;
    }
    return aNode.mInner->compareDocumentPosition(aOtherNode.mInner);
}


txXPathNode*
txXPathNativeNode::createXPathNode(Node* aNode)
{
    if (aNode != nsnull) {
        return new txXPathNode(NS_STATIC_CAST(NodeDefinition*, aNode));
    }
    return nsnull;
}


nsresult
txXPathNativeNode::getElement(const txXPathNode& aNode, Element** aResult)
{
    if (aNode.mInner->getNodeType() != Node::ELEMENT_NODE) {
        return NS_ERROR_FAILURE;
    }

    *aResult = NS_STATIC_CAST(Element*, aNode.mInner);

    return NS_OK;

}


nsresult
txXPathNativeNode::getDocument(const txXPathNode& aNode, Document** aResult)
{
    if (aNode.mInner->getNodeType() != Node::DOCUMENT_NODE) {
        return NS_ERROR_FAILURE;
    }

    *aResult = NS_STATIC_CAST(Document*, aNode.mInner);

    return NS_OK;
}
