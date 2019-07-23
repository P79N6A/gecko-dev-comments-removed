







































#include "xptcprivate.h"
#include "xptc_platforms_unixish_x86.h"

extern "C" {



#if !defined(__SUNPRO_CC)               
static
#endif
PRUint32
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
        result++;
        switch(s->type)
        {
        case nsXPTType::T_I64    :
        case nsXPTType::T_U64    :
        case nsXPTType::T_DOUBLE :
            result++;
            break;
        }
    }
    return result;
}

#if !defined(__SUNPRO_CC)               
static
#endif
void
invoke_copy_to_stack(PRUint32 paramCount, nsXPTCVariant* s, PRUint32* d)
{
    for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }







        *((void**)d) = s->val.p;

        switch(s->type)
        {
        case nsXPTType::T_I64    : *((PRInt64*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U64    : *((PRUint64*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        }
    }
}

}

#if !defined(__SUNPRO_CC)               
EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                 PRUint32 paramCount, nsXPTCVariant* params)
{
#ifdef __GNUC__            
  PRUint32 result;
  PRUint32 n = invoke_count_words (paramCount, params) * 4;
  void (*fn_copy) (unsigned int, nsXPTCVariant *, PRUint32 *) = invoke_copy_to_stack;
  int temp1, temp2, temp3;
 
 __asm__ __volatile__(
    "subl  %8, %%esp\n\t" 
    "pushl %%esp\n\t"
    "pushl %7\n\t"
    "pushl %6\n\t"
    "call  *%0\n\t"       
    "addl  $0xc, %%esp\n\t"
    "movl  %4, %%ecx\n\t"
#ifdef CFRONT_STYLE_THIS_ADJUST
    "movl  (%%ecx), %%edx\n\t"
    "movl  %5, %%eax\n\t"   
    "shl   $3, %%eax\n\t"   
    "addl  $8, %%eax\n\t"   
    "addl  %%eax, %%edx\n\t"
    "movswl (%%edx), %%eax\n\t" 
    "addl  %%eax, %%ecx\n\t"
    "pushl %%ecx\n\t"
    "addl  $4, %%edx\n\t"   
#else 
    "pushl %%ecx\n\t"
    "movl  (%%ecx), %%edx\n\t"
    "movl  %5, %%eax\n\t"   
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
    "leal  (%%edx,%%eax,4), %%edx\n\t"
#else 
    "leal  8(%%edx,%%eax,4), %%edx\n\t"
#endif 
#endif
    "call  *(%%edx)\n\t"    /* safe to not cleanup esp */
    "addl  $4, %%esp\n\t"
    "addl  %8, %%esp"
    : "=a" (result),        
      "=c" (temp1),         
      "=d" (temp2),         
      "=g" (temp3)          
    : "g" (that),           
      "g" (methodIndex),    
      "1" (paramCount),     
      "2" (params),         
      "g" (n),              
      "0" (fn_copy)         
    : "memory"
    );
    
  return result;
#else
#error "can't find a compiler to use"
#endif

}
#endif
