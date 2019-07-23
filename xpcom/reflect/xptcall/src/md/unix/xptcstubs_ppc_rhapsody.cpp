





































#include "xptcprivate.h"
#include "xptiprivate.h"






























extern "C" nsresult
PrepareAndDispatch(
  nsXPTCStubBase *self,
  PRUint32        methodIndex,
  PRUint32       *argsStack,
  PRUint32       *argsGPR,
  double         *argsFPR) {
#define PARAM_BUFFER_COUNT 16
#define PARAM_FPR_COUNT    13
#define PARAM_GPR_COUNT     7

  nsXPTCMiniVariant      paramBuffer[PARAM_BUFFER_COUNT];
  nsXPTCMiniVariant     *dispatchParams = NULL;
  const nsXPTMethodInfo *methodInfo;
  PRUint8                paramCount;
  PRUint8                i;
  nsresult               result         = NS_ERROR_FAILURE;
  PRUint32               argIndex       = 0;
  PRUint32               fprIndex       = 0;

  typedef struct {
    PRUint32 hi;
    PRUint32 lo;
  } DU;

  NS_ASSERTION(self, "no self");

  self->mEntry->GetMethodInfo(PRUint16(methodIndex), &methodInfo);
  NS_ASSERTION(methodInfo, "no method info");

  paramCount = methodInfo->GetParamCount();

  if(paramCount > PARAM_BUFFER_COUNT) {
    dispatchParams = new nsXPTCMiniVariant[paramCount];
  }
  else {
    dispatchParams = paramBuffer;
  }
  NS_ASSERTION(dispatchParams,"no place for params");

  for(i = 0; i < paramCount; i++, argIndex++) {
    const nsXPTParamInfo &param = methodInfo->GetParam(i);
    const nsXPTType      &type  = param.GetType();
    nsXPTCMiniVariant    *dp    = &dispatchParams[i];
    PRUint32              theParam;

    if(argIndex < PARAM_GPR_COUNT)
      theParam =   argsGPR[argIndex];
    else
      theParam = argsStack[argIndex];

    if(param.IsOut() || !type.IsArithmetic())
      dp->val.p = (void *) theParam;
    else {
      switch(type) {
        case nsXPTType::T_I8:
          dp->val.i8  =   (PRInt8) theParam;
          break;
        case nsXPTType::T_I16:
          dp->val.i16 =  (PRInt16) theParam;
          break;
        case nsXPTType::T_I32:
          dp->val.i32 =  (PRInt32) theParam;
          break;
        case nsXPTType::T_U8:
          dp->val.u8  =  (PRUint8) theParam;
          break;
        case nsXPTType::T_U16:
          dp->val.u16 = (PRUint16) theParam;
          break;
        case nsXPTType::T_U32:
          dp->val.u32 = (PRUint32) theParam;
          break;
        case nsXPTType::T_I64:
        case nsXPTType::T_U64:
          ((DU *)dp)->hi = (PRUint32) theParam;
          if(++argIndex < PARAM_GPR_COUNT)
            ((DU *)dp)->lo = (PRUint32)   argsGPR[argIndex];
          else
            ((DU *)dp)->lo = (PRUint32) argsStack[argIndex];
          break;
        case nsXPTType::T_BOOL:
          dp->val.b   =   (PRBool) theParam;
          break;
        case nsXPTType::T_CHAR:
          dp->val.c   =     (char) theParam;
          break;
        case nsXPTType::T_WCHAR:
          dp->val.wc  =  (wchar_t) theParam;
          break;
        case nsXPTType::T_FLOAT:
          if(fprIndex < PARAM_FPR_COUNT)
            dp->val.f = (float) argsFPR[fprIndex++];
          else
            dp->val.f = *(float *) &argsStack[argIndex];
          break;
        case nsXPTType::T_DOUBLE:
          if(fprIndex < PARAM_FPR_COUNT)
            dp->val.d = argsFPR[fprIndex++];
          else
            dp->val.d = *(double *) &argsStack[argIndex];
          argIndex++;
          break;
        default:
          NS_ERROR("bad type");
          break;
      }
    }
  }

  result = self->mOuter->
    CallMethod((PRUint16)methodIndex, methodInfo, dispatchParams);

  if(dispatchParams != paramBuffer)
    delete [] dispatchParams;

  return result;
}

#define STUB_ENTRY(n)
#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"
