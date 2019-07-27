






#include "xptcprivate.h"
#include "xptiprivate.h"








#define PARAM_BUFFER_COUNT     16
#define GPR_COUNT               8
#define FPR_COUNT               8










extern "C" nsresult ATTRIBUTE_USED
PrepareAndDispatch(nsXPTCStubBase* self,
                   uint32_t methodIndex,
                   uint32_t* args,
                   uint32_t *gprData,
                   double *fprData)
{
    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = nullptr;
    const nsXPTMethodInfo* info = nullptr;
    uint32_t paramCount;
    uint32_t i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    self->mEntry->GetMethodInfo(uint16_t(methodIndex), &info);
    NS_ASSERTION(info,"no method info");
    if (! info)
        return NS_ERROR_UNEXPECTED;

    paramCount = info->GetParamCount();

    
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;

    NS_ASSERTION(dispatchParams,"no place for params");
    if (!dispatchParams)
        return NS_ERROR_OUT_OF_MEMORY;

    uint32_t* ap = args;
    uint32_t gpr = 1;    
    uint32_t fpr = 0;
    uint32_t tempu32;
    uint64_t tempu64;

    for(i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];
	
        if (!param.IsOut() && type == nsXPTType::T_DOUBLE) {
            if (fpr < FPR_COUNT)
                dp->val.d = fprData[fpr++];
            else {
                if ((uint32_t) ap & 4) ap++; 
                dp->val.d = *(double*) ap;
                ap += 2;
            }
            continue;
        }
        else if (!param.IsOut() && type == nsXPTType::T_FLOAT) {
            if (fpr < FPR_COUNT)
                dp->val.f = (float) fprData[fpr++]; 
            else
                dp->val.f = *(float*) ap++;
            continue;
        }
        else if (!param.IsOut() && (type == nsXPTType::T_I64
                                    || type == nsXPTType::T_U64)) {
            if (gpr & 1) gpr++; 
            if ((gpr + 1) < GPR_COUNT) {
                tempu64 = *(uint64_t*) &gprData[gpr];
                gpr += 2;
            }
            else {
                if ((uint32_t) ap & 4) ap++; 
                tempu64 = *(uint64_t*) ap;
                ap += 2;
            }
        }
        else {
            if (gpr < GPR_COUNT)
                tempu32 = gprData[gpr++];
            else
                tempu32 = *ap++;
        }

        if(param.IsOut() || !type.IsArithmetic()) {
            if (type == nsXPTType::T_JSVAL)
                dp->val.p = *((void**) tempu32);
            else
                dp->val.p = (void*) tempu32;
            continue;
        }

        switch(type) {
        case nsXPTType::T_I8:      dp->val.i8  = (int8_t)   tempu32; break;
        case nsXPTType::T_I16:     dp->val.i16 = (int16_t)  tempu32; break;
        case nsXPTType::T_I32:     dp->val.i32 = (int32_t)  tempu32; break;
        case nsXPTType::T_I64:     dp->val.i64 = (int64_t)  tempu64; break;
        case nsXPTType::T_U8:      dp->val.u8  = (uint8_t)  tempu32; break;
        case nsXPTType::T_U16:     dp->val.u16 = (uint16_t) tempu32; break;
        case nsXPTType::T_U32:     dp->val.u32 = (uint32_t) tempu32; break;
        case nsXPTType::T_U64:     dp->val.u64 = (uint64_t) tempu64; break;
        case nsXPTType::T_BOOL:    dp->val.b   = (bool)     tempu32; break;
        case nsXPTType::T_CHAR:    dp->val.c   = (char)     tempu32; break;
        case nsXPTType::T_WCHAR:   dp->val.wc  = (wchar_t)  tempu32; break;

        default:
            NS_ERROR("bad type");
            break;
        }
    }

    result = self->mOuter->CallMethod((uint16_t)methodIndex,
                                      info,
                                      dispatchParams);

    if (dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}





















# define STUB_ENTRY(n)							\
__asm__ (								\
	".align	2 \n\t"							\
	".if	"#n" < 10 \n\t"						\
	".globl	_ZN14nsXPTCStubBase5Stub"#n"Ev \n\t"			\
	".type	_ZN14nsXPTCStubBase5Stub"#n"Ev,@function \n\n"		\
"_ZN14nsXPTCStubBase5Stub"#n"Ev: \n\t"					\
									\
	".elseif "#n" < 100 \n\t"					\
	".globl	_ZN14nsXPTCStubBase6Stub"#n"Ev \n\t"			\
	".type	_ZN14nsXPTCStubBase6Stub"#n"Ev,@function \n\n"		\
"_ZN14nsXPTCStubBase6Stub"#n"Ev: \n\t"					\
									\
	".elseif "#n" < 1000 \n\t"					\
	".globl	_ZN14nsXPTCStubBase7Stub"#n"Ev \n\t"			\
	".type	_ZN14nsXPTCStubBase7Stub"#n"Ev,@function \n\n"		\
"_ZN14nsXPTCStubBase7Stub"#n"Ev: \n\t"					\
									\
	".else \n\t"							\
	".err	\"stub number "#n" >= 1000 not yet supported\"\n"	\
	".endif \n\t"							\
									\
	"li	11,"#n" \n\t"						\
	"b	SharedStub@local \n"					\
);

#define SENTINEL_ENTRY(n)                            \
nsresult nsXPTCStubBase::Sentinel##n()               \
{                                                    \
  NS_ERROR("nsXPTCStubBase::Sentinel called"); \
  return NS_ERROR_NOT_IMPLEMENTED;                   \
}

#include "xptcstubsdef.inc"
