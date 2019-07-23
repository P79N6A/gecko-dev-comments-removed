







































#include "Function.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIXPConnect.h"
#include "nsCRT.h"

namespace mozilla {
namespace ctypes {





template<class IntegerType>
static bool
jsvalToIntStrict(jsval aValue, IntegerType *aResult)
{
  if (JSVAL_IS_INT(aValue)) {
    jsint i = JSVAL_TO_INT(aValue);
    *aResult = IntegerType(i);

    
    return jsint(*aResult) == i &&
           (i < 0) == (*aResult < 0);
  }
  if (JSVAL_IS_DOUBLE(aValue)) {
    jsdouble d = *JSVAL_TO_DOUBLE(aValue);
    *aResult = IntegerType(d);

    
    
    return jsdouble(*aResult) == d &&
           (d < 0) == (*aResult < 0);
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
  JS_ReportError(cx, "%s", message);
  return NS_ERROR_FAILURE;
}

static nsresult
GetABI(PRUint16 aCallType, ffi_abi& aResult)
{
  
  
  
  switch (aCallType) {
  case nsIForeignLibrary::DEFAULT:
    aResult = FFI_DEFAULT_ABI;
    return NS_OK;
#if defined(XP_WIN32)
  case nsIForeignLibrary::STDCALL:
    aResult = FFI_STDCALL;
    return NS_OK;
#endif
  default:
    return NS_ERROR_INVALID_ARG;
  }
}

static nsresult
PrepareType(JSContext* aContext, jsval aType, Type& aResult)
{
  
  if (!JSVAL_IS_INT(aType)) {
    JS_ReportError(aContext, "Invalid type specification");
    return NS_ERROR_FAILURE;
  }

  PRInt32 type = JSVAL_TO_INT(aType);

  switch (type) {
  case nsIForeignLibrary::VOID:
    aResult.mFFIType = ffi_type_void;
    break;
  case nsIForeignLibrary::INT8:
    aResult.mFFIType = ffi_type_sint8;
    break;
  case nsIForeignLibrary::INT16:
    aResult.mFFIType = ffi_type_sint16;
    break;
  case nsIForeignLibrary::INT32:
    aResult.mFFIType = ffi_type_sint32;
    break;
  case nsIForeignLibrary::INT64:
    aResult.mFFIType = ffi_type_sint64;
    break;
  case nsIForeignLibrary::BOOL:
  case nsIForeignLibrary::UINT8:
    aResult.mFFIType = ffi_type_uint8;
    break;
  case nsIForeignLibrary::UINT16:
    aResult.mFFIType = ffi_type_uint16;
    break;
  case nsIForeignLibrary::UINT32:
    aResult.mFFIType = ffi_type_uint32;
    break;
  case nsIForeignLibrary::UINT64:
    aResult.mFFIType = ffi_type_uint64;
    break;
  case nsIForeignLibrary::FLOAT:
    aResult.mFFIType = ffi_type_float;
    break;
  case nsIForeignLibrary::DOUBLE:
    aResult.mFFIType = ffi_type_double;
    break;
  case nsIForeignLibrary::STRING:
  case nsIForeignLibrary::USTRING:
    aResult.mFFIType = ffi_type_pointer;
    break;
  default:
    JS_ReportError(aContext, "Invalid type specification");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  aResult.mType = type;

  return NS_OK;
}

static nsresult
PrepareValue(JSContext* aContext, const Type& aType, jsval aValue, Value& aResult)
{
  jsdouble d;

  switch (aType.mType) {
  case nsIForeignLibrary::BOOL:
    
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint8) ||
        aResult.mValue.mUint8 > 1)
      return TypeError(aContext, "Expected boolean value");

    aResult.mData = &aResult.mValue.mUint8;
    break;
  case nsIForeignLibrary::INT8:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt8))
      return TypeError(aContext, "Expected int8 value");

