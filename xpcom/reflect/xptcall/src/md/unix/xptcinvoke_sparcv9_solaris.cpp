










































#include "xptcprivate.h"

#if !defined(__sparc) && !defined(__sparc__)
#error "This code is for Sparc only"
#endif


extern "C" PRUint64
invoke_copy_to_stack(PRUint64* d, PRUint32 paramCount, nsXPTCVariant* s);

extern "C" PRUint64
invoke_copy_to_stack(PRUint64* d, PRUint32 paramCount, nsXPTCVariant* s)
{
  




  PRUint64 *l_d = d;
  nsXPTCVariant *l_s = s;
  PRUint64 l_paramCount = paramCount;
  PRUint64 regCount = 0;  

  for(PRUint64 i = 0; i < l_paramCount; i++, l_d++, l_s++)
  {
    if (regCount < 5) regCount++;

    if (l_s->IsPtrData())
    {
      *l_d = (PRUint64)l_s->ptr;
      continue;
    }
    switch (l_s->type)
    {
      case nsXPTType::T_I8    : *((PRInt64*)l_d)     = l_s->val.i8;    break;
      case nsXPTType::T_I16   : *((PRInt64*)l_d)     = l_s->val.i16;   break;
      case nsXPTType::T_I32   : *((PRInt64*)l_d)     = l_s->val.i32;   break;
      case nsXPTType::T_I64   : *((PRInt64*)l_d)     = l_s->val.i64;   break;
      
      case nsXPTType::T_U8    : *((PRUint64*)l_d)    = l_s->val.u8;    break;
      case nsXPTType::T_U16   : *((PRUint64*)l_d)    = l_s->val.u16;   break;
      case nsXPTType::T_U32   : *((PRUint64*)l_d)    = l_s->val.u32;   break;
      case nsXPTType::T_U64   : *((PRUint64*)l_d)    = l_s->val.u64;   break;

      




      case nsXPTType::T_FLOAT : *(((float*)l_d) + 1) = l_s->val.f;     break;
      case nsXPTType::T_DOUBLE: *((double*)l_d)      = l_s->val.d;     break;
      case nsXPTType::T_BOOL  : *((PRBool*)l_d)      = l_s->val.b;     break;
      case nsXPTType::T_CHAR  : *((PRUint64*)l_d)    = l_s->val.c;     break;
      case nsXPTType::T_WCHAR : *((PRInt64*)l_d)     = l_s->val.wc;    break;

      default:
        
        *((void**)l_d) = l_s->val.p;
        break;
    }
  }
  
  return regCount;
}
