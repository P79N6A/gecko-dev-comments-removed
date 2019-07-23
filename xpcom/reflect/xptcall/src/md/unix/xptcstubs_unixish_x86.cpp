






































#include "xptcprivate.h"
#include "xptc_platforms_unixish_x86.h"
#include "xptiprivate.h"

static nsresult
PrepareAndDispatch(nsXPTCStubBase* self, uint32 methodIndex, PRUint32* args)
{
#define PARAM_BUFFER_COUNT     16

    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = NULL;
    const nsXPTMethodInfo* info;
    PRUint8 paramCount;
    PRUint8 i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self,"no self");

    self->mEntry->GetMethodInfo(PRUint16(methodIndex), &info);
    NS_ASSERTION(info,"no interface info");

    paramCount = info->GetParamCount();

    
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;
    NS_ASSERTION(dispatchParams,"no place for params");

    PRUint32* ap = args;
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
        
	    dp->val.p = (void*) *ap;
        switch(type)
        {
        case nsXPTType::T_I64    : dp->val.i64 = *((PRInt64*) ap); ap++; break;
        case nsXPTType::T_U64    : dp->val.u64 = *((PRUint64*)ap); ap++; break;
        case nsXPTType::T_DOUBLE : dp->val.d   = *((double*)  ap); ap++; break;
        }
    }

    result = self->mOuter->
        CallMethod((PRUint16)methodIndex, info, dispatchParams);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

#ifdef __GNUC__         

#ifdef KEEP_STACK_16_BYTE_ALIGNED



#define ALIGN_STACK_DECL \
  unsigned int saved_esp;

#define ALIGN_STACK_SAVE \
  "movl %%esp, %3\n\t"

#define ALIGN_STACK_ALIGN \
  "addl $0x4, %%esp\n\t" \
  "andl $0xfffffff0, %%esp\n\t" \
  "subl $0x4, %%esp\n\t"

#define STACK_RESTORE \
  "movl %3, %%esp\n"

#define ALIGN_STACK_REGS_IN \
  , "=r"(saved_esp) /* 3 */

#define ALIGN_STACK_REGS_OUT \
  , "3"(saved_esp)

#else
#define ALIGN_STACK_DECL
#define ALIGN_STACK_SAVE
#define ALIGN_STACK_ALIGN
#define STACK_RESTORE \
  "addl $12, %%esp\n"
#define ALIGN_STACK_REGS_IN
#define ALIGN_STACK_REGS_OUT
#endif

#define STUB_ENTRY(n) \
nsresult nsXPTCStubBase::Stub##n() \
{ \
  register nsresult (*method) (nsXPTCStubBase *, uint32, PRUint32 *) = PrepareAndDispatch; \
  int temp0, temp1; \
  register nsresult result; \
  ALIGN_STACK_DECL \
  __asm__ __volatile__( \
    ALIGN_STACK_SAVE \
    ALIGN_STACK_ALIGN \
    "leal   0x0c(%%ebp), %%ecx\n\t"    /* args */ \
    "pushl  %%ecx\n\t" \
    "pushl  $"#n"\n\t"                 /* method index */ \
    "movl   0x08(%%ebp), %%ecx\n\t"    /* this */ \
    "pushl  %%ecx\n\t" \
    "call   *%%edx\n\t"                /* PrepareAndDispatch */ \
    STACK_RESTORE                      /* "addl $12, %%esp" or restore saved */ \
    : "=a" (result),    /* %0 */ \
      "=&c" (temp0),    /* %1 */ \
      "=d" (temp1)      /* %2 */ \
      ALIGN_STACK_REGS_IN \
    : "2" (method)      /* %2 */ \
      ALIGN_STACK_REGS_OUT \
    : "memory" \
	); \
    return result; \
}

#else
#error "can't find a compiler to use"
#endif 

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"
