





































#ifndef txXPathTreeWalker_h__
#define txXPathTreeWalker_h__

#include "txCore.h"
#include "txXPathNode.h"
#include "nsINodeInfo.h"
#include "nsTArray.h"

class nsIAtom;
class nsIDOMDocument;

class txUint32Array : public nsTArray<PRUint32>
{
public:
    PRBool AppendValue(PRUint32 aValue)
    {
        return AppendElement(aValue) != nsnull;
    }
    PRBool RemoveValueAt(PRUint32 aIndex)
    {
        if (aIndex < Length()) {
            RemoveElementAt(aIndex);
        }
        return PR_TRUE;
    }
    PRUint32 ValueAt(PRUint32 aIndex) const
    {
        return (aIndex < Length()) ? ElementAt(aIndex) : 0;
    }
};

class txXPathTreeWalker
{
public:
    txXPathTreeWalker(const txXPathTreeWalker& aOther);
    explicit txXPathTreeWalker(const txXPathNode& aNode);

    PRBool getAttr(nsIAtom* aLocalName, PRInt32 aNSID, nsAString& aValue) const;
    PRInt32 getNamespaceID() const;
    PRUint16 getNodeType() const;
    void appendNodeValue(nsAString& aResult) const;
    void getNodeName(nsAString& aName) const;

    void moveTo(const txXPathTreeWalker& aWalker);

    void moveToRoot();
    PRBool moveToParent();
    PRBool moveToElementById(const nsAString& aID);
    PRBool moveToFirstAttribute();
    PRBool moveToNextAttribute();
    PRBool moveToNamedAttribute(nsIAtom* aLocalName, PRInt32 aNSID);
    PRBool moveToFirstChild();
    PRBool moveToLastChild();
    PRBool moveToNextSibling();
    PRBool moveToPreviousSibling();

    PRBool isOnNode(const txXPathNode& aNode) const;

    const txXPathNode& getCurrentPosition() const;

private:
    txXPathNode mPosition;

    PRBool moveToValidAttribute(PRUint32 aStartIndex);
    PRBool moveToSibling(PRInt32 aDir);

    PRUint32 mCurrentIndex;
    txUint32Array mDescendants;
};

class txXPathNodeUtils
{
public:
    static PRBool getAttr(const txXPathNode& aNode, nsIAtom* aLocalName,
                          PRInt32 aNSID, nsAString& aValue);
    static already_AddRefed<nsIAtom> getLocalName(const txXPathNode& aNode);
    static nsIAtom* getPrefix(const txXPathNode& aNode);
    static void getLocalName(const txXPathNode& aNode, nsAString& aLocalName);
    static void getNodeName(const txXPathNode& aNode,
                            nsAString& aName);
    static PRInt32 getNamespaceID(const txXPathNode& aNode);
    static void getNamespaceURI(const txXPathNode& aNode, nsAString& aURI);
    static PRUint16 getNodeType(const txXPathNode& aNode);
    static void appendNodeValue(const txXPathNode& aNode, nsAString& aResult);
    static PRBool isWhitespace(const txXPathNode& aNode);
    static txXPathNode* getDocument(const txXPathNode& aNode);
    static txXPathNode* getOwnerDocument(const txXPathNode& aNode);
    static PRInt32 getUniqueIdentifier(const txXPathNode& aNode);
    static nsresult getXSLTId(const txXPathNode& aNode,
                              const txXPathNode& aBase, nsAString& aResult);
    static void release(txXPathNode* aNode);
    static void getBaseURI(const txXPathNode& aNode, nsAString& aURI);
    static PRIntn comparePosition(const txXPathNode& aNode,
                                  const txXPathNode& aOtherNode);
    static PRBool localNameEquals(const txXPathNode& aNode,
                                  nsIAtom* aLocalName);
    static PRBool isRoot(const txXPathNode& aNode);
    static PRBool isElement(const txXPathNode& aNode);
    static PRBool isAttribute(const txXPathNode& aNode);
    static PRBool isProcessingInstruction(const txXPathNode& aNode);
    static PRBool isComment(const txXPathNode& aNode);
    static PRBool isText(const txXPathNode& aNode);
    static inline PRBool isHTMLElementInHTMLDocument(const txXPathNode& aNode)
    {
      if (!aNode.isContent()) {
        return PR_FALSE;
      }
      nsIContent* content = aNode.Content();
      return content->IsHTML() && content->IsInHTMLDocument();
    }
};

