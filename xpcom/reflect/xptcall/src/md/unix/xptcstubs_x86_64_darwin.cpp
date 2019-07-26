









#include "xptcprivate.h"
#include "xptiprivate.h"







const PRUint32 PARAM_BUFFER_COUNT   = 16;
const PRUint32 GPR_COUNT            = 6;
const PRUint32 FPR_COUNT            = 8;










extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase * self, PRUint32 methodIndex,
                   PRUint64 * args, PRUint64 * gpregs, double *fpregs)
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
    if (!info)
        return NS_ERROR_UNEXPECTED;

    paramCount = info->GetParamCount();

    
    if (paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;

    NS_ASSERTION(dispatchParams,"no place for params");
    if (!dispatchParams)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint64* ap = args;
    PRUint32 nr_gpr = 1;    
    PRUint32 nr_fpr = 0;
    PRUint64 value;

    for (i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];
	
        if (!param.IsOut() && type == nsXPTType::T_DOUBLE) {
            if (nr_fpr < FPR_COUNT)
                dp->val.d = fpregs[nr_fpr++];
            else
                dp->val.d = *(double*) ap++;
            continue;
        }
        else if (!param.IsOut() && type == nsXPTType::T_FLOAT) {
            if (nr_fpr < FPR_COUNT)
                
                
                
                dp->val.d = fpregs[nr_fpr++];
            else
                dp->val.f = *(float*) ap++;
            continue;
        }
        else {
            if (nr_gpr < GPR_COUNT)
                value = gpregs[nr_gpr++];
            else
                value = *ap++;
        }

        if (param.IsOut() || !type.IsArithmetic()) {
            dp->val.p = (void*) value;
            continue;
        }

        switch (type) {
        case nsXPTType::T_I8:      dp->val.i8  = (PRInt8)   value; break;
        case nsXPTType::T_I16:     dp->val.i16 = (PRInt16)  value; break;
        case nsXPTType::T_I32:     dp->val.i32 = (PRInt32)  value; break;
        case nsXPTType::T_I64:     dp->val.i64 = (PRInt64)  value; break;
        case nsXPTType::T_U8:      dp->val.u8  = (PRUint8)  value; break;
        case nsXPTType::T_U16:     dp->val.u16 = (PRUint16) value; break;
        case nsXPTType::T_U32:     dp->val.u32 = (PRUint32) value; break;
        case nsXPTType::T_U64:     dp->val.u64 = (PRUint64) value; break;
        case nsXPTType::T_BOOL:    dp->val.b   = (bool)   value; break;
        case nsXPTType::T_CHAR:    dp->val.c   = (char)     value; break;
        case nsXPTType::T_WCHAR:   dp->val.wc  = (wchar_t)  value; break;

        default:
            NS_ERROR("bad type");
            break;
        }
    }

    result = self->mOuter->CallMethod((PRUint16) methodIndex, info, dispatchParams);

    if (dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}



#define STUB_ENTRY(n) \
asm(".section	__TEXT,__text\n\t" \
    ".align	3\n\t" \
    ".if	" #n " < 10\n\t" \
    ".globl	__ZN14nsXPTCStubBase5Stub" #n "Ev\n\t" \
    "__ZN14nsXPTCStubBase5Stub" #n "Ev:\n\t" \
    ".elseif	" #n " < 100\n\t" \
    ".globl	__ZN14nsXPTCStubBase6Stub" #n "Ev\n\t" \
    "__ZN14nsXPTCStubBase6Stub" #n "Ev:\n\t" \
    ".elseif    " #n " < 1000\n\t" \
    ".globl     __ZN14nsXPTCStubBase7Stub" #n "Ev\n\t" \
    "__ZN14nsXPTCStubBase7Stub" #n "Ev:\n\t" \
    ".else\n\t" \
    ".err	\"stub number " #n " >= 1000 not yet supported\"\n\t" \
    ".endif\n\t" \
    "movl	$" #n ", %eax\n\t" \
    "jmp	SharedStub\n\t");


asm(".section   __TEXT,__text\n\t"
    ".align     3\n\t"
    "SharedStub:\n\t"
    
    "pushq      %rbp\n\t"
    "movq       %rsp,%rbp\n\t"
    "subq       $112,%rsp\n\t"
    
    "movq       %rdi,-112(%rbp)\n\t"
    "movq       %rsi,-104(%rbp)\n\t"
    "movq       %rdx, -96(%rbp)\n\t"
    "movq       %rcx, -88(%rbp)\n\t"
    "movq       %r8 , -80(%rbp)\n\t"
    "movq       %r9 , -72(%rbp)\n\t"
    "leaq       -112(%rbp),%rcx\n\t"
    
    "movsd      %xmm0,-64(%rbp)\n\t"
    "movsd      %xmm1,-56(%rbp)\n\t"
    "movsd      %xmm2,-48(%rbp)\n\t"
    "movsd      %xmm3,-40(%rbp)\n\t"
    "movsd      %xmm4,-32(%rbp)\n\t"
    "movsd      %xmm5,-24(%rbp)\n\t"
    "movsd      %xmm6,-16(%rbp)\n\t"
    "movsd      %xmm7, -8(%rbp)\n\t"
    "leaq       -64(%rbp),%r8\n\t"
    
    "movl       %eax,%esi\n\t"
    "leaq       16(%rbp),%rdx\n\t"
    "call       _PrepareAndDispatch\n\t"
    "leave\n\t"
    "ret\n\t");

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"
