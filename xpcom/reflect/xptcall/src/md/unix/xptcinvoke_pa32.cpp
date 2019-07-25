






































#include "xptcprivate.h"

#if _HPUX
#error "This code is for HP-PA RISC 32 bit mode only"
#endif

#include <alloca.h>

typedef unsigned nsXPCVariant;

extern "C" PRInt32
invoke_count_bytes(nsISupports* that, const PRUint32 methodIndex,
  const PRUint32 paramCount, const nsXPTCVariant* s)
{
  PRInt32 result = 4; 

  




  {
    PRUint32 indx;
    for (indx = paramCount; indx > 0; --indx, ++s)
    {
      if (! s->IsPtrData())
      {
        if (s->type == nsXPTType::T_I64 || s->type == nsXPTType::T_U64 ||
            s->type == nsXPTType::T_DOUBLE)
        {
          
          result += (result & 4) + 8;
          continue;
        }
      }
      result += 4; 
    }
  }
  result -= 72; 
  if (result < 0)
    return 0;
  {
    
    PRInt32 remainder = result & 63;
    return (remainder == 0) ? result : (result + 64 - remainder);
  }
}

extern "C" PRUint32
invoke_copy_to_stack(PRUint32* d,
  const PRUint32 paramCount, nsXPTCVariant* s)
{

  typedef struct
  {
    PRUint32 hi;
    PRUint32 lo;
  } DU;

  PRUint32* dest = d;
  nsXPTCVariant* source = s;
  

  PRUint32 floatflags = 0;
  

  PRUint32 regwords = 1; 
  PRUint32 indx;

  for (indx = paramCount; indx > 0; --indx, --dest, ++source)
  {
    if (source->IsPtrData())
    {
      *((void**) dest) = source->ptr;
      ++regwords;
      continue;
    }
    switch (source->type)
    {
    case nsXPTType::T_I8    : *((PRInt32*) dest) = source->val.i8;  break;
    case nsXPTType::T_I16   : *((PRInt32*) dest) = source->val.i16; break;
    case nsXPTType::T_I32   : *((PRInt32*) dest) = source->val.i32; break;
    case nsXPTType::T_I64   :
    case nsXPTType::T_U64   :
      if (regwords & 1)
      {
        
        --dest;
        ++regwords;
      }
      *((uint32*) dest) = ((DU *) source)->lo;
      *((uint32*) --dest) = ((DU *) source)->hi;
      
      regwords += 2;
      continue;
    case nsXPTType::T_DOUBLE :
      if (regwords & 1)
      {
        
        --dest;
        ++regwords;
      }
      switch (regwords) 
      {
      case 2:
        floatflags |= 1;
      }
      *((uint32*) dest) = ((DU *) source)->lo;
      *((uint32*) --dest) = ((DU *) source)->hi;
      
      regwords += 2;
      continue;
    case nsXPTType::T_FLOAT :
      switch (regwords) 
      {
      case 1:
        floatflags |= 2;
        break;
      case 2:
        floatflags |= 4;
        break;
      case 3:
        floatflags |= 8;
      }
      *((float*) dest) = source->val.f;
      break;
    case nsXPTType::T_U8    : *((PRUint32*) (dest)) = source->val.u8; break;
    case nsXPTType::T_U16   : *((PRUint32*) (dest)) = source->val.u16; break;
    case nsXPTType::T_U32   : *((PRUint32*) (dest)) = source->val.u32; break;
    case nsXPTType::T_BOOL  : *((PRBool*)   (dest)) = source->val.b; break;
    case nsXPTType::T_CHAR  : *((PRUint32*) (dest)) = source->val.c; break;
    case nsXPTType::T_WCHAR : *((PRInt32*)  (dest)) = source->val.wc; break;

    default:
      
      *((void**) dest) = source->val.p;
    }
    ++regwords;
  }
  return floatflags;
}

