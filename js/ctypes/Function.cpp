







































#include "Function.h"
#include "Library.h"
#include "nsAutoPtr.h"
#include "jscntxt.h"

namespace mozilla {
namespace ctypes {





template<class IntegerType>
static IntegerType
Convert(jsdouble d)
{
  return IntegerType(d);
}

#ifdef _MSC_VER


template<>
static PRUint64
Convert<PRUint64>(jsdouble d)
{
  return d > 0x7fffffffffffffffui64 ?
         PRUint64(d - 0x8000000000000000ui64) + 0x8000000000000000ui64 :
         PRUint64(d);
}
#endif

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
    *aResult = Convert<IntegerType>(d);

    
    
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

JSErrorFormatString ErrorFormatString[CTYPESERR_LIMIT] = {
#define MSG_DEF(name, number, count, exception, format) \
  { format, count, exception } ,
#include "ctypes.msg"
#undef MSG_DEF
};

const JSErrorFormatString*
GetErrorMessage(void* userRef, const char* locale, const uintN errorNumber)
{
  if (0 < errorNumber && errorNumber < CTYPESERR_LIMIT)
    return &ErrorFormatString[errorNumber];
  return NULL;
}

static const char*
ToSource(JSContext* cx, jsval vp)
{
  JSString* str = JS_ValueToSource(cx, vp);
  if (str)
    return JS_GetStringBytes(str);

  JS_ClearPendingException(cx);
  return "<<error converting value to string>>";
}

static bool
TypeError(JSContext* cx, const char* expected, jsval actual)
{
  const char* src = ToSource(cx, actual);
  JS_ReportErrorNumber(cx, GetErrorMessage, NULL,
                       CTYPESMSG_TYPE_ERROR, expected, src);
  return false;
}

static bool
GetABI(JSContext* cx, jsval aCallType, ffi_abi& aResult)
{
  ABICode abi = Module::GetABICode(cx, aCallType);

  
  
  
  switch (abi) {
  case ABI_default_abi:
    aResult = FFI_DEFAULT_ABI;
    return true;
#if defined(_WIN32) && !defined(_WIN64)
  case ABI_stdcall_abi:
    aResult = FFI_STDCALL;
    return true;
#endif
  default:
    return false;
  }
}

static bool
PrepareType(JSContext* aContext, jsval aType, Type& aResult)
{
  aResult.mType = Module::GetTypeCode(aContext, aType);

  switch (aResult.mType) {
  case TYPE_void_t:
    aResult.mFFIType = ffi_type_void;
    break;
  case TYPE_int8_t:
    aResult.mFFIType = ffi_type_sint8;
    break;
  case TYPE_int16_t:
    aResult.mFFIType = ffi_type_sint16;
    break;
  case TYPE_int32_t:
    aResult.mFFIType = ffi_type_sint32;
    break;
  case TYPE_int64_t:
    aResult.mFFIType = ffi_type_sint64;
    break;
  case TYPE_bool:
  case TYPE_uint8_t:
    aResult.mFFIType = ffi_type_uint8;
    break;
  case TYPE_uint16_t:
    aResult.mFFIType = ffi_type_uint16;
    break;
  case TYPE_uint32_t:
    aResult.mFFIType = ffi_type_uint32;
    break;
  case TYPE_uint64_t:
    aResult.mFFIType = ffi_type_uint64;
    break;
  case TYPE_float:
    aResult.mFFIType = ffi_type_float;
    break;
  case TYPE_double:
    aResult.mFFIType = ffi_type_double;
    break;
  case TYPE_string:
  case TYPE_ustring:
    aResult.mFFIType = ffi_type_pointer;
    break;
  default:
    JS_ReportError(aContext, "Invalid type specification");
    return false;
  }

  return true;
}

static bool
PrepareValue(JSContext* aContext, const Type& aType, jsval aValue, Value& aResult)
{
  jsdouble d;

  switch (aType.mType) {
  case TYPE_bool:
    
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint8) ||
        aResult.mValue.mUint8 > 1)
      return TypeError(aContext, "boolean", aValue);

