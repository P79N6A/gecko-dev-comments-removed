







































#include "jscntxt.h"
#include "Function.h"
#include "Library.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace ctypes {





static bool
GetABI(JSContext* cx, jsval aCallType, ffi_abi& aResult)
{
  if (JSVAL_IS_PRIMITIVE(aCallType))
    return false;

  ABICode abi = GetABICode(cx, JSVAL_TO_OBJECT(aCallType));

  
  
  
  switch (abi) {
  case ABI_DEFAULT:
    aResult = FFI_DEFAULT_ABI;
    return true;
  case ABI_STDCALL:
#if (defined(_WIN32) && !defined(_WIN64)) || defined(_OS2)
    aResult = FFI_STDCALL;
    return true;
#endif
  case INVALID_ABI:
    break;
  }
  return false;
}

static JSBool
PrepareType(JSContext* aContext, jsval aType, Type& aResult)
{
  if (JSVAL_IS_PRIMITIVE(aType) ||
      !CType::IsCType(aContext, JSVAL_TO_OBJECT(aType))) {
    JS_ReportError(aContext, "not a ctypes type");
    return false;
  }

  JSObject* typeObj = JSVAL_TO_OBJECT(aType);
  TypeCode typeCode = CType::GetTypeCode(aContext, typeObj);

  if (typeCode == TYPE_array) {
    
    
    JSObject* baseType = ArrayType::GetBaseType(aContext, typeObj);
    typeObj = PointerType::CreateInternal(aContext, NULL, baseType, NULL);
    if (!typeObj)
      return false;

  } else if (typeCode == TYPE_void_t) {
    
    JS_ReportError(aContext, "Cannot have void argument type");
    return false;
  }

  
  JS_ASSERT(CType::GetSize(aContext, typeObj) != 0);

  aResult.mType = typeObj;
  aResult.mFFIType = *CType::GetFFIType(aContext, typeObj);
  return true;
}

static JSBool
PrepareResultType(JSContext* aContext, jsval aType, Type& aResult)
{
  if (JSVAL_IS_PRIMITIVE(aType) ||
      !CType::IsCType(aContext, JSVAL_TO_OBJECT(aType))) {
    JS_ReportError(aContext, "not a ctypes type");
    return false;
  }

  JSObject* typeObj = JSVAL_TO_OBJECT(aType);
  TypeCode typeCode = CType::GetTypeCode(aContext, typeObj);

  
  if (typeCode == TYPE_array) {
    JS_ReportError(aContext, "Result type cannot be an array");
    return false;
  }

  
  JS_ASSERT(typeCode == TYPE_void_t || CType::GetSize(aContext, typeObj) != 0);

  aResult.mType = typeObj;
  aResult.mFFIType = *CType::GetFFIType(aContext, typeObj);
  return true;
}





Function::Function()
  : mNext(NULL)
{
}

Function::~Function()
{
}

JSBool
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

  
  if (!PrepareResultType(aContext, aResultType, mResultType))
    return false;

  
  mArgTypes.SetCapacity(aArgLength);
  for (PRUint32 i = 0; i < aArgLength; ++i) {
    if (!PrepareType(aContext, aArgTypes[i], *mArgTypes.AppendElement()))
      return false;

    
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

JSBool
Function::Execute(JSContext* cx, PRUint32 argc, jsval* vp)
{
  if (argc != mArgTypes.Length()) {
    JS_ReportError(cx, "Number of arguments does not match declaration");
    return false;
  }

  
  nsAutoTArray<AutoValue, 16> values;
  nsAutoTArray<AutoValue, 16> strings;
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i) {
    AutoValue& value = *values.AppendElement();
    jsval arg = JS_ARGV(cx, vp)[i];
    bool freePointer = false;
    if (!value.SizeToType(cx, mArgTypes[i].mType)) {
      JS_ReportAllocationOverflow(cx);
      return false;
    }

    if (!ImplicitConvert(cx, arg, mArgTypes[i].mType, value.mData, true, &freePointer))
      return false;

    if (freePointer) {
      
      
      strings.AppendElement()->mData = *static_cast<char**>(value.mData);
    }
  }

  
  AutoValue resultValue;
  if (CType::GetTypeCode(cx, mResultType.mType) != TYPE_void_t &&
      !resultValue.SizeToType(cx, mResultType.mType)) {
    JS_ReportAllocationOverflow(cx);
    return false;
  }

  
  
  jsrefcount rc = JS_SuspendRequest(cx);

  ffi_call(&mCIF, FFI_FN(mFunc), resultValue.mData, reinterpret_cast<void**>(values.Elements()));

  JS_ResumeRequest(cx, rc);

  
  jsval rval;
  if (!ConvertToJS(cx, mResultType.mType, NULL, resultValue.mData, false, &rval))
    return false;

  JS_SET_RVAL(cx, vp, rval);
  return true;
}

void
Function::Trace(JSTracer *trc)
{
  
  JS_CALL_TRACER(trc, mResultType.mType, JSTRACE_OBJECT, "CType");

  
  for (PRUint32 i = 0; i < mArgTypes.Length(); ++i)
    JS_CALL_TRACER(trc, mArgTypes[i].mType, JSTRACE_OBJECT, "CType");
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
  if (!self) {
    JS_ReportOutOfMemory(aContext);
    return NULL;
  }

  
  if (!self->Init(aContext, aFunc, aCallType, aResultType, aArgTypes, aArgLength))
    return NULL;

  
  JSFunction* fn = JS_NewFunction(aContext, JSNative(Function::Call),
                     aArgLength, JSFUN_FAST_NATIVE, NULL, aName);
  if (!fn)
    return NULL;

  JSObject* fnObj = JS_GetFunctionObject(fn);
  JSAutoTempValueRooter fnRoot(aContext, fnObj);

  
  if (!JS_SetReservedSlot(aContext, fnObj, SLOT_FUNCTION, PRIVATE_TO_JSVAL(self.get())))
    return NULL;

  
  if (!JS_SetReservedSlot(aContext, fnObj, SLOT_LIBRARYOBJ, OBJECT_TO_JSVAL(aLibrary)))
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
  JS_GetReservedSlot(cx, obj, SLOT_FUNCTION, &slot);
  return static_cast<Function*>(JSVAL_TO_PRIVATE(slot));
}

JSBool
Function::Call(JSContext* cx, uintN argc, jsval* vp)
{
  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));

  jsval slot;
  JS_GetReservedSlot(cx, callee, SLOT_LIBRARYOBJ, &slot);

  PRLibrary* library = Library::GetLibrary(cx, JSVAL_TO_OBJECT(slot));
  if (!library) {
    JS_ReportError(cx, "library is not open");
    return JS_FALSE;
  }

  return GetFunction(cx, callee)->Execute(cx, argc, vp);
}

}
}

