





































 




#ifndef nsNodeIterator_h___
#define nsNodeIterator_h___

#include "nsIDOMNodeIterator.h"
#include "nsTraversal.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStubMutationObserver.h"

class nsINode;
class nsIDOMNode;
class nsIDOMNodeFilter;

class nsNodeIterator : public nsIDOMNodeIterator,
                       public nsTraversal,
                       public nsStubMutationObserver2
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMNODEITERATOR

    nsNodeIterator(nsINode *aRoot,
                   PRUint32 aWhatToShow,
                   nsIDOMNodeFilter *aFilter,
                   bool aExpandEntityReferences);
    virtual ~nsNodeIterator();

    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER2_ATTRIBUTECHILDREMOVED

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsNodeIterator, nsIDOMNodeIterator)

private:
    struct NodePointer {
        NodePointer() : mNode(nsnull) {};
        NodePointer(nsINode *aNode, bool aBeforeNode);

        typedef bool (NodePointer::*MoveToMethodType)(nsINode*);
        bool MoveToNext(nsINode *aRoot);
        bool MoveToPrevious(nsINode *aRoot);

        bool MoveForward(nsINode *aRoot, nsINode *aNode);
        void MoveBackward(nsINode *aParent, nsINode *aNode);

        void AdjustAfterRemoval(nsINode *aRoot, nsINode *aContainer, nsIContent *aChild, nsIContent *aPreviousSibling);

        void Clear() { mNode = nsnull; }

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

#endif