    aResult.mData = &aResult.mValue.mInt8;
    break;
  case nsIForeignLibrary::INT16:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt16))
      return TypeError(aContext, "Expected int16 value");

    aResult.mData = &aResult.mValue.mInt16;
    break;
  case nsIForeignLibrary::INT32:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt32))
      return TypeError(aContext, "Expected int32 value");

    aResult.mData = &aResult.mValue.mInt32;
  case nsIForeignLibrary::INT64:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt64))
      return TypeError(aContext, "Expected int64 value");

    aResult.mData = &aResult.mValue.mInt64;
    break;
  case nsIForeignLibrary::UINT8:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint8))
      return TypeError(aContext, "Expected uint8 value");

    aResult.mData = &aResult.mValue.mUint8;
    break;
  case nsIForeignLibrary::UINT16:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint16))
      return TypeError(aContext, "Expected uint16 value");

    aResult.mData = &aResult.mValue.mUint16;
    break;
  case nsIForeignLibrary::UINT32:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint32))
      return TypeError(aContext, "Expected uint32 value");

    aResult.mData = &aResult.mValue.mUint32;
  case nsIForeignLibrary::UINT64:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint64))
      return TypeError(aContext, "Expected uint64 value");

    aResult.mData = &aResult.mValue.mUint64;
    break;
  case nsIForeignLibrary::FLOAT:
    if (!jsvalToDoubleStrict(aValue, &d))
      return TypeError(aContext, "Expected number");

    
    
    
    
    aResult.mValue.mFloat = float(d);
    aResult.mData = &aResult.mValue.mFloat;
    break;
  case nsIForeignLibrary::DOUBLE:
    if (!jsvalToDoubleStrict(aValue, &d))
      return TypeError(aContext, "Expected number");

    aResult.mValue.mDouble = d;
    aResult.mData = &aResult.mValue.mDouble;
    break;
  case nsIForeignLibrary::STRING:
    if (JSVAL_IS_NULL(aValue)) {
      
      aResult.mValue.mPointer = nsnull;
    } else if (JSVAL_IS_STRING(aValue)) {
      aResult.mValue.mPointer = JS_GetStringBytes(JSVAL_TO_STRING(aValue));
    } else {
      
      
      return TypeError(aContext, "Expected string");
    }

    aResult.mData = &aResult.mValue.mPointer;
    break;
  case nsIForeignLibrary::USTRING:
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
PrepareReturnValue(const Type& aType, Value& aResult)
{
  switch (aType.mType) {
  case nsIForeignLibrary::VOID:
    aResult.mData = nsnull;
    break;
  case nsIForeignLibrary::INT8:
    aResult.mData = &aResult.mValue.mInt8;
    break;
  case nsIForeignLibrary::INT16:
    aResult.mData = &aResult.mValue.mInt16;
    break;
  case nsIForeignLibrary::INT32:
    aResult.mData = &aResult.mValue.mInt32;
    break;
  case nsIForeignLibrary::INT64:
    aResult.mData = &aResult.mValue.mInt64;
    break;
  case nsIForeignLibrary::BOOL:
  case nsIForeignLibrary::UINT8:
    aResult.mData = &aResult.mValue.mUint8;
    break;
  case nsIForeignLibrary::UINT16:
    aResult.mData = &aResult.mValue.mUint16;
    break;
  case nsIForeignLibrary::UINT32:
    aResult.mData = &aResult.mValue.mUint32;
    break;
  case nsIForeignLibrary::UINT64:
    aResult.mData = &aResult.mValue.mUint64;
    break;
  case nsIForeignLibrary::FLOAT:
    aResult.mData = &aResult.mValue.mFloat;
    break;
  case nsIForeignLibrary::DOUBLE:
    aResult.mData = &aResult.mValue.mDouble;
    break;
  case nsIForeignLibrary::STRING:
  case nsIForeignLibrary::USTRING:
    aResult.mData = &aResult.mValue.mPointer;
    break;
  default:
    NS_NOTREACHED("invalid type");
    break;
  }
}

