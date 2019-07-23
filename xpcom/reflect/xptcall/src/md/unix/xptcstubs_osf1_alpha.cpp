








































#include "xptcprivate.h"

extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, uint32 methodIndex, PRUint64* args)
{
    const PRUint8 PARAM_BUFFER_COUNT = 16;
    const PRUint8 NUM_ARG_REGS = 6-1;        

    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = NULL;
    nsIInterfaceInfo* iface_info = NULL;
    const nsXPTMethodInfo* info;
    PRUint8 paramCount;
    PRUint8 i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    self->GetInterfaceInfo(&iface_info);
    NS_ASSERTION(iface_info,"no interface info");

    iface_info->GetMethodInfo(PRUint16(methodIndex), &info);
    NS_ASSERTION(info,"no interface info");

    paramCount = info->GetParamCount();

    
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;
    NS_ASSERTION(dispatchParams,"no place for params");

    
    PRUint64* ap = args + NUM_ARG_REGS;
    for(i = 0; i < paramCount; i++, ap++)
    {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];

        if(param.IsOut() || !type.IsArithmetic())
        {
            dp->val.p = (void*) *ap;
            continue;
        }
        
        switch(type)
        {
        case nsXPTType::T_I8     : dp->val.i8  = (PRInt8)    *ap;    break;
        case nsXPTType::T_I16    : dp->val.i16 = (PRInt16)   *ap;    break;
        case nsXPTType::T_I32    : dp->val.i32 = (PRInt32)   *ap;    break;
        case nsXPTType::T_I64    : dp->val.i64 = (PRInt64)   *ap;    break;
        case nsXPTType::T_U8     : dp->val.u8  = (PRUint8)   *ap;    break;
        case nsXPTType::T_U16    : dp->val.u16 = (PRUint16)  *ap;    break;
        case nsXPTType::T_U32    : dp->val.u32 = (PRUint32)  *ap;    break;
        case nsXPTType::T_U64    : dp->val.u64 = (PRUint64)  *ap;    break;
        case nsXPTType::T_FLOAT  :
            if(i < NUM_ARG_REGS)
            {
                
                
                dp->val.u64 = (PRUint64) args[i];
                dp->val.f = (float) dp->val.d;    
            }
            else
                dp->val.u32 = (PRUint32) *ap;
            break;
        case nsXPTType::T_DOUBLE :
            
            
            dp->val.u64 = (i < NUM_ARG_REGS) ? args[i] : *ap;
            break;
        case nsXPTType::T_BOOL   : dp->val.b   = (PRBool)    *ap;    break;
        case nsXPTType::T_CHAR   : dp->val.c   = (char)      *ap;    break;
        case nsXPTType::T_WCHAR  : dp->val.wc  = (PRUnichar) *ap;    break;
        default:
            NS_ERROR("bad type");
            break;
        }
    }

    result = self->CallMethod((PRUint16)methodIndex, info, dispatchParams);

    NS_RELEASE(iface_info);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}






#define STUB_ENTRY(n) \

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

