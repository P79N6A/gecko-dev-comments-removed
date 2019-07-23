





































#ifndef __TX_XPATH_SET_CONTEXT
#define __TX_XPATH_SET_CONTEXT

#include "txIXPathContext.h"
#include "txNodeSet.h"
#include "nsAutoPtr.h"

class txNodeSetContext : public txIEvalContext
{
public:
    txNodeSetContext(txNodeSet* aContextNodeSet, txIMatchContext* aContext)
        : mContextSet(aContextNodeSet), mPosition(0), mInner(aContext)
    {
    }
    virtual ~txNodeSetContext()
    {
    }

    
    MBool hasNext()
    {
        return mPosition < size();
    }
    void next()
    {
        NS_ASSERTION(mPosition < size(), "Out of bounds.");
        mPosition++;
    }
    void setPosition(PRUint32 aPosition)
    {
        NS_ASSERTION(aPosition > 0 &&
                     aPosition <= size(), "Out of bounds.");
        mPosition = aPosition;
    }

    TX_DECL_EVAL_CONTEXT;

protected:
    nsRefPtr<txNodeSet> mContextSet;
    PRUint32 mPosition;
    txIMatchContext* mInner;
};

#endif 