static nsresult
ConvertReturnValue(JSContext* aContext,
                   const Type& aResultType,
                   const Value& aResultValue,
                   jsval* aValue)
{
  switch (aResultType.mType) {
  case nsIForeignLibrary::VOID:
    *aValue = JSVAL_VOID;
    break;
  case nsIForeignLibrary::BOOL:
    *aValue = aResultValue.mValue.mUint8 ? JSVAL_TRUE : JSVAL_FALSE;
    break;
  case nsIForeignLibrary::INT8:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mInt8);
    break;
  case nsIForeignLibrary::INT16:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mInt16);
    break;
  case nsIForeignLibrary::INT32:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mInt32), aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsIForeignLibrary::INT64:
    
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mInt64), aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsIForeignLibrary::UINT8:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mUint8);
    break;
  case nsIForeignLibrary::UINT16:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mUint16);
    break;
  case nsIForeignLibrary::UINT32:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mUint32), aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsIForeignLibrary::UINT64:
    
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mUint64), aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsIForeignLibrary::FLOAT:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mFloat), aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsIForeignLibrary::DOUBLE:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mDouble), aValue))
      return NS_ERROR_OUT_OF_MEMORY;
    break;
  case nsIForeignLibrary::STRING: {
    if (!aResultValue.mValue.mPointer) {
      
      *aValue = JSVAL_NULL;
    } else {
      JSString *jsstring = JS_NewStringCopyZ(aContext,
                             reinterpret_cast<const char*>(aResultValue.mValue.mPointer));
      if (!jsstring)
        return NS_ERROR_OUT_OF_MEMORY;

      *aValue = STRING_TO_JSVAL(jsstring);
    }
    break;
  }
  case nsIForeignLibrary::USTRING: {
    if (!aResultValue.mValue.mPointer) {
      
      *aValue = JSVAL_NULL;
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





NS_IMPL_ISUPPORTS1(Function, nsIXPCScriptable)

Function::Function()
  : mFunc(nsnull)
{
}

Function::~Function()
{
}

nsresult
Function::Init(JSContext* aContext,
               Library* aLibrary,
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

    
    if (mArgTypes[i].mType == nsIForeignLibrary::VOID)
      return TypeError(aContext, "Cannot have void argument type");

    
    mFFITypes.AppendElement(&mArgTypes[i].mFFIType);
  }

  ffi_status status = ffi_prep_cif(&mCIF, mCallType, mFFITypes.Length(),
                                   &mResultType.mFFIType, mFFITypes.Elements());
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
Function::Execute(JSContext* aContext, PRUint32 aArgc, jsval* aArgv, jsval* aValue)
{
  nsresult rv;

  
  nsAutoTArray<Value, 16> values;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    rv = PrepareValue(aContext, mArgTypes[i], aArgv[i], *values.AppendElement());
    if (NS_FAILED(rv)) return PR_FALSE;
  }

  
  nsAutoTArray<void*, 16> ffiValues;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    ffiValues.AppendElement(values[i].mData);
  }

  
  Value resultValue;
  PrepareReturnValue(mResultType, resultValue);

  
  
  jsrefcount rc = JS_SuspendRequest(aContext);

  ffi_call(&mCIF, mFunc, resultValue.mData, ffiValues.Elements());

  JS_ResumeRequest(aContext, rc);

  
  rv = ConvertReturnValue(aContext, mResultType, resultValue, aValue);
  if (NS_FAILED(rv)) return PR_FALSE;

  return PR_TRUE;
}





#define XPC_MAP_CLASSNAME Function
#define XPC_MAP_QUOTED_CLASSNAME "Function"
#define XPC_MAP_WANT_CALL
#define XPC_MAP_FLAGS nsIXPCScriptable::WANT_CALL

#include "xpc_map_end.h"

NS_IMETHODIMP
Function::Call(nsIXPConnectWrappedNative* wrapper,
               JSContext* cx,
               JSObject* obj, 
               PRUint32 argc, 
               jsval* argv, 
               jsval* vp, 
               PRBool* _retval)
{
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

}
}

