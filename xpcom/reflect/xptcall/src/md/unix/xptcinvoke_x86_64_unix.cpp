







































#include "xptcprivate.h"


const PRUint32 GPR_COUNT = 6;


const PRUint32 FPR_COUNT = 8;


static inline void
invoke_count_words(PRUint32 paramCount, nsXPTCVariant * s,
                   PRUint32 & nr_gpr, PRUint32 & nr_fpr, PRUint32 & nr_stack)
{
    nr_gpr = 1; 
    nr_fpr = 0;
    nr_stack = 0;

    
    for (uint32 i = 0; i < paramCount; i++, s++) {
        if (!s->IsPtrData()
            && (s->type == nsXPTType::T_FLOAT || s->type == nsXPTType::T_DOUBLE)) {
            if (nr_fpr < FPR_COUNT)
                nr_fpr++;
            else
                nr_stack++;
        }
        else {
            if (nr_gpr < GPR_COUNT)
                nr_gpr++;
            else
                nr_stack++;
        }
    }
}

static void
invoke_copy_to_stack(PRUint64 * d, PRUint32 paramCount, nsXPTCVariant * s,
                     PRUint64 * gpregs, double * fpregs)
{
    PRUint32 nr_gpr = 1; 
    PRUint32 nr_fpr = 0;
    PRUint64 value;

    for (uint32 i = 0; i < paramCount; i++, s++) {
        if (s->IsPtrData())
            value = (PRUint64) s->ptr;
        else {
            switch (s->type) {
            case nsXPTType::T_FLOAT:                                break;
            case nsXPTType::T_DOUBLE:                               break;
            case nsXPTType::T_I8:     value = s->val.i8;            break;
            case nsXPTType::T_I16:    value = s->val.i16;           break;
            case nsXPTType::T_I32:    value = s->val.i32;           break;
            case nsXPTType::T_I64:    value = s->val.i64;           break;
            case nsXPTType::T_U8:     value = s->val.u8;            break;
            case nsXPTType::T_U16:    value = s->val.u16;           break;
            case nsXPTType::T_U32:    value = s->val.u32;           break;
            case nsXPTType::T_U64:    value = s->val.u64;           break;
            case nsXPTType::T_BOOL:   value = s->val.b;             break;
            case nsXPTType::T_CHAR:   value = s->val.c;             break;
            case nsXPTType::T_WCHAR:  value = s->val.wc;            break;
            default:                  value = (PRUint64) s->val.p;  break;
            }
        }

        if (!s->IsPtrData() && s->type == nsXPTType::T_DOUBLE) {
            if (nr_fpr < FPR_COUNT)
                fpregs[nr_fpr++] = s->val.d;
            else {
                *((double *)d) = s->val.d;
                d++;
            }
        }
        else if (!s->IsPtrData() && s->type == nsXPTType::T_FLOAT) {
            if (nr_fpr < FPR_COUNT)
                
                
                
                fpregs[nr_fpr++] = s->val.d;
            else {
                *((float *)d) = s->val.f;
                d++;
            }
        }
        else {
            if (nr_gpr < GPR_COUNT)
                gpregs[nr_gpr++] = value;
            else
                *d++ = value;
        }
    }
}

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex_P(nsISupports * that, PRUint32 methodIndex,
                 PRUint32 paramCount, nsXPTCVariant * params)
{
    PRUint32 nr_gpr, nr_fpr, nr_stack;
    invoke_count_words(paramCount, params, nr_gpr, nr_fpr, nr_stack);
    
    
    if (nr_stack)
        nr_stack = (nr_stack + 1) & ~1;

    
    PRUint64 *stack = (PRUint64 *) __builtin_alloca(nr_stack * 8);
    PRUint64 gpregs[GPR_COUNT];
    double fpregs[FPR_COUNT];
    invoke_copy_to_stack(stack, paramCount, params, gpregs, fpregs);

    
    double d0, d1, d2, d3, d4, d5, d6, d7;

    switch (nr_fpr) {
#define ARG_FPR(N) \
    case N+1: d##N = fpregs[N];
        ARG_FPR(7);
        ARG_FPR(6);
        ARG_FPR(5);
        ARG_FPR(4);
        ARG_FPR(3);
        ARG_FPR(2);
        ARG_FPR(1);
        ARG_FPR(0);
    case 0:;
#undef ARG_FPR
    }
    
    
    PRUint64 a0, a1, a2, a3, a4, a5;
    
    switch (nr_gpr) {
#define ARG_GPR(N) \
    case N+1: a##N = gpregs[N];
        ARG_GPR(5);
        ARG_GPR(4);
        ARG_GPR(3);
        ARG_GPR(2);
        ARG_GPR(1);
    case 1: a0 = (PRUint64) that;
    case 0:;
#undef ARG_GPR
    }
    
    
    PRUint64 methodAddress = *((PRUint64 *)that);
    methodAddress += 8 * methodIndex;
    methodAddress = *((PRUint64 *)methodAddress);
    
    typedef PRUint32 (*Method)(PRUint64, PRUint64, PRUint64, PRUint64,
                               PRUint64, PRUint64, double, double, double,
                               double, double, double, double, double);
    PRUint32 result = ((Method)methodAddress)(a0, a1, a2, a3, a4, a5,
                                              d0, d1, d2, d3, d4, d5,
                                              d6, d7);
    return result;
}
