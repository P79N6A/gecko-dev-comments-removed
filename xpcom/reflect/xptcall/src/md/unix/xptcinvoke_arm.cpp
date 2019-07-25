







































#include "xptcprivate.h"

#if !defined(__arm__) && !(defined(LINUX) || defined(ANDROID))
#error "This code is for Linux ARM only. Check that it works on your system, too.\nBeware that this code is highly compiler dependent."
#endif









static PRUint32 *
copy_double_word(PRUint32 *start, PRUint32 *current, PRUint32 *end, PRUint64 *dw)
{
#ifdef __ARM_EABI__
    
    current = (PRUint32 *)(((PRUint32)current + 7) & ~7);
    
    if (current == end) current = start;
#else
    


    if (current == end - 1) {
        *current = ((PRUint32*)dw)[0];
        *start = ((PRUint32*)dw)[1];
        return start;
    }
#endif

    *((PRUint64*) current) = *dw;
    return current + 1;
}



#ifndef DEBUG
static
#endif
void
invoke_copy_to_stack(PRUint32* stk, PRUint32 *end,
                     PRUint32 paramCount, nsXPTCVariant* s)
{
    





    PRUint32 *d = end - 3;
    for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        
        if (d == end) d = stk;
        NS_ASSERTION(d >= stk && d < end,
            "invoke_copy_to_stack is copying outside its given buffer");
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
        case nsXPTType::T_I64    :
            d = copy_double_word(stk, d, end, (PRUint64 *)&s->val.i64);
            break;
        case nsXPTType::T_U8     : *((PRUint32*)d) = s->val.u8;          break;
        case nsXPTType::T_U16    : *((PRUint32*)d) = s->val.u16;         break;
        case nsXPTType::T_U32    : *((PRUint32*)d) = s->val.u32;         break;
        case nsXPTType::T_U64    :
            d = copy_double_word(stk, d, end, (PRUint64 *)&s->val.u64);
            break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
        case nsXPTType::T_DOUBLE :
            d = copy_double_word(stk, d, end, (PRUint64 *)&s->val.d);
            break;
        case nsXPTType::T_BOOL   : *((PRInt32*) d) = s->val.b;           break;
        case nsXPTType::T_CHAR   : *((PRInt32*) d) = s->val.c;           break;
        case nsXPTType::T_WCHAR  : *((PRInt32*) d) = s->val.wc;          break;
        default:
            
            *((void**)d) = s->val.p;
            break;
        }
    }
}

typedef PRUint32 (*vtable_func)(nsISupports *, PRUint32, PRUint32, PRUint32);

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{





















 
  register vtable_func *vtable, func;
  register int base_size = (paramCount > 1) ? paramCount : 2;









  PRUint32 *stack_space = (PRUint32 *) __builtin_alloca(base_size * 8);

  invoke_copy_to_stack(stack_space, &stack_space[base_size * 2],
                       paramCount, params);

  vtable = *reinterpret_cast<vtable_func **>(that);
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
  func = vtable[methodIndex];
#else 
  func = vtable[2 + methodIndex];
#endif

  return func(that, stack_space[base_size * 2 - 3],
                    stack_space[base_size * 2 - 2],
                    stack_space[base_size * 2 - 1]);
}    
