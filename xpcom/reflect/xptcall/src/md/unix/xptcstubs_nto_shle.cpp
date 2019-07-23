#include "xptcprivate.h"

const int c_int_register_params = 3;
const int c_float_register_params = 8;











extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, int methodIndex, PRUint32* data, 
				   PRUint32 *intRegParams, float *floatRegParams)
{
#define PARAM_BUFFER_COUNT     16

	nsresult result = NS_ERROR_FAILURE;
	int intCount = 0;
	int floatCount = 0;
    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = NULL;
    nsIInterfaceInfo* iface_info = NULL;
    const nsXPTMethodInfo* info;
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

	for ( i = 0; i < paramCount; ++i ) {
		const nsXPTParamInfo& param = info->GetParam(i);
		nsXPTCMiniVariant* dp = &dispatchParams[i];
		nsXPTType type = param.IsOut() ? nsXPTType::T_I32 : param.GetType();

		switch ( type ) {
		case nsXPTType::T_I64:
		case nsXPTType::T_U64:
			
			if ( (c_int_register_params - intCount) >= 2 ) {
				dp->val.i64 = *((PRInt64 *) intRegParams);
				intRegParams += 2;
				intCount += 2;
			}
			else {
				dp->val.i64 = *((PRInt64*) data);
				data += 2;
			}
			break;
        case nsXPTType::T_FLOAT:
			
			if ( floatCount < c_float_register_params ) {
				dp->val.f = *floatRegParams;
				++floatCount;
				++floatRegParams;
			}
			else {
				dp->val.f = *((float*) data);
				++data;
			}			
			break;
        case nsXPTType::T_DOUBLE:
			
			if ( (c_float_register_params - floatCount) >= 2  ) {
				if ( floatCount & 1 != 0 ) {
					++floatCount;
					++floatRegParams;
				}
				dp->val.d = *(double *)floatRegParams;
				floatCount += 2;
				floatRegParams += 2;
			}
			else {
				dp->val.d = *((double *) data);
				data += 2;
			}			
			break;
		default:		
			
			if ( intCount < c_int_register_params ) {
				dp->val.i32 = *intRegParams;
				++intRegParams;
				++intCount;
			}
			else {
				dp->val.i32 = *data;
				++data;
			}
			break;
        }
	}

	result = self->CallMethod((PRUint16)methodIndex, info, dispatchParams);

	NS_RELEASE(iface_info);

	if(dispatchParams != paramBuffer)
		delete [] dispatchParams;

	return result;
}

#ifdef __GNUC__			





#define STUB_ENTRY(n)

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
	NS_ERROR("nsXPTCStubBase::Sentinel called"); \
	return NS_ERROR_NOT_IMPLEMENTED; \
}

#else
#error "can't find a compiler to use"
#endif 

#include "xptcstubsdef.inc"
