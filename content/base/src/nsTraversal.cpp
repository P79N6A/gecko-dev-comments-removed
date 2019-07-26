





#include "nsTraversal.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsError.h"
#include "nsINode.h"
#include "mozilla/dom/NodeFilterBinding.h"
#include "mozilla/AutoRestore.h"

#include "nsGkAtoms.h"

using namespace mozilla;
using namespace mozilla::dom;

nsTraversal::nsTraversal(nsINode *aRoot,
                         uint32_t aWhatToShow,
                         const NodeFilterHolder &aFilter) :
    mRoot(aRoot),
    mWhatToShow(aWhatToShow),
    mFilter(aFilter),
    mInAcceptNode(false)
{
    NS_ASSERTION(aRoot, "invalid root in call to nsTraversal constructor");
}

nsTraversal::~nsTraversal()
{
    
}








nsresult nsTraversal::TestNode(nsINode* aNode, int16_t* _filtered)
{
    NS_ENSURE_TRUE(!mInAcceptNode, NS_ERROR_DOM_INVALID_STATE_ERR);

    *_filtered = nsIDOMNodeFilter::FILTER_SKIP;

    uint16_t nodeType = aNode->NodeType();

    if (nodeType <= 12 && !((1 << (nodeType-1)) & mWhatToShow)) {
        return NS_OK;
    }

    if (!mFilter.GetISupports()) {
        
        *_filtered = nsIDOMNodeFilter::FILTER_ACCEPT;
        return NS_OK;
    }

    if (mFilter.HasWebIDLCallback()) {
        AutoRestore<bool> inAcceptNode(mInAcceptNode);
        mInAcceptNode = true;
        ErrorResult res;
        *_filtered = mFilter.GetWebIDLCallback()->AcceptNode(*aNode, res);
        return res.ErrorCode();
    }

    nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(aNode);
    AutoRestore<bool> inAcceptNode(mInAcceptNode);
    mInAcceptNode = true;
    return mFilter.GetXPCOMCallback()->AcceptNode(domNode, _filtered);
}
