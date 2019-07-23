







































#include "xptcprivate.h"
#include "xptc_platforms_unixish_x86.h"

extern "C" {

static void
invoke_copy_to_stack(PRUint32 paramCount, nsXPTCVariant* s, PRUint32* d)
{
    for(PRUint32 i = paramCount; i >0; i--, d++, s++)
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

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                 PRUint32 paramCount, nsXPTCVariant* params)
{
#ifdef __GNUC__            
  PRUint32 result;
  


  PRUint32 n = paramCount << 3;
  void (*fn_copy) (unsigned int, nsXPTCVariant *, PRUint32 *) = invoke_copy_to_stack;
  int temp1, temp2;
 
  



  unsigned int saved_esp;
  
 __asm__ __volatile__(
#ifdef KEEP_STACK_16_BYTE_ALIGNED
    "movl  %%esp, %3\n\t"
#endif
    "subl  %8, %%esp\n\t" 
#ifdef KEEP_STACK_16_BYTE_ALIGNED
    


    "subl  $4, %%esp\n\t"
    "andl  $0xfffffff0, %%esp\n\t"
    

    "subl  $4, %%esp\n\t"
    




    "leal  8(%%esp), %8\n\t"
    "pushl %8\n\t"
#else
    "pushl %%esp\n\t"
#endif
    "pushl %7\n\t"
    "pushl %6\n\t"
    "call  *%9\n\t"       
#ifdef KEEP_STACK_16_BYTE_ALIGNED
    


    "addl  $0x14, %%esp\n\t"
#else
    "addl  $0xc, %%esp\n\t"
#endif
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
    "call  *(%%edx)\n\t"    
#ifdef KEEP_STACK_16_BYTE_ALIGNED
    "movl  %3, %%esp\n\t"
#else
    "addl  $4, %%esp\n\t"
    "addl  %8, %%esp"
#endif
    : "=a" (result),        
      "=c" (temp1),         
      "=d" (temp2),         
#ifdef KEEP_STACK_16_BYTE_ALIGNED
      "=&g" (saved_esp)     
#else
      
      "=m" (saved_esp)      
#endif
    : "g" (that),           
      "g" (methodIndex),    
      "1" (paramCount),     
      "2" (params),         
#ifdef KEEP_STACK_16_BYTE_ALIGNED
      
      "r" (n),              
#else
      "g" (n),              
#endif
      "0" (fn_copy)         
    : "memory"
    );
    
  return result;

#else
#error "can't find a compiler to use"
#endif 

}    
