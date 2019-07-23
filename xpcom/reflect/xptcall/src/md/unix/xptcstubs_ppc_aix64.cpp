





































#include "xptcprivate.h"

#if defined(AIX)







extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, PRUint64 methodIndex, PRUint64* args, PRUint64 *gprData, double *fprData)
{

#define PARAM_BUFFER_COUNT     16
#define PARAM_GPR_COUNT         7  

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

    PRUint64* ap = args;
    PRUint32 iCount = 0;
    PRUint32 fpCount = 0;
    for(i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];

        if(param.IsOut() || !type.IsArithmetic())
        {
            if (iCount < PARAM_GPR_COUNT)
                dp->val.p = (void*) gprData[iCount++];
            else
                dp->val.p = (void*) *ap++;
            continue;
        }
        
        switch(type)
        {
        case nsXPTType::T_I8      :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.i8  = (PRInt8) gprData[iCount++];
                                     else
                                         dp->val.i8  = (PRInt8)  *ap++;
                                     break;
        case nsXPTType::T_I16     :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.i16  = (PRInt16) gprData[iCount++];
                                     else
                                         dp->val.i16  = (PRInt16)  *ap++;
                                     break;
        case nsXPTType::T_I32     :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.i32  = (PRInt32) gprData[iCount++];
                                     else
                                         dp->val.i32  = (PRInt32)  *ap++;
                                     break;
        case nsXPTType::T_I64     :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.i64  = (PRInt64) gprData[iCount++];
                                     else
                                         dp->val.i64  = (PRInt64) *ap++;
                                     break;
        case nsXPTType::T_U8      :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.u8  = (PRUint8) gprData[iCount++];
                                     else
                                         dp->val.u8  = (PRUint8)  *ap++;
                                     break;
        case nsXPTType::T_U16     :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.u16  = (PRUint16) gprData[iCount++];
                                     else
                                         dp->val.u16  = (PRUint16)  *ap++;
                                     break;
        case nsXPTType::T_U32     :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.u32  = (PRUint32) gprData[iCount++];
                                     else
                                         dp->val.u32  = (PRUint32)  *ap++;
                                     break;
        case nsXPTType::T_U64     :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.u64  = (PRUint64) gprData[iCount++];
                                     else
                                         dp->val.u64  = (PRUint64)  *ap++;
                                     break;
        case nsXPTType::T_FLOAT   :  if (fpCount < 13) {
                                         dp->val.f  = (float) fprData[fpCount++];
                                         if (iCount < PARAM_GPR_COUNT)
                                             ++iCount;
                                         else
                                             ++ap;
                                     }
                                     else
                                         dp->val.f   = *((float*)   ap++);
                                     break;
        case nsXPTType::T_DOUBLE  :  if (fpCount < 13) {
                                         dp->val.d  = (double) fprData[fpCount++];
                                         if (iCount < PARAM_GPR_COUNT)
                                             ++iCount;
                                         else
                                             ++ap;
                                         if (iCount < PARAM_GPR_COUNT)
                                             ++iCount;
                                         else
                                             ++ap;
                                     }
                                     else {
                                         dp->val.f   = *((double*)   ap);
                                         ap += 2;
                                     }
                                     break;
        case nsXPTType::T_BOOL    :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.b  = (PRBool) gprData[iCount++];
                                     else
                                         dp->val.b  = (PRBool)  *ap++;
                                     break;
        case nsXPTType::T_CHAR    :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.c  = (char) gprData[iCount++];
                                     else
                                         dp->val.c  = (char)  *ap++;
                                     break;
        case nsXPTType::T_WCHAR   :  if (iCount < PARAM_GPR_COUNT)
                                         dp->val.wc  = (wchar_t) gprData[iCount++];
                                     else
                                         dp->val.wc  = (wchar_t)  *ap++;
                                     break;
        default:
            NS_ASSERTION(0, "bad type");
            break;
        }
    }

    result = self->CallMethod((PRUint16)methodIndex, info, dispatchParams);

    NS_RELEASE(iface_info);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

extern "C" int SharedStub(int);

#define STUB_ENTRY(n)                \
nsresult nsXPTCStubBase::Stub##n()   \
{                                    \
    return SharedStub(n);	     \
}                                    \

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ASSERTION(0,"nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

#endif 
