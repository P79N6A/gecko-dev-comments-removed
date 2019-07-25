






































#include "nsTraversal.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsDOMError.h"

#include "nsIContent.h"

#include "nsGkAtoms.h"

nsTraversal::nsTraversal(nsINode *aRoot,
                         PRUint32 aWhatToShow,
                         nsIDOMNodeFilter *aFilter,
                         bool aExpandEntityReferences) :
    mRoot(aRoot),
    mWhatToShow(aWhatToShow),
    mFilter(aFilter),
    mExpandEntityReferences(aExpandEntityReferences),
    mInAcceptNode(false)
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

    PRUint16 nodeType = aNode->NodeType();

    if (nodeType <= 12 && !((1 << (nodeType-1)) & mWhatToShow)) {
        return NS_OK;
    }

    if (mFilter) {
        nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(aNode);
        mInAcceptNode = true;
        rv = mFilter->AcceptNode(domNode, _filtered);
        mInAcceptNode = false;
        return rv;
    }

    *_filtered = nsIDOMNodeFilter::FILTER_ACCEPT;
    return NS_OK;
}
