






































#include "nsNativeMethod.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIXPConnect.h"
#include "nsCRT.h"





template<class IntegerType>
static bool
jsvalToIntStrict(jsval aValue, IntegerType *aResult)
{
  if (JSVAL_IS_INT(aValue)) {
    jsint i = JSVAL_TO_INT(aValue);
    *aResult = i;

    
    return jsint(*aResult) == i;
  }
  if (JSVAL_IS_DOUBLE(aValue)) {
    jsdouble d = *JSVAL_TO_DOUBLE(aValue);
    *aResult = d;

    
    
    return jsdouble(*aResult) == d;
  }
  if (JSVAL_IS_BOOLEAN(aValue)) {
    
    *aResult = JSVAL_TO_BOOLEAN(aValue);
    NS_ASSERTION(*aResult == 0 || *aResult == 1, "invalid boolean");
    return true;
  }
  
  return false;
}

static bool
jsvalToDoubleStrict(jsval aValue, jsdouble *dp)
{
  
  
  if (JSVAL_IS_INT(aValue)) {
    *dp = JSVAL_TO_INT(aValue);
    return true;
  }
  if (JSVAL_IS_DOUBLE(aValue)) {
    *dp = *JSVAL_TO_DOUBLE(aValue);
    return true;
  }
  return false;
}

static nsresult
TypeError(JSContext *cx, const char *message)
{
  JS_ReportError(cx, message);
  return NS_ERROR_FAILURE;
}

static nsresult
GetABI(PRUint16 aCallType, ffi_abi& aResult)
{
  
  
  
  switch (aCallType) {
  case nsINativeTypes::DEFAULT:
    aResult = FFI_DEFAULT_ABI;
    return NS_OK;
#if defined(XP_WIN32)
  case nsINativeTypes::STDCALL:
    aResult = FFI_STDCALL;
    return NS_OK;
#endif
  default:
    return NS_ERROR_INVALID_ARG;
  }
}

static nsresult
PrepareType(JSContext* aContext, jsval aType, nsNativeType& aResult)
{
  
  if (!JSVAL_IS_INT(aType)) {
    JS_ReportError(aContext, "Invalid type specification");
    return NS_ERROR_FAILURE;
  }

  PRInt32 type = JSVAL_TO_INT(aType);

  switch (type) {
  case nsINativeTypes::VOID:
    aResult.mType = ffi_type_void;
    break;
  case nsINativeTypes::INT8:
    aResult.mType = ffi_type_sint8;
    break;
  case nsINativeTypes::INT16:
    aResult.mType = ffi_type_sint16;
    break;
  case nsINativeTypes::INT32:
    aResult.mType = ffi_type_sint32;
    break;
  case nsINativeTypes::INT64:
    aResult.mType = ffi_type_sint64;
    break;
  case nsINativeTypes::BOOL:
  case nsINativeTypes::UINT8:
    aResult.mType = ffi_type_uint8;
    break;
  case nsINativeTypes::UINT16:
    aResult.mType = ffi_type_uint16;
    break;
  case nsINativeTypes::UINT32:
    aResult.mType = ffi_type_uint32;
    break;
  case nsINativeTypes::UINT64:
    aResult.mType = ffi_type_uint64;
    break;
  case nsINativeTypes::FLOAT:
    aResult.mType = ffi_type_float;
    break;
  case nsINativeTypes::DOUBLE:
    aResult.mType = ffi_type_double;
    break;
  case nsINativeTypes::STRING:
  case nsINativeTypes::USTRING:
    aResult.mType = ffi_type_pointer;
    break;
  default:
    JS_ReportError(aContext, "Invalid type specification");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  aResult.mNativeType = type;

  return NS_OK;
}

static nsresult
PrepareValue(JSContext* aContext, const nsNativeType& aType, jsval aValue, nsNativeValue& aResult)
{
  jsdouble d;

  switch (aType.mNativeType) {
  case nsINativeTypes::BOOL:
    
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint8) ||
        aResult.mValue.mUint8 > 1)
      return TypeError(aContext, "Expected boolean value");

    aResult.mData = &aResult.mValue.mUint8;
    break;
  case nsINativeTypes::INT8:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt8))
      return TypeError(aContext, "Expected int8 value");

    aResult.mData = &aResult.mValue.mInt8;
    break;
  case nsINativeTypes::INT16:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt16))
      return TypeError(aContext, "Expected int16 value");

    aResult.mData = &aResult.mValue.mInt16;
    break;
  case nsINativeTypes::INT32:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt32))
      return TypeError(aContext, "Expected int32 value");

    aResult.mData = &aResult.mValue.mInt32;
  case nsINativeTypes::INT64:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt64))
      return TypeError(aContext, "Expected int64 value");

    aResult.mData = &aResult.mValue.mInt64;
    break;
  case nsINativeTypes::UINT8:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint8))
      return TypeError(aContext, "Expected uint8 value");

    aResult.mData = &aResult.mValue.mUint8;
    break;
  case nsINativeTypes::UINT16:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint16))
      return TypeError(aContext, "Expected uint16 value");

    aResult.mData = &aResult.mValue.mUint16;
    break;
  case nsINativeTypes::UINT32:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint32))
      return TypeError(aContext, "Expected uint32 value");

    aResult.mData = &aResult.mValue.mUint32;
  case nsINativeTypes::UINT64:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint64))
      return TypeError(aContext, "Expected uint64 value");

    aResult.mData = &aResult.mValue.mUint64;
    break;
  case nsINativeTypes::FLOAT:
    if (!jsvalToDoubleStrict(aValue, &d))
      return TypeError(aContext, "Expected number");

    
    
    
    
    aResult.mValue.mFloat = d;
    aResult.mData = &aResult.mValue.mFloat;
    break;
  case nsINativeTypes::DOUBLE:
    if (!jsvalToDoubleStrict(aValue, &d))
      return TypeError(aContext, "Expected number");

    aResult.mValue.mDouble = d;
    aResult.mData = &aResult.mValue.mDouble;
    break;
  case nsINativeTypes::STRING:
    if (JSVAL_IS_NULL(aValue)) {
      
      aResult.mValue.mPointer = nsnull;
    } else if (JSVAL_IS_STRING(aValue)) {
      aResult.mValue.mPointer = JS_GetStringBytes(JSVAL_TO_STRING(aValue));
    } else {
      
      
      return TypeError(aContext, "Expected string");
    }

    aResult.mData = &aResult.mValue.mPointer;
    break;
  case nsINativeTypes::USTRING:
    if (JSVAL_IS_NULL(aValue)) {
      
      aResult.mValue.mPointer = nsnull;
    } else if (JSVAL_IS_STRING(aValue)) {
      aResult.mValue.mPointer = JS_GetStringChars(JSVAL_TO_STRING(aValue));
    } else {
      
      
      return TypeError(aContext, "Expected string");
    }

    aResult.mData = &aResult.mValue.mPointer;
    break;
  default:
    NS_NOTREACHED("invalid type");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

