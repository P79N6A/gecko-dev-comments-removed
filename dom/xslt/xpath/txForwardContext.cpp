




#include "txForwardContext.h"
#include "txNodeSet.h"

const txXPathNode& txForwardContext::getContextNode()
{
    return mContextNode;
}

uint32_t txForwardContext::size()
{
    return (uint32_t)mContextSet->size();
}

uint32_t txForwardContext::position()
{
    int32_t pos = mContextSet->indexOf(mContextNode);
    NS_ASSERTION(pos >= 0, "Context is not member of context node list.");
    return (uint32_t)(pos + 1);
}

nsresult txForwardContext::getVariable(int32_t aNamespace, nsIAtom* aLName,
                                       txAExprResult*& aResult)
{
    NS_ASSERTION(mInner, "mInner is null!!!");
    return mInner->getVariable(aNamespace, aLName, aResult);
}

bool txForwardContext::isStripSpaceAllowed(const txXPathNode& aNode)
{
    NS_ASSERTION(mInner, "mInner is null!!!");
    return mInner->isStripSpaceAllowed(aNode);
}

void* txForwardContext::getPrivateContext()
{
    NS_ASSERTION(mInner, "mInner is null!!!");
    return mInner->getPrivateContext();
}

txResultRecycler* txForwardContext::recycler()
{
    NS_ASSERTION(mInner, "mInner is null!!!");
    return mInner->recycler();
}

void txForwardContext::receiveError(const nsAString& aMsg, nsresult aRes)
{
    NS_ASSERTION(mInner, "mInner is null!!!");
#ifdef DEBUG
    nsAutoString error(NS_LITERAL_STRING("forwarded error: "));
    error.Append(aMsg);
    mInner->receiveError(error, aRes);
#else
    mInner->receiveError(aMsg, aRes);
#endif
}
