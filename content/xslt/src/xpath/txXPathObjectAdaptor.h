




#ifndef txXPathObjectAdaptor_h__
#define txXPathObjectAdaptor_h__

#include "txExprResult.h"
#include "txINodeSet.h"
#include "txIXPathObject.h"






class txXPathObjectAdaptor : public txIXPathObject
{
public:
    txXPathObjectAdaptor(txAExprResult *aValue) : mValue(aValue)
    {
        NS_ASSERTION(aValue,
                     "Don't create a txXPathObjectAdaptor if you don't have a "
                     "txAExprResult");
    }
    virtual ~txXPathObjectAdaptor() {}

    NS_DECL_ISUPPORTS

    NS_IMETHODIMP_(txAExprResult*) GetResult()
    {
        return mValue;
    }

protected:
    txXPathObjectAdaptor() : mValue(nsnull)
    {
    }

    nsRefPtr<txAExprResult> mValue;
};

#endif 