static void
PrepareReturnValue(const nsNativeType& aType, nsNativeValue& aResult)
{
  switch (aType.mNativeType) {
  case nsINativeTypes::VOID:
    aResult.mData = nsnull;
    break;
  case nsINativeTypes::INT8:
    aResult.mData = &aResult.mValue.mInt8;
    break;
  case nsINativeTypes::INT16:
    aResult.mData = &aResult.mValue.mInt16;
    break;
  case nsINativeTypes::INT32:
    aResult.mData = &aResult.mValue.mInt32;
    break;
  case nsINativeTypes::INT64:
    aResult.mData = &aResult.mValue.mInt64;
    break;
  case nsINativeTypes::BOOL:
  case nsINativeTypes::UINT8:
    aResult.mData = &aResult.mValue.mUint8;
    break;
  case nsINativeTypes::UINT16:
    aResult.mData = &aResult.mValue.mUint16;
    break;
  case nsINativeTypes::UINT32:
    aResult.mData = &aResult.mValue.mUint32;
    break;
  case nsINativeTypes::UINT64:
    aResult.mData = &aResult.mValue.mUint64;
    break;
  case nsINativeTypes::FLOAT:
    aResult.mData = &aResult.mValue.mFloat;
    break;
  case nsINativeTypes::DOUBLE:
    aResult.mData = &aResult.mValue.mDouble;
    break;
  case nsINativeTypes::STRING:
  case nsINativeTypes::USTRING:
    aResult.mData = &aResult.mValue.mPointer;
    break;
  default:
    NS_NOTREACHED("invalid type");
    break;
  }
}

