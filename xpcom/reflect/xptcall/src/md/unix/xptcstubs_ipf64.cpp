







































#include "xptcprivate.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>





extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, PRUint32 methodIndex,
  uint64_t* intargs, uint64_t* floatargs, uint64_t* restargs)
{

#define PARAM_BUFFER_COUNT     16

  nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
  nsXPTCMiniVariant* dispatchParams = NULL;
  nsIInterfaceInfo* iface_info = NULL;
  const nsXPTMethodInfo* info;
  nsresult result = NS_ERROR_FAILURE;
  uint64_t* iargs = intargs;
  uint64_t* fargs = floatargs;
  PRUint8 paramCount;
  PRUint8 i;

  NS_ASSERTION(self,"no self");

  self->GetInterfaceInfo(&iface_info);
  NS_ASSERTION(iface_info,"no interface info");

  iface_info->GetMethodInfo(PRUint16(methodIndex), &info);
  NS_ASSERTION(info,"no interface info");

  paramCount = info->GetParamCount();

  
  if(paramCount > PARAM_BUFFER_COUNT)
    dispatchParams = new nsXPTCMiniVariant[paramCount];
  else
    dispatchParams = paramBuffer;
  NS_ASSERTION(dispatchParams,"no place for params");

  for(i = 0; i < paramCount; ++i)
  {
    int isfloat = 0;
    const nsXPTParamInfo& param = info->GetParam(i);
    const nsXPTType& type = param.GetType();
    nsXPTCMiniVariant* dp = &dispatchParams[i];

    if(param.IsOut() || !type.IsArithmetic())
    {
#ifdef __LP64__
        
        dp->val.p = (void*) *iargs;
#else
        
        uint32_t* adr = (uint32_t*) iargs;
        dp->val.p = (void*) (*(adr+1));
#endif
    }
    else
    switch(type)
    {
    case nsXPTType::T_I8     : dp->val.i8  = *(iargs); break;
    case nsXPTType::T_I16    : dp->val.i16 = *(iargs); break;
    case nsXPTType::T_I32    : dp->val.i32 = *(iargs); break;
    case nsXPTType::T_I64    : dp->val.i64 = *(iargs); break;
    case nsXPTType::T_U8     : dp->val.u8  = *(iargs); break;
    case nsXPTType::T_U16    : dp->val.u16 = *(iargs); break;
    case nsXPTType::T_U32    : dp->val.u32 = *(iargs); break;
    case nsXPTType::T_U64    : dp->val.u64 = *(iargs); break;
    case nsXPTType::T_FLOAT  :
      isfloat = 1;
      if (i < 7)
        dp->val.f = (float) *((double*) fargs); 
      else
        dp->val.u32 = *(fargs); 
      break;
    case nsXPTType::T_DOUBLE :
      isfloat = 1;
      dp->val.u64 = *(fargs);
      break;
    case nsXPTType::T_BOOL   : dp->val.b   = *(iargs); break;
    case nsXPTType::T_CHAR   : dp->val.c   = *(iargs); break;
    case nsXPTType::T_WCHAR  : dp->val.wc  = *(iargs); break;
    default:
      NS_ASSERTION(0, "bad type");
      break;
    }
    if (i < 7)
    {
      
      if (i == 6)
      {
        
        iargs = restargs;
        fargs = restargs;
      }
      else
      {
        ++iargs; 
        if (isfloat) ++fargs; 
      }
    }
    else
    {
      
      ++iargs;
      ++fargs;
    }
  }

  result = self->CallMethod((PRUint16) methodIndex, info, dispatchParams);

  NS_RELEASE(iface_info);

  if(dispatchParams != paramBuffer)
    delete [] dispatchParams;

  return result;
}

extern "C" int SharedStub(PRUint64,PRUint64,PRUint64,PRUint64,
 PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64 *);




#define STUB_ENTRY(n) \
nsresult nsXPTCStubBase::Stub##n(PRUint64 a1, \
PRUint64 a2,PRUint64 a3,PRUint64 a4,PRUint64 a5,PRUint64 a6,PRUint64 a7, \
PRUint64 a8) \
{ uint64_t a0 = (uint64_t) this; \
 return SharedStub(a0,a1,a2,a3,a4,a5,a6,a7,(PRUint64) n, &a8); \
}

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ASSERTION(0,"nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

