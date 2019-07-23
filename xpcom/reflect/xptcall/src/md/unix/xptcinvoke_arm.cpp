






































#include "prlog.h"

#include "xptcprivate.h"

#if !defined(LINUX) || !defined(__arm__)
#error "This code is for Linux ARM only. Check that it works on your system, too.\nBeware that this code is highly compiler dependent."
#endif






#ifdef __ARM_EABI__
#define DOUBLEWORD_ALIGN(p) ((PRUint32 *)((((PRUint32)(p)) + 7) & 0xfffffff8))
#define VAR_STACK_SIZE_64 3
#else
#define DOUBLEWORD_ALIGN(p) (p)
#define VAR_STACK_SIZE_64 2
#endif



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
            result+=VAR_STACK_SIZE_64;
            break;
        case nsXPTType::T_U8     :
        case nsXPTType::T_U16    :
        case nsXPTType::T_U32    :
            result++;
            break;
        case nsXPTType::T_U64    :
            result+=VAR_STACK_SIZE_64;
            break;
        case nsXPTType::T_FLOAT  :
            result++;
            break;
        case nsXPTType::T_DOUBLE :
            result+=VAR_STACK_SIZE_64;
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

#ifdef __ARM_EABI__
    


    if (result % 2 == 0)
        result++;
#endif

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
        case nsXPTType::T_I8     : *((PRInt32*) d) = s->val.i8;          break;
        case nsXPTType::T_I16    : *((PRInt32*) d) = s->val.i16;         break;
        case nsXPTType::T_I32    : *((PRInt32*) d) = s->val.i32;         break;
        case nsXPTType::T_I64    : d = DOUBLEWORD_ALIGN(d);
                                   *((PRInt64*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U8     : *((PRUint32*)d) = s->val.u8;          break;
        case nsXPTType::T_U16    : *((PRUint32*)d) = s->val.u16;         break;
        case nsXPTType::T_U32    : *((PRUint32*)d) = s->val.u32;         break;
        case nsXPTType::T_U64    : d = DOUBLEWORD_ALIGN(d);
                                   *((PRUint64*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
        case nsXPTType::T_DOUBLE : d = DOUBLEWORD_ALIGN(d);
                                   *((double*)  d) = s->val.d;   d++;    break;
        case nsXPTType::T_BOOL   : *((PRInt32*) d) = s->val.b;           break;
        case nsXPTType::T_CHAR   : *((PRInt32*) d) = s->val.c;           break;
        case nsXPTType::T_WCHAR  : *((PRInt32*) d) = s->val.wc;          break;
        default:
            
            *((void**)d) = s->val.p;
            break;
        }
    }
}

extern "C" {
    struct my_params_struct {
        nsISupports* that;      
        PRUint32 Index;         
        PRUint32 Count;         
        nsXPTCVariant* params;  
        PRUint32 fn_count;     
        PRUint32 fn_copy;      
    };
}

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    PRUint32 result;
    struct my_params_struct my_params;
    my_params.that = that;
    my_params.Index = methodIndex;
    my_params.Count = paramCount;
    my_params.params = params;
    my_params.fn_copy = (PRUint32) &invoke_copy_to_stack;
    my_params.fn_count = (PRUint32) &invoke_count_words;

























 
  __asm__ __volatile__(
    "ldr	r1, [%1, #12]	\n\t"	
    "ldr	ip, [%1, #16]	\n\t"	
    "ldr	r0, [%1,  #8]	\n\t"
    "mov	lr, pc		\n\t"	
    "mov	pc, ip		\n\t"
    "mov	r4, r0, lsl #2	\n\t"	
    "sub	sp, sp, r4	\n\t"	
    "mov	r0, sp		\n\t"	
    "ldr	r1, [%1,  #8]	\n\t"	
    "ldr	r2, [%1, #12]	\n\t"	
    "ldr	ip, [%1, #20]	\n\t"	
    "mov	lr, pc		\n\t"	
    "mov	pc, ip		\n\t"	
    "ldr	r0, [%1]	\n\t"	
    "ldr	r1, [r0, #0]	\n\t"	
    "ldr	r2, [%1, #4]	\n\t"
    "mov	r2, r2, lsl #2	\n\t"	
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
    "ldr        ip, [r1, r2]    \n\t"   
#else 
    "add	r2, r2, #8	\n\t"	
    "ldr	ip, [r1, r2]	\n\t"	
#endif
    "cmp	r4, #12		\n\t"	
    "ldmgtia	sp!, {r1, r2, r3}\n\t"	
    "subgt	r4, r4, #12	\n\t"	
    "ldmleia	sp, {r1, r2, r3}\n\t"	 
    "addle	sp, sp, r4	\n\t"	
    "movle	r4, #0		\n\t"	
    "ldr	r0, [%1, #0]	\n\t"	
    "mov	lr, pc		\n\t"	
    "mov	pc, ip		\n\t"
    "add	sp, sp, r4	\n\t"	
    "mov	%0, r0		\n\t"	
    : "=r" (result)
    : "r" (&my_params), "m" (my_params)
    : "r0", "r1", "r2", "r3", "r4", "ip", "lr", "sp"
    );
    
  return result;
}    
