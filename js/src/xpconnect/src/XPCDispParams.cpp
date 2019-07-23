









































#include "xpcprivate.h"

XPCDispParams::XPCDispParams(PRUint32 args) : 
    mRefBuffer(mStackRefBuffer + sizeof(VARIANT)),
    mDispParamsAllocated(nsnull),
    mRefBufferAllocated(nsnull),
    mPropID(DISPID_PROPERTYPUT)
#ifdef DEBUG
    ,mInserted(PR_FALSE)
#endif
{
    if(args >= DEFAULT_ARG_ARRAY_SIZE)
    {
        mRefBufferAllocated = new char[RefBufferSize(args)];
        mRefBuffer = mRefBufferAllocated + sizeof(VARIANT);
    }
    
    memset(mRefBuffer - sizeof(VARIANT), 0, RefBufferSize(args));
    
    mDispParams.cArgs = args;
    if(args == 0)
        mDispParams.rgvarg = nsnull;
    else if (args <= DEFAULT_ARG_ARRAY_SIZE)
        mDispParams.rgvarg = mStackArgs + 1;
    else
    {
        mDispParamsAllocated = new VARIANT[args + 1];
        mDispParams.rgvarg =  mDispParamsAllocated + 1;
    }
    mDispParams.rgdispidNamedArgs = nsnull;
    mDispParams.cNamedArgs = 0;
}


XPCDispParams::~XPCDispParams()
{
    
    for(PRUint32 index = 0; index < mDispParams.cArgs; ++index)
        VariantClear(mDispParams.rgvarg + index);
    
    
    delete [] mRefBufferAllocated;
    delete [] mDispParamsAllocated;
}

void XPCDispParams::InsertParam(_variant_t & var)
{
#ifdef DEBUG
    NS_ASSERTION(!mInserted, 
                 "XPCDispParams::InsertParam cannot be called more than once");
    mInserted = PR_TRUE;
#endif
    
    --mDispParams.rgvarg;
    mRefBuffer -= sizeof(VARIANT);
    ++mDispParams.cArgs;
    
    mDispParams.rgvarg[0] = var.Detach();
    
    memset(mRefBuffer, 0, sizeof(VARIANT));
}
