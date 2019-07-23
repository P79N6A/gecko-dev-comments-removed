






































#ifndef txNodeSetAdaptor_h__
#define txNodeSetAdaptor_h__

#include "txINodeSet.h"
#include "txNodeSet.h"





class txNodeSetAdaptor : public txINodeSet 
{
public:
    txNodeSetAdaptor();
    txNodeSetAdaptor(txNodeSet *aNodeSet);

    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_TXINODESET

private:
    nsRefPtr<txNodeSet> mNodeSet;
    PRBool mWritable;
};

#endif 