static nsresult
ConvertReturnValue(JSContext* aContext,
                   const nsNativeType& aResultType,
                   const nsNativeValue& aResultValue,
                   jsval* aValue)
{
  switch (aResultType.mNativeType) {
  case nsINativeTypes::VOID:
    *aValue = JSVAL_VOID;
    break;
  case nsINativeTypes::BOOL:
    *aValue = aResultValue.mValue.mUint8 ? JSVAL_TRUE : JSVAL_FALSE;
    break;
  case nsINativeTypes::INT8:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mInt8);
    break;
  case nsINativeTypes::INT16:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mInt16);
    break;
  case nsINativeTypes::INT32:
    if (!JS_NewNumberValue(aContext, aResultValue.mValue.mInt32, aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsINativeTypes::INT64:
    
    if (!JS_NewNumberValue(aContext, aResultValue.mValue.mInt64, aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsINativeTypes::UINT8:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mUint8);
    break;
  case nsINativeTypes::UINT16:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mUint16);
    break;
  case nsINativeTypes::UINT32:
    if (!JS_NewNumberValue(aContext, aResultValue.mValue.mUint32, aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsINativeTypes::UINT64:
    
    if (!JS_NewNumberValue(aContext, aResultValue.mValue.mUint64, aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsINativeTypes::FLOAT:
    if (!JS_NewNumberValue(aContext, aResultValue.mValue.mFloat, aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsINativeTypes::DOUBLE:
    if (!JS_NewNumberValue(aContext, aResultValue.mValue.mDouble, aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsINativeTypes::STRING: {
    if (!aResultValue.mValue.mPointer) {
      
      *aValue = JSVAL_VOID;
    } else {
      JSString *jsstring = JS_NewStringCopyZ(aContext,
                             reinterpret_cast<const char*>(aResultValue.mValue.mPointer));
      if (!jsstring)
        return NS_ERROR_OUT_OF_MEMORY;

      *aValue = STRING_TO_JSVAL(jsstring);
    }
    break;
  }
  case nsINativeTypes::USTRING: {
    if (!aResultValue.mValue.mPointer) {
      
      *aValue = JSVAL_VOID;
    } else {
      JSString *jsstring = JS_NewUCStringCopyZ(aContext,
                             reinterpret_cast<const jschar*>(aResultValue.mValue.mPointer));
      if (!jsstring)
        return NS_ERROR_OUT_OF_MEMORY;

      *aValue = STRING_TO_JSVAL(jsstring);
    }
    break;
  }
  default:
    NS_NOTREACHED("invalid type");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}





NS_IMPL_ISUPPORTS1(nsNativeMethod, nsIXPCScriptable)

nsNativeMethod::nsNativeMethod()
  : mFunc(nsnull)
{
}

nsNativeMethod::~nsNativeMethod()
{
}

nsresult
nsNativeMethod::Init(JSContext* aContext,
                     nsNativeTypes* aLibrary,
                     PRFuncPtr aFunc,
                     PRUint16 aCallType,
                     jsval aResultType,
                     const nsTArray<jsval>& aArgTypes)
{
  nsresult rv;

  mLibrary = aLibrary;
  mFunc = aFunc;

  
  rv = GetABI(aCallType, mCallType);
  if (NS_FAILED(rv)) {
    JS_ReportError(aContext, "Invalid ABI specification");
    return rv;
  }

  
  rv = PrepareType(aContext, aResultType, mResultType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (PRUint32 i = 0; i < aArgTypes.Length(); ++i) {
    rv = PrepareType(aContext, aArgTypes[i], *mArgTypes.AppendElement());
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (mArgTypes[i].mNativeType == nsINativeTypes::VOID)
      return TypeError(aContext, "Cannot have void argument type");

    
    mFFITypes.AppendElement(&mArgTypes[i].mType);
  }

  ffi_status status = ffi_prep_cif(&mCIF, mCallType, mFFITypes.Length(),
                                   &mResultType.mType, mFFITypes.Elements());
  switch (status) {
  case FFI_OK:
    return NS_OK;
  case FFI_BAD_ABI:
    JS_ReportError(aContext, "Invalid ABI specification");
    return NS_ERROR_INVALID_ARG;
  case FFI_BAD_TYPEDEF:
    JS_ReportError(aContext, "Invalid type specification");
    return NS_ERROR_INVALID_ARG;
  default:
    JS_ReportError(aContext, "Unknown libffi error");
    return NS_ERROR_FAILURE;
  }
}

PRBool
nsNativeMethod::Execute(JSContext* aContext, PRUint32 aArgc, jsval* aArgv, jsval* aValue)
{
  nsresult rv;

  
  nsAutoTArray<nsNativeValue, 16> nativeValues;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    rv = PrepareValue(aContext, mArgTypes[i], aArgv[i], *nativeValues.AppendElement());
    if (NS_FAILED(rv)) return PR_FALSE;
  }

  
  nsAutoTArray<void*, 16> values;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    values.AppendElement(nativeValues[i].mData);
  }

  
  nsNativeValue resultValue;
  PrepareReturnValue(mResultType, resultValue);

  
  
  jsrefcount rc = JS_SuspendRequest(aContext);

  ffi_call(&mCIF, mFunc, resultValue.mData, values.Elements());

  JS_ResumeRequest(aContext, rc);

  
  rv = ConvertReturnValue(aContext, mResultType, resultValue, aValue);
  if (NS_FAILED(rv)) return PR_FALSE;

  return PR_TRUE;
}





#define XPC_MAP_CLASSNAME nsNativeMethod
#define XPC_MAP_QUOTED_CLASSNAME "ctypes"
#define XPC_MAP_WANT_CALL
#define XPC_MAP_FLAGS nsIXPCScriptable::WANT_CALL

#include "xpc_map_end.h"

NS_IMETHODIMP
nsNativeMethod::Call(nsIXPConnectWrappedNative* wrapper,
                     JSContext* cx,
                     JSObject* obj, 
                     PRUint32 argc, 
                     jsval* argv, 
                     jsval* vp, 
                     PRBool* _retval)
{
  JSAutoRequest ar(cx);

  if (!mLibrary->IsOpen()) {
    JS_ReportError(cx, "Library is not open");
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  if (argc != mArgTypes.Length()) {
    JS_ReportError(cx, "Number of arguments does not match declaration");
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  *_retval = Execute(cx, argc, argv, vp);
  if (!*_retval)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

