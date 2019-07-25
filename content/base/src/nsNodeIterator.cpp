










































#include "nsNodeIterator.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsDOMError.h"

#include "nsIContent.h"
#include "nsIDocument.h"

#include "nsContentUtils.h"
#include "nsCOMPtr.h"




nsNodeIterator::NodePointer::NodePointer(nsINode *aNode,
                                         PRBool aBeforeNode) :
    mNode(aNode),
    mBeforeNode(aBeforeNode)
{
}

PRBool nsNodeIterator::NodePointer::MoveToNext(nsINode *aRoot)
{
    NS_ASSERTION(mNode, "Iterating an uninitialized NodePointer");

    if (mBeforeNode) {
        mBeforeNode = PR_FALSE;
        return PR_TRUE;
    }

    return MoveForward(aRoot, mNode, -1);
}

PRBool nsNodeIterator::NodePointer::MoveToPrevious(nsINode *aRoot)
{
    NS_ASSERTION(mNode, "Iterating an uninitialized NodePointer");

    if (!mBeforeNode) {
        mBeforeNode = PR_TRUE;
        return PR_TRUE;
    }

    if (mNode == aRoot)
        return PR_FALSE;

    NS_ASSERTION(mNodeParent == mNode->GetNodeParent(), "Parent node incorrect in MoveToPrevious");
    NS_ASSERTION(mIndexInParent == mNodeParent->IndexOf(mNode), "Index mismatch in MoveToPrevious");
    MoveBackward(mNodeParent, mIndexInParent);

    return PR_TRUE;
}

void nsNodeIterator::NodePointer::AdjustAfterInsertion(nsINode *aRoot,
                                                       nsINode *aContainer,
                                                       PRInt32 aIndexInContainer)
{
    
    
    
    if (!mNode || mNode == aRoot)
        return;

    
    if (aContainer == mNodeParent && aIndexInContainer <= mIndexInParent)
        mIndexInParent++;
}

void nsNodeIterator::NodePointer::AdjustAfterRemoval(nsINode *aRoot,
                                                     nsINode *aContainer,
                                                     nsIContent *aChild,
                                                     PRInt32 aIndexInContainer)
{
    
    
    
    if (!mNode || mNode == aRoot)
        return;

    
    if (aContainer == mNodeParent && aIndexInContainer < mIndexInParent) {
        --mIndexInParent;
        return;
    }

    
    if (!nsContentUtils::ContentIsDescendantOf(mNode, aChild))
        return;

    if (mBeforeNode) {

        if (MoveForward(aRoot, aContainer, aIndexInContainer-1))
            return;

        
        mBeforeNode = PR_FALSE;
    }

    MoveBackward(aContainer, aIndexInContainer);
}

PRBool nsNodeIterator::NodePointer::MoveForward(nsINode *aRoot, nsINode *aParent, PRInt32 aChildNum)
{
    while (1) {
        nsINode *node = aParent->GetChildAt(aChildNum+1);
        if (node) {
            mNode = node;
            mIndexInParent = aChildNum+1;
            mNodeParent = aParent;
            return PR_TRUE;
        }

        if (aParent == aRoot)
            break;

        node = aParent;

        if (node == mNode) {
            NS_ASSERTION(mNodeParent == mNode->GetNodeParent(), "Parent node incorrect in MoveForward");
            NS_ASSERTION(mIndexInParent == mNodeParent->IndexOf(mNode), "Index mismatch in MoveForward");

            aParent = mNodeParent;
            aChildNum = mIndexInParent;
        } else {
            aParent = node->GetNodeParent();
            aChildNum = aParent->IndexOf(node);
        }
    }

    return PR_FALSE;
}

void nsNodeIterator::NodePointer::MoveBackward(nsINode *aParent, PRInt32 aChildNum)
{
    nsINode *sibling = aParent->GetChildAt(aChildNum-1);
    mNode = aParent;
    if (sibling) {
        do {
            mIndexInParent = aChildNum-1;
            mNodeParent = mNode;
            mNode = sibling;

            aChildNum = mNode->GetChildCount();
            sibling = mNode->GetChildAt(aChildNum-1);
        } while (sibling);
    } else {
        mNodeParent = mNode->GetNodeParent();
        if (mNodeParent)
            mIndexInParent = mNodeParent->IndexOf(mNode);
    }
}





