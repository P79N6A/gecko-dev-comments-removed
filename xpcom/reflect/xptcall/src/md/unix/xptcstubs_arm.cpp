






#include "xptcprivate.h"
#include "xptiprivate.h"

#if !defined(__arm__) && !(defined(LINUX) || defined(ANDROID))
#error "This code is for Linux ARM only. Please check if it works for you, too.\nDepends strongly on gcc behaviour."
#endif

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4))



#define DONT_DROP_OR_WARN __attribute__((used))
#else



#define DONT_DROP_OR_WARN __attribute__((unused))
#endif


static nsresult PrepareAndDispatch(nsXPTCStubBase* self, uint32 methodIndex, uint32_t* args) asm("_PrepareAndDispatch")
DONT_DROP_OR_WARN;

#ifdef __ARM_EABI__
#define DOUBLEWORD_ALIGN(p) ((uint32_t *)((((uint32_t)(p)) + 7) & 0xfffffff8))
#else
#define DOUBLEWORD_ALIGN(p) (p)
#endif


#ifdef __APPLE__
#define CFI(str)
#else
#define CFI(str) str
#endif

static nsresult
PrepareAndDispatch(nsXPTCStubBase* self, uint32 methodIndex, uint32_t* args)
{
#define PARAM_BUFFER_COUNT     16

    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = NULL;
    const nsXPTMethodInfo* info;
    uint8_t paramCount;
    uint8_t i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    self->mEntry->GetMethodInfo(uint16_t(methodIndex), &info);
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
        case nsXPTType::T_I64    : ap = DOUBLEWORD_ALIGN(ap);
				   dp->val.i64 = *((int64_t*) ap); ap++; break;
        case nsXPTType::T_U8     : dp->val.u8  = *((uint8_t*) ap);       break;
        case nsXPTType::T_U16    : dp->val.u16 = *((uint16_t*)ap);       break;
        case nsXPTType::T_U32    : dp->val.u32 = *((uint32_t*)ap);       break;
        case nsXPTType::T_U64    : ap = DOUBLEWORD_ALIGN(ap);
				   dp->val.u64 = *((uint64_t*)ap); ap++; break;
        case nsXPTType::T_FLOAT  : dp->val.f   = *((float*)   ap);       break;
        case nsXPTType::T_DOUBLE : ap = DOUBLEWORD_ALIGN(ap);
				   dp->val.d   = *((double*)  ap); ap++; break;
        case nsXPTType::T_BOOL   : dp->val.b   = *((bool*)  ap);       break;
        case nsXPTType::T_CHAR   : dp->val.c   = *((char*)    ap);       break;
        case nsXPTType::T_WCHAR  : dp->val.wc  = *((wchar_t*) ap);       break;
        default:
            NS_ERROR("bad type");
            break;
        }
    }

    result = self->mOuter->CallMethod((uint16_t)methodIndex, info, dispatchParams);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}





























__asm__ ("\n"
         ".text\n"
         ".align 2\n"
         "SharedStub:\n"
         ".fnstart\n"
         CFI(".cfi_startproc\n")
         "stmfd	sp!, {r1, r2, r3}\n"
         ".save	{r1, r2, r3}\n"
         CFI(".cfi_def_cfa_offset 12\n")
         CFI(".cfi_offset r3, -4\n")
         CFI(".cfi_offset r2, -8\n")
         CFI(".cfi_offset r1, -12\n")
         "mov	r2, sp\n"
         "str	lr, [sp, #-4]!\n"
         ".save	{lr}\n"
         CFI(".cfi_def_cfa_offset 16\n")
         CFI(".cfi_offset lr, -16\n")
         "mov	r1, ip\n"
         "bl	_PrepareAndDispatch\n"
         "ldr	pc, [sp], #16\n"
         CFI(".cfi_endproc\n")
         ".fnend");





















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
