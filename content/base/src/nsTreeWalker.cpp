










































#include "nsTreeWalker.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsDOMError.h"

#include "nsIContent.h"
#include "nsIDocument.h"

#include "nsContentUtils.h"
#include "nsMemory.h"
#include "nsCOMArray.h"
#include "nsGkAtoms.h"





nsresult
NS_NewTreeWalker(nsIDOMNode *aRoot,
                 PRUint32 aWhatToShow,
                 nsIDOMNodeFilter *aFilter,
                 PRBool aEntityReferenceExpansion,
                 nsIDOMTreeWalker **aInstancePtrResult)
{
    NS_ENSURE_ARG_POINTER(aInstancePtrResult);

    nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
    NS_ENSURE_TRUE(root, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

    nsTreeWalker* walker = new nsTreeWalker(root,
                                            aWhatToShow,
                                            aFilter,
                                            aEntityReferenceExpansion);
    NS_ENSURE_TRUE(walker, NS_ERROR_OUT_OF_MEMORY);

    return CallQueryInterface(walker, aInstancePtrResult);
}

nsTreeWalker::nsTreeWalker(nsINode *aRoot,
                           PRUint32 aWhatToShow,
                           nsIDOMNodeFilter *aFilter,
                           PRBool aExpandEntityReferences) :
    mRoot(aRoot),
    mWhatToShow(aWhatToShow),
    mFilter(aFilter),
    mExpandEntityReferences(aExpandEntityReferences),
    mCurrentNode(aRoot),
    mPossibleIndexesPos(-1)
{
    NS_ASSERTION(aRoot, "invalid root in call to nsTreeWalker constructor");
}

nsTreeWalker::~nsTreeWalker()
{
    
}






NS_INTERFACE_MAP_BEGIN(nsTreeWalker)
    NS_INTERFACE_MAP_ENTRY(nsIDOMTreeWalker)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMTreeWalker)
    NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(TreeWalker)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsTreeWalker)
NS_IMPL_RELEASE(nsTreeWalker)






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

    nsCOMPtr<nsIDOMNodeFilter> filter = mFilter;
    filter.swap((*aFilter = nsnull));

    return NS_OK;
}


NS_IMETHODIMP
nsTreeWalker::GetExpandEntityReferences(PRBool *aExpandEntityReferences)
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

    
    nsCOMPtr<nsIDOMNode> domRoot = do_QueryInterface(mRoot);
    nsresult rv = nsContentUtils::CheckSameOrigin(domRoot, aCurrentNode);
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentNode = do_QueryInterface(aCurrentNode);

    return NS_OK;
}






NS_IMETHODIMP nsTreeWalker::ParentNode(nsIDOMNode **_retval)
{
    *_retval = nsnull;
    
    nsresult rv;

    PRInt32 indexPos = mPossibleIndexesPos;
    nsCOMPtr<nsINode> node = mCurrentNode;
    
    while (node && node != mRoot) {
        node = node->GetNodeParent();
        
        indexPos--;

        if (node) {
            PRInt16 filtered;
            rv = TestNode(node, &filtered);
            NS_ENSURE_SUCCESS(rv, rv);
            if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
                mCurrentNode = node;
                mPossibleIndexesPos = indexPos >= 0 ? indexPos : -1;

                return CallQueryInterface(node, _retval);
            }
        }
    }

    return NS_OK;
}


