




































#include "xptcprivate.h"
#include "xptiprivate.h"

#ifndef WIN32
#error "This code is for Win32 only"
#endif

extern "C" {


nsresult __stdcall
PrepareAndDispatch(nsXPTCStubBase* self, PRUint32 methodIndex,
                   PRUint32* args)
{
#define PARAM_BUFFER_COUNT     16

	nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
	nsXPTCMiniVariant* dispatchParams = NULL;
	const nsXPTMethodInfo* info;
	PRUint8 paramCount;
	PRUint8 i;
	nsresult result = NS_ERROR_FAILURE;

	NS_ASSERTION(self,"no self");

  self->mEntry->GetMethodInfo(PRUint16(methodIndex), &info);
  NS_ASSERTION(info,"no method info");

	paramCount = info->GetParamCount();

    
	if(paramCount > PARAM_BUFFER_COUNT)
		dispatchParams = new nsXPTCMiniVariant[paramCount];
	else
		dispatchParams = paramBuffer;
	NS_ASSERTION(dispatchParams,"no place for params");

	PRUint32* ap = args;
	for(i = 0; i < paramCount; i++, ap++)
	{
		const nsXPTParamInfo& param = info->GetParam(i);
		const nsXPTType& type = param.GetType();
		nsXPTCMiniVariant* dp = &dispatchParams[i];

		if(param.IsOut() || !type.IsArithmetic())
		{
			dp->val.p = (void*) *ap;
			continue;
		}
	
		switch(type)
		{
			case nsXPTType::T_I8     : dp->val.i8  = *((PRInt8*)  ap);       break;
			case nsXPTType::T_I16    : dp->val.i16 = *((PRInt16*) ap);       break;
			case nsXPTType::T_I32    : dp->val.i32 = *((PRInt32*) ap);       break;
			case nsXPTType::T_I64    : dp->val.i64 = *((PRInt64*) ap); ap++; break;
			case nsXPTType::T_U8     : dp->val.u8  = *((PRUint8*) ap);       break;
			case nsXPTType::T_U16    : dp->val.u16 = *((PRUint16*)ap);       break;
			case nsXPTType::T_U32    : dp->val.u32 = *((PRUint32*)ap);       break;
			case nsXPTType::T_U64    : dp->val.u64 = *((PRUint64*)ap); ap++; break;
			case nsXPTType::T_FLOAT  : dp->val.f   = *((float*)   ap);       break;
			case nsXPTType::T_DOUBLE : dp->val.d   = *((double*)  ap); ap++; break;
			case nsXPTType::T_BOOL   : dp->val.b   = *((PRBool*)  ap);       break;
			case nsXPTType::T_CHAR   : dp->val.c   = *((char*)    ap);       break;
			case nsXPTType::T_WCHAR  : dp->val.wc  = *((wchar_t*) ap);       break;
			default:
				NS_ERROR("bad type");
				break;
		}
	}

  result = self->mOuter->CallMethod((PRUint16)methodIndex, info, dispatchParams);

	if(dispatchParams != paramBuffer)
		delete [] dispatchParams;

	return result;
}

  
} 




#define STUB_ENTRY(n)

#define SENTINEL_ENTRY(n)                              \
nsresult __stdcall nsXPTCStubBase::Sentinel##n()       \
{                                                      \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED;                   \
}                                                      
#include "xptcstubsdef.inc"

void xptc_dummy()
{
}
