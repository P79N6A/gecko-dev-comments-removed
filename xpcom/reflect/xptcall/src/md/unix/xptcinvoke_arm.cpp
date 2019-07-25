






#include "xptcprivate.h"

#if !defined(__arm__) && !(defined(LINUX) || defined(ANDROID))
#error "This code is for Linux ARM only. Check that it works on your system, too.\nBeware that this code is highly compiler dependent."
#endif

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)) \
    && defined(__ARM_EABI__) && !defined(__ARM_PCS_VFP) && !defined(__ARM_PCS)
#error "Can't identify floating point calling conventions.\nPlease ensure that your toolchain defines __ARM_PCS or __ARM_PCS_VFP."
#endif

#ifndef __ARM_PCS_VFP









static uint32_t *
copy_double_word(uint32_t *start, uint32_t *current, uint32_t *end, uint64_t *dw)
{
#ifdef __ARM_EABI__
    
    current = (uint32_t *)(((uint32_t)current + 7) & ~7);
    
    if (current == end) current = start;
#else
    


    if (current == end - 1) {
        *current = ((uint32_t*)dw)[0];
        *start = ((uint32_t*)dw)[1];
        return start;
    }
#endif

    *((uint64_t*) current) = *dw;
    return current + 1;
}



#ifndef DEBUG
static
#endif
void
invoke_copy_to_stack(uint32_t* stk, uint32_t *end,
                     uint32_t paramCount, nsXPTCVariant* s)
{
    





    uint32_t *d = end - 3;
    for(uint32_t i = 0; i < paramCount; i++, d++, s++)
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
        case nsXPTType::T_I8     : *((int32_t*) d) = s->val.i8;          break;
        case nsXPTType::T_I16    : *((int32_t*) d) = s->val.i16;         break;
        case nsXPTType::T_I32    : *((int32_t*) d) = s->val.i32;         break;
        case nsXPTType::T_I64    :
            d = copy_double_word(stk, d, end, (uint64_t *)&s->val.i64);
            break;
        case nsXPTType::T_U8     : *((uint32_t*)d) = s->val.u8;          break;
        case nsXPTType::T_U16    : *((uint32_t*)d) = s->val.u16;         break;
        case nsXPTType::T_U32    : *((uint32_t*)d) = s->val.u32;         break;
        case nsXPTType::T_U64    :
            d = copy_double_word(stk, d, end, (uint64_t *)&s->val.u64);
            break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
        case nsXPTType::T_DOUBLE :
            d = copy_double_word(stk, d, end, (uint64_t *)&s->val.d);
            break;
        case nsXPTType::T_BOOL   : *((int32_t*) d) = s->val.b;           break;
        case nsXPTType::T_CHAR   : *((int32_t*) d) = s->val.c;           break;
        case nsXPTType::T_WCHAR  : *((int32_t*) d) = s->val.wc;          break;
        default:
            
            *((void**)d) = s->val.p;
            break;
        }
    }
}

typedef nsresult (*vtable_func)(nsISupports *, uint32_t, uint32_t, uint32_t);

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                   uint32_t paramCount, nsXPTCVariant* params)
{





















 
  register vtable_func *vtable, func;
  register int base_size = (paramCount > 1) ? paramCount : 2;









  uint32_t *stack_space = (uint32_t *) __builtin_alloca(base_size * 8);

  invoke_copy_to_stack(stack_space, &stack_space[base_size * 2],
                       paramCount, params);

  vtable = *reinterpret_cast<vtable_func **>(that);
  func = vtable[methodIndex];

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


















static inline void copy_word(uint32_t* &ireg_args,
                             uint32_t* &stack_args,
                             uint32_t* end,
                             uint32_t  data)
{
  if (ireg_args < end) {
    *ireg_args = data;
    ireg_args++;
  } else {
    *stack_args = data;
    stack_args++;
  }
}

static inline void copy_dword(uint32_t* &ireg_args,
                              uint32_t* &stack_args,
                              uint32_t* end,
                              uint64_t  data)
{
  if (ireg_args + 1 < end) {
    if ((uint32_t)ireg_args & 4) {
      ireg_args++;
    }
    *(uint64_t *)ireg_args = data;
    ireg_args += 2;
  } else {
    if ((uint32_t)stack_args & 4) {
      stack_args++;
    }
    *(uint64_t *)stack_args = data;
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
invoke_copy_to_stack(uint32_t* stk, uint32_t *end,
                     uint32_t paramCount, nsXPTCVariant* s)
{
  uint32_t *ireg_args  = end - 3;
  float    *vfp_s_args = (float *)end;
  double   *vfp_d_args = (double *)end;
  float    *vfp_end    = vfp_s_args + 16;

  for (uint32_t i = 0; i < paramCount; i++, s++) {
    if (s->IsPtrData()) {
      copy_word(ireg_args, stk, end, (uint32_t)s->ptr);
      continue;
    }
    
    
    switch (s->type)
    {
      case nsXPTType::T_FLOAT:
        if (!copy_vfp_single(vfp_s_args, vfp_d_args, vfp_end, s->val.f)) {
          copy_word(end, stk, end, reinterpret_cast<uint32_t&>(s->val.f));
        }
        break;
      case nsXPTType::T_DOUBLE:
        if (!copy_vfp_double(vfp_s_args, vfp_d_args, vfp_end, s->val.d)) {
          copy_dword(end, stk, end, reinterpret_cast<uint64_t&>(s->val.d));
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
        
        copy_word(ireg_args, stk, end, reinterpret_cast<uint32_t>(s->val.p));
        break;
    }
  }
}

typedef uint32_t (*vtable_func)(nsISupports *, uint32_t, uint32_t, uint32_t);

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                   uint32_t paramCount, nsXPTCVariant* params)
{
  vtable_func *vtable = *reinterpret_cast<vtable_func **>(that);
  vtable_func func = vtable[methodIndex];
  
  
  uint32_t result;
  asm (
    "mov    r3, sp\n"
    "mov    %[stack_space_size], %[param_count_plus_2], lsl #3\n"
    "tst    r3, #4\n" 

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
