






#include "xptcprivate.h"

#if defined(sparc) || defined(__sparc__)

extern "C" nsresult ATTRIBUTE_USED
PrepareAndDispatch(nsXPTCStubBase* self, uint32_t methodIndex, uint32_t* args)
{

    typedef struct {
        uint32_t hi;
        uint32_t lo;
    } DU;               
                        

#define PARAM_BUFFER_COUNT     16

    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = nullptr;
    nsIInterfaceInfo* iface_info = nullptr;
    const nsXPTMethodInfo* info;
    uint8_t paramCount;
    uint8_t i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    self->GetInterfaceInfo(&iface_info);
    NS_ASSERTION(iface_info,"no interface info");

    iface_info->GetMethodInfo(uint16_t(methodIndex), &info);
    NS_ASSERTION(info,"no interface info");

    paramCount = info->GetParamCount();

    
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;

    NS_ASSERTION(dispatchParams,"no place for params");
    if (!dispatchParams)
        return NS_ERROR_OUT_OF_MEMORY;

    uint32_t* ap = args;
    for(i = 0; i < paramCount; i++, ap++)
    {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];

        if(param.IsOut() || !type.IsArithmetic())
        {
            if (type == nsXPTType::T_JSVAL)
                dp->val.p = *((void**) *ap);
            else
                dp->val.p = (void*) *ap;
            continue;
        }
        
        switch(type)
        {
        case nsXPTType::T_I8     : dp->val.i8  = *((int32_t*) ap);       break;
        case nsXPTType::T_I16    : dp->val.i16 = *((int32_t*) ap);       break;
        case nsXPTType::T_I32    : dp->val.i32 = *((int32_t*) ap);       break;
        case nsXPTType::T_DOUBLE :
        case nsXPTType::T_U64    :
        case nsXPTType::T_I64    : ((DU *)dp)->hi = ((DU *)ap)->hi; 
                                   ((DU *)dp)->lo = ((DU *)ap)->lo;
                                   ap++;
                                   break;
        case nsXPTType::T_U8     : dp->val.u8  = *((uint32_t*)ap);       break;
        case nsXPTType::T_U16    : dp->val.u16 = *((uint32_t*)ap);       break;
        case nsXPTType::T_U32    : dp->val.u32 = *((uint32_t*)ap);       break;
        case nsXPTType::T_FLOAT  : dp->val.f   = *((float*)   ap);       break;
        case nsXPTType::T_BOOL   : dp->val.b   = *((uint32_t*)ap);       break;
        case nsXPTType::T_CHAR   : dp->val.c   = *((uint32_t*)ap);       break;
        case nsXPTType::T_WCHAR  : dp->val.wc  = *((int32_t*) ap);       break;
        default:
            NS_ERROR("bad type");
            break;
        }
    }

    result = self->CallMethod((uint16_t)methodIndex, info, dispatchParams);

    NS_RELEASE(iface_info);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

extern "C" nsresult SharedStub(int, int*);

#define STUB_ENTRY(n) \
nsresult nsXPTCStubBase::Stub##n() \
{ \
	int dummy; /* defeat tail-call optimization */ \
	return SharedStub(n, &dummy); \
}

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

#endif 
