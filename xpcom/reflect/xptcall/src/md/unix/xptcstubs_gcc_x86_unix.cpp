






































#ifdef __GNUC__         

#include "xptcprivate.h"
#include "xptiprivate.h"
#include "xptc_platforms_unixish_x86.h"
#include "xptc_gcc_x86_unix.h"

extern "C" {
static nsresult ATTRIBUTE_USED
__attribute__ ((regparm (3)))
PrepareAndDispatch(uint32 methodIndex, nsXPTCStubBase* self, PRUint32* args)
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

    result = self->mOuter->CallMethod((PRUint16)methodIndex, info, dispatchParams);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}
} 


#if (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ == 0))
PRUint32
xptc_PrepareAndDispatch_keeper (void)
{
    PRUint32 dummy1;
    nsresult ATTRIBUTE_USED __attribute__ ((regparm (3))) (*dummy2)
        (uint32, nsXPTCStubBase *, PRUint32*) = PrepareAndDispatch;

    __asm__ __volatile__ (
        ""
        : "=&a" (dummy1)
        : "g"   (dummy2)
    );
    return dummy1 & 0xF0F00000;
}
#endif

#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 

#define STUB_ENTRY(n) \
asm(".text\n\t" \
    ".align	2\n\t" \
    ".if	" #n " < 10\n\t" \
    ".globl	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase5Stub" #n "Ev\n\t" \
    ".hidden	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase5Stub" #n "Ev\n\t" \
    ".type	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase5Stub" #n "Ev,@function\n" \
    SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase5Stub" #n "Ev:\n\t" \
    ".elseif	" #n " < 100\n\t" \
    ".globl	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase6Stub" #n "Ev\n\t" \
    ".hidden	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase6Stub" #n "Ev\n\t" \
    ".type	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase6Stub" #n "Ev,@function\n" \
    SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase6Stub" #n "Ev:\n\t" \
    ".elseif    " #n " < 1000\n\t" \
    ".globl     " SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase7Stub" #n "Ev\n\t" \
    ".hidden    " SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase7Stub" #n "Ev\n\t" \
    ".type      " SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase7Stub" #n "Ev,@function\n" \
    SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase7Stub" #n "Ev:\n\t" \
    ".else\n\t" \
    ".err	\"stub number " #n " >= 1000 not yet supported\"\n\t" \
    ".endif\n\t" \
    "movl	$" #n ", %eax\n\t" \
    "jmp	" SYMBOL_UNDERSCORE "SharedStub\n\t" \
    ".if	" #n " < 10\n\t" \
    ".size	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase5Stub" #n "Ev,.-" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase5Stub" #n "Ev\n\t" \
    ".elseif	" #n " < 100\n\t" \
    ".size	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase6Stub" #n "Ev,.-" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase6Stub" #n "Ev\n\t" \
    ".else\n\t" \
    ".size	" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase7Stub" #n "Ev,.-" SYMBOL_UNDERSCORE "_ZN14nsXPTCStubBase7Stub" #n "Ev\n\t" \
    ".endif");
#else
#define STUB_ENTRY(n) \
asm(".text\n\t" \
    ".align	2\n\t" \
    ".globl	" SYMBOL_UNDERSCORE "Stub" #n "__14nsXPTCStubBase\n\t" \
    ".hidden	" SYMBOL_UNDERSCORE "Stub" #n "__14nsXPTCStubBase\n\t" \
    ".type	" SYMBOL_UNDERSCORE "Stub" #n "__14nsXPTCStubBase,@function\n" \
    SYMBOL_UNDERSCORE "Stub" #n "__14nsXPTCStubBase:\n\t" \
    "movl	$" #n ", %eax\n\t" \
    "jmp	" SYMBOL_UNDERSCORE "SharedStub\n\t" \
    ".size	" SYMBOL_UNDERSCORE "Stub" #n "__14nsXPTCStubBase,.-" SYMBOL_UNDERSCORE "Stub" #n "__14nsXPTCStubBase");
#endif


asm(".text\n\t"
    ".align	2\n\t"
    ".type	" SYMBOL_UNDERSCORE "SharedStub,@function\n\t"
    SYMBOL_UNDERSCORE "SharedStub:\n\t"
    "leal	0x08(%esp), %ecx\n\t"
    "movl	0x04(%esp), %edx\n\t"
    "jmp	" SYMBOL_UNDERSCORE "PrepareAndDispatch\n\t"
    ".size	" SYMBOL_UNDERSCORE "SharedStub,.-" SYMBOL_UNDERSCORE "SharedStub");

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

void
xptc_dummy()
{
}

#else
#error "can't find a compiler to use"
#endif 
