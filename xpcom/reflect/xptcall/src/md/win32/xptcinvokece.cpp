




































#include "prlog.h"

#include "xptcprivate.h"

extern "C" PRUint32 *
writeArgs(PRUint32 *d,
          PRUint32  paramCount,
          nsXPTCVariant *params)
{
  
  
  
  
  
  
  
  
  
  
  
  
  d--;
  for (nsXPTCVariant * s = &params[paramCount-1]; paramCount != 0; paramCount--, s--, d--) {
    
    
    if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }
        switch(s->type)
        {
        case nsXPTType::T_I8     :  *((PRInt32*)   d) = s->val.i8;     break;
        case nsXPTType::T_I16    :  *((PRInt32*)   d) = s->val.i16;    break;
        case nsXPTType::T_I32    :  *((PRInt32*)   d) = s->val.i32;    break;
        case nsXPTType::T_I64    :  *((PRInt64*) --d) = s->val.i64;    break;
        case nsXPTType::T_U8     :  *((PRUint32*)  d) = s->val.u8;     break;
        case nsXPTType::T_U16    :  *((PRUint32*)  d) = s->val.u16;    break;
        case nsXPTType::T_U32    :  *((PRUint32*)  d) = s->val.u32;    break;
        case nsXPTType::T_U64    :  *((PRUint64*)--d) = s->val.u64;    break;
        case nsXPTType::T_FLOAT  :  *((float*)     d) = s->val.f;      break;
        case nsXPTType::T_DOUBLE :  *((double*)  --d) = s->val.d;      break;
        case nsXPTType::T_BOOL   :  *((PRInt32*)   d) = s->val.b;      break;
        case nsXPTType::T_CHAR   :  *((PRInt32*)   d) = s->val.c;      break;
        case nsXPTType::T_WCHAR  :  *((PRInt32*)   d) = s->val.wc;     break;
        default:
            
            *((void**)d) = s->val.p;
            break;
        }

  }
  return ++d;
}

extern "C" nsresult
asmXPTC_InvokeByIndex(nsISupports* that,
		      PRUint32 methodIndex,
		      PRUint32 paramCount,
		      nsXPTCVariant* params);  

extern "C" NS_EXPORT  nsresult NS_FROZENCALL
NS_InvokeByIndex_P(nsISupports* that, PRUint32 methodIndex,
		   PRUint32 paramCount, nsXPTCVariant* params)
{
  return asmXPTC_InvokeByIndex(that, methodIndex, paramCount, params);
}


int g_xptcinvokece_marker;

