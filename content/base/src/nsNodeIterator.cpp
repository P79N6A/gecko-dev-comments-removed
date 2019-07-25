










































#include "nsNodeIterator.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsDOMError.h"

#include "nsIContent.h"
#include "nsIDocument.h"

#include "nsContentUtils.h"
#include "nsCOMPtr.h"




nsNodeIterator::NodePointer::NodePointer(nsINode *aNode,
                                         bool aBeforeNode) :
    mNode(aNode),
    mBeforeNode(aBeforeNode)
{
}

bool nsNodeIterator::NodePointer::MoveToNext(nsINode *aRoot)
{
    if (!mNode)
      return PR_FALSE;

    if (mBeforeNode) {
        mBeforeNode = PR_FALSE;
        return PR_TRUE;
    }

    nsINode* child = mNode->GetFirstChild();
    if (child) {
        mNode = child;
        return PR_TRUE;
    }

    return MoveForward(aRoot, mNode);
}

bool nsNodeIterator::NodePointer::MoveToPrevious(nsINode *aRoot)
{
    if (!mNode)
      return PR_FALSE;

    if (!mBeforeNode) {
        mBeforeNode = PR_TRUE;
        return PR_TRUE;
    }

    if (mNode == aRoot)
        return PR_FALSE;

    MoveBackward(mNode->GetNodeParent(), mNode->GetPreviousSibling());

    return PR_TRUE;
}

void nsNodeIterator::NodePointer::AdjustAfterRemoval(nsINode *aRoot,
                                                     nsINode *aContainer,
                                                     nsIContent *aChild,
                                                     nsIContent *aPreviousSibling)
{
    
    if (!mNode || mNode == aRoot)
        return;

    
    if (!nsContentUtils::ContentIsDescendantOf(mNode, aChild))
        return;

    if (mBeforeNode) {

        
        nsINode *nextSibling = aPreviousSibling ? aPreviousSibling->GetNextSibling()
                                                : aContainer->GetFirstChild();

        if (nextSibling) {
            mNode = nextSibling;
            return;
        }

        
        if (MoveForward(aRoot, aContainer))
            return;

        
        mBeforeNode = PR_FALSE;
    }

    MoveBackward(aContainer, aPreviousSibling);
}

bool nsNodeIterator::NodePointer::MoveForward(nsINode *aRoot, nsINode *aNode)
{
    while (1) {
        if (aNode == aRoot)
            break;

        nsINode *sibling = aNode->GetNextSibling();
        if (sibling) {
            mNode = sibling;
            return PR_TRUE;
        }
        aNode = aNode->GetNodeParent();
    }

    return PR_FALSE;
}

void nsNodeIterator::NodePointer::MoveBackward(nsINode *aParent, nsINode *aNode)
{
    if (aNode) {
        do {
            mNode = aNode;
            aNode = aNode->GetLastChild();
        } while (aNode);
    } else {
        mNode = aParent;
    }
}





nsNodeIterator::nsNodeIterator(nsINode *aRoot,
                               PRUint32 aWhatToShow,
                               nsIDOMNodeFilter *aFilter,
                               bool aExpandEntityReferences) :
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
    NS_INTERFACE_MAP_ENTRY(nsIMutationObserver2)
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

    NS_IF_ADDREF(*aFilter = mFilter);

    return NS_OK;
}


NS_IMETHODIMP nsNodeIterator::GetExpandEntityReferences(bool *aExpandEntityReferences)
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


NS_IMETHODIMP nsNodeIterator::GetPointerBeforeReferenceNode(bool *aBeforeNode)
{
    *aBeforeNode = mPointer.mBeforeNode;
    return NS_OK;
}





void nsNodeIterator::ContentRemoved(nsIDocument *aDocument,
                                    nsIContent *aContainer,
                                    nsIContent *aChild,
                                    PRInt32 aIndexInContainer,
                                    nsIContent *aPreviousSibling)
{
    nsINode *container = NODE_FROM(aContainer, aDocument);

    mPointer.AdjustAfterRemoval(mRoot, container, aChild, aPreviousSibling);
    mWorkingPointer.AdjustAfterRemoval(mRoot, container, aChild, aPreviousSibling);
}

void nsNodeIterator::AttributeChildRemoved(nsINode* aAttribute,
                                           nsIContent* aChild)
{
  mPointer.AdjustAfterRemoval(mRoot, aAttribute, aChild, 0);
  mWorkingPointer.AdjustAfterRemoval(mRoot, aAttribute, aChild, 0);
}