    aResult.mData = &aResult.mValue.mUint8;
    break;
  case TYPE_int8_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt8))
      return TypeError(aContext, "int8", aValue);

    aResult.mData = &aResult.mValue.mInt8;
    break;
  case TYPE_int16_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt16))
      return TypeError(aContext, "int16", aValue);

    aResult.mData = &aResult.mValue.mInt16;
    break;
  case TYPE_int32_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt32))
      return TypeError(aContext, "int32", aValue);

    aResult.mData = &aResult.mValue.mInt32;
    break;
  case TYPE_int64_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mInt64))
      return TypeError(aContext, "int64", aValue);

    aResult.mData = &aResult.mValue.mInt64;
    break;
  case TYPE_uint8_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint8))
      return TypeError(aContext, "uint8", aValue);

    aResult.mData = &aResult.mValue.mUint8;
    break;
  case TYPE_uint16_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint16))
      return TypeError(aContext, "uint16", aValue);

    aResult.mData = &aResult.mValue.mUint16;
    break;
  case TYPE_uint32_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint32))
      return TypeError(aContext, "uint32", aValue);

    aResult.mData = &aResult.mValue.mUint32;
    break;
  case TYPE_uint64_t:
    
    if (!jsvalToIntStrict(aValue, &aResult.mValue.mUint64))
      return TypeError(aContext, "uint64", aValue);

    aResult.mData = &aResult.mValue.mUint64;
    break;
  case TYPE_float:
    if (!jsvalToDoubleStrict(aValue, &d))
      return TypeError(aContext, "float", aValue);

    
    
    
    
    aResult.mValue.mFloat = float(d);
    aResult.mData = &aResult.mValue.mFloat;
    break;
  case TYPE_double:
    if (!jsvalToDoubleStrict(aValue, &d))
      return TypeError(aContext, "double", aValue);

    aResult.mValue.mDouble = d;
    aResult.mData = &aResult.mValue.mDouble;
    break;
  case TYPE_string:
    if (JSVAL_IS_NULL(aValue)) {
      
      aResult.mValue.mPointer = nsnull;
    } else if (JSVAL_IS_STRING(aValue)) {
      aResult.mValue.mPointer = JS_GetStringBytes(JSVAL_TO_STRING(aValue));
    } else {
      
      
      return TypeError(aContext, "string", aValue);
    }

    aResult.mData = &aResult.mValue.mPointer;
    break;
  case TYPE_ustring:
    if (JSVAL_IS_NULL(aValue)) {
      
      aResult.mValue.mPointer = nsnull;
    } else if (JSVAL_IS_STRING(aValue)) {
      aResult.mValue.mPointer = JS_GetStringChars(JSVAL_TO_STRING(aValue));
    } else {
      
      
      return TypeError(aContext, "ustring", aValue);
    }

    aResult.mData = &aResult.mValue.mPointer;
    break;
  default:
    NS_NOTREACHED("invalid type");
    return false;
  }

  return true;
}

static void
PrepareReturnValue(const Type& aType, Value& aResult)
{
  switch (aType.mType) {
  case TYPE_void_t:
    aResult.mData = nsnull;
    break;
  case TYPE_int8_t:
    aResult.mData = &aResult.mValue.mInt8;
    break;
  case TYPE_int16_t:
    aResult.mData = &aResult.mValue.mInt16;
    break;
  case TYPE_int32_t:
    aResult.mData = &aResult.mValue.mInt32;
    break;
  case TYPE_int64_t:
    aResult.mData = &aResult.mValue.mInt64;
    break;
  case TYPE_bool:
  case TYPE_uint8_t:
    aResult.mData = &aResult.mValue.mUint8;
    break;
  case TYPE_uint16_t:
    aResult.mData = &aResult.mValue.mUint16;
    break;
  case TYPE_uint32_t:
    aResult.mData = &aResult.mValue.mUint32;
    break;
  case TYPE_uint64_t:
    aResult.mData = &aResult.mValue.mUint64;
    break;
  case TYPE_float:
    aResult.mData = &aResult.mValue.mFloat;
    break;
  case TYPE_double:
    aResult.mData = &aResult.mValue.mDouble;
    break;
  case TYPE_string:
  case TYPE_ustring:
    aResult.mData = &aResult.mValue.mPointer;
    break;
  default:
    NS_NOTREACHED("invalid type");
    break;
  }
}

static bool
ConvertReturnValue(JSContext* aContext,
                   const Type& aResultType,
                   const Value& aResultValue,
                   jsval* aValue)
{
  switch (aResultType.mType) {
  case TYPE_void_t:
    *aValue = JSVAL_VOID;
    break;
  case TYPE_bool:
    *aValue = aResultValue.mValue.mUint8 ? JSVAL_TRUE : JSVAL_FALSE;
    break;
  case TYPE_int8_t:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mInt8);
    break;
  case TYPE_int16_t:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mInt16);
    break;
  case TYPE_int32_t:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mInt32), aValue))
      return false;
    break;
  case TYPE_int64_t:
    
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mInt64), aValue))
      return false;
    break;
  case TYPE_uint8_t:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mUint8);
    break;
  case TYPE_uint16_t:
    *aValue = INT_TO_JSVAL(aResultValue.mValue.mUint16);
    break;
  case TYPE_uint32_t:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mUint32), aValue))
      return false;
    break;
  case TYPE_uint64_t:
    
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mUint64), aValue))
      return false;
    break;
  case TYPE_float:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mFloat), aValue))
      return false;
    break;
  case TYPE_double:
    if (!JS_NewNumberValue(aContext, jsdouble(aResultValue.mValue.mDouble), aValue))
      return false;
    break;
  case TYPE_string: {
    if (!aResultValue.mValue.mPointer) {
      
      *aValue = JSVAL_NULL;
    } else {
      JSString *jsstring = JS_NewStringCopyZ(aContext,
                             reinterpret_cast<const char*>(aResultValue.mValue.mPointer));
      if (!jsstring)
        return false;

      *aValue = STRING_TO_JSVAL(jsstring);
    }
    break;
  }
  case TYPE_ustring: {
    if (!aResultValue.mValue.mPointer) {
      
      *aValue = JSVAL_NULL;
    } else {
      JSString *jsstring = JS_NewUCStringCopyZ(aContext,
                             reinterpret_cast<const jschar*>(aResultValue.mValue.mPointer));
      if (!jsstring)
        return false;

      *aValue = STRING_TO_JSVAL(jsstring);
    }
    break;
  }
  default:
    NS_NOTREACHED("invalid type");
    return false;
  }

  return true;
}





