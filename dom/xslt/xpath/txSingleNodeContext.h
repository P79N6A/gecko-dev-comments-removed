




#ifndef __TX_XPATH_SINGLENODE_CONTEXT
#define __TX_XPATH_SINGLENODE_CONTEXT

#include "mozilla/Attributes.h"
#include "txIXPathContext.h"

class txSingleNodeContext : public txIEvalContext
{
public:
    txSingleNodeContext(const txXPathNode& aContextNode,
                        txIMatchContext* aContext)
        : mNode(aContextNode),
          mInner(aContext)
    {
        NS_ASSERTION(aContext, "txIMatchContext must be given");
    }

    nsresult getVariable(int32_t aNamespace, nsIAtom* aLName,
                         txAExprResult*& aResult) override
    {
        NS_ASSERTION(mInner, "mInner is null!!!");
        return mInner->getVariable(aNamespace, aLName, aResult);
    }

    bool isStripSpaceAllowed(const txXPathNode& aNode) override
    {
        NS_ASSERTION(mInner, "mInner is null!!!");
        return mInner->isStripSpaceAllowed(aNode);
    }

    void* getPrivateContext() override
    {
        NS_ASSERTION(mInner, "mInner is null!!!");
        return mInner->getPrivateContext();
    }

    txResultRecycler* recycler() override
    {
        NS_ASSERTION(mInner, "mInner is null!!!");
        return mInner->recycler();
    }

    void receiveError(const nsAString& aMsg, nsresult aRes) override
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

    const txXPathNode& getContextNode() override
    {
        return mNode;
    }

    uint32_t size() override
    {
        return 1;
    }

    uint32_t position() override
    {
        return 1;
    }

private:
    const txXPathNode& mNode;
    txIMatchContext* mInner;
};

#endif 
