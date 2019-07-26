

















#include <stdio.h>
#include "xptcprivate.h"


#define GPR_COUNT     7



#define FPR_COUNT     13

extern "C" uint32_t
invoke_count_words(uint32_t paramCount, nsXPTCVariant* s)
{
    return uint32_t(((paramCount * 2) + 3) & ~3);
}

extern "C" void
invoke_copy_to_stack(uint64_t* gpregs,
                     double* fpregs,
                     uint32_t paramCount,
                     nsXPTCVariant* s, 
                     uint64_t* d)
{
    uint64_t tempu64;

    for(uint32_t i = 0; i < paramCount; i++, s++) {
        if(s->IsPtrData())
            tempu64 = (uint64_t) s->ptr;
        else {
            switch(s->type) {
            case nsXPTType::T_FLOAT:                                  break;
            case nsXPTType::T_DOUBLE:                                 break;
            case nsXPTType::T_I8:     tempu64 = s->val.i8;            break;
            case nsXPTType::T_I16:    tempu64 = s->val.i16;           break;
            case nsXPTType::T_I32:    tempu64 = s->val.i32;           break;
            case nsXPTType::T_I64:    tempu64 = s->val.i64;           break;
            case nsXPTType::T_U8:     tempu64 = s->val.u8;            break;
            case nsXPTType::T_U16:    tempu64 = s->val.u16;           break;
            case nsXPTType::T_U32:    tempu64 = s->val.u32;           break;
            case nsXPTType::T_U64:    tempu64 = s->val.u64;           break;
            case nsXPTType::T_BOOL:   tempu64 = s->val.b;             break;
            case nsXPTType::T_CHAR:   tempu64 = s->val.c;             break;
            case nsXPTType::T_WCHAR:  tempu64 = s->val.wc;            break;
            default:                  tempu64 = (uint64_t) s->val.p;  break;
            }
        }

        if (!s->IsPtrData() && s->type == nsXPTType::T_DOUBLE) {
            if (i < FPR_COUNT)
                fpregs[i]    = s->val.d;
            else
                *(double *)d = s->val.d;
        }
        else if (!s->IsPtrData() && s->type == nsXPTType::T_FLOAT) {
            if (i < FPR_COUNT) {
                fpregs[i]   = s->val.f; 
            } else {
                float *p = (float *)d;
                p++;
                *p = s->val.f;
            }
        }
        else {
            if (i < GPR_COUNT)
                gpregs[i] = tempu64;
            else
                *d = tempu64;
        }
        if (i >= 7)
            d++;
    }
}

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                   uint32_t paramCount, nsXPTCVariant* params);

