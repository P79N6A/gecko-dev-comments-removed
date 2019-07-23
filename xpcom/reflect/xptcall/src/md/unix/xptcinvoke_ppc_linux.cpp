


















































#include "xptcprivate.h"


#define GPR_COUNT     8




#ifndef __NO_FPRS__
#define FPR_COUNT     8
#endif
extern "C" PRUint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
  return PRUint32(((paramCount * 2) + 3) & ~3);
}

extern "C" void
invoke_copy_to_stack(PRUint32* d,
                     PRUint32 paramCount,
                     nsXPTCVariant* s, 
                     PRUint32* gpregs,
                     double* fpregs)
{
    PRUint32 gpr = 1; 
#ifndef __NO_FPRS__
    PRUint32 fpr = 0;
#endif
    PRUint32 tempu32;
    PRUint64 tempu64;
    
    for(uint32 i = 0; i < paramCount; i++, s++) {
        if(s->IsPtrData())
            tempu32 = (PRUint32) s->ptr;
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
            default:                  tempu32 = (PRUint32) s->val.p;  break;
            }
        }

        if (!s->IsPtrData() && s->type == nsXPTType::T_DOUBLE) {
#ifndef __NO_FPRS__
            if (fpr < FPR_COUNT)
                fpregs[fpr++]    = s->val.d;
#else
            if (gpr & 1)
                gpr++;
            if ((gpr + 1) < GPR_COUNT) {
                *((double*) &gpregs[gpr]) = s->val.d;
                gpr += 2;
            }
#endif
            else {
                if ((PRUint32) d & 4) d++; 
                *((double*) d) = s->val.d;
                d += 2;
            }
        }
        else if (!s->IsPtrData() && s->type == nsXPTType::T_FLOAT) {
#ifndef __NO_FPRS__
            if (fpr < FPR_COUNT)
                fpregs[fpr++]   = s->val.f; 
#else
            if (gpr < GPR_COUNT)
                *((float*) &gpregs[gpr++]) = s->val.f;
#endif
            else
                *((float*) d++) = s->val.f;
        }
        else if (!s->IsPtrData() && (s->type == nsXPTType::T_I64
                                     || s->type == nsXPTType::T_U64)) {
            if (gpr & 1) gpr++; 
            if ((gpr + 1) < GPR_COUNT) {
                *((PRUint64*) &gpregs[gpr]) = tempu64;
                gpr += 2;
            }
            else {
                if ((PRUint32) d & 4) d++; 
                *((PRUint64*) d)            = tempu64;
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
NS_InvokeByIndex_P(nsISupports* that, PRUint32 methodIndex,
                 PRUint32 paramCount, nsXPTCVariant* params);
