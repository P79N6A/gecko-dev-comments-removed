






#include "xptcprivate.h"

#if _HPUX
#error "This code is for HP-PA RISC 32 bit mode only"
#endif

#include <alloca.h>

typedef unsigned nsXPCVariant;

extern "C" int32_t
invoke_count_bytes(nsISupports* that, const uint32_t methodIndex,
  const uint32_t paramCount, const nsXPTCVariant* s)
{
  int32_t result = 4; 

  




  {
    uint32_t indx;
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
    
    int32_t remainder = result & 63;
    return (remainder == 0) ? result : (result + 64 - remainder);
  }
}

extern "C" uint32_t
invoke_copy_to_stack(uint32_t* d,
  const uint32_t paramCount, nsXPTCVariant* s)
{

  typedef struct
  {
    uint32_t hi;
    uint32_t lo;
  } DU;

  uint32_t* dest = d;
  nsXPTCVariant* source = s;
  

  uint32_t floatflags = 0;
  

  uint32_t regwords = 1; 
  uint32_t indx;

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
    case nsXPTType::T_I8    : *((int32_t*) dest) = source->val.i8;  break;
    case nsXPTType::T_I16   : *((int32_t*) dest) = source->val.i16; break;
    case nsXPTType::T_I32   : *((int32_t*) dest) = source->val.i32; break;
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
    case nsXPTType::T_U8    : *((uint32_t*) (dest)) = source->val.u8; break;
    case nsXPTType::T_U16   : *((uint32_t*) (dest)) = source->val.u16; break;
    case nsXPTType::T_U32   : *((uint32_t*) (dest)) = source->val.u32; break;
    case nsXPTType::T_BOOL  : *((uint32_t*) (dest)) = source->val.b; break;
    case nsXPTType::T_CHAR  : *((uint32_t*) (dest)) = source->val.c; break;
    case nsXPTType::T_WCHAR : *((int32_t*)  (dest)) = source->val.wc; break;

    default:
      
      *((void**) dest) = source->val.p;
    }
    ++regwords;
  }
  return floatflags;
}

