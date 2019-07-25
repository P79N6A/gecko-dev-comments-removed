








































#include "xptcprivate.h"

#if !defined(__arm__) && !(defined(LINUX) || defined(ANDROID))
#error "This code is for Linux ARM only. Check that it works on your system, too.\nBeware that this code is highly compiler dependent."
#endif

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)) \
    && defined(__ARM_EABI__) && !defined(__ARM_PCS_VFP) && !defined(__ARM_PCS)
#error "Can't identify floating point calling conventions.\nPlease ensure that your toolchain defines __ARM_PCS or __ARM_PCS_VFP."
#endif

#ifndef __ARM_PCS_VFP









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

#else 








#if defined(__thumb__) && !defined(__thumb2__)
#error "Thumb1 is not supported"
#endif

#ifndef __ARMEL__
#error "Only little endian compatibility was tested"
#endif


















static inline void copy_word(PRUint32* &ireg_args,
                             PRUint32* &stack_args,
                             PRUint32* end,
                             PRUint32  data)
{
  if (ireg_args < end) {
    *ireg_args = data;
    ireg_args++;
  } else {
    *stack_args = data;
    stack_args++;
  }
}

static inline void copy_dword(PRUint32* &ireg_args,
                              PRUint32* &stack_args,
                              PRUint32* end,
                              PRUint64  data)
{
  if (ireg_args + 1 < end) {
    if ((PRUint32)ireg_args & 4) {
      ireg_args++;
    }
    *(PRUint64 *)ireg_args = data;
    ireg_args += 2;
  } else {
    if ((PRUint32)stack_args & 4) {
      stack_args++;
    }
    *(PRUint64 *)stack_args = data;
    stack_args += 2;
  }
}




























static inline bool copy_vfp_single(float* &vfp_s_args, double* &vfp_d_args,
                                   float* end, float data)
{
  if (vfp_s_args >= end)
    return false;

  *vfp_s_args = data;
  vfp_s_args++;
  if (vfp_s_args < (float *)vfp_d_args) {
    
    
    vfp_s_args = (float *)vfp_d_args;
  } else if (vfp_s_args > (float *)vfp_d_args) {
    
    vfp_d_args++;
  }
  return true;
}

static inline bool copy_vfp_double(float* &vfp_s_args, double* &vfp_d_args,
                                   float* end, double data)
{
  if (vfp_d_args >= (double *)end) {
    
    
    
    vfp_s_args = end;
    return false;
  }

  if (vfp_s_args == (float *)vfp_d_args) {
    
    vfp_s_args += 2;
  }
  *vfp_d_args = data;
  vfp_d_args++;
  return true;
}

static void
invoke_copy_to_stack(PRUint32* stk, PRUint32 *end,
                     PRUint32 paramCount, nsXPTCVariant* s)
{
  PRUint32 *ireg_args  = end - 3;
  float    *vfp_s_args = (float *)end;
  double   *vfp_d_args = (double *)end;
  float    *vfp_end    = vfp_s_args + 16;

  for (PRUint32 i = 0; i < paramCount; i++, s++) {
    if (s->IsPtrData()) {
      copy_word(ireg_args, stk, end, (PRUint32)s->ptr);
      continue;
    }
    
    
    switch (s->type)
    {
      case nsXPTType::T_FLOAT:
        if (!copy_vfp_single(vfp_s_args, vfp_d_args, vfp_end, s->val.f)) {
          copy_word(end, stk, end, reinterpret_cast<PRUint32&>(s->val.f));
        }
        break;
      case nsXPTType::T_DOUBLE:
        if (!copy_vfp_double(vfp_s_args, vfp_d_args, vfp_end, s->val.d)) {
          copy_dword(end, stk, end, reinterpret_cast<PRUint64&>(s->val.d));
        }
        break;
      case nsXPTType::T_I8:  copy_word(ireg_args, stk, end, s->val.i8);   break;
      case nsXPTType::T_I16: copy_word(ireg_args, stk, end, s->val.i16);  break;
      case nsXPTType::T_I32: copy_word(ireg_args, stk, end, s->val.i32);  break;
      case nsXPTType::T_I64: copy_dword(ireg_args, stk, end, s->val.i64); break;
      case nsXPTType::T_U8:  copy_word(ireg_args, stk, end, s->val.u8);   break;
      case nsXPTType::T_U16: copy_word(ireg_args, stk, end, s->val.u16);  break;
      case nsXPTType::T_U32: copy_word(ireg_args, stk, end, s->val.u32);  break;
      case nsXPTType::T_U64: copy_dword(ireg_args, stk, end, s->val.u64); break;
      case nsXPTType::T_BOOL: copy_word(ireg_args, stk, end, s->val.b);   break;
      case nsXPTType::T_CHAR: copy_word(ireg_args, stk, end, s->val.c);   break;
      case nsXPTType::T_WCHAR: copy_word(ireg_args, stk, end, s->val.wc); break;
      default:
        
        copy_word(ireg_args, stk, end, reinterpret_cast<PRUint32>(s->val.p));
        break;
    }
  }
}

typedef PRUint32 (*vtable_func)(nsISupports *, PRUint32, PRUint32, PRUint32);

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
  vtable_func *vtable = *reinterpret_cast<vtable_func **>(that);
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
  vtable_func func = vtable[methodIndex];
#else 
  vtable_func func = vtable[2 + methodIndex];
#endif
  
  
  PRUint32 result;
  asm (
    "mov    %[stack_space_size], %[param_count_plus_2], lsl #3\n"
    "tst    sp, #4\n" 

    "add    %[stack_space_size], #(4 * 16)\n" 
    "mov    r3, %[params]\n"

    "it     ne\n"
    "addne  %[stack_space_size], %[stack_space_size], #4\n"
    "sub    r0, sp, %[stack_space_size]\n" 

    "sub    r2, %[param_count_plus_2], #2\n"
    "mov    sp, r0\n"

    "add    r1, r0, %[param_count_plus_2], lsl #3\n"
    "blx    %[invoke_copy_to_stack]\n"

    "add    ip, sp, %[param_count_plus_2], lsl #3\n"
    "mov    r0, %[that]\n"
    "ldmdb  ip, {r1, r2, r3}\n"
    "vldm   ip, {d0, d1, d2, d3, d4, d5, d6, d7}\n"
    "blx    %[func]\n"

    "add    sp, sp, %[stack_space_size]\n" 
    "mov    %[stack_space_size], r0\n" 
    : [stack_space_size]     "=&r" (result)
    : [func]                 "r"   (func),
      [that]                 "r"   (that),
      [params]               "r"   (params),
      [param_count_plus_2]   "r"   (paramCount + 2),
      [invoke_copy_to_stack] "r"   (invoke_copy_to_stack)
    : "cc", "memory",
      
      
      "r0", "r1", "r2", "r3", "ip", "lr",
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      
      
      
      
      
      
      
      
      
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
  );
  return result;
}

#endif
