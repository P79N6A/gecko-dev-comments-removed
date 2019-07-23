





































 




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
                       public nsStubMutationObserver
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMNODEITERATOR

    nsNodeIterator(nsINode *aRoot,
                   PRUint32 aWhatToShow,
                   nsIDOMNodeFilter *aFilter,
                   PRBool aExpandEntityReferences);
    virtual ~nsNodeIterator();

    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsNodeIterator, nsIDOMNodeIterator)

private:
    struct NodePointer {
        NodePointer() : mNode(nsnull) {};
        NodePointer(nsINode *aNode, PRBool aBeforeNode);

        typedef PRBool (NodePointer::*MoveToMethodType)(nsINode*);
        PRBool MoveToNext(nsINode *aRoot);
        PRBool MoveToPrevious(nsINode *aRoot);

        PRBool MoveForward(nsINode *aRoot, nsINode *aParent, PRInt32 aChildNum);
        void MoveBackward(nsINode *aParent, PRInt32 aChildNum);

        void AdjustAfterInsertion(nsINode *aRoot, nsINode *aContainer, PRInt32 aIndexInContainer);
        void AdjustAfterRemoval(nsINode *aRoot, nsINode *aContainer, nsIContent *aChild, PRInt32 aIndexInContainer);

        void Clear() { mNode = nsnull; }

        nsINode *mNode;
        
        
        nsINode *mNodeParent;
        PRBool mBeforeNode;
        
        
        PRInt32 mIndexInParent;
    };

    inline nsresult
    NextOrPrevNode(NodePointer::MoveToMethodType aMove,
                   nsIDOMNode **_retval);

    PRBool mDetached;
    NodePointer mPointer;
    NodePointer mWorkingPointer;
};

#endif
