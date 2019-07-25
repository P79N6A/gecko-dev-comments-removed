











































#include "nsTreeWalker.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsDOMError.h"
#include "nsINode.h"
#include "nsIContent.h"

#include "nsContentUtils.h"





nsTreeWalker::nsTreeWalker(nsINode *aRoot,
                           PRUint32 aWhatToShow,
                           nsIDOMNodeFilter *aFilter,
                           bool aExpandEntityReferences) :
    nsTraversal(aRoot, aWhatToShow, aFilter, aExpandEntityReferences),
    mCurrentNode(aRoot)
{
}

nsTreeWalker::~nsTreeWalker()
{
    
}





NS_IMPL_CYCLE_COLLECTION_3(nsTreeWalker, mFilter, mCurrentNode, mRoot)

DOMCI_DATA(TreeWalker, nsTreeWalker)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTreeWalker)
    NS_INTERFACE_MAP_ENTRY(nsIDOMTreeWalker)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMTreeWalker)
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TreeWalker)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTreeWalker)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTreeWalker)








NS_IMETHODIMP nsTreeWalker::GetRoot(nsIDOMNode * *aRoot)
{
    if (mRoot) {
        return CallQueryInterface(mRoot, aRoot);
    }

    *aRoot = nsnull;

    return NS_OK;
}


NS_IMETHODIMP nsTreeWalker::GetWhatToShow(PRUint32 *aWhatToShow)
{
    *aWhatToShow = mWhatToShow;
    return NS_OK;
}


NS_IMETHODIMP nsTreeWalker::GetFilter(nsIDOMNodeFilter * *aFilter)
{
    NS_ENSURE_ARG_POINTER(aFilter);

    NS_IF_ADDREF(*aFilter = mFilter);

    return NS_OK;
}


NS_IMETHODIMP
nsTreeWalker::GetExpandEntityReferences(bool *aExpandEntityReferences)
{
    *aExpandEntityReferences = mExpandEntityReferences;
    return NS_OK;
}


NS_IMETHODIMP nsTreeWalker::GetCurrentNode(nsIDOMNode * *aCurrentNode)
{
    if (mCurrentNode) {
        return CallQueryInterface(mCurrentNode, aCurrentNode);
    }

    *aCurrentNode = nsnull;

    return NS_OK;
}
NS_IMETHODIMP nsTreeWalker::SetCurrentNode(nsIDOMNode * aCurrentNode)
{
    NS_ENSURE_TRUE(aCurrentNode, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

    nsresult rv = nsContentUtils::CheckSameOrigin(mRoot, aCurrentNode);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsINode> node = do_QueryInterface(aCurrentNode);
    NS_ENSURE_TRUE(node, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

    mCurrentNode.swap(node);
    return NS_OK;
}






NS_IMETHODIMP nsTreeWalker::ParentNode(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsresult rv;

    nsCOMPtr<nsINode> node = mCurrentNode;

    while (node && node != mRoot) {
        node = node->GetNodeParent();

        if (node) {
            PRInt16 filtered;
            rv = TestNode(node, &filtered);
            NS_ENSURE_SUCCESS(rv, rv);
            if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
                mCurrentNode = node;
                return CallQueryInterface(node, _retval);
            }
        }
    }

    return NS_OK;
}


NS_IMETHODIMP nsTreeWalker::FirstChild(nsIDOMNode **_retval)
{
    return FirstChildInternal(false, _retval);
}


NS_IMETHODIMP nsTreeWalker::LastChild(nsIDOMNode **_retval)
{
    return FirstChildInternal(true, _retval);
}


NS_IMETHODIMP nsTreeWalker::PreviousSibling(nsIDOMNode **_retval)
{
    return NextSiblingInternal(true, _retval);
}


NS_IMETHODIMP nsTreeWalker::NextSibling(nsIDOMNode **_retval)
{
    return NextSiblingInternal(false, _retval);
}


NS_IMETHODIMP nsTreeWalker::PreviousNode(nsIDOMNode **_retval)
{
    nsresult rv;
    PRInt16 filtered;

    *_retval = nsnull;

    nsCOMPtr<nsINode> node = mCurrentNode;

    while (node != mRoot) {
        while (nsINode *previousSibling = node->GetPreviousSibling()) {
            node = previousSibling;

            rv = TestNode(node, &filtered);
            NS_ENSURE_SUCCESS(rv, rv);

            nsINode *lastChild;
            while (filtered != nsIDOMNodeFilter::FILTER_REJECT &&
                   (lastChild = node->GetLastChild())) {
                node = lastChild;
                rv = TestNode(node, &filtered);
                NS_ENSURE_SUCCESS(rv, rv);
            }

            if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
                mCurrentNode = node;
                return CallQueryInterface(node, _retval);
            }
        }

        if (node == mRoot)
            break;

        node = node->GetNodeParent();
        if (!node)
            break;

        rv = TestNode(node, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);

        if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
            mCurrentNode = node;
            return CallQueryInterface(node, _retval);
        }
    }

    return NS_OK;
}


