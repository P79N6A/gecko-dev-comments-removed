






































#include "xptcprivate.h"



#if !defined(__NetBSD__) || !defined(__m68k__)
#error This code is for NetBSD/m68k only
#endif

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

    static void
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
            case nsXPTType::T_BOOL   : *((PRBool*)  d) = s->val.b;           break;
            case nsXPTType::T_CHAR   : *((PRUint32*)d) = s->val.c;           break;
            
            case nsXPTType::T_WCHAR  : *((wchar_t*) d) = s->val.wc;          break;
            default:
                
                *((void**)d) = s->val.p;
                break;
            }
        }
    }
}

XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    PRUint32 result;

 __asm__ __volatile__(
    "movl  %4, sp@-\n\t"
    "movl  %3, sp@-\n\t"
    "jbsr  _invoke_count_words\n\t"     
    "addql #8, sp\n\t"
    "lsll  #2, d0\n\t"      
    "movl  sp, a2\n\t"	    
    "subl  d0, sp\n\t"      
    "movl  sp, a0\n\t"
    "movl  %4, sp@-\n\t"
    "movl  %3, sp@-\n\t"
    "movl  a0, sp@-\n\t"
    "jbsr  _invoke_copy_to_stack\n\t"   
    "addl  #12, sp\n\t"
    "movl  %1, a0\n\t"
    "movl  a0@, a1\n\t"
    "movl  %2, d0\n\t"      
    "movl  a0, d1\n\t"
    "movw  a1@(8,d0:l:8), a0\n\t"
    "addl  a0, d1\n\t"
    "movl  a1@(12,d0:l:8), a1\n\t"
    "movl  d1, sp@-\n\t"
    "jbsr  a1@\n\t"
    "movl  a2, sp\n\t"	    
    "movl  d0, %0\n\t"
    : "=g" (result)         
    : "g" (that),           
      "g" (methodIndex),    
      "g" (paramCount),     
      "g" (params)          
    : "a0", "a1", "a2", "d0", "d1", "memory"
    );
  
  return result;
}
