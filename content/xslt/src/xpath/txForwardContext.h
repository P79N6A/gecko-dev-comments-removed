





































#ifndef __TX_XPATH_CONTEXT
#define __TX_XPATH_CONTEXT

#include "txIXPathContext.h"
#include "nsAutoPtr.h"
#include "txNodeSet.h"

class txForwardContext : public txIEvalContext
{
public:
    txForwardContext(txIMatchContext* aContext,
                     const txXPathNode& aContextNode,
                     txNodeSet* aContextNodeSet)
        : mInner(aContext),
          mContextNode(aContextNode),
          mContextSet(aContextNodeSet)
    {}
    ~txForwardContext()
    {}

    TX_DECL_EVAL_CONTEXT;

private:
    txIMatchContext* mInner;
    const txXPathNode& mContextNode;
    nsRefPtr<txNodeSet> mContextSet;
};

#endif 
