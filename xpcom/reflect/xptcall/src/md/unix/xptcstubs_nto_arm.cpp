






#include "xptcprivate.h"

#if !defined(__QNXNTO__) || !defined(__arm__)
#error "This code is for Neutrino ARM only. Please check if it works for you, too.\nDepends strongly on gcc behaviour."
#endif


static nsresult PrepareAndDispatch(nsXPTCStubBase* self, uint32 methodIndex, uint32_t* args) asm("_PrepareAndDispatch");

static nsresult
PrepareAndDispatch(nsXPTCStubBase* self, uint32 methodIndex, uint32_t* args)
{
#define PARAM_BUFFER_COUNT     16

    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = NULL;
    nsIInterfaceInfo* iface_info = NULL;
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

    uint32_t* ap = args;
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
        case nsXPTType::T_I8     : dp->val.i8  = *((int8_t*)  ap);       break;
        case nsXPTType::T_I16    : dp->val.i16 = *((int16_t*) ap);       break;
        case nsXPTType::T_I32    : dp->val.i32 = *((int32_t*) ap);       break;
        case nsXPTType::T_I64    : dp->val.i64 = *((int64_t*) ap); ap++; break;
        case nsXPTType::T_U8     : dp->val.u8  = *((uint8_t*) ap);       break;
        case nsXPTType::T_U16    : dp->val.u16 = *((uint16_t*)ap);       break;
        case nsXPTType::T_U32    : dp->val.u32 = *((uint32_t*)ap);       break;
        case nsXPTType::T_U64    : dp->val.u64 = *((uint64_t*)ap); ap++; break;
        case nsXPTType::T_FLOAT  : dp->val.f   = *((float*)   ap);       break;
        case nsXPTType::T_DOUBLE : dp->val.d   = *((double*)  ap); ap++; break;
        case nsXPTType::T_BOOL   : dp->val.b   = *((bool*)  ap);       break;
        case nsXPTType::T_CHAR   : dp->val.c   = *((char*)    ap);       break;
        case nsXPTType::T_WCHAR  : dp->val.wc  = *((wchar_t*) ap);       break;
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





























__asm__ ("\n\
SharedStub:							\n\
	stmfd	sp!, {r1, r2, r3}				\n\
	mov	r2, sp						\n\
	str	lr, [sp, #-4]!					\n\
	mov	r1, ip						\n\
	bl	_PrepareAndDispatch	                        \n\
	ldr	pc, [sp], #16");




















#define STUB_ENTRY(n)						\
  __asm__(							\
	".section \".text\"\n"					\
"	.align 2\n"						\
"	.iflt ("#n" - 10)\n"                                    \
"	.globl	_ZN14nsXPTCStubBase5Stub"#n"Ev\n"		\
"	.type	_ZN14nsXPTCStubBase5Stub"#n"Ev,#function\n"	\
"_ZN14nsXPTCStubBase5Stub"#n"Ev:\n"				\
"	.else\n"                                                \
"	.iflt  ("#n" - 100)\n"                                  \
"	.globl	_ZN14nsXPTCStubBase6Stub"#n"Ev\n"		\
"	.type	_ZN14nsXPTCStubBase6Stub"#n"Ev,#function\n"	\
"_ZN14nsXPTCStubBase6Stub"#n"Ev:\n"				\
"	.else\n"                                                \
"	.iflt ("#n" - 1000)\n"                                  \
"	.globl	_ZN14nsXPTCStubBase7Stub"#n"Ev\n"		\
"	.type	_ZN14nsXPTCStubBase7Stub"#n"Ev,#function\n"	\
"_ZN14nsXPTCStubBase7Stub"#n"Ev:\n"				\
"	.else\n"                                                \
"	.err \"stub number "#n"> 1000 not yet supported\"\n"    \
"	.endif\n"                                               \
"	.endif\n"                                               \
"	.endif\n"                                               \
"	mov	ip, #"#n"\n"					\
"	b	SharedStub\n\t");

#if 0





#define STUB_ENTRY(n)  \
nsresult nsXPTCStubBase::Stub##n ()  \
{ \
  __asm__ (	  		        \
"	mov	ip, #"#n"\n"					\
"	b	SharedStub\n\t");                               \
  return 0; /* avoid warnings */                                \
}
#endif


#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"
