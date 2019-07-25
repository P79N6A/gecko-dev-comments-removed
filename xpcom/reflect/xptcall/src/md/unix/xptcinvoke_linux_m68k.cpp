






































#include "xptcprivate.h"



extern "C" {
    static PRUint32
    invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
    {
        PRUint32 result = 0;
        for(PRUint32 i = 0; i < paramCount; i++, s++)
        {
            if(s->IsPtrData())
            {
                result++;
                continue;
            }
            switch(s->type)
            {
            case nsXPTType::T_I8     :
            case nsXPTType::T_I16    :
            case nsXPTType::T_I32    :
                result++;
                break;
            case nsXPTType::T_I64    :
                result+=2;
                break;
            case nsXPTType::T_U8     :
            case nsXPTType::T_U16    :
            case nsXPTType::T_U32    :
                result++;
                break;
            case nsXPTType::T_U64    :
                result+=2;
                break;
            case nsXPTType::T_FLOAT  :
                result++;
                break;
            case nsXPTType::T_DOUBLE :
                result+=2;
                break;
            case nsXPTType::T_BOOL   :
            case nsXPTType::T_CHAR   :
            case nsXPTType::T_WCHAR  :
                result++;
                break;
            default:
                
                result++;
                break;
            }
        }
        return result;
    }

    void
    invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount, nsXPTCVariant* s)
    {
        for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
        {
            if(s->IsPtrData())
            {
                *((void**)d) = s->ptr;
                continue;
            }
            switch(s->type)
            {
            
            
            case nsXPTType::T_I8     : *((PRUint32*)d) = s->val.i8;          break;
            case nsXPTType::T_I16    : *((PRUint32*)d) = s->val.i16;         break;
            case nsXPTType::T_I32    : *((PRInt32*) d) = s->val.i32;         break;
            case nsXPTType::T_I64    : *((PRInt64*) d) = s->val.i64; d++;    break;
            case nsXPTType::T_U8     : *((PRUint32*)d) = s->val.u8;          break;
            case nsXPTType::T_U16    : *((PRUint32*)d) = s->val.u16;         break;
            case nsXPTType::T_U32    : *((PRUint32*)d) = s->val.u32;         break;
            case nsXPTType::T_U64    : *((PRUint64*)d) = s->val.u64; d++;    break;
            case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
            case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
            case nsXPTType::T_BOOL   : *((PRUint32*)d) = s->val.b;           break;
            case nsXPTType::T_CHAR   : *((PRUint32*)d) = s->val.c;           break;
            case nsXPTType::T_WCHAR  : *((wchar_t*) d) = s->val.wc;          break;

            default:
                
                *((void**)d) = s->val.p;
                break;
            }
        }
    }
}

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex_P(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    PRUint32 result, n;

    n = invoke_count_words(paramCount, params) * 4;

 __asm__ __volatile__(
    "subl  %5, %%sp\n\t"      
    "movel %4, %%sp@-\n\t"
    "movel %3, %%sp@-\n\t"
    "pea   %%sp@(8)\n\t"
    "jbsr  invoke_copy_to_stack\n\t"   
    "addw  #12, %%sp\n\t"
    "movel %1, %%sp@-\n\t"
    "movel %1@, %%a0\n\t"
    "movel %%a0@(%2:l:4), %%a0\n\t"
    "jbsr  %%a0@\n\t"         
    "lea   %%sp@(4,%5:l), %%sp\n\t"
    "movel %%d0, %0"
    : "=d" (result)         
    : "a" (that),           
      "d" (methodIndex),    
      "g" (paramCount),     
      "g" (params),         
      "d" (n)               
    : "a0", "a1", "d0", "d1", "memory"
    );
  
  return result;
}
