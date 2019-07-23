





































#ifndef txXPathNode_h__
#define txXPathNode_h__

#ifdef TX_EXE
#include "txDOM.h"
#else
#include "nsAutoPtr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"
#include "nsContentUtils.h"
#endif

#ifdef TX_EXE
typedef Node txXPathNodeType;
#else
typedef nsIDOMNode txXPathNodeType;
#endif

class txXPathNode
{
public:
    PRBool operator==(const txXPathNode& aNode) const;
    PRBool operator!=(const txXPathNode& aNode) const
    {
        return !(*this == aNode);
    }
    ~txXPathNode();

private:
    friend class txNodeSet;
    friend class txXPathNativeNode;
    friend class txXPathNodeUtils;
    friend class txXPathTreeWalker;

    txXPathNode(const txXPathNode& aNode);

#ifdef TX_EXE
    txXPathNode(NodeDefinition* aNode) : mInner(aNode)
    {
    }

    NodeDefinition* mInner;
#else
    txXPathNode(nsIDocument* aDocument) : mNode(aDocument),
                                          mRefCountRoot(0),
                                          mIndex(eDocument)
    {
        MOZ_COUNT_CTOR(txXPathNode);
    }
    txXPathNode(nsINode *aNode, PRUint32 aIndex, nsINode *aRoot)
        : mNode(aNode),
          mRefCountRoot(aRoot ? 1 : 0),
          mIndex(aIndex)
    {
        MOZ_COUNT_CTOR(txXPathNode);
        if (aRoot) {
            NS_ADDREF(aRoot);
        }
    }

    static nsINode *RootOf(nsINode *aNode)
    {
        nsINode *ancestor, *root = aNode;
        while ((ancestor = root->GetNodeParent())) {
            root = ancestor;
        }
        return root;
    }
    nsINode *Root() const
    {
        return RootOf(mNode);
    }
    nsINode *GetRootToAddRef() const
    {
        return mRefCountRoot ? Root() : nsnull;
    }

    PRBool isDocument() const
    {
        return mIndex == eDocument;
    }
    PRBool isContent() const
    {
        return mIndex == eContent;
    }
    PRBool isAttribute() const
    {
        return mIndex != eDocument && mIndex != eContent;
    }

    nsIContent* Content() const
    {
        NS_ASSERTION(isContent() || isAttribute(), "wrong type");
        return NS_STATIC_CAST(nsIContent*, mNode);
    }
    nsIDocument* Document() const
    {
        NS_ASSERTION(isDocument(), "wrong type");
        return NS_STATIC_CAST(nsIDocument*, mNode);
    }

    enum PositionType
    {
        eDocument = (1 << 30),
        eContent = eDocument - 1
    };

    nsINode* mNode;
    PRUint32 mRefCountRoot : 1;
    PRUint32 mIndex : 31;
#endif
};

class txNamespaceManager
{
public:
    static PRInt32 getNamespaceID(const nsAString& aNamespaceURI);
    static nsresult getNamespaceURI(const PRInt32 aID, nsAString& aResult);
};


inline PRInt32
txNamespaceManager::getNamespaceID(const nsAString& aNamespaceURI)
{
#ifdef TX_EXE
    return txStandaloneNamespaceManager::getNamespaceID(aNamespaceURI);
#else
    PRInt32 namespaceID = kNameSpaceID_Unknown;
    nsContentUtils::NameSpaceManager()->
        RegisterNameSpace(aNamespaceURI, namespaceID);
    return namespaceID;
#endif
}


inline nsresult
txNamespaceManager::getNamespaceURI(const PRInt32 aID, nsAString& aResult)
{
#ifdef TX_EXE
    return txStandaloneNamespaceManager::getNamespaceURI(aID, aResult);
#else
    return nsContentUtils::NameSpaceManager()->
        GetNameSpaceURI(aID, aResult);
#endif
}

inline PRBool
txXPathNode::operator==(const txXPathNode& aNode) const
{
#ifdef TX_EXE
    return (mInner == aNode.mInner);
#else
    return mIndex == aNode.mIndex && mNode == aNode.mNode;
#endif
}

#endif 
