








#include "xptcprivate.h"

#if !defined(__sparc) && !defined(__sparc__)
#error "This code is for Sparc only"
#endif


extern "C" uint64_t
invoke_copy_to_stack(uint64_t* d, uint32_t paramCount, nsXPTCVariant* s);

extern "C" uint64_t
invoke_copy_to_stack(uint64_t* d, uint32_t paramCount, nsXPTCVariant* s)
{
  




  uint64_t *l_d = d;
  nsXPTCVariant *l_s = s;
  uint64_t l_paramCount = paramCount;
  uint64_t regCount = 0;  

  for(uint64_t i = 0; i < l_paramCount; i++, l_d++, l_s++)
  {
    if (regCount < 5) regCount++;

    if (l_s->IsPtrData())
    {
      *l_d = (uint64_t)l_s->ptr;
      continue;
    }
    switch (l_s->type)
    {
      case nsXPTType::T_I8    : *((int64_t*)l_d)     = l_s->val.i8;    break;
      case nsXPTType::T_I16   : *((int64_t*)l_d)     = l_s->val.i16;   break;
      case nsXPTType::T_I32   : *((int64_t*)l_d)     = l_s->val.i32;   break;
      case nsXPTType::T_I64   : *((int64_t*)l_d)     = l_s->val.i64;   break;
      
      case nsXPTType::T_U8    : *((uint64_t*)l_d)    = l_s->val.u8;    break;
      case nsXPTType::T_U16   : *((uint64_t*)l_d)    = l_s->val.u16;   break;
      case nsXPTType::T_U32   : *((uint64_t*)l_d)    = l_s->val.u32;   break;
      case nsXPTType::T_U64   : *((uint64_t*)l_d)    = l_s->val.u64;   break;

      




      case nsXPTType::T_FLOAT : *(((float*)l_d) + 1) = l_s->val.f;     break;
      case nsXPTType::T_DOUBLE: *((double*)l_d)      = l_s->val.d;     break;
      case nsXPTType::T_BOOL  : *((uint64_t*)l_d)    = l_s->val.b;     break;
      case nsXPTType::T_CHAR  : *((uint64_t*)l_d)    = l_s->val.c;     break;
      case nsXPTType::T_WCHAR : *((int64_t*)l_d)     = l_s->val.wc;    break;

      default:
        
        *((void**)l_d) = l_s->val.p;
        break;
    }
  }
  
  return regCount;
}