NS_IMETHODIMP nsTreeWalker::NextNode(nsIDOMNode **_retval)
{
    nsresult rv;
    PRInt16 filtered = nsIDOMNodeFilter::FILTER_ACCEPT; 

    *_retval = nsnull;

    nsCOMPtr<nsINode> node = mCurrentNode;

    while (1) {

        nsINode *firstChild;
        while (filtered != nsIDOMNodeFilter::FILTER_REJECT &&
               (firstChild = node->GetFirstChild())) {
            node = firstChild;

            rv = TestNode(node, &filtered);
            NS_ENSURE_SUCCESS(rv, rv);

            if (filtered ==  nsIDOMNodeFilter::FILTER_ACCEPT) {
                
                mCurrentNode = node;
                return CallQueryInterface(node, _retval);
            }
        }

        nsINode *sibling = nsnull;
        nsINode *temp = node;
        do {
            if (temp == mRoot)
                break;

            sibling = temp->GetNextSibling();
            if (sibling)
                break;

            temp = temp->GetNodeParent();
        } while (temp);

        if (!sibling)
            break;

        node = sibling;

        
        rv = TestNode(node, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);

        if (filtered ==  nsIDOMNodeFilter::FILTER_ACCEPT) {
            
            mCurrentNode = node;
            return CallQueryInterface(node, _retval);
        }
    }

    return NS_OK;
}












nsresult nsTreeWalker::FirstChildInternal(bool aReversed, nsIDOMNode **_retval)
{
    nsresult rv;
    PRInt16 filtered;

    *_retval = nsnull;

    nsCOMPtr<nsINode> node = aReversed ? mCurrentNode->GetLastChild()
                                       : mCurrentNode->GetFirstChild();

    while (node) {
        rv = TestNode(node, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);

        switch (filtered) {
            case nsIDOMNodeFilter::FILTER_ACCEPT:
                
                mCurrentNode = node;
                return CallQueryInterface(node, _retval);
            case nsIDOMNodeFilter::FILTER_SKIP: {
                    nsINode *child = aReversed ? node->GetLastChild()
                                               : node->GetFirstChild();
                    if (child) {
                        node = child;
                        continue;
                    }
                    break;
                }
            case nsIDOMNodeFilter::FILTER_REJECT:
                
                break;
        }

        do {
            nsINode *sibling = aReversed ? node->GetPreviousSibling()
                                         : node->GetNextSibling();
            if (sibling) {
                node = sibling;
                break;
            }

            nsINode *parent = node->GetNodeParent();

            if (!parent || parent == mRoot || parent == mCurrentNode) {
                return NS_OK;
            }

            node = parent;

        } while (node);
    }

    return NS_OK;
}








nsresult nsTreeWalker::NextSiblingInternal(bool aReversed, nsIDOMNode **_retval)
{
    nsresult rv;
    PRInt16 filtered;

    *_retval = nsnull;

    nsCOMPtr<nsINode> node = mCurrentNode;

    if (node == mRoot)
        return NS_OK;

    while (1) {
        nsINode* sibling = aReversed ? node->GetPreviousSibling()
                                     : node->GetNextSibling();

        while (sibling) {
            node = sibling;

            rv = TestNode(node, &filtered);
            NS_ENSURE_SUCCESS(rv, rv);

            if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
                
                mCurrentNode.swap(node);
                return CallQueryInterface(mCurrentNode, _retval);
            }

            
            if (filtered == nsIDOMNodeFilter::FILTER_REJECT ||
                !(sibling = aReversed ? node->GetLastChild()
                                      : node->GetFirstChild())) {
                sibling = aReversed ? node->GetPreviousSibling()
                                    : node->GetNextSibling();
            }
        }

        node = node->GetNodeParent();

        if (!node || node == mRoot)
            return NS_OK;

        
        rv = TestNode(node, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);
        if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT)
            return NS_OK;
    }
}
