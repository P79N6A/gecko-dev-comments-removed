






#include "xptcprivate.h"



extern "C" {
    static uint32_t
    invoke_count_words(uint32_t paramCount, nsXPTCVariant* s)
    {
        uint32_t result = 0;
        for(uint32_t i = 0; i < paramCount; i++, s++)
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
    invoke_copy_to_stack(uint32_t* d, uint32_t paramCount, nsXPTCVariant* s)
    {
        for(uint32_t i = 0; i < paramCount; i++, d++, s++)
        {
            if(s->IsPtrData())
            {
                *((void**)d) = s->ptr;
                continue;
            }
            switch(s->type)
            {
            
            
            case nsXPTType::T_I8     : *((uint32_t*)d) = s->val.i8;          break;
            case nsXPTType::T_I16    : *((uint32_t*)d) = s->val.i16;         break;
            case nsXPTType::T_I32    : *((int32_t*) d) = s->val.i32;         break;
            case nsXPTType::T_I64    : *((int64_t*) d) = s->val.i64; d++;    break;
            case nsXPTType::T_U8     : *((uint32_t*)d) = s->val.u8;          break;
            case nsXPTType::T_U16    : *((uint32_t*)d) = s->val.u16;         break;
            case nsXPTType::T_U32    : *((uint32_t*)d) = s->val.u32;         break;
            case nsXPTType::T_U64    : *((uint64_t*)d) = s->val.u64; d++;    break;
            case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
            case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
            case nsXPTType::T_BOOL   : *((uint32_t*)d) = s->val.b;           break;
            case nsXPTType::T_CHAR   : *((uint32_t*)d) = s->val.c;           break;
            case nsXPTType::T_WCHAR  : *((wchar_t*) d) = s->val.wc;          break;

            default:
                
                *((void**)d) = s->val.p;
                break;
            }
        }
    }
}

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex_P(nsISupports* that, uint32_t methodIndex,
                   uint32_t paramCount, nsXPTCVariant* params)
{
    uint32_t result, n;

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