nsNodeIterator::nsNodeIterator(nsINode *aRoot,
                               PRUint32 aWhatToShow,
                               nsIDOMNodeFilter *aFilter,
                               PRBool aExpandEntityReferences) :
    nsTraversal(aRoot, aWhatToShow, aFilter, aExpandEntityReferences),
    mDetached(PR_FALSE),
    mPointer(mRoot, PR_TRUE)
{
    aRoot->AddMutationObserver(this);
}

nsNodeIterator::~nsNodeIterator()
{
    
    if (!mDetached && mRoot)
        mRoot->RemoveMutationObserver(this);
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsNodeIterator)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsNodeIterator)
    if (!tmp->mDetached && tmp->mRoot)
        tmp->mRoot->RemoveMutationObserver(tmp);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRoot)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFilter)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsNodeIterator)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFilter)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(NodeIterator, nsNodeIterator)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNodeIterator)
    NS_INTERFACE_MAP_ENTRY(nsIDOMNodeIterator)
    NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNodeIterator)
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NodeIterator)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNodeIterator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNodeIterator)


NS_IMETHODIMP nsNodeIterator::GetRoot(nsIDOMNode * *aRoot)
{
    if (mRoot)
        return CallQueryInterface(mRoot, aRoot);

    *aRoot = nsnull;

    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::GetWhatToShow(PRUint32 *aWhatToShow)
{
    *aWhatToShow = mWhatToShow;
    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::GetFilter(nsIDOMNodeFilter **aFilter)
{
    NS_ENSURE_ARG_POINTER(aFilter);

    nsCOMPtr<nsIDOMNodeFilter> filter = mFilter;
    filter.swap((*aFilter = nsnull));

    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::GetExpandEntityReferences(PRBool *aExpandEntityReferences)
{
    *aExpandEntityReferences = mExpandEntityReferences;
    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::NextNode(nsIDOMNode **_retval)
{
    return NextOrPrevNode(&NodePointer::MoveToNext, _retval);
}


NS_IMETHODIMP nsNodeIterator::PreviousNode(nsIDOMNode **_retval)
{
    return NextOrPrevNode(&NodePointer::MoveToPrevious, _retval);
}

nsresult
nsNodeIterator::NextOrPrevNode(NodePointer::MoveToMethodType aMove,
                               nsIDOMNode **_retval)
{
    nsresult rv;
    PRInt16 filtered;

    *_retval = nsnull;

    if (mDetached || mInAcceptNode)
        return NS_ERROR_DOM_INVALID_STATE_ERR;

    mWorkingPointer = mPointer;

    struct AutoClear {
        NodePointer* mPtr;
        AutoClear(NodePointer* ptr) : mPtr(ptr) {}
       ~AutoClear() { mPtr->Clear(); }
    } ac(&mWorkingPointer);

    while ((mWorkingPointer.*aMove)(mRoot)) {
        nsCOMPtr<nsINode> testNode = mWorkingPointer.mNode;
        rv = TestNode(testNode, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);

        if (mDetached)
            return NS_ERROR_DOM_INVALID_STATE_ERR;

        if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
            mPointer = mWorkingPointer;
            return CallQueryInterface(testNode, _retval);
        }
    }

    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::Detach(void)
{
    if (!mDetached) {
        mRoot->RemoveMutationObserver(this);

        mPointer.Clear();

        mDetached = PR_TRUE;
    }

    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::GetReferenceNode(nsIDOMNode * *aRefNode)
{
    if (mPointer.mNode)
        return CallQueryInterface(mPointer.mNode, aRefNode);

    *aRefNode = nsnull;
    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::GetPointerBeforeReferenceNode(PRBool *aBeforeNode)
{
    *aBeforeNode = mPointer.mBeforeNode;
    return NS_OK;
}





void nsNodeIterator::ContentInserted(nsIDocument *aDocument,
                                     nsIContent *aContainer,
                                     nsIContent *aChild,
                                     PRInt32 aIndexInContainer)
{
    nsINode *container = NODE_FROM(aContainer, aDocument);

    mPointer.AdjustAfterInsertion(mRoot, container, aIndexInContainer);
    mWorkingPointer.AdjustAfterInsertion(mRoot, container, aIndexInContainer);
}


void nsNodeIterator::ContentRemoved(nsIDocument *aDocument,
                                    nsIContent *aContainer,
                                    nsIContent *aChild,
                                    PRInt32 aIndexInContainer,
                                    nsIContent *aPreviousSibling)
{
    nsINode *container = NODE_FROM(aContainer, aDocument);

    mPointer.AdjustAfterRemoval(mRoot, container, aChild, aIndexInContainer);
    mWorkingPointer.AdjustAfterRemoval(mRoot, container, aChild, aIndexInContainer);
}
