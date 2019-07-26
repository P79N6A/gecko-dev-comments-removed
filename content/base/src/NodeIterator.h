




 




#ifndef mozilla_dom_NodeIterator_h
#define mozilla_dom_NodeIterator_h

#include "nsIDOMNodeIterator.h"
#include "nsTraversal.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStubMutationObserver.h"

class nsINode;
class nsIDOMNode;

namespace mozilla {
namespace dom {

class NodeIterator : public nsIDOMNodeIterator,
                     public nsTraversal,
                     public nsStubMutationObserver
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMNODEITERATOR

    NodeIterator(nsINode *aRoot,
                 uint32_t aWhatToShow,
                 const NodeFilterHolder &aFilter);
    virtual ~NodeIterator();

    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(NodeIterator, nsIDOMNodeIterator)

    
    nsINode* Root() const
    {
        return mRoot;
    }
    nsINode* GetReferenceNode() const
    {
        return mPointer.mNode;
    }
    bool PointerBeforeReferenceNode() const
    {
        return mPointer.mBeforeNode;
    }
    uint32_t WhatToShow() const
    {
        return mWhatToShow;
    }
    already_AddRefed<NodeFilter> GetFilter()
    {
        return mFilter.ToWebIDLCallback();
    }
    already_AddRefed<nsINode> NextNode(ErrorResult& aResult)
    {
        return NextOrPrevNode(&NodePointer::MoveToNext, aResult);
    }
    already_AddRefed<nsINode> PreviousNode(ErrorResult& aResult)
    {
        return NextOrPrevNode(&NodePointer::MoveToPrevious, aResult);
    }
    

private:
    struct NodePointer {
        NodePointer() : mNode(nullptr) {}
        NodePointer(nsINode *aNode, bool aBeforeNode);

        typedef bool (NodePointer::*MoveToMethodType)(nsINode*);
        bool MoveToNext(nsINode *aRoot);
        bool MoveToPrevious(nsINode *aRoot);

        bool MoveForward(nsINode *aRoot, nsINode *aNode);
        void MoveBackward(nsINode *aParent, nsINode *aNode);

        void AdjustAfterRemoval(nsINode *aRoot, nsINode *aContainer, nsIContent *aChild, nsIContent *aPreviousSibling);

        void Clear() { mNode = nullptr; }

        nsINode *mNode;
        bool mBeforeNode;
    };

    
    typedef already_AddRefed<nsINode> (NodeIterator::*NodeGetter)(ErrorResult&);
    inline nsresult ImplNodeGetter(NodeGetter aGetter, nsIDOMNode** aRetval)
    {
        mozilla::ErrorResult rv;
        nsCOMPtr<nsINode> node = (this->*aGetter)(rv);
        if (rv.Failed()) {
            return rv.ErrorCode();
        }
        *aRetval = node ? node.forget().get()->AsDOMNode() : nullptr;
        return NS_OK;
    }

    
    
    already_AddRefed<nsINode>
    NextOrPrevNode(NodePointer::MoveToMethodType aMove, ErrorResult& aResult);

    bool mDetached;
    NodePointer mPointer;
    NodePointer mWorkingPointer;
};

} 
} 

#endif 
