




 




#ifndef mozilla_dom_NodeIterator_h
#define mozilla_dom_NodeIterator_h

#include "nsIDOMNodeIterator.h"
#include "nsTraversal.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStubMutationObserver.h"

class nsINode;
class nsIDOMNode;
class nsIDOMNodeFilter;

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

    inline nsresult
    NextOrPrevNode(NodePointer::MoveToMethodType aMove,
                   nsIDOMNode **_retval);

    bool mDetached;
    NodePointer mPointer;
    NodePointer mWorkingPointer;
};

} 
} 

#endif 
