










































#include "xptcprivate.h"
#include "xptiprivate.h"












#define PARAM_BUFFER_COUNT      16
#define GPR_COUNT                7
#define FPR_COUNT               13









#include <stdio.h>
extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self,
                   PRUint64 methodIndex,
                   PRUint64* args,
                   PRUint64 *gprData,
                   double *fprData)
{
    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = NULL;
    const nsXPTMethodInfo* info;
    PRUint32 paramCount;
    PRUint32 i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    self->mEntry->GetMethodInfo(PRUint16(methodIndex), &info);
    NS_ASSERTION(info,"no method info");
    if (! info)
        return NS_ERROR_UNEXPECTED;

    paramCount = info->GetParamCount();

    
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;

    NS_ASSERTION(dispatchParams,"no place for params");
    if (! dispatchParams)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint64* ap = args;
    PRUint64 tempu64;

    for(i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];
	
        if (!param.IsOut() && type == nsXPTType::T_DOUBLE) {
            if (i < FPR_COUNT)
                dp->val.d = fprData[i];
            else
                dp->val.d = *(double*) ap;
        } else if (!param.IsOut() && type == nsXPTType::T_FLOAT) {
            if (i < FPR_COUNT)
                dp->val.f = (float) fprData[i]; 
            else {
                float *p = (float *)ap;
                p++;
                dp->val.f = *p;
            }
        } else { 
            if (i < GPR_COUNT)
                tempu64 = gprData[i];
            else
                tempu64 = *ap;

            if (param.IsOut() || !type.IsArithmetic())
                dp->val.p = (void*) tempu64;
            else if (type == nsXPTType::T_I8)
                dp->val.i8  = (PRInt8)   tempu64;
            else if (type == nsXPTType::T_I16)
                dp->val.i16 = (PRInt16)  tempu64;
            else if (type == nsXPTType::T_I32)
                dp->val.i32 = (PRInt32)  tempu64;
            else if (type == nsXPTType::T_I64)
                dp->val.i64 = (PRInt64)  tempu64;
            else if (type == nsXPTType::T_U8)
                dp->val.u8  = (PRUint8)  tempu64;
            else if (type == nsXPTType::T_U16)
                dp->val.u16 = (PRUint16) tempu64;
            else if (type == nsXPTType::T_U32)
                dp->val.u32 = (PRUint32) tempu64;
            else if (type == nsXPTType::T_U64)
                dp->val.u64 = (PRUint64) tempu64;
            else if (type == nsXPTType::T_BOOL)
                dp->val.b   = (PRBool)   tempu64;
            else if (type == nsXPTType::T_CHAR)
                dp->val.c   = (char)     tempu64;
            else if (type == nsXPTType::T_WCHAR)
                dp->val.wc  = (wchar_t)  tempu64;
            else
                NS_ERROR("bad type");
        }

        if (i >= 7)
            ap++;
    }

    result = self->mOuter->CallMethod((PRUint16) methodIndex, info,
                                      dispatchParams);

    if (dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}







#if __GXX_ABI_VERSION < 100
#error Prehistoric GCC not supported here
#else













# define STUB_ENTRY(n)                                                  \
__asm__ (                                                               \
        ".section \".toc\",\"aw\" \n\t"                                 \
        ".section \".text\" \n\t"                                       \
        ".align 2 \n\t"                                                 \
        ".if "#n" < 10 \n\t"                                            \
        ".globl _ZN14nsXPTCStubBase5Stub"#n"Ev \n\t"                    \
        ".section \".opd\",\"aw\" \n\t"                                 \
        ".align 3 \n\t"                                                 \
"_ZN14nsXPTCStubBase5Stub"#n"Ev: \n\t"                                  \
        ".quad  ._ZN14nsXPTCStubBase5Stub"#n"Ev,.TOC.@tocbase \n\t"     \
        ".previous \n\t"                                                \
        ".type  _ZN14nsXPTCStubBase5Stub"#n"Ev,@function \n\n"          \
"._ZN14nsXPTCStubBase5Stub"#n"Ev: \n\t"                                 \
                                                                        \
        ".elseif "#n" < 100 \n\t"                                       \
        ".globl _ZN14nsXPTCStubBase6Stub"#n"Ev \n\t"                    \
        ".section \".opd\",\"aw\" \n\t"                                 \
        ".align 3 \n\t"                                                 \
"_ZN14nsXPTCStubBase6Stub"#n"Ev: \n\t"                                  \
        ".quad  ._ZN14nsXPTCStubBase6Stub"#n"Ev,.TOC.@tocbase \n\t"     \
        ".previous \n\t"                                                \
        ".type  _ZN14nsXPTCStubBase6Stub"#n"Ev,@function \n\n"          \
"._ZN14nsXPTCStubBase6Stub"#n"Ev: \n\t"                                 \
                                                                        \
        ".elseif "#n" < 1000 \n\t"                                      \
        ".globl _ZN14nsXPTCStubBase7Stub"#n"Ev \n\t"                    \
        ".section \".opd\",\"aw\" \n\t"                                 \
        ".align 3 \n\t"                                                 \
"_ZN14nsXPTCStubBase7Stub"#n"Ev: \n\t"                                  \
        ".quad  ._ZN14nsXPTCStubBase7Stub"#n"Ev,.TOC.@tocbase \n\t"     \
        ".previous \n\t"                                                \
        ".type  _ZN14nsXPTCStubBase7Stub"#n"Ev,@function \n\n"          \
"._ZN14nsXPTCStubBase7Stub"#n"Ev: \n\t"                                 \
                                                                        \
        ".else  \n\t"                                                   \
        ".err   \"stub number "#n" >= 1000 not yet supported\"\n"       \
        ".endif \n\t"                                                   \
                                                                        \
        "li     11,"#n" \n\t"                                           \
        "b      SharedStub \n"                                          \
);
#endif

#define SENTINEL_ENTRY(n)                                               \
nsresult nsXPTCStubBase::Sentinel##n()                                  \
{                                                                       \
    NS_ERROR("nsXPTCStubBase::Sentinel called");                  \
    return NS_ERROR_NOT_IMPLEMENTED;                                    \
}

#include "xptcstubsdef.inc"
