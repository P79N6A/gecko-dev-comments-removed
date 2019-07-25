






































#include "nsTraversal.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsDOMError.h"

#include "nsIContent.h"

#include "nsGkAtoms.h"

nsTraversal::nsTraversal(nsINode *aRoot,
                         PRUint32 aWhatToShow,
                         nsIDOMNodeFilter *aFilter,
                         PRBool aExpandEntityReferences) :
    mRoot(aRoot),
    mWhatToShow(aWhatToShow),
    mFilter(aFilter),
    mExpandEntityReferences(aExpandEntityReferences),
    mInAcceptNode(PR_FALSE)
{
    NS_ASSERTION(aRoot, "invalid root in call to nsTraversal constructor");
}

nsTraversal::~nsTraversal()
{
    
}








nsresult nsTraversal::TestNode(nsINode* aNode, PRInt16* _filtered)
{
    NS_ENSURE_TRUE(!mInAcceptNode, NS_ERROR_DOM_INVALID_STATE_ERR);

    nsresult rv;

    *_filtered = nsIDOMNodeFilter::FILTER_SKIP;

    PRUint16 nodeType = 0;
    
    if (aNode->IsElement()) {
        nodeType = nsIDOMNode::ELEMENT_NODE;
    }
    else if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
        nsIAtom* tag = static_cast<nsIContent*>(aNode)->Tag();
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

        mInAcceptNode = PR_TRUE;
        rv = mFilter->AcceptNode(domNode, _filtered);
        mInAcceptNode = PR_FALSE;
        return rv;
    }

    *_filtered = nsIDOMNodeFilter::FILTER_ACCEPT;
    return NS_OK;
}