NS_IMETHODIMP nsTreeWalker::FirstChild(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsINode> result;
    nsresult rv =  FirstChildOf(mCurrentNode,
                                PR_FALSE,
                                mPossibleIndexesPos + 1,
                                getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    return result ? CallQueryInterface(result, _retval) : NS_OK;
}


NS_IMETHODIMP nsTreeWalker::LastChild(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsINode> result;
    nsresult rv =  FirstChildOf(mCurrentNode,
                                PR_TRUE,
                                mPossibleIndexesPos + 1,
                                getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    return result ? CallQueryInterface(result, _retval) : NS_OK;
}


NS_IMETHODIMP nsTreeWalker::PreviousSibling(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsINode> result;
    nsresult rv = NextSiblingOf(mCurrentNode,
                                PR_TRUE,
                                mPossibleIndexesPos,
                                getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    return result ? CallQueryInterface(result, _retval) : NS_OK;
}


NS_IMETHODIMP nsTreeWalker::NextSibling(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsINode> result;
    nsresult rv = NextSiblingOf(mCurrentNode,
                                PR_FALSE,
                                mPossibleIndexesPos,
                                getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    return result ? CallQueryInterface(result, _retval) : NS_OK;
}


NS_IMETHODIMP nsTreeWalker::PreviousNode(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsINode> result;
    nsresult rv = NextInDocumentOrderOf(mCurrentNode,
                                        PR_TRUE,
                                        mPossibleIndexesPos,
                                        getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    return result ? CallQueryInterface(result, _retval) : NS_OK;
}


NS_IMETHODIMP nsTreeWalker::NextNode(nsIDOMNode **_retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsINode> result;
    nsresult rv = NextInDocumentOrderOf(mCurrentNode,
                                        PR_FALSE,
                                        mPossibleIndexesPos,
                                        getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    return result ? CallQueryInterface(result, _retval) : NS_OK;
}
















nsresult
nsTreeWalker::FirstChildOf(nsINode* aNode,
                           PRBool aReversed,
                           PRInt32 aIndexPos,
                           nsINode** _retval)
{
    *_retval = nsnull;
    PRInt32 start = aReversed ? (PRInt32)aNode->GetChildCount() : -1;

    return ChildOf(aNode, start, aReversed, aIndexPos, _retval);
}











nsresult
nsTreeWalker::NextSiblingOf(nsINode* aNode,
                            PRBool aReversed,
                            PRInt32 aIndexPos,
                            nsINode** _retval)
{
    nsresult rv;
    nsCOMPtr<nsINode> node = aNode;
    PRInt16 filtered;
    PRInt32 childNum;

    if (node == mRoot) {
        *_retval = nsnull;
        return NS_OK;
    }

    while (1) {
        nsCOMPtr<nsINode> parent = node->GetNodeParent();

        if (!parent)
            break;

        childNum = IndexOf(parent, node, aIndexPos);
        NS_ENSURE_TRUE(childNum >= 0, NS_ERROR_UNEXPECTED);

        
        rv = ChildOf(parent, childNum, aReversed, aIndexPos, _retval);
        NS_ENSURE_SUCCESS(rv, rv);

        if (*_retval)
            return NS_OK;

        
        if (parent == mRoot)
            break;

        
        rv = TestNode(parent, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);
        if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT)
            break;

        node = parent;
        aIndexPos = aIndexPos < 0 ? -1 : aIndexPos-1;
    }

    *_retval = nsnull;
    return NS_OK;
}











nsresult
nsTreeWalker::NextInDocumentOrderOf(nsINode* aNode,
                                    PRBool aReversed,
                                    PRInt32 aIndexPos,
                                    nsINode** _retval)
{
    nsresult rv;

    if (!aReversed) {
        rv = FirstChildOf(aNode, aReversed, aIndexPos+1, _retval);
        NS_ENSURE_SUCCESS(rv, rv);

        if (*_retval)
            return NS_OK;
    }

    if (aNode == mRoot){
        *_retval = nsnull;
        return NS_OK;
    }

    nsCOMPtr<nsINode> node = aNode;
    nsCOMPtr<nsINode> currentNodeBackup = mCurrentNode;
    PRInt16 filtered;
    PRInt32 childNum;

    while (1) {
        
        nsCOMPtr<nsINode> parent = node->GetNodeParent();
        if (!parent)
            break;

        childNum = IndexOf(parent, node, aIndexPos);
        NS_ENSURE_TRUE(childNum >= 0, NS_ERROR_UNEXPECTED);

        
        nsCOMPtr<nsINode> sibling;
        rv = ChildOf(parent, childNum, aReversed, aIndexPos,
                     getter_AddRefs(sibling));
        NS_ENSURE_SUCCESS(rv, rv);

        if (sibling) {
            if (aReversed) {
                
                
                nsCOMPtr<nsINode> child = sibling;
                while (child) {
                    sibling = child;
                    rv = FirstChildOf(sibling,
                                      PR_TRUE,
                                      aIndexPos,
                                      getter_AddRefs(child));
                    if (NS_FAILED(rv)) {
                        
                        
                        mCurrentNode = currentNodeBackup;
                        mPossibleIndexesPos = -1;
                        return rv;
                    }
                }
            }
            *_retval = sibling;
            NS_ADDREF(*_retval);
            return NS_OK;
        }

        aIndexPos = aIndexPos < 0 ? -1 : aIndexPos-1;

        if (aReversed) {
            
            rv = TestNode(parent, &filtered);
            NS_ENSURE_SUCCESS(rv, rv);
            if (filtered == nsIDOMNodeFilter::FILTER_ACCEPT) {
                mCurrentNode = parent;
                mPossibleIndexesPos = aIndexPos;
                *_retval = parent;
                NS_ADDREF(*_retval);
                return NS_OK;
            }
        }

        
        if (parent == mRoot)
            break;

        node = parent;
    }

    *_retval = nsnull;
    return NS_OK;
}













nsresult
nsTreeWalker::ChildOf(nsINode* aNode,
                      PRInt32 childNum,
                      PRBool aReversed,
                      PRInt32 aIndexPos,
                      nsINode** _retval)
{
    PRInt16 filtered;
    nsresult rv;

    PRInt32 dir = aReversed ? -1 : 1;

    
    PRInt32 i = childNum;
    while (1) {
        i += dir;
        nsCOMPtr<nsINode> child = aNode->GetChildAt(i);
        if (!child) {
            break;
        }

        rv = TestNode(child, &filtered);
        NS_ENSURE_SUCCESS(rv, rv);

        switch (filtered) {
            case nsIDOMNodeFilter::FILTER_ACCEPT:
                
                mCurrentNode = child;
                mPossibleIndexesPos = aIndexPos;
                *_retval = child;
                NS_ADDREF(*_retval);

                SetChildIndex(aIndexPos, i);

                return NS_OK;

            case nsIDOMNodeFilter::FILTER_SKIP:
                
                rv = FirstChildOf(child, aReversed, aIndexPos+1, _retval);
                NS_ENSURE_SUCCESS(rv, rv);

                if (*_retval) {
                    SetChildIndex(aIndexPos, i);
                    return NS_OK;
                }
                break;

            case nsIDOMNodeFilter::FILTER_REJECT:
                
                break;

            default:
                return NS_ERROR_UNEXPECTED;
        }
    }

    *_retval = nsnull;
    return NS_OK;
}








nsresult nsTreeWalker::TestNode(nsINode* aNode, PRInt16* _filtered)
{
    nsresult rv;

    *_filtered = nsIDOMNodeFilter::FILTER_SKIP;

    PRUint16 nodeType = 0;
    
    if (aNode->IsNodeOfType(nsINode::eELEMENT)) {
        nodeType = nsIDOMNode::ELEMENT_NODE;
    }
    else if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
        nsIAtom* tag = NS_STATIC_CAST(nsIContent*, aNode)->Tag();
        if (tag == nsGkAtoms::textTagName) {
            nodeType = nsIDOMNode::TEXT_NODE;
        }
        else if (tag == nsGkAtoms::cdataTagName) {
            nodeType = nsIDOMNode::CDATA_SECTION_NODE;
        }
        else if (tag == nsGkAtoms::commentTagName) {
            nodeType = nsIDOMNode::COMMENT_NODE;
        }
        else if (tag == nsGkAtoms::processingInstructionTagName) {
            nodeType = nsIDOMNode::PROCESSING_INSTRUCTION_NODE;
        }
    }

    nsCOMPtr<nsIDOMNode> domNode;
    if (!nodeType) {
        domNode = do_QueryInterface(aNode);
        rv = domNode->GetNodeType(&nodeType);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (nodeType <= 12 && !((1 << (nodeType-1)) & mWhatToShow)) {
        return NS_OK;
    }

    if (mFilter) {
        if (!domNode) {
            domNode = do_QueryInterface(aNode);
        }

        return mFilter->AcceptNode(domNode, _filtered);
    }

    *_filtered = nsIDOMNodeFilter::FILTER_ACCEPT;
    return NS_OK;
}











PRInt32 nsTreeWalker::IndexOf(nsINode* aParent,
                              nsINode* aChild,
                              PRInt32 aIndexPos)
{
    if (aIndexPos >= 0 && aIndexPos < mPossibleIndexes.Count()) {
        PRInt32 possibleIndex =
            NS_PTR_TO_INT32(mPossibleIndexes.FastElementAt(aIndexPos));
        if (aChild == aParent->GetChildAt(possibleIndex)) {
            return possibleIndex;
        }
    }

    return aParent->IndexOf(aChild);
}
