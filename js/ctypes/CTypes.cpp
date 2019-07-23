





































#include "CTypes.h"
#include "nsAutoPtr.h"
#include "nsUTF8Utils.h"
#include "nsCRTGlue.h"
#include "prlog.h"
#include "prdtoa.h"
#include "jscntxt.h"
#include "jsnum.h"

namespace mozilla {
namespace ctypes {





static JSClass sCABIClass = {
  "CABI",
  JSCLASS_HAS_RESERVED_SLOTS(CABI_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};




static JSClass sCTypeProto = {
  "CType",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPEPROTO_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static JSClass sCTypeClass = {
  "CType",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPE_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CType::Finalize,
  NULL, NULL, CType::ConstructData, CType::ConstructData, NULL, NULL, NULL, NULL
};

static JSClass sCDataClass = {
  "CData",
  JSCLASS_HAS_RESERVED_SLOTS(CDATA_SLOTS),
  JS_PropertyStub, JS_PropertyStub, ArrayType::Getter, ArrayType::Setter,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CData::Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

#define CTYPESFN_FLAGS \
  (JSFUN_FAST_NATIVE | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec sCTypeFunctions[] = {
  JS_FN("array", CType::Array, 0, CTYPESFN_FLAGS),
  JS_FN("toString", CType::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", CType::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sCDataFunctions[] = {
  JS_FN("address", CData::Address, 0, CTYPESFN_FLAGS),
  JS_FN("readString", CData::ReadString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", CData::ToSource, 0, CTYPESFN_FLAGS),
  JS_FN("toString", CData::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sPointerFunction =
  JS_FN("PointerType", PointerType::Create, 1, CTYPESFN_FLAGS);

static JSFunctionSpec sArrayFunction =
  JS_FN("ArrayType", ArrayType::Create, 1, CTYPESFN_FLAGS);

static JSFunctionSpec sStructFunction =
  JS_FN("StructType", StructType::Create, 2, CTYPESFN_FLAGS);

static JSFunctionSpec sArrayInstanceFunctions[] = {
  JS_FN("addressOfElement", ArrayType::AddressOfElement, 1, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sStructInstanceFunctions[] = {
  JS_FN("addressOfField", StructType::AddressOfField, 1, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSClass sInt64Proto = {
  "Int64",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sUInt64Proto = {
  "UInt64",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sInt64Class = {
  "Int64",
  JSCLASS_HAS_RESERVED_SLOTS(INT64_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Int64Base::Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sUInt64Class = {
  "UInt64",
  JSCLASS_HAS_RESERVED_SLOTS(INT64_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Int64Base::Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec sInt64StaticFunctions[] = {
  JS_FN("compare", Int64::Compare, 2, CTYPESFN_FLAGS),
  JS_FN("lo", Int64::Lo, 1, CTYPESFN_FLAGS),
  JS_FN("hi", Int64::Hi, 1, CTYPESFN_FLAGS),
  JS_FN("join", Int64::Join, 2, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sUInt64StaticFunctions[] = {
  JS_FN("compare", UInt64::Compare, 2, CTYPESFN_FLAGS),
  JS_FN("lo", UInt64::Lo, 1, CTYPESFN_FLAGS),
  JS_FN("hi", UInt64::Hi, 1, CTYPESFN_FLAGS),
  JS_FN("join", UInt64::Join, 2, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sInt64Functions[] = {
  JS_FN("toString", Int64::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", Int64::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sUInt64Functions[] = {
  JS_FN("toString", UInt64::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", UInt64::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

ABICode
GetABICode(JSContext* cx, JSObject* obj)
{
  
  
  if (JS_GET_CLASS(cx, obj) != &sCABIClass)
    return INVALID_ABI;

  jsval result;
  JS_GetReservedSlot(cx, obj, SLOT_ABICODE, &result);

  return ABICode(JSVAL_TO_INT(result));
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
    return JS_GetStringBytesZ(cx, str);

  JS_ClearPendingException(cx);
  return "<<error converting value to string>>";
}

bool
TypeError(JSContext* cx, const char* expected, jsval actual)
{
  const char* src = ToSource(cx, actual);
  JS_ReportErrorNumber(cx, GetErrorMessage, NULL,
                       CTYPESMSG_TYPE_ERROR, expected, src);
  return false;
}

static bool
DefineABIConstant(JSContext* cx,
                  JSObject* parent,
                  const char* name,
                  ABICode code)
{
  JSObject* obj = JS_DefineObject(cx, parent, name, &sCABIClass, NULL,
                    JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
  if (!obj)
    return false;
  if (!JS_SetReservedSlot(cx, obj, SLOT_ABICODE, INT_TO_JSVAL(code)))
    return false;
  return JS_SealObject(cx, obj, JS_FALSE) != JS_FALSE;
}

static JSObject*
InitSpecialType(JSContext* cx,
                JSObject* parent,
                JSObject* CTypeProto,
                JSFunctionSpec spec)
{
  JSFunction* fun = JS_DefineFunction(cx, parent, spec.name, spec.call, 
                      spec.nargs, spec.flags);
  if (!fun)
    return NULL;

  JSObject* obj = JS_GetFunctionObject(fun);
  if (!obj)
    return NULL;

  
  JSObject* prototype = JS_NewObject(cx, NULL, CTypeProto, obj);
  if (!prototype)
    return NULL;

  if (!JS_DefineProperty(cx, obj, "prototype", OBJECT_TO_JSVAL(prototype),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(obj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_SealObject(cx, obj, JS_FALSE) ||
      !JS_SealObject(cx, prototype, JS_FALSE))
    return NULL;

  return prototype;
}

JSObject*
InitInt64Class(JSContext* cx,
               JSObject* parent,
               JSClass* clasp,
               JSNative construct,
               JSFunctionSpec* fs,
               JSFunctionSpec* static_fs)
{
  
  JSObject* prototype = JS_InitClass(cx, parent, NULL, clasp, construct,
    0, NULL, fs, NULL, static_fs);
  if (!prototype)
    return NULL;

  
  
  JSObject* ctor = JS_GetConstructor(cx, prototype);
  if (!ctor)
    return NULL;
  if (!JS_SealObject(cx, ctor, JS_FALSE))
    return NULL;
  JSObject* proto = JS_GetPrototype(cx, ctor);
  if (!proto)
    return NULL;
  if (!JS_SetPrototype(cx, prototype, proto))
    return NULL;

  
  
  jsval join;
  JS_GetProperty(cx, ctor, "join", &join);
  if (!JS_SetReservedSlot(cx, JSVAL_TO_OBJECT(join), SLOT_FN_INT64PROTO,
         OBJECT_TO_JSVAL(prototype)))
    return NULL;

  if (!JS_SealObject(cx, prototype, JS_FALSE))
    return NULL;

  return prototype;
}

bool
InitTypeClasses(JSContext* cx, JSObject* parent)
{
  
  
  
  
  
  
  
  
  
  
  JSObject* CTypeProto = JS_InitClass(cx, parent, NULL, &sCTypeProto,
    CType::ConstructAbstract, 0, NULL, sCTypeFunctions, NULL, NULL);
  if (!CTypeProto)
    return false;

  
  
  JSObject* ctor = JS_GetConstructor(cx, CTypeProto);
  if (!ctor)
    return false;
  if (!JS_SealObject(cx, ctor, JS_FALSE))
    return false;
  JSObject* proto = JS_GetPrototype(cx, ctor);
  if (!proto)
    return false;
  if (!JS_SetPrototype(cx, CTypeProto, proto))
    return false;
  if (!JS_SealObject(cx, CTypeProto, JS_FALSE))
    return false;

  
  if (!DefineABIConstant(cx, parent, "default_abi", ABI_DEFAULT) ||
      !DefineABIConstant(cx, parent, "stdcall_abi", ABI_STDCALL))
    return false;

  
  
  
  
  
  
  
  
  
  
  JSObject* typeObj;
#define DEFINE_TYPE(name, type, ffiType)                                       \
  typeObj = CType::DefineBuiltin(cx, parent, #name, CTypeProto, #name,         \
              TYPE_##name, INT_TO_JSVAL(sizeof(type)),                         \
              INT_TO_JSVAL(ffiType.alignment), &ffiType);                      \
  if (!typeObj)                                                                \
    return false;
#include "typedefs.h"

  
  
  
  
  
  
  
  
  
  
  JSObject* pointerProto = InitSpecialType(cx, parent, CTypeProto, sPointerFunction);
  JSObject* arrayProto = InitSpecialType(cx, parent, CTypeProto, sArrayFunction);
  JSObject* structProto = InitSpecialType(cx, parent, CTypeProto, sStructFunction);

  
  
  
  
  
  
  
  
  
  JSObject* Int64Proto = InitInt64Class(cx, parent, &sInt64Proto,
    Int64::Construct, sInt64Functions, sInt64StaticFunctions);
  if (!Int64Proto)
    return false;

  JSObject* UInt64Proto = InitInt64Class(cx, parent, &sUInt64Proto,
    UInt64::Construct, sUInt64Functions, sUInt64StaticFunctions);
  if (!UInt64Proto)
    return false;

  
  
  
  
  
  
  
  
  
  
  
  if (!JS_SetReservedSlot(cx, CTypeProto, SLOT_POINTERPROTO, OBJECT_TO_JSVAL(pointerProto)) ||
      !JS_SetReservedSlot(cx, CTypeProto, SLOT_ARRAYPROTO, OBJECT_TO_JSVAL(arrayProto)) ||
      !JS_SetReservedSlot(cx, CTypeProto, SLOT_STRUCTPROTO, OBJECT_TO_JSVAL(structProto)) ||
      !JS_SetReservedSlot(cx, CTypeProto, SLOT_INT64PROTO, OBJECT_TO_JSVAL(Int64Proto)) ||
      !JS_SetReservedSlot(cx, CTypeProto, SLOT_UINT64PROTO, OBJECT_TO_JSVAL(UInt64Proto)))
    return false;

  
  typeObj = CType::DefineBuiltin(cx, parent, "void_t", CTypeProto, "void",
               TYPE_void_t, JSVAL_VOID, JSVAL_VOID, &ffi_type_void);
  if (!typeObj)
    return false;

  typeObj = PointerType::CreateInternal(cx, NULL, typeObj, NULL);
  if (!typeObj)
    return false;
  if (!JS_DefineProperty(cx, parent, "voidptr_t", OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return true;
}










PR_STATIC_ASSERT(sizeof(bool) == 1 || sizeof(bool) == 4);
PR_STATIC_ASSERT(sizeof(char) == 1);
PR_STATIC_ASSERT(sizeof(short) == 2);
PR_STATIC_ASSERT(sizeof(int) == 4);
PR_STATIC_ASSERT(sizeof(unsigned) == 4);
PR_STATIC_ASSERT(sizeof(long) == 4 || sizeof(long) == 8);
PR_STATIC_ASSERT(sizeof(long long) == 8);
PR_STATIC_ASSERT(sizeof(size_t) == sizeof(uintptr_t));
PR_STATIC_ASSERT(sizeof(float) == 4);

template<class IntegerType>
static JS_ALWAYS_INLINE IntegerType
Convert(jsdouble d)
{
  return IntegerType(d);
}

#ifdef _MSC_VER


template<>
JS_ALWAYS_INLINE PRUint64
Convert<PRUint64>(jsdouble d)
{
  return d > 0x7fffffffffffffffui64 ?
         PRUint64(d - 0x8000000000000000ui64) + 0x8000000000000000ui64 :
         PRUint64(d);
}
#endif

template<class Type> static JS_ALWAYS_INLINE bool IsUnsigned() { return (Type(0) - Type(1)) > Type(0); }

template<class FloatType> static JS_ALWAYS_INLINE bool IsDoublePrecision();
template<> JS_ALWAYS_INLINE bool IsDoublePrecision<float> () { return false; }
template<> JS_ALWAYS_INLINE bool IsDoublePrecision<double>() { return true; }



static bool
jsvalToBool(JSContext* cx, jsval val, bool* result)
{
  if (JSVAL_IS_BOOLEAN(val)) {
    *result = JSVAL_TO_BOOLEAN(val) != JS_FALSE;
    return true;
  }
  if (JSVAL_IS_INT(val)) {
    jsint i = JSVAL_TO_INT(val);
    *result = i != 0;
    return i == 0 || i == 1;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    jsdouble d = *JSVAL_TO_DOUBLE(val);
    *result = d != 0;
    return d == 1 || d == 0;
  }
  
  return false;
}




template<class IntegerType>
static bool
jsvalToInteger(JSContext* cx, jsval val, IntegerType* result)
{
  if (JSVAL_IS_INT(val)) {
    jsint i = JSVAL_TO_INT(val);
    *result = IntegerType(i);

    
    
    if (IsUnsigned<IntegerType>() && i < 0)
      return false;
    return jsint(*result) == i;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    jsdouble d = *JSVAL_TO_DOUBLE(val);
    *result = Convert<IntegerType>(d);

    
    
    if (IsUnsigned<IntegerType>() && d < 0)
      return false;
    return jsdouble(*result) == d;
  }
  if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (CData::IsCData(cx, obj)) {
      JSObject* typeObj = CData::GetCType(cx, obj);
      void* data = CData::GetData(cx, obj);

      
      
      switch (CType::GetTypeCode(cx, typeObj)) {
#define DEFINE_INT_TYPE(name, fromType, ffiType)                               \
      case TYPE_##name:                                                        \
        if (IsUnsigned<fromType>() && IsUnsigned<IntegerType>() &&             \
            sizeof(IntegerType) < sizeof(fromType))                            \
          return false;                                                        \
        if (!IsUnsigned<fromType>() && !IsUnsigned<IntegerType>() &&           \
            sizeof(IntegerType) < sizeof(fromType))                            \
          return false;                                                        \
        if (IsUnsigned<fromType>() && !IsUnsigned<IntegerType>() &&            \
            sizeof(IntegerType) <= sizeof(fromType))                           \
          return false;                                                        \
        if (!IsUnsigned<fromType>() && IsUnsigned<IntegerType>())              \
          return false;                                                        \
        *result = *static_cast<fromType*>(data);                               \
        break;
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "typedefs.h"
      case TYPE_void_t:
      case TYPE_bool:
      case TYPE_float:
      case TYPE_double:
      case TYPE_float32_t:
      case TYPE_float64_t:
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char:
      case TYPE_jschar:
      case TYPE_pointer:
      case TYPE_array:
      case TYPE_struct:
        
        return false;
      }

      return true;
    }

    if (Int64::IsInt64(cx, obj)) {
      PRInt64 i = Int64Base::GetInt(cx, obj);
      *result = IntegerType(i);

      
      if (IsUnsigned<IntegerType>() && i < 0)
        return false;
      return PRInt64(*result) == i;
    }

    if (UInt64::IsUInt64(cx, obj)) {
      PRUint64 i = Int64Base::GetInt(cx, obj);
      *result = IntegerType(i);

      
      if (!IsUnsigned<IntegerType>() && *result < 0)
        return false;
      return PRUint64(*result) == i;
    }

    return false; 
  }
  if (JSVAL_IS_BOOLEAN(val)) {
    
    *result = JSVAL_TO_BOOLEAN(val);
    JS_ASSERT(*result == 0 || *result == 1);
    return true;
  }
  
  return false;
}




template<class FloatType>
static bool
jsvalToFloat(JSContext *cx, jsval val, FloatType* result)
{
  
  
  
  
  if (JSVAL_IS_INT(val)) {
    *result = FloatType(JSVAL_TO_INT(val));
    return true;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    *result = FloatType(*JSVAL_TO_DOUBLE(val));
    return true;
  }
  if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (CData::IsCData(cx, obj)) {
      JSObject* typeObj = CData::GetCType(cx, obj);
      void* data = CData::GetData(cx, obj);

      
      
      switch (CType::GetTypeCode(cx, typeObj)) {
#define DEFINE_FLOAT_TYPE(name, fromType, ffiType)                             \
      case TYPE_##name:                                                        \
        if (!IsDoublePrecision<FloatType>() && IsDoublePrecision<fromType>())  \
          return false;                                                        \
        *result = *static_cast<fromType*>(data);                               \
        break;
#define DEFINE_INT_TYPE(name, fromType, ffiType)                               \
      case TYPE_##name:                                                        \
        if (sizeof(fromType) > 4)                                              \
          return false;                                                        \
        if (sizeof(fromType) == 4 && !IsDoublePrecision<FloatType>())          \
          return false;                                                        \
        *result = *static_cast<fromType*>(data);                               \
        break;
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "typedefs.h"
      case TYPE_void_t:
      case TYPE_bool:
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char:
      case TYPE_jschar:
      case TYPE_pointer:
      case TYPE_array:
      case TYPE_struct:
        
        return false;
      }

      return true;
    }

    if (Int64::IsInt64(cx, obj)) {
      PRInt64 i = Int64Base::GetInt(cx, obj);
      *result = FloatType(i);
      return true;
    }

    if (UInt64::IsUInt64(cx, obj)) {
      PRUint64 i = Int64Base::GetInt(cx, obj);
      *result = FloatType(i);
      return true;
    }
  }
  
  
  return false;
}






template<class IntegerType>
static bool
jsvalToBigInteger(JSContext* cx,
                  jsval val,
                  bool allowString,
                  IntegerType* result)
{
  if (JSVAL_IS_INT(val)) {
    jsint i = JSVAL_TO_INT(val);
    *result = IntegerType(i);

    
    
    if (IsUnsigned<IntegerType>() && i < 0)
      return false;
    return jsint(*result) == i;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    jsdouble d = *JSVAL_TO_DOUBLE(val);
    *result = Convert<IntegerType>(d);

    
    
    if (IsUnsigned<IntegerType>() && d < 0)
      return false;
    return jsdouble(*result) == d;
  }
  if (allowString && JSVAL_IS_STRING(val)) {
    
    
    
    
    return StringToInteger(cx, JSVAL_TO_STRING(val), result);
  }
  if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val)) {
    
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (UInt64::IsUInt64(cx, obj)) {
      PRUint64 i = Int64Base::GetInt(cx, obj);
      *result = IntegerType(i);

      
      if (!IsUnsigned<IntegerType>() && *result < 0)
        return false;
      return PRUint64(*result) == i;
    }

    if (Int64::IsInt64(cx, obj)) {
      PRInt64 i = Int64Base::GetInt(cx, obj);
      *result = IntegerType(i);

      
      if (IsUnsigned<IntegerType>() && i < 0)
        return false;
      return PRInt64(*result) == i;
    }
  }
  return false;
}



static bool
jsvalToSize(JSContext* cx, jsval val, bool allowString, size_t* result)
{
  if (!jsvalToBigInteger(cx, val, allowString, result))
    return false;

  
  return Convert<size_t>(jsdouble(*result)) == *result;
}



static bool
SizeTojsval(JSContext* cx, size_t size, jsval* result)
{
  if (Convert<size_t>(jsdouble(size)) != size) {
    JS_ReportError(cx, "size overflow");
    return false;
  }

  return JS_NewNumberValue(cx, jsdouble(size), result) != JS_FALSE;
}


template<class IntegerType>
static bool
jsvalToIntegerExplicit(JSContext* cx, jsval val, IntegerType* result)
{
  if (JSVAL_IS_DOUBLE(val)) {
    
    jsdouble d = *JSVAL_TO_DOUBLE(val);
    *result = JSDOUBLE_IS_FINITE(d) ? IntegerType(d) : 0;
    return true;
  }
  if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val)) {
    
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (Int64::IsInt64(cx, obj)) {
      PRInt64 i = Int64Base::GetInt(cx, obj);
      *result = IntegerType(i);
      return true;
    }
    if (UInt64::IsUInt64(cx, obj)) {
      PRUint64 i = Int64Base::GetInt(cx, obj);
      *result = IntegerType(i);
      return true;
    }
  }
  return false;
}


static bool
jsvalToPtrExplicit(JSContext* cx, jsval val, uintptr_t* result)
{
  if (JSVAL_IS_INT(val)) {
    
    
    jsint i = JSVAL_TO_INT(val);
    *result = i < 0 ? uintptr_t(intptr_t(i)) : uintptr_t(i);
    return true;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    jsdouble d = *JSVAL_TO_DOUBLE(val);
    if (d < 0) {
      
      intptr_t i = Convert<intptr_t>(d);
      if (jsdouble(i) != d)
        return false;

      *result = uintptr_t(i);
      return true;
    }

    
    
    *result = Convert<uintptr_t>(d);
    return jsdouble(*result) == d;
  }
  if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (Int64::IsInt64(cx, obj)) {
      PRInt64 i = Int64Base::GetInt(cx, obj);
      if (i < 0) {
        
        if (PRInt64(intptr_t(i)) != i)
          return false;

        *result = uintptr_t(intptr_t(i));
        return true;
      }

      
      *result = uintptr_t(i);
      return PRInt64(*result) == i;
    }

    if (UInt64::IsUInt64(cx, obj)) {
      PRUint64 i = Int64Base::GetInt(cx, obj);

      
      *result = uintptr_t(i);
      return PRUint64(*result) == i;
    }
  }
  return false;
}

template<class IntegerType>
nsCAutoString
IntegerToString(IntegerType i, jsuint radix)
{
  
  
  char buffer[sizeof(IntegerType) * 8 + 1];
  char *cp = buffer + sizeof(buffer);

  
  
  bool isNegative = !IsUnsigned<IntegerType>() && i < 0;
  size_t sign = isNegative ? -1 : 1;
  do {
    IntegerType ii = i / IntegerType(radix);
    size_t index = sign * size_t(i - ii * IntegerType(radix));
    *--cp = "0123456789abcdefghijklmnopqrstuvwxyz"[index];
    i = ii;
  } while (i != 0);

  if (isNegative)
    *--cp = '-';

  JS_ASSERT(cp >= buffer);
  return nsCAutoString(cp, buffer + sizeof(buffer) - cp);
}

template<class IntegerType>
static bool
StringToInteger(JSContext* cx, JSString* string, IntegerType* result)
{
  const char* cp = JS_GetStringBytesZ(cx, string);
  if (!cp)
    return false;

  const char* end = cp + JS_GetStringLength(string);
  if (cp == end)
    return false;

  IntegerType sign = 1;
  if (cp[0] == '-') {
    if (IsUnsigned<IntegerType>())
      return false;

    sign = -1;
    ++cp;
  }

  
  IntegerType base = 10;
  if (end - cp > 2 && cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X')) {
    cp += 2;
    base = 16;
  }

  
  
  IntegerType i = 0;
  while (cp != end) {
    unsigned char c = *cp++;
    if (c >= '0' && c <= '9')
      c -= '0';
    else if (base == 16 && c >= 'a' && c <= 'f')
      c = c - 'a' + 10;
    else if (base == 16 && c >= 'A' && c <= 'F')
      c = c - 'A' + 10;
    else
      return false;

    IntegerType ii = i;
    i = ii * base + sign * c;
    if (i / base != ii) 
      return false;
  }

  *result = i;
  return true;
}

static bool
IsUTF16(const jschar* string, size_t length)
{
  PRBool error;
  const PRUnichar* buffer = reinterpret_cast<const PRUnichar*>(string);
  const PRUnichar* end = buffer + length;
  while (buffer != end) {
    UTF16CharEnumerator::NextChar(&buffer, end, &error);
    if (error)
      return false;
  }

  return true;
}

template<class CharType>
static size_t
strnlen(const CharType* begin, size_t max)
{
  for (const CharType* s = begin; s != begin + max; ++s)
    if (*s == 0)
      return s - begin;

  return max;
}











bool
ConvertToJS(JSContext* cx,
            JSObject* typeObj,
            JSObject* parentObj,
            void* data,
            bool wantPrimitive,
            jsval* result)
{
  JS_ASSERT(!parentObj || CData::IsCData(cx, parentObj));

  TypeCode typeCode = CType::GetTypeCode(cx, typeObj);

  switch (typeCode) {
  case TYPE_void_t:
    *result = JSVAL_VOID;
    break;
  case TYPE_bool:
    *result = *static_cast<bool*>(data) ? JSVAL_TRUE : JSVAL_FALSE;
    break;
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name: {                                                          \
    type value = *static_cast<type*>(data);                                    \
    if (sizeof(type) < 4)                                                      \
      *result = INT_TO_JSVAL(jsint(value));                                    \
    else if (!JS_NewNumberValue(cx, jsdouble(value), result))                  \
      return false;                                                            \
    break;                                                                     \
  }
#define DEFINE_WRAPPED_INT_TYPE(name, type, ffiType)                           \
  case TYPE_##name: {                                                          \
    /* Return an Int64 or UInt64 object - do not convert to a JS number. */    \
    PRUint64 value;                                                            \
    JSObject* proto;                                                           \
    if (IsUnsigned<type>()) {                                                  \
      value = *static_cast<type*>(data);                                       \
      /* Get ctypes.UInt64.prototype from ctypes.CType.prototype. */           \
      proto = CType::GetProtoFromType(cx, typeObj, SLOT_UINT64PROTO);          \
    } else {                                                                   \
      value = PRInt64(*static_cast<type*>(data));                              \
      /* Get ctypes.Int64.prototype from ctypes.CType.prototype. */            \
      proto = CType::GetProtoFromType(cx, typeObj, SLOT_INT64PROTO);           \
    }                                                                          \
                                                                               \
    JSObject* obj = Int64Base::Construct(cx, proto, value, IsUnsigned<type>());\
    if (!obj)                                                                  \
      return false;                                                            \
    *result = OBJECT_TO_JSVAL(obj);                                            \
    break;                                                                     \
  }
#define DEFINE_FLOAT_TYPE(name, type, ffiType)                                 \
  case TYPE_##name: {                                                          \
    type value = *static_cast<type*>(data);                                    \
    if (!JS_NewNumberValue(cx, jsdouble(value), result))                       \
      return false;                                                            \
    break;                                                                     \
  }
#include "typedefs.h"
  case TYPE_jschar: {
    
    JSString* str = JS_NewUCStringCopyN(cx, static_cast<jschar*>(data), 1);
    if (!str)
      return false;

    *result = STRING_TO_JSVAL(str);
    break;
  }
  case TYPE_char:
  case TYPE_signed_char:
  case TYPE_unsigned_char: {
    
    
    
    jschar promoted = *static_cast<unsigned char*>(data);
    JSString* str = JS_NewUCStringCopyN(cx, &promoted, 1);
    if (!str)
      return false;

    *result = STRING_TO_JSVAL(str);
    break;
  }
  case TYPE_pointer: {
    
    
    if (wantPrimitive) {
      JS_ReportError(cx, "cannot convert to primitive value");
      return false;
    }

    JSObject* obj = PointerType::ConstructInternal(cx, typeObj, parentObj, data);
    if (!obj)
      return false;

    *result = OBJECT_TO_JSVAL(obj);
    break;
  }
  case TYPE_array: {
    
    
    if (wantPrimitive) {
      JS_ReportError(cx, "cannot convert to primitive value");
      return false;
    }

    JSObject* obj = ArrayType::ConstructInternal(cx, typeObj, parentObj, data);
    if (!obj)
      return false;

    *result = OBJECT_TO_JSVAL(obj);
    break;
  }
  case TYPE_struct: {
    
    
    if (wantPrimitive) {
      JS_ReportError(cx, "cannot convert to primitive value");
      return false;
    }

    JSObject* obj = StructType::ConstructInternal(cx, typeObj, parentObj, data);
    if (!obj)
      return false;

    *result = OBJECT_TO_JSVAL(obj);
    break;
  }
  }

  return true;
}












bool
ImplicitConvert(JSContext* cx,
                jsval val,
                JSObject* targetType,
                void* buffer,
                bool isArgument,
                bool* freePointer)
{
  JS_ASSERT(CType::IsSizeDefined(cx, targetType));

  
  JSObject* sourceData = NULL;
  JSObject* sourceType = NULL;
  if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val) &&
      CData::IsCData(cx, JSVAL_TO_OBJECT(val))) {
    sourceData = JSVAL_TO_OBJECT(val);
    sourceType = CData::GetCType(cx, sourceData);

    
    
    if (CType::TypesEqual(cx, sourceType, targetType)) {
      size_t size = CType::GetSize(cx, sourceType);
      memmove(buffer, CData::GetData(cx, sourceData), size);
      return true;
    }
  }

  TypeCode targetCode = CType::GetTypeCode(cx, targetType);

  switch (targetCode) {
  case TYPE_bool: {
    
    
    bool result;
    if (!jsvalToBool(cx, val, &result))
      return TypeError(cx, "boolean", val);
    *static_cast<bool*>(buffer) = result;
    break;
  }
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name: {                                                          \
    /* Do not implicitly lose bits. */                                         \
    type result;                                                               \
    if (!jsvalToInteger(cx, val, &result))                                     \
      return TypeError(cx, #name, val);                                        \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_FLOAT_TYPE(name, type, ffiType)                                 \
  case TYPE_##name: {                                                          \
    type result;                                                               \
    if (!jsvalToFloat(cx, val, &result))                                       \
      return TypeError(cx, #name, val);                                        \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#define DEFINE_CHAR_TYPE(name, type, ffiType)                                  \
  case TYPE_##name: {                                                          \
    /* Convert from a 1-character string, regardless of encoding, */           \
    /* or from an integer, provided the result fits in 'type'. */              \
    /* TODO: Check IsASCII(chars) for 8-bit char types? */                     \
    type result;                                                               \
    if (JSVAL_IS_STRING(val)) {                                                \
      JSString* str = JSVAL_TO_STRING(val);                                    \
      if (JS_GetStringLength(str) != 1)                                        \
        return TypeError(cx, #name, val);                                      \
                                                                               \
      jschar c = *JS_GetStringCharsZ(cx, str);                                 \
      result = c;                                                              \
      if (jschar(result) != c)                                                 \
        return TypeError(cx, #name, val);                                      \
                                                                               \
    } else if (!jsvalToInteger(cx, val, &result)) {                            \
      return TypeError(cx, #name, val);                                        \
    }                                                                          \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#include "typedefs.h"
  case TYPE_pointer: {
    JSObject* baseType = PointerType::GetBaseType(cx, targetType);

    if (JSVAL_IS_NULL(val)) {
      
      *static_cast<void**>(buffer) = NULL;
      break;

    } else if (sourceData) {
      
      TypeCode sourceCode = CType::GetTypeCode(cx, sourceType);
      void* sourceBuffer = CData::GetData(cx, sourceData);
      bool voidptrTarget = baseType &&
                           CType::GetTypeCode(cx, baseType) == TYPE_void_t;

      if (sourceCode == TYPE_pointer && voidptrTarget) {
        
        *static_cast<void**>(buffer) = *static_cast<void**>(sourceBuffer);
        break;
      }
      if (sourceCode == TYPE_array) {
        
        
        JSObject* elementType = ArrayType::GetBaseType(cx, sourceType);
        if (voidptrTarget || CType::TypesEqual(cx, baseType, elementType)) {
          *static_cast<void**>(buffer) = sourceBuffer;
          break;
        }
      }

    } else if (isArgument && baseType && JSVAL_IS_STRING(val)) {
      
      
      
      JSString* sourceString = JSVAL_TO_STRING(val);
      const jschar* sourceChars = JS_GetStringCharsZ(cx, sourceString);
      size_t sourceLength = JS_GetStringLength(sourceString);

      switch (CType::GetTypeCode(cx, baseType)) {
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char: {
        
        if (!IsUTF16(sourceChars, sourceLength))
          return TypeError(cx, "UTF-16 string", val);

        NS_ConvertUTF16toUTF8 converted(
          reinterpret_cast<const PRUnichar*>(sourceChars), sourceLength);

        char** charBuffer = static_cast<char**>(buffer);
        *charBuffer = new char[converted.Length() + 1];
        if (!*charBuffer) {
          JS_ReportAllocationOverflow(cx);
          return false;
        }

        *freePointer = true;
        memcpy(*charBuffer, converted.get(), converted.Length() + 1);
        break;
      }
      case TYPE_jschar: {
        
        
        
        jschar** jscharBuffer = static_cast<jschar**>(buffer);
        *jscharBuffer = new jschar[sourceLength + 1];
        if (!*jscharBuffer) {
          JS_ReportAllocationOverflow(cx);
          return false;
        }

        *freePointer = true;
        memcpy(*jscharBuffer, sourceChars, (sourceLength + 1) * sizeof(jschar));
        break;
      }
      default:
        return TypeError(cx, "pointer", val);
      }
      break;
    }
    return TypeError(cx, "pointer", val);
  }
  case TYPE_array: {
    JSObject* baseType = ArrayType::GetBaseType(cx, targetType);
    size_t targetLength = ArrayType::GetLength(cx, targetType);

    if (JSVAL_IS_STRING(val)) {
      JSString* sourceString = JSVAL_TO_STRING(val);
      const jschar* sourceChars = JS_GetStringCharsZ(cx, sourceString);
      size_t sourceLength = JS_GetStringLength(sourceString);

      switch (CType::GetTypeCode(cx, baseType)) {
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char: {
        
        if (!IsUTF16(sourceChars, sourceLength))
          return TypeError(cx, "UTF-16 string", val);

        NS_ConvertUTF16toUTF8 converted(
          reinterpret_cast<const PRUnichar*>(sourceChars), sourceLength);

        if (targetLength < converted.Length()) {
          JS_ReportError(cx, "ArrayType has insufficient length");
          return false;
        }

        memcpy(buffer, converted.get(), converted.Length());
        if (targetLength > converted.Length())
          static_cast<char*>(buffer)[converted.Length()] = 0;

        break;
      }
      case TYPE_jschar: {
        
        
        if (targetLength < sourceLength) {
          JS_ReportError(cx, "ArrayType has insufficient length");
          return false;
        }

        memcpy(buffer, sourceChars, sourceLength * sizeof(jschar));
        if (targetLength > sourceLength)
          static_cast<jschar*>(buffer)[sourceLength] = 0;

        break;
      }
      default:
        return TypeError(cx, "array", val);
      }

    } else if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val) &&
               JS_IsArrayObject(cx, JSVAL_TO_OBJECT(val))) {
      
      JSObject* sourceArray = JSVAL_TO_OBJECT(val);
      jsuint sourceLength;
      if (!JS_GetArrayLength(cx, sourceArray, &sourceLength) ||
          targetLength != size_t(sourceLength)) {
        JS_ReportError(cx, "ArrayType length does not match source array length");
        return false;
      }

      
      size_t elementSize = CType::GetSize(cx, baseType);
      size_t arraySize = elementSize * targetLength;
      nsAutoPtr<char> intermediate(new char[arraySize]);
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      for (jsuint i = 0; i < sourceLength; ++i) {
        jsval item;
        if (!JS_GetElement(cx, sourceArray, i, &item))
          return false;

        char* data = intermediate + elementSize * i;
        if (!ImplicitConvert(cx, item, baseType, data, false, NULL))
          return false;
      }

      memcpy(buffer, intermediate, arraySize);

    } else {
      
      
      return TypeError(cx, "array", val);
    }
    break;
  }
  case TYPE_struct: {
    if (JSVAL_IS_OBJECT(val) && !JSVAL_IS_NULL(val) && !sourceData) {
      nsTArray<FieldInfo>* fields = StructType::GetFieldInfo(cx, targetType);

      
      
      JSObject* obj = JSVAL_TO_OBJECT(val);
      JSObject* iter = JS_NewPropertyIterator(cx, obj);
      if (!iter)
        return false;
      JSAutoTempValueRooter iterroot(cx, iter);

      
      size_t structSize = CType::GetSize(cx, targetType);
      nsAutoPtr<char> intermediate(new char[structSize]);
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      jsid id;
      jsuint i = 0;
      while (JS_NextProperty(cx, iter, &id) && !JSVAL_IS_VOID(id)) {
        jsval fieldVal;
        if (!JS_IdToValue(cx, id, &fieldVal) || !JSVAL_IS_STRING(fieldVal)) {
          JS_ReportError(cx, "property name is not a string");
          return false;
        }
        JSAutoTempValueRooter nameroot(cx, fieldVal);

        const char* name = JS_GetStringBytesZ(cx, JSVAL_TO_STRING(fieldVal));
        FieldInfo* field = StructType::LookupField(cx, targetType, fieldVal);
        if (!field) {
          JS_ReportError(cx, "couldn't locate field %s", name);
          return false;
        }

        jsval prop;
        if (!JS_GetProperty(cx, obj, name, &prop))
          return false;

        
        char* fieldData = intermediate + field->mOffset;
        if (!ImplicitConvert(cx, prop, field->mType, fieldData, false, NULL))
          return false;

        ++i;
      }

      if (i != fields->Length()) {
        JS_ReportError(cx, "missing fields");
        return false;
      }

      memcpy(buffer, intermediate, structSize);
      break;
    }

    return TypeError(cx, "struct", val);
  }
  case TYPE_void_t:
    JS_NOT_REACHED("invalid type");
    return false;
  }

  return true;
}




bool
ExplicitConvert(JSContext* cx, jsval val, JSObject* targetType, void* buffer)
{
  
  if (ImplicitConvert(cx, val, targetType, buffer, false, NULL))
    return true;

  
  
  
  jsval ex;
  if (!JS_GetPendingException(cx, &ex))
    return false;

  
  
  JSAutoTempValueRooter root(cx, ex);
  JS_ClearPendingException(cx);

  TypeCode type = CType::GetTypeCode(cx, targetType);

  switch (type) {
  case TYPE_bool: {
    
    JSBool result;
    JS_ValueToBoolean(cx, val, &result);
    *static_cast<bool*>(buffer) = result != JS_FALSE;
    break;
  }
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name: {                                                          \
    /* Convert numeric values with a C-style cast. */                          \
    type result;                                                               \
    if (!jsvalToIntegerExplicit(cx, val, &result))                             \
      return TypeError(cx, #name, val);                                        \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_WRAPPED_INT_TYPE(name, type, ffiType)                           \
  case TYPE_##name: {                                                          \
    /* Convert numeric values with a C-style cast, and */                      \
    /* allow conversion from a base-10 or base-16 string. */                   \
    type result;                                                               \
    if (!jsvalToIntegerExplicit(cx, val, &result) &&                           \
        (!JSVAL_IS_STRING(val) ||                                              \
         !StringToInteger(cx, JSVAL_TO_STRING(val), &result)))                 \
      return TypeError(cx, #name, val);                                        \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#include "typedefs.h"
  case TYPE_pointer: {
    
    uintptr_t result;
    if (!jsvalToPtrExplicit(cx, val, &result))
      return TypeError(cx, "pointer", val);
    *static_cast<uintptr_t*>(buffer) = result;
    break;
  }
  case TYPE_float32_t:
  case TYPE_float64_t:
  case TYPE_float:
  case TYPE_double:
  case TYPE_array:
  case TYPE_struct:
    
    JS_SetPendingException(cx, ex);
    return false;
  case TYPE_void_t:
    JS_NOT_REACHED("invalid type");
    return false;
  }
  return true;
}





static nsCAutoString
BuildTypeName(JSContext* cx, JSObject* typeObj)
{
  
  
  
  
  
  
  nsCAutoString result;
  JSObject* currentType = typeObj;
  JSObject* nextType;
  TypeCode prevGrouping = CType::GetTypeCode(cx, currentType), currentGrouping;
  while (1) {
    currentGrouping = CType::GetTypeCode(cx, currentType);
    switch (currentGrouping) {
    case TYPE_pointer: {
      nextType = PointerType::GetBaseType(cx, currentType);
      if (!nextType) {
        
        break;
      }

      
      result.Insert('*', 0);

      currentType = nextType;
      prevGrouping = currentGrouping;
      continue;
    }
    case TYPE_array: {
      if (prevGrouping == TYPE_pointer) {
        
        result.Insert('(', 0);
        result.Append(')');
      }

      
      result.Append('[');
      size_t length;
      if (ArrayType::GetSafeLength(cx, currentType, &length)) {
        result.Append(IntegerToString(length, 10));
      }
      result.Append(']');

      currentType = ArrayType::GetBaseType(cx, currentType);
      prevGrouping = currentGrouping;
      continue;
    }
    default:
      
      break;
    }
    break;
  }

  
  JSString* baseName = CType::GetName(cx, currentType);
  result.Insert(JS_GetStringBytesZ(cx, baseName), 0);
  return result;
}








static nsCAutoString
BuildTypeSource(JSContext* cx, JSObject* typeObj, bool makeShort)
{
  
  nsCAutoString result;
  switch (CType::GetTypeCode(cx, typeObj)) {
  case TYPE_void_t:
    result.Append("ctypes.void_t");
    break;
#define DEFINE_TYPE(name, type, ffiType)  \
  case TYPE_##name:                       \
    result.Append("ctypes." #name);       \
    break;
#include "typedefs.h"
  case TYPE_pointer: {
    JSObject* baseType = PointerType::GetBaseType(cx, typeObj);
    if (!baseType) {
      
      JSString* baseName = CType::GetName(cx, typeObj);
      result.Append("ctypes.PointerType(\"");
      result.Append(JS_GetStringBytesZ(cx, baseName));
      result.Append('"');
      break;
    }

    
    if (CType::GetTypeCode(cx, baseType) == TYPE_void_t) {
      result.Append("ctypes.voidptr_t");
      break;
    }

    
    result.Append(BuildTypeSource(cx, baseType, makeShort));
    result.Append(".ptr");
    break;
  }
  case TYPE_array: {
    
    
    
    JSObject* baseType = ArrayType::GetBaseType(cx, typeObj);
    result.Append(BuildTypeSource(cx, baseType, makeShort));
    result.Append(".array(");

    size_t length;
    if (ArrayType::GetSafeLength(cx, typeObj, &length))
      result.Append(IntegerToString(length, 10));

    result.Append(')');
    break;
  }
  case TYPE_struct: {
    JSString* name = CType::GetName(cx, typeObj);

    if (makeShort) {
      
      
      result.Append(JS_GetStringBytesZ(cx, name));
      break;
    }

    
    result.Append("ctypes.StructType(\"");
    result.Append(JS_GetStringBytesZ(cx, name));
    result.Append("\", [");

    nsTArray<FieldInfo>* fields = StructType::GetFieldInfo(cx, typeObj);
    for (PRUint32 i = 0; i < fields->Length(); ++i) {
      const FieldInfo& field = fields->ElementAt(i);
      result.Append("{ \"");
      result.Append(field.mName);
      result.Append("\": ");
      result.Append(BuildTypeSource(cx, field.mType, makeShort));
      result.Append(" }");
      if (i != fields->Length() - 1)
        result.Append(", ");
    }

    result.Append("])");
    break;
  }
  }

  return result;
}











static nsCAutoString
BuildDataSource(JSContext* cx, JSObject* typeObj, void* data, bool isImplicit)
{
  nsCAutoString result;
  TypeCode type = CType::GetTypeCode(cx, typeObj);
  switch (type) {
  case TYPE_bool:
    result.Append(*static_cast<bool*>(data) ? "true" : "false");
    break;
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name:                                                            \
    /* Serialize as a primitive decimal integer. */                            \
    result.Append(IntegerToString(*static_cast<type*>(data), 10));             \
    break;
#define DEFINE_WRAPPED_INT_TYPE(name, type, ffiType)                           \
  case TYPE_##name:                                                            \
    /* Serialize as a wrapped decimal integer. */                              \
    if (IsUnsigned<type>())                                                    \
      result.Append("ctypes.UInt64(\"");                                       \
    else                                                                       \
      result.Append("ctypes.Int64(\"");                                        \
                                                                               \
    result.Append(IntegerToString(*static_cast<type*>(data), 10));             \
    result.Append("\")");                                                      \
    break;
#define DEFINE_FLOAT_TYPE(name, type, ffiType)                                 \
  case TYPE_##name: {                                                          \
    /* Serialize as a primitive double. */                                     \
    PRFloat64 fp = *static_cast<type*>(data);                                  \
    PRIntn decpt, sign;                                                        \
    char buf[128];                                                             \
    PR_dtoa(fp, 0, 0, &decpt, &sign, NULL, buf, sizeof(buf));                  \
    result.Append(buf);                                                        \
    break;                                                                     \
  }
#define DEFINE_CHAR_TYPE(name, type, ffiType)                                  \
  case TYPE_##name:                                                            \
    /* Serialize as an integer. */                                             \
    result.Append(IntegerToString(*static_cast<type*>(data), 10));             \
    break;
#include "typedefs.h"
  case TYPE_pointer: {
    if (isImplicit) {
      
      
      result.Append(BuildTypeSource(cx, typeObj, true));
      result.Append('(');
    }

    
    uintptr_t ptr = *static_cast<uintptr_t*>(data);
    result.Append("ctypes.UInt64(\"0x");
    result.Append(IntegerToString(ptr, 16));
    result.Append("\")");

    if (isImplicit)
      result.Append(')');

    break;
  }
  case TYPE_array: {
    
    
    JSObject* baseType = ArrayType::GetBaseType(cx, typeObj);
    result.Append("[ ");


    size_t length = ArrayType::GetLength(cx, typeObj);
    size_t elementSize = CType::GetSize(cx, baseType);
    for (size_t i = 0; i < length; ++i) {
      char* element = static_cast<char*>(data) + elementSize * i;
      result.Append(BuildDataSource(cx, baseType, element, true));
      if (i + 1 < length)
        result.Append(", ");
    }
    result.Append(" ]");
    break;
  }
  case TYPE_struct: {
    if (isImplicit) {
      
      
      
      result.Append("{ ");
    }

    
    
    nsTArray<FieldInfo>* fields = StructType::GetFieldInfo(cx, typeObj);
    for (size_t i = 0; i < fields->Length(); ++i) {
      const FieldInfo& field = fields->ElementAt(i);
      char* fieldData = static_cast<char*>(data) + field.mOffset;

      if (isImplicit) {
        result.Append('"');
        result.Append(field.mName);
        result.Append("\": ");
      }

      result.Append(BuildDataSource(cx, field.mType, fieldData, true));
      if (i + 1 != fields->Length())
        result.Append(", ");
    }

    if (isImplicit)
      result.Append(" }");

    break;
  }
  case TYPE_void_t:
    JS_NOT_REACHED("invalid type");
    break;
  }

  return result;
}





JSBool
CType::ConstructAbstract(JSContext* cx,
                         JSObject* obj,
                         uintN argc,
                         jsval* argv,
                         jsval* rval)
{
  
  JS_ReportError(cx, "cannot construct from abstract type");
  return JS_FALSE;
}

JSBool
CType::ConstructData(JSContext* cx,
                     JSObject* obj,
                     uintN argc,
                     jsval* argv,
                     jsval* rval)
{
  
  obj = JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv));
  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  
  
  
  
  switch (GetTypeCode(cx, obj)) {
  case TYPE_void_t:
    JS_ReportError(cx, "cannot construct from void_t");
    return JS_FALSE;
  case TYPE_pointer:
    return PointerType::ConstructData(cx, obj, argc, argv, rval);
  case TYPE_array:
    return ArrayType::ConstructData(cx, obj, argc, argv, rval);
  case TYPE_struct:
    return StructType::ConstructData(cx, obj, argc, argv, rval);
  default:
    return ConstructBasic(cx, obj, argc, argv, rval);
  }
}

JSBool
CType::ConstructBasic(JSContext* cx,
                      JSObject* obj,
                      uintN argc,
                      jsval* argv,
                      jsval* rval)
{
  if (argc > 1) {
    JS_ReportError(cx, "CType constructor takes zero or one argument");
    return JS_FALSE;
  }

  
  JSObject* result = CData::Create(cx, obj, NULL, NULL);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);

  if (argc == 1) {
    
    if (!ExplicitConvert(cx, argv[0], obj, CData::GetData(cx, result)))
      return JS_FALSE;
  }

  return JS_TRUE;
}

JSObject*
CType::Create(JSContext* cx,
              JSObject* proto,
              JSString* name,
              TypeCode type,
              jsval size,
              jsval align,
              ffi_type* ffiType)
{
  
  
  
  
  
  
  
  
  
  
  
  JSObject* typeObj = JS_NewObject(cx, &sCTypeClass, proto, NULL);
  if (!typeObj)
    return NULL;
  JSAutoTempValueRooter root(cx, typeObj);

  
  if (name && 
      !JS_DefineProperty(cx, typeObj, "name", STRING_TO_JSVAL(name),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;
  if (!JS_DefineProperty(cx, typeObj, "ptr", JSVAL_VOID,
         PtrGetter, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;
  if (!JS_DefineProperty(cx, typeObj, "size", size,
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_TYPECODE, INT_TO_JSVAL(type)))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_FFITYPE, PRIVATE_TO_JSVAL(ffiType)))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_ALIGN, align))
    return NULL;

  
  JSObject* prototype = JS_NewObject(cx, NULL, NULL, typeObj);
  if (!prototype)
    return NULL;

  if (!JS_DefineProperty(cx, typeObj, "prototype", OBJECT_TO_JSVAL(prototype),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  
  if (!JS_SealObject(cx, prototype, JS_FALSE))
    return NULL;

  return typeObj;
}

JSObject*
CType::DefineBuiltin(JSContext* cx,
                     JSObject* parent,
                     const char* propName,
                     JSObject* proto,
                     const char* name,
                     TypeCode type,
                     jsval size,
                     jsval align,
                     ffi_type* ffiType)
{
  JSString* nameStr = JS_NewStringCopyZ(cx, name);
  if (!nameStr)
    return NULL;
  JSAutoTempValueRooter nameRoot(cx, nameStr);

  
  JSObject* typeObj = Create(cx, proto, nameStr, type, size, align, ffiType);
  if (!typeObj)
    return NULL;

  
  if (!JS_DefineProperty(cx, parent, propName, OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_SealObject(cx, typeObj, JS_FALSE))
    return NULL;

  return typeObj;
}

void
CType::Finalize(JSContext* cx, JSObject* obj)
{
  
  jsval slot;
  if (!JS_GetReservedSlot(cx, obj, SLOT_TYPECODE, &slot) || JSVAL_IS_VOID(slot))
    return;

  
  switch (GetTypeCode(cx, obj)) {
  case TYPE_struct:
    
    JS_GetReservedSlot(cx, obj, SLOT_FIELDINFO, &slot);
    if (!JSVAL_IS_VOID(slot))
      delete static_cast<nsTArray<FieldInfo>*>(JSVAL_TO_PRIVATE(slot));

    
  case TYPE_array: {
    
    jsval slot;
    JS_GetReservedSlot(cx, obj, SLOT_FFITYPE, &slot);
    if (!JSVAL_IS_VOID(slot) && JSVAL_TO_PRIVATE(slot)) {
      ffi_type* ffiType = static_cast<ffi_type*>(JSVAL_TO_PRIVATE(slot));
      delete ffiType->elements;
      delete ffiType;
    }

    break;
  }
  default:
    
    break;
  }
}

bool
CType::IsCType(JSContext* cx, JSObject* obj)
{
  return JS_GET_CLASS(cx, obj) == &sCTypeClass;
}

TypeCode
CType::GetTypeCode(JSContext* cx, JSObject* typeObj)
{
  JS_ASSERT(IsCType(cx, typeObj));

  jsval result;
  JS_GetReservedSlot(cx, typeObj, SLOT_TYPECODE, &result);
  return TypeCode(JSVAL_TO_INT(result));
}

bool
CType::TypesEqual(JSContext* cx, JSObject* t1, JSObject* t2)
{
  JS_ASSERT(IsCType(cx, t1) && IsCType(cx, t2));

  
  TypeCode c1 = GetTypeCode(cx, t1);
  TypeCode c2 = GetTypeCode(cx, t2);
  if (c1 != c2)
    return false;

  
  switch (c1) {
  case TYPE_pointer: {
    JSObject* b1 = PointerType::GetBaseType(cx, t1);
    JSObject* b2 = PointerType::GetBaseType(cx, t2);

    if (!b1 || !b2) {
      
      
      JSString* n1 = GetName(cx, t1);
      JSString* n2 = GetName(cx, t2);
      return b1 == b2 && JS_CompareStrings(n1, n2) == 0;
    }

    
    return TypesEqual(cx, b1, b2);
  }
  case TYPE_array: {
    
    
    size_t s1, s2;
    bool d1 = ArrayType::GetSafeLength(cx, t1, &s1);
    bool d2 = ArrayType::GetSafeLength(cx, t2, &s2);
    if (d1 && d2 && s1 != s2)
      return false;

    JSObject* b1 = ArrayType::GetBaseType(cx, t1);
    JSObject* b2 = ArrayType::GetBaseType(cx, t2);
    return TypesEqual(cx, b1, b2);
  }
  case TYPE_struct:
    
    return t1 == t2;
  default:
    
    return true;
  }
}

bool
CType::GetSafeSize(JSContext* cx, JSObject* obj, size_t* result)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval size;
  JS_GetProperty(cx, obj, "size", &size);

  
  
  if (JSVAL_IS_INT(size)) {
    *result = JSVAL_TO_INT(size);
    return true;
  }
  if (JSVAL_IS_DOUBLE(size)) {
    *result = Convert<size_t>(*JSVAL_TO_DOUBLE(size));
    return true;
  }

  JS_ASSERT(JSVAL_IS_VOID(size));
  return false;
}

size_t
CType::GetSize(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval size;
  JS_GetProperty(cx, obj, "size", &size);

  JS_ASSERT(!JSVAL_IS_VOID(size));

  
  
  
  if (JSVAL_IS_INT(size))
    return JSVAL_TO_INT(size);
  return Convert<size_t>(*JSVAL_TO_DOUBLE(size));
}

bool
CType::IsSizeDefined(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval size;
  JS_GetProperty(cx, obj, "size", &size);

  
  
  JS_ASSERT(JSVAL_IS_INT(size) || JSVAL_IS_DOUBLE(size) || JSVAL_IS_VOID(size));
  return !JSVAL_IS_VOID(size);
}

size_t
CType::GetAlignment(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval slot;
  JS_GetReservedSlot(cx, obj, SLOT_ALIGN, &slot);
  return static_cast<size_t>(JSVAL_TO_INT(slot));
}

ffi_type*
CType::GetFFIType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval slot;
  JS_GetReservedSlot(cx, obj, SLOT_FFITYPE, &slot);

  ffi_type* result = static_cast<ffi_type*>(JSVAL_TO_PRIVATE(slot));
  JS_ASSERT(result);
  return result;
}

JSString*
CType::GetName(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval string;
  JS_GetProperty(cx, obj, "name", &string);
  return JSVAL_TO_STRING(string);
}

JSObject*
CType::GetProtoFromCtor(JSContext* cx, JSObject* obj, CTypeProtoSlot slot)
{
  
  jsval prototype;
  JS_GetProperty(cx, obj, "prototype", &prototype);
  JS_ASSERT(JSVAL_IS_OBJECT(prototype) && !JSVAL_IS_NULL(prototype));

  
  JSObject* proto = JS_GetPrototype(cx, JSVAL_TO_OBJECT(prototype));
  JS_ASSERT(proto);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sCTypeProto);

  
  jsval result;
  JS_GetReservedSlot(cx, proto, slot, &result);
  return JSVAL_TO_OBJECT(result);
}

JSObject*
CType::GetProtoFromType(JSContext* cx, JSObject* obj, CTypeProtoSlot slot)
{
  JS_ASSERT(IsCType(cx, obj));

  
  JSObject* proto = JS_GetPrototype(cx, obj);
  JS_ASSERT(proto);

  if (JS_GET_CLASS(cx, proto) != &sCTypeProto) {
    
    proto = JS_GetPrototype(cx, proto);
    JS_ASSERT(proto);
    JS_ASSERT(JS_GET_CLASS(cx, proto) == &sCTypeProto);
  }

  
  jsval result;
  JS_GetReservedSlot(cx, proto, slot, &result);
  return JSVAL_TO_OBJECT(result);
}

JSBool
CType::PtrGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  JSObject* pointerType = PointerType::CreateInternal(cx, NULL, obj, NULL);
  if (!pointerType)
    return JS_FALSE;

  *vp = OBJECT_TO_JSVAL(pointerType);
  return JS_TRUE;
}

JSBool
CType::Array(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* baseType = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(baseType);

  if (!CType::IsCType(cx, baseType)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  
  if (argc > 1) {
    JS_ReportError(cx, "array takes zero or one argument");
    return JS_FALSE;
  }

  
  jsval* argv = JS_ARGV(cx, vp);
  size_t length = 0;
  if (argc == 1 && !jsvalToSize(cx, argv[0], false, &length)) {
    JS_ReportError(cx, "argument must be a nonnegative integer");
    return JS_FALSE;
  }

  JSObject* result = ArrayType::CreateInternal(cx, baseType, length, argc == 1);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CType::ToString(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(obj);

  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  nsCAutoString type("type ");
  JSString* right = GetName(cx, obj);
  type.Append(JS_GetStringBytesZ(cx, right));

  JSString* result = JS_NewStringCopyN(cx, type.get(), type.Length());
  if (!result)
    return JS_FALSE;
  
  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CType::ToSource(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(obj);

  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  nsCAutoString source = BuildTypeSource(cx, obj, false);
  JSString* result = JS_NewStringCopyN(cx, source.get(), source.Length());
  if (!result)
    return JS_FALSE;
  
  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}





JSBool
PointerType::Create(JSContext* cx, uintN argc, jsval* vp)
{
  
  if (argc != 1) {
    JS_ReportError(cx, "PointerType takes one argument");
    return JS_FALSE;
  }

  jsval arg = JS_ARGV(cx, vp)[0];
  JSObject* baseType = NULL;
  JSString* name = NULL;
  if (JSVAL_IS_OBJECT(arg) && !JSVAL_IS_NULL(arg) &&
      CType::IsCType(cx, JSVAL_TO_OBJECT(arg))) {
    baseType = JSVAL_TO_OBJECT(arg);

  } else if (JSVAL_IS_STRING(arg)) {
    
    name = JSVAL_TO_STRING(arg);

  } else {
    JS_ReportError(cx, "first argument must be a CType or a string");
    return JS_FALSE;
  }

  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  JSObject* result = CreateInternal(cx, callee, baseType, name);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSObject*
PointerType::CreateInternal(JSContext* cx,
                            JSObject* ctor,
                            JSObject* baseType,
                            JSString* name)
{
  JS_ASSERT(ctor || baseType);
  JS_ASSERT((baseType && !name) || (!baseType && name));

  if (baseType) {
    
    jsval slot;
    JS_GetReservedSlot(cx, baseType, SLOT_PTR, &slot);
    if (JSVAL_IS_OBJECT(slot))
      return JSVAL_TO_OBJECT(slot);
  }

  
  
  JSObject* proto;
  if (ctor)
    proto = CType::GetProtoFromCtor(cx, ctor, SLOT_POINTERPROTO);
  else
    proto = CType::GetProtoFromType(cx, baseType, SLOT_POINTERPROTO);

  
  JSObject* typeObj = CType::Create(cx, proto, name, TYPE_pointer,
                        INT_TO_JSVAL(sizeof(void*)),
                        INT_TO_JSVAL(ffi_type_pointer.alignment),
                        &ffi_type_pointer);
  if (!typeObj)
    return NULL;
  JSAutoTempValueRooter root(cx, typeObj);

  
  if (!JS_DefineProperty(cx, typeObj, "targetType", OBJECT_TO_JSVAL(baseType),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (baseType) {
    
    nsCAutoString typeName = BuildTypeName(cx, typeObj);
    name = JS_NewStringCopyN(cx, typeName.get(), typeName.Length());
    if (!name ||
        !JS_DefineProperty(cx, typeObj, "name", STRING_TO_JSVAL(name),
           NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
      return NULL;
  }

  
  
  if (baseType &&
      !JS_SetReservedSlot(cx, baseType, SLOT_PTR, OBJECT_TO_JSVAL(typeObj)))
    return NULL;

  if (!JS_SealObject(cx, typeObj, JS_FALSE))
    return NULL;

  return typeObj;
}

JSBool
PointerType::ConstructData(JSContext* cx,
                           JSObject* obj,
                           uintN argc,
                           jsval* argv,
                           jsval* rval)
{
  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  if (argc > 1) {
    JS_ReportError(cx, "constructor takes zero or one argument");
    return JS_FALSE;
  }

  JSObject* result = ConstructInternal(cx, obj, NULL, NULL);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);

  if (argc == 1) {
    
    if (!ExplicitConvert(cx, argv[0], obj, CData::GetData(cx, result)))
      return JS_FALSE;
  }

  return JS_TRUE;
}

JSObject*
PointerType::ConstructInternal(JSContext* cx,
                               JSObject* typeObj,
                               JSObject* parentObj,
                               void* data)
{
  
  JSObject* result = CData::Create(cx, typeObj, parentObj, data);
  if (!result)
    return NULL;
  JSAutoTempValueRooter root(cx, result);

  if (!JS_DefineProperty(cx, result, "contents", JSVAL_VOID,
         PointerType::ContentsGetter, PointerType::ContentsSetter,
         JSPROP_ENUMERATE | JSPROP_PERMANENT))
    return NULL;

  return result;
}

JSObject*
PointerType::GetBaseType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_pointer);

  jsval type;
  JS_GetProperty(cx, obj, "targetType", &type);
  return JSVAL_TO_OBJECT(type);
}

JSBool
PointerType::ContentsGetter(JSContext* cx,
                            JSObject* obj,
                            jsval idval,
                            jsval* vp)
{
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(cx, typeObj);
  if (!baseType) {
    JS_ReportError(cx, "cannot get contents of an opaque pointer type");
    return JS_FALSE;
  }

  if (!CType::IsSizeDefined(cx, baseType)) {
    JS_ReportError(cx, "cannot get contents of undefined size");
    return JS_FALSE;
  }

  void* data = *static_cast<void**>(CData::GetData(cx, obj));
  if (data == NULL) {
    JS_ReportError(cx, "cannot read contents of null pointer");
    return JS_FALSE;
  }

  jsval result;
  if (!ConvertToJS(cx, baseType, obj, data, false, &result))
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
PointerType::ContentsSetter(JSContext* cx,
                            JSObject* obj,
                            jsval idval,
                            jsval* vp)
{
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(cx, typeObj);
  if (!baseType) {
    JS_ReportError(cx, "cannot set contents of an opaque pointer type");
    return JS_FALSE;
  }

  if (!CType::IsSizeDefined(cx, baseType)) {
    JS_ReportError(cx, "cannot get contents of undefined size");
    return JS_FALSE;
  }

  void* data = *static_cast<void**>(CData::GetData(cx, obj));
  if (data == NULL) {
    JS_ReportError(cx, "cannot write contents to null pointer");
    return JS_FALSE;
  }

  return ImplicitConvert(cx, *vp, baseType, data, false, NULL);
}





JSBool
ArrayType::Create(JSContext* cx, uintN argc, jsval* vp)
{
  
  if (argc > 2) {
    JS_ReportError(cx, "ArrayType takes one or two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (!JSVAL_IS_OBJECT(argv[0]) || JSVAL_IS_NULL(argv[0] ||
      !CType::IsCType(cx, JSVAL_TO_OBJECT(argv[0])))) {
    JS_ReportError(cx, "first argument must be a CType");
    return JS_FALSE;
  }

  
  size_t length = 0;
  if (argc == 2 && !jsvalToSize(cx, argv[1], false, &length)) {
    JS_ReportError(cx, "second argument must be a nonnegative integer");
    return JS_FALSE;
  }

  JSObject* baseType = JSVAL_TO_OBJECT(argv[0]);
  JSObject* result = CreateInternal(cx, baseType, length, argc == 2);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSObject*
ArrayType::CreateInternal(JSContext* cx,
                          JSObject* baseType,
                          size_t length,
                          bool lengthDefined)
{
  
  JSObject* proto = CType::GetProtoFromType(cx, baseType, SLOT_ARRAYPROTO);

  
  
  
  size_t baseSize;
  if (!CType::GetSafeSize(cx, baseType, &baseSize)) {
    JS_ReportError(cx, "base size must be defined");
    return NULL;
  }

  size_t size;
  jsval sizeVal = JSVAL_VOID;
  jsval lengthVal = JSVAL_VOID;
  if (lengthDefined) {
    
    size = length * baseSize;
    if (length > 0 && size / length != baseSize) {
      JS_ReportError(cx, "size overflow");
      return NULL;
    }
    if (!SizeTojsval(cx, size, &sizeVal) ||
        !SizeTojsval(cx, length, &lengthVal))
      return NULL;
  }

  size_t align = CType::GetAlignment(cx, baseType);

  ffi_type* ffiType = NULL;
  if (lengthDefined) {
    
    
    
    
    
    ffiType = new ffi_type;
    if (!ffiType) {
      JS_ReportOutOfMemory(cx);
      return NULL;
    }

    ffiType->type = FFI_TYPE_STRUCT;
    ffiType->size = size;
    ffiType->alignment = align;
    ffiType->elements = new ffi_type*[length + 1];
    if (!ffiType->elements) {
      delete ffiType;
      JS_ReportAllocationOverflow(cx);
      return NULL;
    }

    ffi_type* ffiBaseType = CType::GetFFIType(cx, baseType);
    for (size_t i = 0; i < length; ++i)
      ffiType->elements[i] = ffiBaseType;
    ffiType->elements[length] = NULL;
  }

  
  JSObject* typeObj = CType::Create(cx, proto, NULL, TYPE_array,
                        sizeVal, INT_TO_JSVAL(align), ffiType);
  if (!typeObj)
    return NULL;
  JSAutoTempValueRooter root(cx, typeObj);

  
  if (!JS_DefineProperty(cx, typeObj, "length", lengthVal,
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;
  if (!JS_DefineProperty(cx, typeObj, "elementType", OBJECT_TO_JSVAL(baseType),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  
  nsCAutoString typeName = BuildTypeName(cx, typeObj);
  JSString* name = JS_NewStringCopyN(cx, typeName.get(), typeName.Length());
  if (!name ||
      !JS_DefineProperty(cx, typeObj, "name", STRING_TO_JSVAL(name),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_SealObject(cx, typeObj, JS_FALSE))
    return NULL;

  return typeObj;
}

JSBool
ArrayType::ConstructData(JSContext* cx,
                         JSObject* obj,
                         uintN argc,
                         jsval* argv,
                         jsval* rval)
{
  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  
  
  bool convertObject = argc == 1;

  
  
  if (CType::IsSizeDefined(cx, obj)) {
    if (argc > 1) {
      JS_ReportError(cx, "constructor takes zero or one argument");
      return JS_FALSE;
    }

  } else {
    if (argc != 1) {
      JS_ReportError(cx, "constructor takes one argument");
      return JS_FALSE;
    }

    JSObject* baseType = GetBaseType(cx, obj);

    size_t length;
    if (jsvalToSize(cx, argv[0], false, &length)) {
      
      convertObject = false;

    } else if (JSVAL_IS_OBJECT(argv[0])) {
      
      
      JSObject* arg = JSVAL_TO_OBJECT(argv[0]);
      jsval lengthVal;
      if (!JS_GetProperty(cx, arg, "length", &lengthVal) ||
          !jsvalToSize(cx, lengthVal, false, &length)) {
        JS_ReportError(cx, "argument must be an array object or length");
        return JS_FALSE;
      }

    } else if (JSVAL_IS_STRING(argv[0])) {
      
      
      JSString* sourceString = JSVAL_TO_STRING(argv[0]);
      const jschar* sourceChars = JS_GetStringCharsZ(cx, sourceString);
      size_t sourceLength = JS_GetStringLength(sourceString);

      switch (CType::GetTypeCode(cx, baseType)) {
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char: {
        
        if (!IsUTF16(sourceChars, sourceLength))
          return TypeError(cx, "UTF-16 string", argv[0]);

        NS_ConvertUTF16toUTF8 converted(
          reinterpret_cast<const PRUnichar*>(sourceChars), sourceLength);

        length = converted.Length() + 1;
        break;
      }
      case TYPE_jschar:
        length = sourceLength + 1;
        break;
      default:
        return TypeError(cx, "array", argv[0]);
      }

    } else {
      JS_ReportError(cx, "argument must be an array object or length");
      return JS_FALSE;
    }

    
    obj = CreateInternal(cx, baseType, length, true);
    if (!obj)
      return JS_FALSE;
  }

  
  JSAutoTempValueRooter root(cx, obj);

  JSObject* result = ConstructInternal(cx, obj, NULL, NULL);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);

  if (convertObject) {
    
    if (!ExplicitConvert(cx, argv[0], obj, CData::GetData(cx, result)))
      return JS_FALSE;
  }

  return JS_TRUE;
}

JSObject*
ArrayType::ConstructInternal(JSContext* cx,
                             JSObject* typeObj,
                             JSObject* parentObj,
                             void* data)
{
  
  JSObject* result = CData::Create(cx, typeObj, parentObj, data);
  if (!result)
    return NULL;
  JSAutoTempValueRooter root(cx, result);

  if (!JS_DefineFunctions(cx, result, sArrayInstanceFunctions))
    return NULL;

  
  
  jsval lengthVal;
  if (!JS_GetProperty(cx, typeObj, "length", &lengthVal) ||
      !JS_DefineProperty(cx, result, "length", lengthVal,
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  return result;
}

JSObject*
ArrayType::GetBaseType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_array);

  jsval type;
  JS_GetProperty(cx, obj, "elementType", &type);
  return JSVAL_TO_OBJECT(type);
}

bool
ArrayType::GetSafeLength(JSContext* cx, JSObject* obj, size_t* result)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_array);

  jsval length;
  JS_GetProperty(cx, obj, "length", &length);

  
  
  if (JSVAL_IS_INT(length)) {
    *result = JSVAL_TO_INT(length);
    return true;
  }
  if (JSVAL_IS_DOUBLE(length)) {
    *result = Convert<size_t>(*JSVAL_TO_DOUBLE(length));
    return true;
  }

  JS_ASSERT(JSVAL_IS_VOID(length));
  return false;
}

size_t
ArrayType::GetLength(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_array);

  jsval length;
  JS_GetProperty(cx, obj, "length", &length);

  JS_ASSERT(!JSVAL_IS_VOID(length));

  
  
  
  if (JSVAL_IS_INT(length))
    return JSVAL_TO_INT(length);
  return Convert<size_t>(*JSVAL_TO_DOUBLE(length));
}

JSBool
ArrayType::Getter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CData::IsCData(cx, obj))
    return JS_TRUE;

  
  
  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_array)
    return JS_TRUE;

  
  size_t index;
  size_t length = GetLength(cx, typeObj);
  bool ok = jsvalToSize(cx, idval, true, &index);
  if (!ok && JSVAL_IS_STRING(idval)) {
    
    
    return JS_TRUE;
  }
  if (!ok || index >= length) {
    JS_ReportError(cx, "invalid index");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(cx, typeObj);
  size_t elementSize = CType::GetSize(cx, baseType);
  char* data = static_cast<char*>(CData::GetData(cx, obj)) + elementSize * index;
  return ConvertToJS(cx, baseType, obj, data, false, vp);
}

JSBool
ArrayType::Setter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CData::IsCData(cx, obj))
    return JS_TRUE;

  
  
  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_array)
    return JS_TRUE;

  
  size_t index;
  size_t length = GetLength(cx, typeObj);
  bool ok = jsvalToSize(cx, idval, true, &index);
  if (!ok && JSVAL_IS_STRING(idval)) {
    
    
    return JS_TRUE;
  }
  if (!ok || index >= length) {
    JS_ReportError(cx, "invalid index");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(cx, typeObj);
  size_t elementSize = CType::GetSize(cx, baseType);
  char* data = static_cast<char*>(CData::GetData(cx, obj)) + elementSize * index;
  return ImplicitConvert(cx, *vp, baseType, data, false, NULL);
}

JSBool
ArrayType::AddressOfElement(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(obj);

  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  if (argc != 1) {
    JS_ReportError(cx, "addressOfElement takes one argument");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(cx, typeObj);
  JSObject* pointerType = PointerType::CreateInternal(cx, NULL, baseType, NULL);
  if (!pointerType)
    return JS_FALSE;
  JSAutoTempValueRooter root(cx, pointerType);

  
  JSObject* result = PointerType::ConstructInternal(cx, pointerType, NULL, NULL);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));

  
  size_t index;
  size_t length = GetLength(cx, typeObj);
  if (!jsvalToSize(cx, JS_ARGV(cx, vp)[0], false, &index) ||
      index >= length) {
    JS_ReportError(cx, "invalid index");
    return JS_FALSE;
  }

  
  void** data = static_cast<void**>(CData::GetData(cx, result));
  size_t elementSize = CType::GetSize(cx, baseType);
  *data = static_cast<char*>(CData::GetData(cx, obj)) + elementSize * index;
  return JS_TRUE;
}







static bool
ExtractStructField(JSContext* cx, jsval val, FieldInfo* field)
{
  if (!JSVAL_IS_OBJECT(val) || JSVAL_IS_NULL(val))
    return false;

  JSObject* obj = JSVAL_TO_OBJECT(val);
  JSObject* iter = JS_NewPropertyIterator(cx, obj);
  if (!iter)
    return false;
  JSAutoTempValueRooter iterroot(cx, iter);

  jsid id;
  if (!JS_NextProperty(cx, iter, &id))
    return false;

  jsval nameVal;
  if (!JS_IdToValue(cx, id, &nameVal) || !JSVAL_IS_STRING(nameVal))
    return false;
  JSAutoTempValueRooter nameroot(cx, nameVal);

  
  if (JS_NextProperty(cx, iter, &id) && !JSVAL_IS_VOID(id))
    return false;

  const char* name = JS_GetStringBytesZ(cx, JSVAL_TO_STRING(nameVal));
  field->mName = name;

  jsval propVal;
  if (!JS_GetProperty(cx, obj, name, &propVal) ||
      !JSVAL_IS_OBJECT(propVal) || JSVAL_IS_NULL(propVal) ||
      !CType::IsCType(cx, JSVAL_TO_OBJECT(propVal)))
    return false;

  
  
  
  field->mType = JSVAL_TO_OBJECT(propVal);
  size_t size;
  if (!CType::GetSafeSize(cx, field->mType, &size) || size == 0)
    return false;

  return true;
}



static bool
AddFieldToArray(JSContext* cx,
                JSObject* arrayObj,
                jsuint index,
                const char* name,
                JSObject* typeObj)
{
  JSObject* fieldObj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!fieldObj)
    return false;

  if (!JS_DefineElement(cx, arrayObj, index, OBJECT_TO_JSVAL(fieldObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (!JS_DefineProperty(cx, fieldObj, name, OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return JS_SealObject(cx, fieldObj, JS_FALSE) != JS_FALSE;
}

JSBool
StructType::Create(JSContext* cx, uintN argc, jsval* vp)
{
  
  if (argc < 2) {
    JS_ReportError(cx, "StructType takes at least two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  jsval name = argv[0];
  if (!JSVAL_IS_STRING(name)) {
    JS_ReportError(cx, "first argument must be a string");
    return JS_FALSE;
  }

  if (!JSVAL_IS_OBJECT(argv[1]) ||
      !JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "second argument must be an array");
    return JS_FALSE;
  }

  
  JSObject* fieldsProp = JS_NewArrayObject(cx, 0, NULL);
  if (!fieldsProp)
    return JS_FALSE;
  JSAutoTempValueRooter root(cx, fieldsProp);

  
  JSObject* fieldsObj = JSVAL_TO_OBJECT(argv[1]);
  jsuint len;
  JS_GetArrayLength(cx, fieldsObj, &len);

  nsAutoPtr<ffi_type> ffiType(new ffi_type);
  if (!ffiType) {
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
  }
  ffiType->type = FFI_TYPE_STRUCT;

  nsAutoPtr< nsTArray<FieldInfo> > fields(new nsTArray<FieldInfo>(len));
  if (!fields) {
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
  }
  nsAutoPtr<ffi_type*> elements;

  size_t structSize = 0, structAlign = 0;
  if (len != 0) {
    elements = new ffi_type*[len + 1];
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return JS_FALSE;
    }
    elements[len] = NULL;

    for (jsuint i = 0; i < len; ++i) {
      jsval item;
      JS_GetElement(cx, fieldsObj, i, &item);

      FieldInfo* info = fields->AppendElement();
      if (!info) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
      }
      if (!ExtractStructField(cx, item, info)) {
        JS_ReportError(cx, "struct field descriptors require a valid name and type");
        return JS_FALSE;
      }

      
      if (!AddFieldToArray(cx, fieldsProp, i, info->mName.get(), info->mType))
        return JS_FALSE;

      
      for (PRUint32 j = 0; j < fields->Length() - 1; ++j) {
        if (fields->ElementAt(j).mName == info->mName) {
          JS_ReportError(cx, "struct fields must have unique names");
          return JS_FALSE;
        }
      }

      elements[i] = CType::GetFFIType(cx, info->mType);

      size_t fieldAlign = CType::GetAlignment(cx, info->mType);
      size_t padding = (fieldAlign - structSize % fieldAlign) % fieldAlign;
      info->mOffset = structSize + padding;
      size_t delta = padding + CType::GetSize(cx, info->mType);
      size_t oldSize = structSize;
      structSize = structSize + delta;
      if (structSize - delta != oldSize) {
        JS_ReportError(cx, "size overflow");
        return JS_FALSE;
      }

      if (fieldAlign > structAlign)
        structAlign = fieldAlign;
    }

    
    size_t oldSize = structSize;
    size_t delta = (structAlign - structSize % structAlign) % structAlign;
    structSize = structSize + delta;
    if (structSize - delta != oldSize) {
      JS_ReportError(cx, "size overflow");
      return JS_FALSE;
    }

  } else {
    
    
    
    
    structSize = 1;
    structAlign = 1;
    elements = new ffi_type*[2];
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return JS_FALSE;
    }
    elements[0] = &ffi_type_uint8;
    elements[1] = NULL;
  }

  ffiType->elements = elements;

#ifdef DEBUG
  
  
  
  ffi_cif cif;
  ffiType->size = 0;
  ffiType->alignment = 0;
  ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, ffiType, NULL);
  JS_ASSERT(status == FFI_OK);
  JS_ASSERT(structSize == ffiType->size);
  JS_ASSERT(structAlign == ffiType->alignment);
#else
  
  
  
  
  ffiType->size = structSize;
  ffiType->alignment = structAlign;
#endif

  jsval sizeVal;
  if (!SizeTojsval(cx, structSize, &sizeVal))
    return JS_FALSE;

  
  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  JSObject* proto = CType::GetProtoFromCtor(cx, callee, SLOT_STRUCTPROTO);

  
  JSObject* typeObj = CType::Create(cx, proto, JSVAL_TO_STRING(name),
                        TYPE_struct, sizeVal, INT_TO_JSVAL(structAlign), ffiType);
  if (!typeObj)
    return JS_FALSE;
  ffiType.forget();
  elements.forget();

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(typeObj));

  
  
  if (!JS_SealObject(cx, fieldsProp, JS_FALSE) ||
      !JS_DefineProperty(cx, typeObj, "fields", OBJECT_TO_JSVAL(fieldsProp),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return JS_FALSE;

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_FIELDINFO,
         PRIVATE_TO_JSVAL(fields.get())))
    return JS_FALSE;
  fields.forget();

  if (!JS_SealObject(cx, typeObj, JS_FALSE))
    return JS_FALSE;

  return JS_TRUE;
}

JSBool
StructType::ConstructData(JSContext* cx,
                          JSObject* obj,
                          uintN argc,
                          jsval* argv,
                          jsval* rval)
{
  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  nsTArray<FieldInfo>* fields = GetFieldInfo(cx, obj);

  if (argc != 0 && argc != fields->Length()) {
    JS_ReportError(cx, "constructor takes zero or %u arguments", fields->Length());
    return JS_FALSE;
  }

  JSObject* result = ConstructInternal(cx, obj, NULL, NULL);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);

  if (argc != 0) {
    
    char* buffer = static_cast<char*>(CData::GetData(cx, result));
    for (PRUint32 i = 0; i < fields->Length(); ++i) {
      FieldInfo& field = fields->ElementAt(i);
      if (!ExplicitConvert(cx, argv[i], field.mType, buffer + field.mOffset))
        return JS_FALSE;
    }
  }

  return JS_TRUE;
}

JSObject*
StructType::ConstructInternal(JSContext* cx,
                              JSObject* typeObj,
                              JSObject* parentObj,
                              void* data)
{
  
  JSObject* result = CData::Create(cx, typeObj, parentObj, data);
  if (!result)
    return NULL;
  JSAutoTempValueRooter root(cx, result);

  nsTArray<FieldInfo>* fields = GetFieldInfo(cx, typeObj);

  
  for (PRUint32 i = 0; i < fields->Length(); ++i) {
    FieldInfo& field = fields->ElementAt(i);

    if (!JS_DefineProperty(cx, result, field.mName.get(), JSVAL_VOID,
           StructType::FieldGetter, StructType::FieldSetter,
           JSPROP_ENUMERATE | JSPROP_PERMANENT))
      return NULL;
  }

  if (!JS_DefineFunctions(cx, result, sStructInstanceFunctions))
    return NULL;

  return result;
}

nsTArray<FieldInfo>*
StructType::GetFieldInfo(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_struct);

  jsval slot;
  JS_GetReservedSlot(cx, obj, SLOT_FIELDINFO, &slot);
  JS_ASSERT(!JSVAL_IS_VOID(slot) && JSVAL_TO_PRIVATE(slot));

  return static_cast<nsTArray<FieldInfo>*>(JSVAL_TO_PRIVATE(slot));
}

FieldInfo*
StructType::LookupField(JSContext* cx, JSObject* obj, jsval idval)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_struct);

  nsTArray<FieldInfo>* fields = GetFieldInfo(cx, obj);

  PRUint32 i;
  const char* name = JS_GetStringBytesZ(cx, JSVAL_TO_STRING(idval));
  for (i = 0; i < fields->Length(); ++i) {
    if (fields->ElementAt(i).mName.Equals(name))
      break;
  }

  if (i == fields->Length())
    return NULL;
  return &fields->ElementAt(i);
}

JSBool
StructType::FieldGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  FieldInfo* field = LookupField(cx, typeObj, idval);
  JS_ASSERT(field);

  char* data = static_cast<char*>(CData::GetData(cx, obj)) + field->mOffset;
  return ConvertToJS(cx, field->mType, obj, data, false, vp);
}

JSBool
StructType::FieldSetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  FieldInfo* field = LookupField(cx, typeObj, idval);
  JS_ASSERT(field);

  char* data = static_cast<char*>(CData::GetData(cx, obj)) + field->mOffset;
  return ImplicitConvert(cx, *vp, field->mType, data, false, NULL);
}

JSBool
StructType::AddressOfField(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(obj);

  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(cx, obj);
  if (CType::GetTypeCode(cx, typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  if (argc != 1) {
    JS_ReportError(cx, "addressOfField takes one argument");
    return JS_FALSE;
  }

  FieldInfo* field = LookupField(cx, typeObj, JS_ARGV(cx, vp)[0]);
  if (!field) {
    JS_ReportError(cx, "argument does not name a field");
    return JS_FALSE;
  }

  JSObject* baseType = field->mType;
  JSObject* pointerType = PointerType::CreateInternal(cx, NULL, baseType, NULL);
  if (!pointerType)
    return JS_FALSE;
  JSAutoTempValueRooter root(cx, pointerType);

  
  JSObject* result = PointerType::ConstructInternal(cx, pointerType, NULL, NULL);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));

  
  void** data = static_cast<void**>(CData::GetData(cx, result));
  *data = static_cast<char*>(CData::GetData(cx, obj)) + field->mOffset;
  return JS_TRUE;
}

















JSObject*
CData::Create(JSContext* cx, JSObject* typeObj, JSObject* baseObj, void* source)
{
  JS_ASSERT(typeObj);
  JS_ASSERT(CType::IsCType(cx, typeObj));
  JS_ASSERT(CType::IsSizeDefined(cx, typeObj));
  JS_ASSERT(!baseObj || CData::IsCData(cx, baseObj));
  JS_ASSERT(!baseObj || source);

  
  jsval protoVal;
  JS_GetProperty(cx, typeObj, "prototype", &protoVal);
  JS_ASSERT(JSVAL_IS_OBJECT(protoVal) && !JSVAL_IS_NULL(protoVal));

  JSObject* proto = JSVAL_TO_OBJECT(protoVal);
  JSObject* dataObj = JS_NewObject(cx, &sCDataClass, proto, NULL);
  if (!dataObj)
    return NULL;
  JSAutoTempValueRooter root(cx, dataObj);

  if (!JS_DefineFunctions(cx, dataObj, sCDataFunctions))
    return NULL;

  if (!JS_DefineProperty(cx, dataObj, "value", JSVAL_VOID,
         ValueGetter, ValueSetter, JSPROP_ENUMERATE | JSPROP_PERMANENT))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, dataObj, SLOT_CTYPE, OBJECT_TO_JSVAL(typeObj)))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, dataObj, SLOT_REFERENT, OBJECT_TO_JSVAL(baseObj)))
    return NULL;

  
  
  char** buffer = new char*;
  if (!buffer) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }

  char* data;
  if (baseObj) {
    data = static_cast<char*>(source);
  } else {
    
    size_t size = CType::GetSize(cx, typeObj);
    data = new char[size];
    if (!data) {
      
      JS_ReportAllocationOverflow(cx);
      delete buffer;
      return NULL;
    }

    if (!source)
      memset(data, 0, size);
    else
      memcpy(data, source, size);
  }

  *buffer = data;
  if (!JS_SetReservedSlot(cx, dataObj, SLOT_DATA, PRIVATE_TO_JSVAL(buffer))) {
    if (!baseObj)
      delete data;
    delete buffer;
    return NULL;
  }

  return dataObj;
}

void
CData::Finalize(JSContext* cx, JSObject* obj)
{
  
  jsval slot;
  if (!JS_GetReservedSlot(cx, obj, SLOT_REFERENT, &slot) || JSVAL_IS_VOID(slot))
    return;
  JSBool owns = JSVAL_IS_NULL(slot);

  if (!JS_GetReservedSlot(cx, obj, SLOT_DATA, &slot) || JSVAL_IS_VOID(slot))
    return;
  char** buffer = static_cast<char**>(JSVAL_TO_PRIVATE(slot));

  if (owns)
    delete *buffer;
  delete buffer;
}

JSObject*
CData::GetCType(JSContext* cx, JSObject* dataObj)
{
  JS_ASSERT(CData::IsCData(cx, dataObj));

  jsval slot;
  JS_GetReservedSlot(cx, dataObj, SLOT_CTYPE, &slot);
  JSObject* typeObj = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(CType::IsCType(cx, typeObj));
  return typeObj;
}

void*
CData::GetData(JSContext* cx, JSObject* dataObj)
{
  JS_ASSERT(CData::IsCData(cx, dataObj));

  jsval slot;
  JS_GetReservedSlot(cx, dataObj, SLOT_DATA, &slot);

  void** buffer = static_cast<void**>(JSVAL_TO_PRIVATE(slot));
  JS_ASSERT(buffer);
  JS_ASSERT(*buffer);
  return *buffer;
}

bool
CData::IsCData(JSContext* cx, JSObject* obj)
{
  return JS_GET_CLASS(cx, obj) == &sCDataClass;
}

JSBool
CData::ValueGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  if (!ConvertToJS(cx, GetCType(cx, obj), NULL, GetData(cx, obj), true, vp))
    return JS_FALSE;

  return JS_TRUE;
}

JSBool
CData::ValueSetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  return ImplicitConvert(cx, *vp, GetCType(cx, obj), GetData(cx, obj), false, NULL);
}

JSBool
CData::Address(JSContext* cx, uintN argc, jsval *vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "address takes zero arguments");
    return JS_FALSE;
  }

  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(obj);

  if (!IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(cx, obj);
  JSObject* pointerType = PointerType::CreateInternal(cx, NULL, typeObj, NULL);
  if (!pointerType)
    return JS_FALSE;
  JSAutoTempValueRooter root(cx, pointerType);

  
  JSObject* result = PointerType::ConstructInternal(cx, pointerType, NULL, NULL);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));

  
  void** data = static_cast<void**>(GetData(cx, result));
  *data = GetData(cx, obj);
  return JS_TRUE;
}

JSBool
CData::Cast(JSContext* cx, uintN argc, jsval *vp)
{
  if (argc != 2) {
    JS_ReportError(cx, "cast takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (!JSVAL_IS_OBJECT(argv[0]) || JSVAL_IS_NULL(argv[0]) ||
      !CData::IsCData(cx, JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "first argument must be a CData");
    return JS_FALSE;
  }
  JSObject* sourceData = JSVAL_TO_OBJECT(argv[0]);
  JSObject* sourceType = CData::GetCType(cx, sourceData);

  if (!JSVAL_IS_OBJECT(argv[1]) || JSVAL_IS_NULL(argv[1]) ||
      !CType::IsCType(cx, JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "second argument must be a CType");
    return JS_FALSE;
  }

  JSObject* targetType = JSVAL_TO_OBJECT(argv[1]);
  size_t targetSize;
  if (!CType::GetSafeSize(cx, targetType, &targetSize) ||
      targetSize > CType::GetSize(cx, sourceType)) {
    JS_ReportError(cx,
      "target CType has undefined or larger size than source CType");
    return JS_FALSE;
  }

  
  
  JSObject* result;
  void* data = CData::GetData(cx, sourceData);
  switch (CType::GetTypeCode(cx, targetType)) {
  case TYPE_pointer:
    result = PointerType::ConstructInternal(cx, targetType, sourceData, data);
    break;
  case TYPE_array:
    result = ArrayType::ConstructInternal(cx, targetType, sourceData, data);
    break;
  case TYPE_struct:
    result = StructType::ConstructInternal(cx, targetType, sourceData, data);
    break;
  default:
    result = CData::Create(cx, targetType, sourceData, data);
    break;
  }

  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CData::ReadString(JSContext* cx, uintN argc, jsval *vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "readString takes zero arguments");
    return JS_FALSE;
  }

  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  JS_ASSERT(obj);

  if (!IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  
  JSObject* baseType;
  JSObject* typeObj = GetCType(cx, obj);
  TypeCode typeCode = CType::GetTypeCode(cx, typeObj);
  void* data;
  size_t maxLength = -1;
  switch (typeCode) {
  case TYPE_pointer:
    baseType = PointerType::GetBaseType(cx, typeObj);
    if (!baseType) {
      JS_ReportError(cx, "cannot read contents of pointer to opaque type");
      return JS_FALSE;
    }

    data = *static_cast<void**>(GetData(cx, obj));
    if (data == NULL) {
      JS_ReportError(cx, "cannot read contents of null pointer");
      return JS_FALSE;
    }
    break;
  case TYPE_array:
    baseType = ArrayType::GetBaseType(cx, typeObj);
    data = GetData(cx, obj);
    maxLength = ArrayType::GetLength(cx, typeObj);
    break;
  default:
    JS_ReportError(cx, "not a PointerType or ArrayType");
    return JS_FALSE;
  }

  
  
  JSString* result;
  switch (CType::GetTypeCode(cx, baseType)) {
  case TYPE_int8_t:
  case TYPE_uint8_t:
  case TYPE_char:
  case TYPE_signed_char:
  case TYPE_unsigned_char: {
    char* bytes = static_cast<char*>(data);
    size_t length = strnlen(bytes, maxLength);
    nsDependentCSubstring string(bytes, bytes + length);
    if (!IsUTF8(string)) {
      JS_ReportError(cx, "not a UTF-8 string");
      return JS_FALSE;
    }

    NS_ConvertUTF8toUTF16 converted(string);
    result = JS_NewUCStringCopyN(cx, converted.get(), converted.Length());
    break;
  }
  case TYPE_int16_t:
  case TYPE_uint16_t:
  case TYPE_short:
  case TYPE_unsigned_short:
  case TYPE_jschar: {
    jschar* chars = static_cast<jschar*>(data);
    size_t length = strnlen(chars, maxLength);
    result = JS_NewUCStringCopyN(cx, chars, length);
    break;
  }
  default:
    JS_ReportError(cx,
      "base type is not an 8-bit or 16-bit integer or character type");
    return JS_FALSE;
  }

  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CData::ToSource(JSContext* cx, uintN argc, jsval *vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return JS_FALSE;
  }

  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(cx, obj);
  void* data = CData::GetData(cx, obj);

  
  
  
  
  
  
  nsCAutoString source = BuildTypeSource(cx, typeObj, true);
  source.Append('(');
  source.Append(BuildDataSource(cx, typeObj, data, false));
  source.Append(')');

  JSString* result = JS_NewStringCopyN(cx, source.get(), source.Length());
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}





JSObject*
Int64Base::Construct(JSContext* cx,
                     JSObject* proto,
                     PRUint64 data,
                     bool isUnsigned)
{
  JSClass* clasp = isUnsigned ? &sUInt64Class : &sInt64Class;
  JSObject* result = JS_NewObject(cx, clasp, proto, NULL);
  if (!result)
    return NULL;
  JSAutoTempValueRooter root(cx, result);

  
  PRUint64* buffer = new PRUint64(data);
  if (!buffer) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }

  if (!JS_SetReservedSlot(cx, result, SLOT_INT64, PRIVATE_TO_JSVAL(buffer))) {
    delete buffer;
    return NULL;
  }

  if (!JS_SealObject(cx, result, JS_FALSE))
    return NULL;

  return result;
}

void
Int64Base::Finalize(JSContext* cx, JSObject* obj)
{
  jsval slot;
  if (!JS_GetReservedSlot(cx, obj, SLOT_INT64, &slot) || JSVAL_IS_VOID(slot))
    return;

  delete static_cast<char*>(JSVAL_TO_PRIVATE(slot));
}

PRUint64
Int64Base::GetInt(JSContext* cx, JSObject* obj) {
  JS_ASSERT(Int64::IsInt64(cx, obj) || UInt64::IsUInt64(cx, obj));

  jsval slot;
  JS_GetReservedSlot(cx, obj, SLOT_INT64, &slot);
  return *static_cast<PRUint64*>(JSVAL_TO_PRIVATE(slot));
}

JSBool
Int64Base::ToString(JSContext* cx,
                    JSObject* obj,
                    uintN argc,
                    jsval *vp,
                    bool isUnsigned)
{
  if (argc > 1) {
    JS_ReportError(cx, "toString takes zero or one argument");
    return JS_FALSE;
  }

  jsuint radix = 10;
  if (argc == 1) {
    jsval arg = JS_ARGV(cx, vp)[0];
    if (JSVAL_IS_INT(arg))
      radix = JSVAL_TO_INT(arg);
    if (!JSVAL_IS_INT(arg) || radix < 2 || radix > 36) {
      JS_ReportError(cx, "radix argument must be an integer between 2 and 36");
      return JS_FALSE;
    }
  }

  nsCAutoString intString;
  if (isUnsigned) {
    intString = IntegerToString(GetInt(cx, obj), radix);
  } else {
    intString = IntegerToString(static_cast<PRInt64>(GetInt(cx, obj)), radix);
  }

  JSString *result = JS_NewStringCopyN(cx, intString.get(), intString.Length());
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
Int64Base::ToSource(JSContext* cx,
                    JSObject* obj,
                    uintN argc,
                    jsval *vp,
                    bool isUnsigned)
{
  if (argc != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return JS_FALSE;
  }

  
  nsCAutoString source;
  if (isUnsigned) {
    source.Append("ctypes.UInt64(\"");
    source.Append(IntegerToString(GetInt(cx, obj), 10));
  } else {
    source.Append("ctypes.Int64(\"");
    source.Append(IntegerToString(static_cast<PRInt64>(GetInt(cx, obj)), 10));
  }
  source.Append(')');

  JSString *result = JS_NewStringCopyN(cx, source.get(), source.Length());
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
Int64::Construct(JSContext* cx,
                 JSObject* obj,
                 uintN argc,
                 jsval* argv,
                 jsval* rval)
{
  
  if (argc != 1) {
    JS_ReportError(cx, "Int64 takes one argument");
    return JS_FALSE;
  }

  PRInt64 i;
  if (!jsvalToBigInteger(cx, argv[0], true, &i))
    return TypeError(cx, "int64", argv[0]);

  
  jsval slot;
  JS_GetProperty(cx, JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv)), "prototype", &slot);
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sInt64Proto);

  JSObject* result = Int64Base::Construct(cx, proto, i, false);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);
  return JS_TRUE;
}

bool
Int64::IsInt64(JSContext* cx, JSObject* obj)
{
  return JS_GET_CLASS(cx, obj) == &sInt64Class;
}

JSBool
Int64::ToString(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!Int64::IsInt64(cx, obj)) {
    JS_ReportError(cx, "not an Int64");
    return JS_FALSE;
  }

  return Int64Base::ToString(cx, obj, argc, vp, false);
}

JSBool
Int64::ToSource(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!Int64::IsInt64(cx, obj)) {
    JS_ReportError(cx, "not an Int64");
    return JS_FALSE;
  }

  return Int64Base::ToSource(cx, obj, argc, vp, false);
}

JSBool
Int64::Compare(JSContext* cx, uintN argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 2 ||
      !JSVAL_IS_OBJECT(argv[0]) || JSVAL_IS_NULL(argv[0]) ||
      !JSVAL_IS_OBJECT(argv[1]) || JSVAL_IS_NULL(argv[1]) ||
      !Int64::IsInt64(cx, JSVAL_TO_OBJECT(argv[0])) ||
      !Int64::IsInt64(cx, JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "compare takes two Int64 arguments");
    return JS_FALSE;
  }

  JSObject* obj1 = JSVAL_TO_OBJECT(argv[0]);
  JSObject* obj2 = JSVAL_TO_OBJECT(argv[1]);

  PRInt64 i1 = GetInt(cx, obj1);
  PRInt64 i2 = GetInt(cx, obj2);

  if (i1 == i2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
  else if (i1 < i2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(-1));
  else
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(1));

  return JS_TRUE;
}

#define LO_MASK ((PRUint64(1) << 32) - 1)
#define INT64_LO(i) ((i) & LO_MASK)
#define INT64_HI(i) ((i) >> 32)

JSBool
Int64::Lo(JSContext* cx, uintN argc, jsval* vp)
{
  jsval arg = JS_ARGV(cx, vp)[0];
  if (argc != 1 || !JSVAL_IS_OBJECT(arg) || JSVAL_IS_NULL(arg) ||
      !Int64::IsInt64(cx, JSVAL_TO_OBJECT(arg))) {
    JS_ReportError(cx, "lo takes one Int64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(arg);
  PRInt64 u = GetInt(cx, obj);
  jsdouble d = PRUint32(INT64_LO(u));

  jsval result;
  if (!JS_NewNumberValue(cx, d, &result))
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
Int64::Hi(JSContext* cx, uintN argc, jsval* vp)
{
  jsval arg = JS_ARGV(cx, vp)[0];
  if (argc != 1 || !JSVAL_IS_OBJECT(arg) || JSVAL_IS_NULL(arg) ||
      !Int64::IsInt64(cx, JSVAL_TO_OBJECT(arg))) {
    JS_ReportError(cx, "lo takes one Int64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(arg);
  PRInt64 u = GetInt(cx, obj);
  jsdouble d = PRInt32(INT64_HI(u));

  jsval result;
  if (!JS_NewNumberValue(cx, d, &result))
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
Int64::Join(JSContext* cx, uintN argc, jsval* vp)
{
  if (argc != 2) {
    JS_ReportError(cx, "join takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  PRInt32 hi;
  PRUint32 lo;
  if (!jsvalToInteger(cx, argv[0], &hi))
    return TypeError(cx, "int32", argv[0]);
  if (!jsvalToInteger(cx, argv[1], &lo))
    return TypeError(cx, "uint32", argv[1]);

  PRInt64 i = (PRInt64(hi) << 32) + PRInt64(lo);

  
  JSObject* callee = JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv));

  jsval slot;
  JS_GetReservedSlot(cx, callee, SLOT_FN_INT64PROTO, &slot);
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sInt64Proto);

  JSObject* result = Int64Base::Construct(cx, proto, i, false);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
UInt64::Construct(JSContext* cx,
                  JSObject* obj,
                  uintN argc,
                  jsval* argv,
                  jsval* rval)
{
  
  if (argc != 1) {
    JS_ReportError(cx, "UInt64 takes one argument");
    return JS_FALSE;
  }

  PRUint64 u;
  if (!jsvalToBigInteger(cx, argv[0], true, &u))
    return TypeError(cx, "uint64", argv[0]);

  
  jsval slot;
  JS_GetProperty(cx, JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv)), "prototype", &slot);
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sUInt64Proto);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);
  return JS_TRUE;
}

bool
UInt64::IsUInt64(JSContext* cx, JSObject* obj)
{
  return JS_GET_CLASS(cx, obj) == &sUInt64Class;
}

JSBool
UInt64::ToString(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!UInt64::IsUInt64(cx, obj)) {
    JS_ReportError(cx, "not a UInt64");
    return JS_FALSE;
  }

  return Int64Base::ToString(cx, obj, argc, vp, true);
}

JSBool
UInt64::ToSource(JSContext* cx, uintN argc, jsval *vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!UInt64::IsUInt64(cx, obj)) {
    JS_ReportError(cx, "not a UInt64");
    return JS_FALSE;
  }

  return Int64Base::ToSource(cx, obj, argc, vp, true);
}

JSBool
UInt64::Compare(JSContext* cx, uintN argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 2 ||
      !JSVAL_IS_OBJECT(argv[0]) || JSVAL_IS_NULL(argv[0]) ||
      !JSVAL_IS_OBJECT(argv[1]) || JSVAL_IS_NULL(argv[1]) ||
      !UInt64::IsUInt64(cx, JSVAL_TO_OBJECT(argv[0])) ||
      !UInt64::IsUInt64(cx, JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "compare takes two UInt64 arguments");
    return JS_FALSE;
  }

  JSObject* obj1 = JSVAL_TO_OBJECT(argv[0]);
  JSObject* obj2 = JSVAL_TO_OBJECT(argv[1]);

  PRUint64 u1 = GetInt(cx, obj1);
  PRUint64 u2 = GetInt(cx, obj2);

  if (u1 == u2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
  else if (u1 < u2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(-1));
  else
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(1));

  return JS_TRUE;
}

JSBool
UInt64::Lo(JSContext* cx, uintN argc, jsval* vp)
{
  jsval arg = JS_ARGV(cx, vp)[0];
  if (argc != 1 || !JSVAL_IS_OBJECT(arg) || JSVAL_IS_NULL(arg) ||
      !UInt64::IsUInt64(cx, JSVAL_TO_OBJECT(arg))) {
    JS_ReportError(cx, "lo takes one UInt64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(arg);
  PRUint64 u = GetInt(cx, obj);
  jsdouble d = PRUint32(INT64_LO(u));

  jsval result;
  if (!JS_NewNumberValue(cx, d, &result))
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
UInt64::Hi(JSContext* cx, uintN argc, jsval* vp)
{
  jsval arg = JS_ARGV(cx, vp)[0];
  if (argc != 1 || !JSVAL_IS_OBJECT(arg) || JSVAL_IS_NULL(arg) ||
      !UInt64::IsUInt64(cx, JSVAL_TO_OBJECT(arg))) {
    JS_ReportError(cx, "lo takes one UInt64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(arg);
  PRUint64 u = GetInt(cx, obj);
  jsdouble d = PRUint32(INT64_HI(u));

  jsval result;
  if (!JS_NewNumberValue(cx, d, &result))
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
UInt64::Join(JSContext* cx, uintN argc, jsval* vp)
{
  if (argc != 2) {
    JS_ReportError(cx, "join takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  PRUint32 hi;
  PRUint32 lo;
  if (!jsvalToInteger(cx, argv[0], &hi))
    return TypeError(cx, "uint32_t", argv[0]);
  if (!jsvalToInteger(cx, argv[1], &lo))
    return TypeError(cx, "uint32_t", argv[1]);

  PRUint64 u = (PRUint64(hi) << 32) + PRUint64(lo);

  
  JSObject* callee = JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv));

  jsval slot;
  JS_GetReservedSlot(cx, callee, SLOT_FN_INT64PROTO, &slot);
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sUInt64Proto);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

}
}

