





#include "nsTraversal.h"

#include "nsIDOMNode.h"
#include "nsError.h"
#include "nsINode.h"
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








int16_t
nsTraversal::TestNode(nsINode* aNode, mozilla::ErrorResult& aResult)
{
    if (mInAcceptNode) {
        aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
        return 0;
    }

    uint16_t nodeType = aNode->NodeType();

    if (nodeType <= 12 && !((1 << (nodeType-1)) & mWhatToShow)) {
        return nsIDOMNodeFilter::FILTER_SKIP;
    }

    if (!mFilter.GetISupports()) {
        
        return nsIDOMNodeFilter::FILTER_ACCEPT;
    }

    if (mFilter.HasWebIDLCallback()) {
        AutoRestore<bool> inAcceptNode(mInAcceptNode);
        mInAcceptNode = true;
        
        
        return mFilter.GetWebIDLCallback()->
            AcceptNode(*aNode, aResult, nullptr,
                       CallbackObject::eRethrowExceptions);
    }

    nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(aNode);
    AutoRestore<bool> inAcceptNode(mInAcceptNode);
    mInAcceptNode = true;
    int16_t filtered;
    nsresult rv = mFilter.GetXPCOMCallback()->AcceptNode(domNode, &filtered);
    if (NS_FAILED(rv)) {
        aResult.Throw(rv);
    }
    return filtered;
}