class txXPathNativeNode
{
public:
    static txXPathNode* createXPathNode(nsIDOMNode* aNode,
                                        PRBool aKeepRootAlive = PR_FALSE);
    static txXPathNode* createXPathNode(nsIContent* aContent,
                                        PRBool aKeepRootAlive = PR_FALSE);
    static txXPathNode* createXPathNode(nsIDOMDocument* aDocument);
    static nsresult getNode(const txXPathNode& aNode, nsIDOMNode** aResult);
    static nsIContent* getContent(const txXPathNode& aNode);
    static nsIDocument* getDocument(const txXPathNode& aNode);
    static void addRef(const txXPathNode& aNode)
    {
        NS_ADDREF(aNode.mNode);
    }
    static void release(const txXPathNode& aNode)
    {
        nsINode *node = aNode.mNode;
        NS_RELEASE(node);
    }
};

inline const txXPathNode&
txXPathTreeWalker::getCurrentPosition() const
{
    return mPosition;
}

inline PRBool
txXPathTreeWalker::getAttr(nsIAtom* aLocalName, PRInt32 aNSID,
                           nsAString& aValue) const
{
    return txXPathNodeUtils::getAttr(mPosition, aLocalName, aNSID, aValue);
}

inline PRInt32
txXPathTreeWalker::getNamespaceID() const
{
    return txXPathNodeUtils::getNamespaceID(mPosition);
}

inline void
txXPathTreeWalker::appendNodeValue(nsAString& aResult) const
{
    txXPathNodeUtils::appendNodeValue(mPosition, aResult);
}

inline void
txXPathTreeWalker::getNodeName(nsAString& aName) const
{
    txXPathNodeUtils::getNodeName(mPosition, aName);
}

inline void
txXPathTreeWalker::moveTo(const txXPathTreeWalker& aWalker)
{
    nsINode *root = nsnull;
    if (mPosition.mRefCountRoot) {
        root = mPosition.Root();
    }
    mPosition.mIndex = aWalker.mPosition.mIndex;
    mPosition.mRefCountRoot = aWalker.mPosition.mRefCountRoot;
    mPosition.mNode = aWalker.mPosition.mNode;
    nsINode *newRoot = nsnull;
    if (mPosition.mRefCountRoot) {
        newRoot = mPosition.Root();
    }
    if (root != newRoot) {
        NS_IF_ADDREF(newRoot);
        NS_IF_RELEASE(root);
    }

    mCurrentIndex = aWalker.mCurrentIndex;
    mDescendants.Clear();
}

inline PRBool
txXPathTreeWalker::isOnNode(const txXPathNode& aNode) const
{
    return (mPosition == aNode);
}


inline PRInt32
txXPathNodeUtils::getUniqueIdentifier(const txXPathNode& aNode)
{
    NS_PRECONDITION(!aNode.isAttribute(),
                    "Not implemented for attributes.");
    return NS_PTR_TO_INT32(aNode.mNode);
}


inline void
txXPathNodeUtils::release(txXPathNode* aNode)
{
    NS_RELEASE(aNode->mNode);
}


inline PRBool
txXPathNodeUtils::localNameEquals(const txXPathNode& aNode,
                                  nsIAtom* aLocalName)
{
    if (aNode.isContent() &&
        aNode.Content()->IsElement()) {
        return aNode.Content()->NodeInfo()->Equals(aLocalName);
    }

    nsCOMPtr<nsIAtom> localName = txXPathNodeUtils::getLocalName(aNode);

    return localName == aLocalName;
}


inline PRBool
txXPathNodeUtils::isRoot(const txXPathNode& aNode)
{
    return !aNode.isAttribute() && !aNode.mNode->GetNodeParent();
}


inline PRBool
txXPathNodeUtils::isElement(const txXPathNode& aNode)
{
    return aNode.isContent() &&
           aNode.Content()->IsElement();
}



inline PRBool
txXPathNodeUtils::isAttribute(const txXPathNode& aNode)
{
    return aNode.isAttribute();
}


inline PRBool
txXPathNodeUtils::isProcessingInstruction(const txXPathNode& aNode)
{
    return aNode.isContent() &&
           aNode.Content()->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION);
}


inline PRBool
txXPathNodeUtils::isComment(const txXPathNode& aNode)
{
    return aNode.isContent() &&
           aNode.Content()->IsNodeOfType(nsINode::eCOMMENT);
}


inline PRBool
txXPathNodeUtils::isText(const txXPathNode& aNode)
{
    return aNode.isContent() &&
           aNode.Content()->IsNodeOfType(nsINode::eTEXT);
}

#endif 