Function::Function()
  : mNext(NULL)
{
}

Function::~Function()
{
}

bool
Function::Init(JSContext* aContext,
               PRFuncPtr aFunc,
               jsval aCallType,
               jsval aResultType,
               jsval* aArgTypes,
               uintN aArgLength)
{
  mFunc = aFunc;

  
  if (!GetABI(aContext, aCallType, mCallType)) {
    JS_ReportError(aContext, "Invalid ABI specification");
    return false;
  }

  
  if (!PrepareType(aContext, aResultType, mResultType))
    return false;

  
  mArgTypes.SetCapacity(aArgLength);
  for (PRUint32 i = 0; i < aArgLength; ++i) {
    if (!PrepareType(aContext, aArgTypes[i], *mArgTypes.AppendElement()))
      return false;

    
    if (mArgTypes[i].mType == TYPE_void_t) {
      JS_ReportError(aContext, "Cannot have void argument type");
      return false;
    }

    
    mFFITypes.AppendElement(&mArgTypes[i].mFFIType);
  }

  ffi_status status = ffi_prep_cif(&mCIF, mCallType, mFFITypes.Length(),
                                   &mResultType.mFFIType, mFFITypes.Elements());
  switch (status) {
  case FFI_OK:
    return true;
  case FFI_BAD_ABI:
    JS_ReportError(aContext, "Invalid ABI specification");
    return false;
  case FFI_BAD_TYPEDEF:
    JS_ReportError(aContext, "Invalid type specification");
    return false;
  default:
    JS_ReportError(aContext, "Unknown libffi error");
    return false;
  }
}

bool
Function::Execute(JSContext* cx, PRUint32 argc, jsval* vp)
{
  if (argc != mArgTypes.Length()) {
    JS_ReportError(cx, "Number of arguments does not match declaration");
    return false;
  }

  
  nsAutoTArray<Value, 16> values;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    if (!PrepareValue(cx, mArgTypes[i], JS_ARGV(cx, vp)[i], *values.AppendElement()))
      return false;
  }

  
  nsAutoTArray<void*, 16> ffiValues;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    ffiValues.AppendElement(values[i].mData);
  }

  
  Value resultValue;
  PrepareReturnValue(mResultType, resultValue);

  
  
  jsrefcount rc = JS_SuspendRequest(cx);

  ffi_call(&mCIF, FFI_FN(mFunc), resultValue.mData, ffiValues.Elements());

  JS_ResumeRequest(cx, rc);

  
  jsval rval;
  if (!ConvertReturnValue(cx, mResultType, resultValue, &rval))
    return false;

  JS_SET_RVAL(cx, vp, rval);
  return true;
}





JSObject*
Function::Create(JSContext* aContext,
                 JSObject* aLibrary,
                 PRFuncPtr aFunc,
                 const char* aName,
                 jsval aCallType,
                 jsval aResultType,
                 jsval* aArgTypes,
                 uintN aArgLength)
{
  
  nsAutoPtr<Function> self(new Function());
  if (!self)
    return NULL;

  
  if (!self->Init(aContext, aFunc, aCallType, aResultType, aArgTypes, aArgLength))
    return NULL;

  
  JSFunction* fn = JS_NewFunction(aContext, JSNative(Function::Call),
                     aArgLength, JSFUN_FAST_NATIVE, NULL, aName);
  if (!fn)
    return NULL;

  JSObject* fnObj = JS_GetFunctionObject(fn);
  JSAutoTempValueRooter fnRoot(aContext, fnObj);

  
  if (!JS_SetReservedSlot(aContext, fnObj, 0, PRIVATE_TO_JSVAL(self.get())))
    return NULL;

  
  if (!JS_SetReservedSlot(aContext, fnObj, 1, OBJECT_TO_JSVAL(aLibrary)))
    return NULL;

  
  
  if (!Library::AddFunction(aContext, aLibrary, self))
    return NULL;

  self.forget();
  return fnObj;
}

static Function*
GetFunction(JSContext* cx, JSObject* obj)
{
  jsval slot;
  JS_GetReservedSlot(cx, obj, 0, &slot);
  return static_cast<Function*>(JSVAL_TO_PRIVATE(slot));
}

JSBool
Function::Call(JSContext* cx, uintN argc, jsval* vp)
{
  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));

  jsval slot;
  JS_GetReservedSlot(cx, callee, 1, &slot);

  PRLibrary* library = Library::GetLibrary(cx, JSVAL_TO_OBJECT(slot));
  if (!library) {
    JS_ReportError(cx, "library is not open");
    return JS_FALSE;
  }

  return GetFunction(cx, callee)->Execute(cx, argc, vp);
}

}
}

