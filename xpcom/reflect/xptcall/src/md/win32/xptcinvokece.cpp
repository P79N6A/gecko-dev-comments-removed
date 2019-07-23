




































#include "xptcprivate.h"

extern "C" nsresult
asmXPTC_InvokeByIndex( nsISupports* that,
                       PRUint32 methodIndex,
  					   PRUint32 paramCount,
					   nsXPTCVariant* params,
					   PRUint32 pfn_CopyToStack,
					   PRUint32 pfn_CountWords);  





extern "C" static void
invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount, nsXPTCVariant* s)
{
  for(; paramCount > 0; paramCount--, d++, s++)
  {
    if(s->IsPtrData())
    {
      *((void**)d) = s->ptr;
      continue;
    }
    switch(s->type)
    {
      case nsXPTType::T_I8     : *((PRInt8*)  d) = s->val.i8;          break;
      case nsXPTType::T_I16    : *((PRInt16*) d) = s->val.i16;         break;
      case nsXPTType::T_I32    : *((PRInt32*) d) = s->val.i32;         break;
      case nsXPTType::T_I64    : *((PRInt64*) d) = s->val.i64; d++;    break;
      case nsXPTType::T_U8     : *((PRUint8*) d) = s->val.u8;          break;
      case nsXPTType::T_U16    : *((PRUint16*)d) = s->val.u16;         break;
      case nsXPTType::T_U32    : *((PRUint32*)d) = s->val.u32;         break;
      case nsXPTType::T_U64    : *((PRUint64*)d) = s->val.u64; d++;    break;
      case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
      case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
      case nsXPTType::T_BOOL   : *((PRBool*)  d) = s->val.b;           break;
      case nsXPTType::T_CHAR   : *((char*)    d) = s->val.c;           break;
      case nsXPTType::T_WCHAR  : *((wchar_t*) d) = s->val.wc;          break;
      default:
        
        *((void**)d) = s->val.p;
        break;
    }
  }
}


extern "C" static uint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
  uint32 nCnt = 0;
  
  for(; paramCount > 0; paramCount--, s++)
  {
    if(s->IsPtrData())
    {
      nCnt++;
      continue;
    }
    switch(s->type)
    {
      case nsXPTType::T_I64    : 
      case nsXPTType::T_U64    : 
      case nsXPTType::T_DOUBLE : 
        nCnt += 2;
        break;
        
      case nsXPTType::T_I8     : 
      case nsXPTType::T_I16    : 
      case nsXPTType::T_I32    : 
      case nsXPTType::T_U8     : 
      case nsXPTType::T_U16    : 
      case nsXPTType::T_U32    : 
      case nsXPTType::T_FLOAT  : 
      case nsXPTType::T_BOOL   : 
      case nsXPTType::T_CHAR   : 
      case nsXPTType::T_WCHAR  : 
      default:
        nCnt++;
        break;
    }
  }
  
  
  
  return nCnt;
}


XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex, PRUint32 paramCount, nsXPTCVariant* params)
{
  return asmXPTC_InvokeByIndex(that,
                               methodIndex,
                               paramCount,
                               params,
                               (PRUint32) &invoke_copy_to_stack,
                               (PRUint32) &invoke_count_words);
}


int g_xptcinvokece_marker;

