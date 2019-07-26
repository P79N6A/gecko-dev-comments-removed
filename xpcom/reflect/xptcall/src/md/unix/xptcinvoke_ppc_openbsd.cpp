














#include "xptcprivate.h"


#define GPR_COUNT     8



#define FPR_COUNT     8

extern "C" uint32_t
invoke_count_words(uint32_t paramCount, nsXPTCVariant* s)
{
  return uint32_t(((paramCount * 2) + 3) & ~3);
}

extern "C" void
invoke_copy_to_stack(uint32_t* d,
                     uint32_t paramCount,
                     nsXPTCVariant* s, 
                     uint32_t* gpregs,
                     double* fpregs)
{
    uint32_t gpr = 1; 
    uint32_t fpr = 0;
    uint32_t tempu32;
    uint64_t tempu64;
    
    for(uint32_t i = 0; i < paramCount; i++, s++) {
        if(s->IsPtrData())
            tempu32 = (uint32_t) s->ptr;
        else {
            switch(s->type) {
            case nsXPTType::T_FLOAT:                                  break;
            case nsXPTType::T_DOUBLE:                                 break;
            case nsXPTType::T_I8:     tempu32 = s->val.i8;            break;
            case nsXPTType::T_I16:    tempu32 = s->val.i16;           break;
            case nsXPTType::T_I32:    tempu32 = s->val.i32;           break;
            case nsXPTType::T_I64:    tempu64 = s->val.i64;           break;
            case nsXPTType::T_U8:     tempu32 = s->val.u8;            break;
            case nsXPTType::T_U16:    tempu32 = s->val.u16;           break;
            case nsXPTType::T_U32:    tempu32 = s->val.u32;           break;
            case nsXPTType::T_U64:    tempu64 = s->val.u64;           break;
            case nsXPTType::T_BOOL:   tempu32 = s->val.b;             break;
            case nsXPTType::T_CHAR:   tempu32 = s->val.c;             break;
            case nsXPTType::T_WCHAR:  tempu32 = s->val.wc;            break;
            default:                  tempu32 = (uint32_t) s->val.p;  break;
            }
        }

        if (!s->IsPtrData() && s->type == nsXPTType::T_DOUBLE) {
            if (fpr < FPR_COUNT)
                fpregs[fpr++]    = s->val.d;
            else {
                if ((uint32_t) d & 4) d++; 
                *((double*) d) = s->val.d;
                d += 2;
            }
        }
        else if (!s->IsPtrData() && s->type == nsXPTType::T_FLOAT) {
            if (fpr < FPR_COUNT)
                fpregs[fpr++]   = s->val.f; 
            else
                *((float*) d++) = s->val.f;
        }
        else if (!s->IsPtrData() && (s->type == nsXPTType::T_I64
                                     || s->type == nsXPTType::T_U64)) {
            if ((gpr + 1) < GPR_COUNT) {
                if (gpr & 1) gpr++; 
                *((uint64_t*) &gpregs[gpr]) = tempu64;
                gpr += 2;
            }
            else {
                if ((uint32_t) d & 4) d++; 
                *((uint64_t*) d)            = tempu64;
                d += 2;
            }
        }
        else {
            if (gpr < GPR_COUNT)
                gpregs[gpr++] = tempu32;
            else
                *d++          = tempu32;
        }
        
    }
}

extern "C"
EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                 uint32_t paramCount, nsXPTCVariant* params);
