




#ifndef txNodeSetAdaptor_h__
#define txNodeSetAdaptor_h__

#include "txINodeSet.h"
#include "txNodeSet.h"
#include "txXPathObjectAdaptor.h"





class txNodeSetAdaptor : public txXPathObjectAdaptor,
                         public txINodeSet 
{
public:
    txNodeSetAdaptor();
    txNodeSetAdaptor(txNodeSet *aNodeSet);

    nsresult Init();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_TXINODESET

protected:
    ~txNodeSetAdaptor() {}

private:
    txNodeSet* NodeSet()
    {
        return static_cast<txNodeSet*>(mValue.get());
    }

    bool mWritable;
};

#endif 
