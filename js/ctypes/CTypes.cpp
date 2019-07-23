





































#include "jscntxt.h"
#include "CTypes.h"
#include "nsAutoPtr.h"
#include "nsUTF8Utils.h"
#include "nsCRTGlue.h"
#include "prlog.h"
#include "prdtoa.h"
#include "jsnum.h"
#ifdef HAVE_SSIZE_T
#include <sys/types.h>
#endif

namespace mozilla {
namespace ctypes {





static JSBool ConstructAbstract(JSContext* cx, JSObject* obj, uintN argc,
  jsval* argv, jsval* rval);





static JSClass sCABIClass = {
  "CABI",
  JSCLASS_HAS_RESERVED_SLOTS(CABI_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};




static JSClass sCTypeProtoClass = {
  "CType",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPEPROTO_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  NULL, NULL, ConstructAbstract, ConstructAbstract, NULL, NULL, NULL, NULL
};



static JSClass sCDataProtoClass = {
  "CData",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sCTypeClass = {
  "CType",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPE_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CType::Finalize,
  NULL, NULL, CType::ConstructData, CType::ConstructData, NULL,
  CType::HasInstance, NULL, NULL
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

#define CTYPESPROP_FLAGS \
  (JSPROP_SHARED | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

#define CDATAFN_FLAGS \
  (JSFUN_FAST_NATIVE | JSPROP_READONLY | JSPROP_PERMANENT)

static JSPropertySpec sCTypeProps[] = {
  { "name", 0, CTYPESPROP_FLAGS, CType::NameGetter, NULL },
  { "size", 0, CTYPESPROP_FLAGS, CType::SizeGetter, NULL },
  { "ptr", 0, CTYPESPROP_FLAGS, CType::PtrGetter, NULL },
  { "prototype", 0, CTYPESPROP_FLAGS, CType::PrototypeGetter, NULL },
  { 0, 0, 0, NULL, NULL }
};

static JSFunctionSpec sCTypeFunctions[] = {
  JS_FN("array", CType::Array, 0, CTYPESFN_FLAGS),
  JS_FN("toString", CType::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", CType::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSPropertySpec sCDataProps[] = {
  { "value", 0, JSPROP_SHARED | JSPROP_PERMANENT,
    CData::ValueGetter, CData::ValueSetter },
  { 0, 0, 0, NULL, NULL }
};

static JSFunctionSpec sCDataFunctions[] = {
  JS_FN("address", CData::Address, 0, CDATAFN_FLAGS),
  JS_FN("readString", CData::ReadString, 0, CDATAFN_FLAGS),
  JS_FN("toSource", CData::ToSource, 0, CDATAFN_FLAGS),
  JS_FN("toString", CData::ToSource, 0, CDATAFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sPointerFunction =
  JS_FN("PointerType", PointerType::Create, 1, CTYPESFN_FLAGS);

static JSPropertySpec sPointerProps[] = {
  { "targetType", 0, CTYPESPROP_FLAGS, PointerType::TargetTypeGetter, NULL },
  { 0, 0, 0, NULL, NULL }
};

static JSPropertySpec sPointerInstanceProps[] = {
  { "contents", 0, JSPROP_SHARED | JSPROP_PERMANENT,
    PointerType::ContentsGetter, PointerType::ContentsSetter },
  { 0, 0, 0, NULL, NULL }
};

static JSFunctionSpec sArrayFunction =
  JS_FN("ArrayType", ArrayType::Create, 1, CTYPESFN_FLAGS);

static JSPropertySpec sArrayProps[] = {
  { "elementType", 0, CTYPESPROP_FLAGS, ArrayType::ElementTypeGetter, NULL },
  { "length", 0, CTYPESPROP_FLAGS, ArrayType::LengthGetter, NULL },
  { 0, 0, 0, NULL, NULL }
};

static JSFunctionSpec sArrayInstanceFunctions[] = {
  JS_FN("addressOfElement", ArrayType::AddressOfElement, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static JSPropertySpec sArrayInstanceProps[] = {
  { "length", 0, JSPROP_SHARED | JSPROP_READONLY | JSPROP_PERMANENT,
    ArrayType::LengthGetter, NULL },
  { 0, 0, 0, NULL, NULL }
};

static JSFunctionSpec sStructFunction =
  JS_FN("StructType", StructType::Create, 2, CTYPESFN_FLAGS);

static JSPropertySpec sStructProps[] = {
  { "fields", 0, CTYPESPROP_FLAGS, StructType::FieldsArrayGetter, NULL },
  { 0, 0, 0, NULL, NULL }
};

static JSFunctionSpec sStructInstanceFunctions[] = {
  JS_FN("addressOfField", StructType::AddressOfField, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static JSClass sInt64ProtoClass = {
  "Int64",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sUInt64ProtoClass = {
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

JS_ALWAYS_INLINE void
ASSERT_OK(JSBool ok)
{
  JS_ASSERT(ok);
}

JS_ALWAYS_INLINE JSString*
NewUCString(JSContext* cx, const nsString& from)
{
  JS_ASSERT(from.get());
  return JS_NewUCStringCopyN(cx,
    reinterpret_cast<const jschar*>(from.get()), from.Length());
}

JS_ALWAYS_INLINE const nsDependentString
GetString(JSString* str)
{
  JS_ASSERT(str);
  const jschar* chars = JS_GetStringChars(str);
  size_t length = JS_GetStringLength(str);
  return nsDependentString(reinterpret_cast<const PRUnichar*>(chars), length);
}

JS_ALWAYS_INLINE size_t
Align(size_t val, size_t align)
{
  return ((val - 1) | (align - 1)) + 1;
}

ABICode
GetABICode(JSContext* cx, JSObject* obj)
{
  
  
  if (JS_GET_CLASS(cx, obj) != &sCABIClass)
    return INVALID_ABI;

  jsval result;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_ABICODE, &result));

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

JSBool
TypeError(JSContext* cx, const char* expected, jsval actual)
{
  JSString* str = JS_ValueToSource(cx, actual);
  JSAutoTempValueRooter root(cx, str);

  const char* src;
  if (str) {
    src = JS_GetStringBytesZ(cx, str);
    if (!src)
      return false;
  } else {
    JS_ClearPendingException(cx);
    src = "<<error converting value to string>>";
  }

  JS_ReportErrorNumber(cx, GetErrorMessage, NULL,
                       CTYPESMSG_TYPE_ERROR, expected, src);
  return false;
}

static JSObject*
InitCTypeClass(JSContext* cx, JSObject* parent)
{
  JSFunction* fun = JS_DefineFunction(cx, parent, "CType", ConstructAbstract, 0,
                      CTYPESFN_FLAGS);
  if (!fun)
    return NULL;

  JSObject* ctor = JS_GetFunctionObject(fun);
  JSObject* fnproto = JS_GetPrototype(cx, ctor);
  JS_ASSERT(ctor);
  JS_ASSERT(fnproto);

  
  JSObject* prototype = JS_NewObject(cx, &sCTypeProtoClass, fnproto, parent);
  if (!prototype)
    return NULL;

  if (!JS_DefineProperty(cx, ctor, "prototype", OBJECT_TO_JSVAL(prototype),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(ctor),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  
  if (!JS_DefineProperties(cx, prototype, sCTypeProps) ||
      !JS_DefineFunctions(cx, prototype, sCTypeFunctions))
    return NULL;

  if (!JS_SealObject(cx, ctor, JS_FALSE) ||
      !JS_SealObject(cx, prototype, JS_FALSE))
    return NULL;

  return prototype;
}

static JSObject*
InitCDataClass(JSContext* cx, JSObject* parent, JSObject* CTypeProto)
{
  JSFunction* fun = JS_DefineFunction(cx, parent, "CData", ConstructAbstract, 0,
                      CTYPESFN_FLAGS);
  if (!fun)
    return NULL;

  JSObject* ctor = JS_GetFunctionObject(fun);
  JS_ASSERT(ctor);

  
  
  
  if (!JS_SetPrototype(cx, ctor, CTypeProto))
    return NULL;

  
  JSObject* prototype = JS_NewObject(cx, &sCDataProtoClass, NULL, parent);
  if (!prototype)
    return NULL;

  if (!JS_DefineProperty(cx, ctor, "prototype", OBJECT_TO_JSVAL(prototype),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(ctor),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  
  if (!JS_DefineProperties(cx, prototype, sCDataProps) ||
      !JS_DefineFunctions(cx, prototype, sCDataFunctions))
    return NULL;

  if (
      !JS_SealObject(cx, ctor, JS_FALSE))
    return NULL;

  return prototype;
}

static JSBool
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
  return JS_SealObject(cx, obj, JS_FALSE);
}


static JSBool
InitTypeConstructor(JSContext* cx,
                    JSObject* parent,
                    JSObject* CTypeProto,
                    JSObject* CDataProto,
                    JSFunctionSpec spec,
                    JSPropertySpec* props,
                    JSFunctionSpec* instanceFns,
                    JSPropertySpec* instanceProps,
                    JSObject*& typeProto,
                    JSObject*& dataProto)
{
  JSFunction* fun = JS_DefineFunction(cx, parent, spec.name, spec.call, 
                      spec.nargs, spec.flags);
  if (!fun)
    return false;

  JSObject* obj = JS_GetFunctionObject(fun);
  if (!obj)
    return false;

  
  typeProto = JS_NewObject(cx, &sCTypeProtoClass, CTypeProto, parent);
  if (!typeProto)
    return false;

  
  if (!JS_DefineProperty(cx, obj, "prototype", OBJECT_TO_JSVAL(typeProto),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (!JS_DefineProperties(cx, typeProto, props))
    return false;

  if (!JS_DefineProperty(cx, typeProto, "constructor", OBJECT_TO_JSVAL(obj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  
  
  if (!JS_SetReservedSlot(cx, obj, SLOT_FN_CTORPROTO, OBJECT_TO_JSVAL(typeProto)))
    return false;

  
  
  
  
  dataProto = JS_NewObject(cx, &sCDataProtoClass, CDataProto, parent);
  if (!dataProto)
    return false;
  JSAutoTempValueRooter protoroot(cx, dataProto);

  
  
  
  if (instanceFns && !JS_DefineFunctions(cx, dataProto, instanceFns))
    return false;

  if (instanceProps && !JS_DefineProperties(cx, dataProto, instanceProps))
    return false;

  if (!JS_SealObject(cx, obj, JS_FALSE) ||
      
      !JS_SealObject(cx, typeProto, JS_FALSE))
    return false;

  return true;
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

  
  
  jsval join;
  ASSERT_OK(JS_GetProperty(cx, ctor, "join", &join));
  if (!JS_SetReservedSlot(cx, JSVAL_TO_OBJECT(join), SLOT_FN_INT64PROTO,
         OBJECT_TO_JSVAL(prototype)))
    return NULL;

  if (!JS_SealObject(cx, prototype, JS_FALSE))
    return NULL;

  return prototype;
}

static JSBool
AttachProtos(JSContext* cx, JSObject* proto, JSObject** protos)
{
  
  
  for (PRUint32 i = 0; i < CTYPEPROTO_SLOTS; ++i) {
    if (!JS_SetReservedSlot(cx, proto, i, OBJECT_TO_JSVAL(protos[i])))
      return false;
  }

  return true;
}

JSBool
InitTypeClasses(JSContext* cx, JSObject* parent)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  JSObject* CTypeProto = InitCTypeClass(cx, parent);
  if (!CTypeProto)
    return false;

  
  
  
  
  
  
  
  
  
  
  
  JSObject* CDataProto = InitCDataClass(cx, parent, CTypeProto);
  if (!CDataProto)
    return false;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  JSObject* protos[CTYPEPROTO_SLOTS];
  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sPointerFunction, sPointerProps, NULL, sPointerInstanceProps,
         protos[SLOT_POINTERPROTO], protos[SLOT_POINTERDATAPROTO]))
    return false;
  JSAutoTempValueRooter proot(cx, protos[SLOT_POINTERDATAPROTO]);

  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sArrayFunction, sArrayProps, sArrayInstanceFunctions, sArrayInstanceProps,
         protos[SLOT_ARRAYPROTO], protos[SLOT_ARRAYDATAPROTO]))
    return false;
  JSAutoTempValueRooter aroot(cx, protos[SLOT_ARRAYDATAPROTO]);

  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sStructFunction, sStructProps, sStructInstanceFunctions, NULL,
         protos[SLOT_STRUCTPROTO], protos[SLOT_STRUCTDATAPROTO]))
    return false;
  JSAutoTempValueRooter sroot(cx, protos[SLOT_STRUCTDATAPROTO]);

  protos[SLOT_CDATAPROTO] = CDataProto;

  
  
  
  
  
  
  
  
  protos[SLOT_INT64PROTO] = InitInt64Class(cx, parent, &sInt64ProtoClass,
    Int64::Construct, sInt64Functions, sInt64StaticFunctions);
  if (!protos[SLOT_INT64PROTO])
    return false;
  protos[SLOT_UINT64PROTO] = InitInt64Class(cx, parent, &sUInt64ProtoClass,
    UInt64::Construct, sUInt64Functions, sUInt64StaticFunctions);
  if (!protos[SLOT_UINT64PROTO])
    return false;

  
  
  
  if (!AttachProtos(cx, CTypeProto, protos) ||
      !AttachProtos(cx, protos[SLOT_POINTERPROTO], protos) ||
      !AttachProtos(cx, protos[SLOT_ARRAYPROTO], protos) ||
      !AttachProtos(cx, protos[SLOT_STRUCTPROTO], protos))
     return false;

  
  if (!DefineABIConstant(cx, parent, "default_abi", ABI_DEFAULT) ||
      !DefineABIConstant(cx, parent, "stdcall_abi", ABI_STDCALL))
    return false;

  
  
  
  
  
  
  
  
  
  
#define DEFINE_TYPE(name, type, ffiType)                                       \
  JSObject* typeObj_##name =                                                   \
    CType::DefineBuiltin(cx, parent, #name, CTypeProto, CDataProto, #name,     \
      TYPE_##name, INT_TO_JSVAL(sizeof(type)),                                 \
      INT_TO_JSVAL(ffiType.alignment), &ffiType);                              \
  if (!typeObj_##name)                                                         \
    return false;
#include "typedefs.h"

  
  
  if (!JS_DefineProperty(cx, parent, "unsigned",
         OBJECT_TO_JSVAL(typeObj_unsigned_int), NULL, NULL,
         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  
  JSObject* typeObj =
    CType::DefineBuiltin(cx, parent, "void_t", CTypeProto, CDataProto, "void",
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
PR_STATIC_ASSERT(sizeof(jschar) == sizeof(PRUnichar));

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

template<class IntegerType, class FromType>
static JS_ALWAYS_INLINE bool IsWider()
{
  if (IsUnsigned<FromType>() && IsUnsigned<IntegerType>() &&
      sizeof(IntegerType) < sizeof(FromType))
    return false;
  if (!IsUnsigned<FromType>() && !IsUnsigned<IntegerType>() &&
      sizeof(IntegerType) < sizeof(FromType))
    return false;
  if (IsUnsigned<FromType>() && !IsUnsigned<IntegerType>() &&
      sizeof(IntegerType) <= sizeof(FromType))
    return false;
  if (!IsUnsigned<FromType>() && IsUnsigned<IntegerType>())
    return false;
  return true;
}



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
  if (!JSVAL_IS_PRIMITIVE(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (CData::IsCData(cx, obj)) {
      JSObject* typeObj = CData::GetCType(cx, obj);
      void* data = CData::GetData(cx, obj);

      
      
      switch (CType::GetTypeCode(cx, typeObj)) {
#define DEFINE_INT_TYPE(name, fromType, ffiType)                               \
      case TYPE_##name:                                                        \
        if (!IsWider<IntegerType, fromType>())                                 \
          return false;                                                        \
        *result = *static_cast<fromType*>(data);                               \
        return true;
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
  if (!JSVAL_IS_PRIMITIVE(val)) {
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
        return true;
#define DEFINE_INT_TYPE(name, fromType, ffiType)                               \
      case TYPE_##name:                                                        \
        if (sizeof(fromType) > 4)                                              \
          return false;                                                        \
        if (sizeof(fromType) == 4 && !IsDoublePrecision<FloatType>())          \
          return false;                                                        \
        *result = *static_cast<fromType*>(data);                               \
        return true;
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
  if (!JSVAL_IS_PRIMITIVE(val)) {
    
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



static JSBool
SizeTojsval(JSContext* cx, size_t size, jsval* result)
{
  if (Convert<size_t>(jsdouble(size)) != size) {
    JS_ReportError(cx, "size overflow");
    return false;
  }

  return JS_NewNumberValue(cx, jsdouble(size), result);
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
  if (!JSVAL_IS_PRIMITIVE(val)) {
    
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
  if (!JSVAL_IS_PRIMITIVE(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
    if (Int64::IsInt64(cx, obj)) {
      PRInt64 i = Int64Base::GetInt(cx, obj);
      intptr_t p = intptr_t(i);

      
      if (PRInt64(p) != i)
        return false;
      *result = uintptr_t(p);
      return true;
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
nsAutoString
IntegerToString(IntegerType i, jsuint radix)
{
  
  
  PRUnichar buffer[sizeof(IntegerType) * 8 + 1];
  PRUnichar* cp = buffer + sizeof(buffer) / sizeof(PRUnichar);

  
  
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
  return nsAutoString(cp, buffer + sizeof(buffer) / sizeof(PRUnichar) - cp);
}

template<class IntegerType>
static bool
StringToInteger(JSContext* cx, JSString* string, IntegerType* result)
{
  const jschar* cp = JS_GetStringChars(string);
  const jschar* end = cp + JS_GetStringLength(string);
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
    jschar c = *cp++;
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












JSBool
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
#define DEFINE_CHAR_TYPE(name, type, ffiType)                                  \
  case TYPE_##name:                                                            \
    /* Convert to an integer. We have no idea what character encoding to */    \
    /* use, if any. */                                                         \
    *result = INT_TO_JSVAL(*static_cast<type*>(data));                         \
    break;
#include "typedefs.h"
  case TYPE_jschar: {
    
    JSString* str = JS_NewUCStringCopyN(cx, static_cast<jschar*>(data), 1);
    if (!str)
      return false;

    *result = STRING_TO_JSVAL(str);
    break;
  }
  case TYPE_pointer:
  case TYPE_array:
  case TYPE_struct: {
    
    
    if (wantPrimitive) {
      JS_ReportError(cx, "cannot convert to primitive value");
      return false;
    }

    JSObject* obj = CData::Create(cx, typeObj, parentObj, data);
    if (!obj)
      return false;

    *result = OBJECT_TO_JSVAL(obj);
    break;
  }
  }

  return true;
}












JSBool
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
  if (!JSVAL_IS_PRIMITIVE(val) &&
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
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_JSCHAR_TYPE(name, type, ffiType)                                \
  case TYPE_##name: {                                                          \
    /* Convert from a 1-character string, regardless of encoding, */           \
    /* or from an integer, provided the result fits in 'type'. */              \
    type result;                                                               \
    if (JSVAL_IS_STRING(val)) {                                                \
      JSString* str = JSVAL_TO_STRING(val);                                    \
      if (JS_GetStringLength(str) != 1)                                        \
        return TypeError(cx, #name, val);                                      \
                                                                               \
      jschar c = *JS_GetStringChars(str);                                      \
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
    }

    if (sourceData) {
      
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
      const jschar* sourceChars = JS_GetStringChars(sourceString);
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
        memcpy(*jscharBuffer, sourceChars, sourceLength * sizeof(jschar));
        (*jscharBuffer)[sourceLength] = 0;
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
      const jschar* sourceChars = JS_GetStringChars(sourceString);
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

    } else if (!JSVAL_IS_PRIMITIVE(val) &&
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
      nsAutoArrayPtr<char> intermediate(new char[arraySize]);
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      for (jsuint i = 0; i < sourceLength; ++i) {
        JSAutoTempValueRooter item(cx);
        if (!JS_GetElement(cx, sourceArray, i, item.addr()))
          return false;

        char* data = intermediate + elementSize * i;
        if (!ImplicitConvert(cx, item.value(), baseType, data, false, NULL))
          return false;
      }

      memcpy(buffer, intermediate, arraySize);

    } else {
      
      
      return TypeError(cx, "array", val);
    }
    break;
  }
  case TYPE_struct: {
    if (!JSVAL_IS_PRIMITIVE(val) && !sourceData) {
      nsTArray<FieldInfo>* fields = StructType::GetFieldInfo(cx, targetType);

      
      
      JSObject* obj = JSVAL_TO_OBJECT(val);
      JSObject* iter = JS_NewPropertyIterator(cx, obj);
      if (!iter)
        return false;
      JSAutoTempValueRooter iterroot(cx, iter);

      
      size_t structSize = CType::GetSize(cx, targetType);
      nsAutoArrayPtr<char> intermediate(new char[structSize]);
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      jsid id;
      jsuint i = 0;
      while (1) {
        if (!JS_NextProperty(cx, iter, &id))
          return false;
        if (JSVAL_IS_VOID(id))
          break;

        JSAutoTempValueRooter fieldVal(cx);
        if (!JS_IdToValue(cx, id, fieldVal.addr()))
          return false;
        if (!JSVAL_IS_STRING(fieldVal.value())) {
          JS_ReportError(cx, "property name is not a string");
          return false;
        }

        FieldInfo* field = StructType::LookupField(cx, targetType,
          fieldVal.value());
        if (!field)
          return false;

        JSString* nameStr = JSVAL_TO_STRING(fieldVal.value());
        const jschar* name = JS_GetStringChars(nameStr);
        size_t namelen = JS_GetStringLength(nameStr);

        JSAutoTempValueRooter prop(cx);
        if (!JS_GetUCProperty(cx, obj, name, namelen, prop.addr()))
          return false;

        
        char* fieldData = intermediate + field->mOffset;
        if (!ImplicitConvert(cx, prop.value(), field->mType, fieldData, false, NULL))
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




JSBool
ExplicitConvert(JSContext* cx, jsval val, JSObject* targetType, void* buffer)
{
  
  if (ImplicitConvert(cx, val, targetType, buffer, false, NULL))
    return true;

  
  
  
  JSAutoTempValueRooter ex(cx);
  if (!JS_GetPendingException(cx, ex.addr()))
    return false;

  
  
  JS_ClearPendingException(cx);

  TypeCode type = CType::GetTypeCode(cx, targetType);

  switch (type) {
  case TYPE_bool: {
    
    JSBool result;
    ASSERT_OK(JS_ValueToBoolean(cx, val, &result));
    *static_cast<bool*>(buffer) = result != JS_FALSE;
    break;
  }
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
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
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_JSCHAR_TYPE(x, y, z) DEFINE_CHAR_TYPE(x, y, z)
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
    
    JS_SetPendingException(cx, ex.value());
    return false;
  case TYPE_void_t:
    JS_NOT_REACHED("invalid type");
    return false;
  }
  return true;
}





static nsAutoString
BuildTypeName(JSContext* cx, JSObject* typeObj)
{
  
  
  
  
  
  
  nsAutoString result;
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
  result.Insert(GetString(baseName), 0);
  return result;
}








static nsAutoString
BuildTypeSource(JSContext* cx, JSObject* typeObj, bool makeShort)
{
  
  nsAutoString result;
  switch (CType::GetTypeCode(cx, typeObj)) {
  case TYPE_void_t:
#define DEFINE_TYPE(name, type, ffiType)  \
  case TYPE_##name:
#include "typedefs.h"
  {
    result.Append(NS_LITERAL_STRING("ctypes."));
    JSString* nameStr = CType::GetName(cx, typeObj);
    result.Append(GetString(nameStr));
    break;
  }
  case TYPE_pointer: {
    JSObject* baseType = PointerType::GetBaseType(cx, typeObj);
    if (!baseType) {
      
      result.Append(NS_LITERAL_STRING("ctypes.PointerType(\""));
      JSString* baseName = CType::GetName(cx, typeObj);
      result.Append(GetString(baseName));
      result.Append(NS_LITERAL_STRING("\")"));
      break;
    }

    
    if (CType::GetTypeCode(cx, baseType) == TYPE_void_t) {
      result.Append(NS_LITERAL_STRING("ctypes.voidptr_t"));
      break;
    }

    
    result.Append(BuildTypeSource(cx, baseType, makeShort));
    result.Append(NS_LITERAL_STRING(".ptr"));
    break;
  }
  case TYPE_array: {
    
    
    
    JSObject* baseType = ArrayType::GetBaseType(cx, typeObj);
    result.Append(BuildTypeSource(cx, baseType, makeShort));
    result.Append(NS_LITERAL_STRING(".array("));

    size_t length;
    if (ArrayType::GetSafeLength(cx, typeObj, &length))
      result.Append(IntegerToString(length, 10));

    result.Append(')');
    break;
  }
  case TYPE_struct: {
    JSString* name = CType::GetName(cx, typeObj);

    if (makeShort) {
      
      
      result.Append(GetString(name));
      break;
    }

    
    result.Append(NS_LITERAL_STRING("ctypes.StructType(\""));
    result.Append(GetString(name));
    result.Append(NS_LITERAL_STRING("\", ["));

    nsTArray<FieldInfo>* fields = StructType::GetFieldInfo(cx, typeObj);
    for (PRUint32 i = 0; i < fields->Length(); ++i) {
      const FieldInfo& field = fields->ElementAt(i);
      result.Append(NS_LITERAL_STRING("{ \""));
      result.Append(field.mName);
      result.Append(NS_LITERAL_STRING("\": "));
      result.Append(BuildTypeSource(cx, field.mType, true));
      result.Append(NS_LITERAL_STRING(" }"));
      if (i != fields->Length() - 1)
        result.Append(NS_LITERAL_STRING(", "));
    }

    result.Append(NS_LITERAL_STRING("])"));
    break;
  }
  }

  return result;
}











static nsAutoString
BuildDataSource(JSContext* cx, JSObject* typeObj, void* data, bool isImplicit)
{
  nsAutoString result;
  TypeCode type = CType::GetTypeCode(cx, typeObj);
  switch (type) {
  case TYPE_bool:
    result.Append(*static_cast<bool*>(data) ?
                  NS_LITERAL_STRING("true") :
                  NS_LITERAL_STRING("false"));
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
      result.Append(NS_LITERAL_STRING("ctypes.UInt64(\""));                    \
    else                                                                       \
      result.Append(NS_LITERAL_STRING("ctypes.Int64(\""));                     \
                                                                               \
    result.Append(IntegerToString(*static_cast<type*>(data), 10));             \
    result.Append(NS_LITERAL_STRING("\")"));                                   \
    break;
#define DEFINE_FLOAT_TYPE(name, type, ffiType)                                 \
  case TYPE_##name: {                                                          \
    /* Serialize as a primitive double. */                                     \
    PRFloat64 fp = *static_cast<type*>(data);                                  \
    PRIntn decpt, sign;                                                        \
    char buf[128];                                                             \
    PRStatus rv = PR_dtoa(fp, 0, 0, &decpt, &sign, NULL, buf, sizeof(buf));    \
    JS_ASSERT(rv == PR_SUCCESS);                                               \
    result.AppendASCII(buf);                                                   \
    break;                                                                     \
  }
#define DEFINE_CHAR_TYPE(name, type, ffiType)                                  \
  case TYPE_##name:                                                            \
    /* Serialize as an integer. */                                             \
    result.Append(IntegerToString(*static_cast<type*>(data), 10));             \
    break;
#include "typedefs.h"
  case TYPE_jschar: {
    
    JSString* str = JS_NewUCStringCopyN(cx, static_cast<jschar*>(data), 1);
    if (!str)
      break;

    JSString* src = JS_ValueToSource(cx, STRING_TO_JSVAL(str));
    if (!src)
      break;

    result.Append(GetString(src));
    break;
  }
  case TYPE_pointer: {
    if (isImplicit) {
      
      
      result.Append(BuildTypeSource(cx, typeObj, true));
      result.Append('(');
    }

    
    uintptr_t ptr = *static_cast<uintptr_t*>(data);
    result.Append(NS_LITERAL_STRING("ctypes.UInt64(\"0x"));
    result.Append(IntegerToString(ptr, 16));
    result.Append(NS_LITERAL_STRING("\")"));

    if (isImplicit)
      result.Append(')');

    break;
  }
  case TYPE_array: {
    
    
    JSObject* baseType = ArrayType::GetBaseType(cx, typeObj);
    result.Append('[');

    size_t length = ArrayType::GetLength(cx, typeObj);
    size_t elementSize = CType::GetSize(cx, baseType);
    for (size_t i = 0; i < length; ++i) {
      char* element = static_cast<char*>(data) + elementSize * i;
      result.Append(BuildDataSource(cx, baseType, element, true));
      if (i + 1 < length)
        result.Append(NS_LITERAL_STRING(", "));
    }
    result.Append(']');
    break;
  }
  case TYPE_struct: {
    if (isImplicit) {
      
      
      
      result.Append('{');
    }

    
    
    nsTArray<FieldInfo>* fields = StructType::GetFieldInfo(cx, typeObj);
    for (size_t i = 0; i < fields->Length(); ++i) {
      const FieldInfo& field = fields->ElementAt(i);
      char* fieldData = static_cast<char*>(data) + field.mOffset;

      if (isImplicit) {
        result.Append('"');
        result.Append(field.mName);
        result.Append(NS_LITERAL_STRING("\": "));
      }

      result.Append(BuildDataSource(cx, field.mType, fieldData, true));
      if (i + 1 != fields->Length())
        result.Append(NS_LITERAL_STRING(", "));
    }

    if (isImplicit)
      result.Append('}');

    break;
  }
  case TYPE_void_t:
    JS_NOT_REACHED("invalid type");
    break;
  }

  return result;
}





JSBool
ConstructAbstract(JSContext* cx,
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
              JSObject* typeProto,
              JSObject* dataProto,
              TypeCode type,
              JSString* name,
              jsval size,
              jsval align,
              ffi_type* ffiType,
              PropertySpec* ps)
{
  JSObject* parent = JS_GetParent(cx, typeProto);
  JS_ASSERT(parent);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  JSObject* typeObj = JS_NewObject(cx, &sCTypeClass, typeProto, parent);
  if (!typeObj)
    return NULL;
  JSAutoTempValueRooter root(cx, typeObj);

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_TYPECODE, INT_TO_JSVAL(type)) ||
      !JS_SetReservedSlot(cx, typeObj, SLOT_FFITYPE, PRIVATE_TO_JSVAL(ffiType)) ||
      (name && !JS_SetReservedSlot(cx, typeObj, SLOT_NAME, STRING_TO_JSVAL(name))) ||
      !JS_SetReservedSlot(cx, typeObj, SLOT_SIZE, size) ||
      !JS_SetReservedSlot(cx, typeObj, SLOT_ALIGN, align))
    return NULL;

  
  JSObject* prototype = JS_NewObject(cx, &sCDataProtoClass, dataProto, parent);
  if (!prototype)
    return NULL;
  JSAutoTempValueRooter protoroot(cx, prototype);

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  
  
  if (ps) {
    while (ps->name) {
      if (!JS_DefineUCProperty(cx, prototype, ps->name, ps->namelen, JSVAL_VOID,
             ps->getter, ps->setter, ps->flags))
        return NULL;

      ++ps;
    }
  }

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_PROTO, OBJECT_TO_JSVAL(prototype)))
    return NULL;

  if (
      !JS_SealObject(cx, typeObj, JS_FALSE))
    return NULL;

  
  
  JS_ASSERT_IF(IsSizeDefined(cx, typeObj),
               GetSize(cx, typeObj) % GetAlignment(cx, typeObj) == 0);

  return typeObj;
}

JSObject*
CType::DefineBuiltin(JSContext* cx,
                     JSObject* parent,
                     const char* propName,
                     JSObject* typeProto,
                     JSObject* dataProto,
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

  
  JSObject* typeObj = Create(cx, typeProto, dataProto, type, nameStr, size,
                        align, ffiType, NULL);
  if (!typeObj)
    return NULL;

  
  if (!JS_DefineProperty(cx, parent, propName, OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  return typeObj;
}

void
CType::Finalize(JSContext* cx, JSObject* obj)
{
  
  jsval slot;
  if (!JS_GetReservedSlot(cx, obj, SLOT_TYPECODE, &slot) || JSVAL_IS_VOID(slot))
    return;

  
  switch (TypeCode(JSVAL_TO_INT(slot))) {
  case TYPE_struct:
    
    ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_FIELDINFO, &slot));
    if (!JSVAL_IS_VOID(slot))
      delete static_cast<nsTArray<FieldInfo>*>(JSVAL_TO_PRIVATE(slot));

    
  case TYPE_array: {
    
    jsval slot;
    ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_FFITYPE, &slot));
    if (!JSVAL_IS_VOID(slot) && JSVAL_TO_PRIVATE(slot)) {
      ffi_type* ffiType = static_cast<ffi_type*>(JSVAL_TO_PRIVATE(slot));
      delete[] ffiType->elements;
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
  ASSERT_OK(JS_GetReservedSlot(cx, typeObj, SLOT_TYPECODE, &result));
  return TypeCode(JSVAL_TO_INT(result));
}

bool
CType::TypesEqual(JSContext* cx, JSObject* t1, JSObject* t2)
{
  JS_ASSERT(IsCType(cx, t1) && IsCType(cx, t2));

  
  if (t1 == t2)
    return true;

  
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
    if (d1 != d2 || (d1 && s1 != s2))
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
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_SIZE, &size));

  
  
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
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_SIZE, &size));

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
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_SIZE, &size));

  
  
  JS_ASSERT(JSVAL_IS_INT(size) || JSVAL_IS_DOUBLE(size) || JSVAL_IS_VOID(size));
  return !JSVAL_IS_VOID(size);
}

size_t
CType::GetAlignment(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_ALIGN, &slot));
  return static_cast<size_t>(JSVAL_TO_INT(slot));
}

ffi_type*
CType::GetFFIType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_FFITYPE, &slot));

  ffi_type* result = static_cast<ffi_type*>(JSVAL_TO_PRIVATE(slot));
  JS_ASSERT(result);
  return result;
}

JSString*
CType::GetName(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval string;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_NAME, &string));
  return JSVAL_TO_STRING(string);
}

JSObject*
CType::GetProtoFromCtor(JSContext* cx, JSObject* obj, CTypeProtoSlot slot)
{
  
  
  jsval protoslot;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_FN_CTORPROTO, &protoslot));
  JSObject* proto = JSVAL_TO_OBJECT(protoslot);
  JS_ASSERT(proto);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sCTypeProtoClass);

  
  jsval result;
  ASSERT_OK(JS_GetReservedSlot(cx, proto, slot, &result));
  return JSVAL_TO_OBJECT(result);
}

JSObject*
CType::GetProtoFromType(JSContext* cx, JSObject* obj, CTypeProtoSlot slot)
{
  JS_ASSERT(IsCType(cx, obj));

  
  JSObject* proto = JS_GetPrototype(cx, obj);
  JS_ASSERT(proto);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sCTypeProtoClass);

  
  jsval result;
  ASSERT_OK(JS_GetReservedSlot(cx, proto, slot, &result));
  return JSVAL_TO_OBJECT(result);
}

JSBool
CType::PrototypeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_PROTO, vp));
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp));
  return JS_TRUE;
}

JSBool
CType::NameGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_NAME, vp));
  JS_ASSERT(JSVAL_IS_STRING(*vp));
  return JS_TRUE;
}

JSBool
CType::SizeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CType::IsCType(cx, obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_SIZE, vp));
  JS_ASSERT(JSVAL_IS_NUMBER(*vp) || JSVAL_IS_VOID(*vp));
  return JS_TRUE;
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

  nsAutoString type(NS_LITERAL_STRING("type "));
  JSString* right = GetName(cx, obj);
  type.Append(GetString(right));

  JSString* result = NewUCString(cx, type);
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

  nsAutoString source = BuildTypeSource(cx, obj, false);
  JSString* result = NewUCString(cx, source);
  if (!result)
    return JS_FALSE;
  
  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CType::HasInstance(JSContext* cx, JSObject* obj, jsval v, JSBool* bp)
{
  JS_ASSERT(CType::IsCType(cx, obj));

  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_PROTO, &slot));
  JSObject* prototype = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(prototype);
  JS_ASSERT(JS_GET_CLASS(cx, prototype) == &sCDataProtoClass);

  *bp = JS_FALSE;
  if (JSVAL_IS_PRIMITIVE(v))
    return JS_TRUE;

  JSObject* proto = JSVAL_TO_OBJECT(v);
  while ((proto = JS_GetPrototype(cx, proto))) {
    if (proto == prototype) {
      *bp = JS_TRUE;
      break;
    }
  }
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
  if (!JSVAL_IS_PRIMITIVE(arg) &&
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
    ASSERT_OK(JS_GetReservedSlot(cx, baseType, SLOT_PTR, &slot));
    if (!JSVAL_IS_VOID(slot))
      return JSVAL_TO_OBJECT(slot);
  }

  
  
  JSObject* typeProto;
  JSObject* dataProto;
  if (ctor) {
    typeProto = CType::GetProtoFromCtor(cx, ctor, SLOT_POINTERPROTO);
    dataProto = CType::GetProtoFromCtor(cx, ctor, SLOT_POINTERDATAPROTO);
  } else {
    typeProto = CType::GetProtoFromType(cx, baseType, SLOT_POINTERPROTO);
    dataProto = CType::GetProtoFromType(cx, baseType, SLOT_POINTERDATAPROTO);
  }

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_pointer,
                        name, INT_TO_JSVAL(sizeof(void*)),
                        INT_TO_JSVAL(ffi_type_pointer.alignment),
                        &ffi_type_pointer, NULL);
  if (!typeObj)
    return NULL;
  JSAutoTempValueRooter root(cx, typeObj);

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_TARGET_T, OBJECT_TO_JSVAL(baseType)))
    return NULL;

  if (baseType) {
    
    nsAutoString typeName = BuildTypeName(cx, typeObj);
    JSString* nameStr = NewUCString(cx, typeName);
    if (!nameStr ||
        !JS_SetReservedSlot(cx, typeObj, SLOT_NAME, STRING_TO_JSVAL(nameStr)))
      return NULL;

    
    if (!JS_SetReservedSlot(cx, baseType, SLOT_PTR, OBJECT_TO_JSVAL(typeObj)))
      return NULL;
  }

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
PointerType::GetBaseType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_pointer);

  jsval type;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_TARGET_T, &type));
  return JSVAL_TO_OBJECT(type);
}

JSBool
PointerType::TargetTypeGetter(JSContext* cx,
                              JSObject* obj,
                              jsval idval,
                              jsval* vp)
{
  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_TARGET_T, vp));
  JS_ASSERT(JSVAL_IS_OBJECT(*vp));
  return JS_TRUE;
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
  if (CType::GetTypeCode(cx, typeObj) != TYPE_pointer) {
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
  
  if (argc < 1 || argc > 2) {
    JS_ReportError(cx, "ArrayType takes one or two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (JSVAL_IS_PRIMITIVE(argv[0]) ||
      !CType::IsCType(cx, JSVAL_TO_OBJECT(argv[0]))) {
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
  
  
  JSObject* typeProto = CType::GetProtoFromType(cx, baseType, SLOT_ARRAYPROTO);
  JSObject* dataProto = CType::GetProtoFromType(cx, baseType, SLOT_ARRAYDATAPROTO);

  
  
  
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

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_array, NULL,
                        sizeVal, INT_TO_JSVAL(align), ffiType, NULL);
  if (!typeObj)
    return NULL;
  JSAutoTempValueRooter root(cx, typeObj);

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_ELEMENT_T, OBJECT_TO_JSVAL(baseType)))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_LENGTH, lengthVal))
    return NULL;

  
  nsAutoString typeName = BuildTypeName(cx, typeObj);
  JSString* name = NewUCString(cx, typeName);
  if (!name ||
      !JS_SetReservedSlot(cx, typeObj, SLOT_NAME, STRING_TO_JSVAL(name)))
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

    } else if (!JSVAL_IS_PRIMITIVE(argv[0])) {
      
      
      JSObject* arg = JSVAL_TO_OBJECT(argv[0]);
      JSAutoTempValueRooter lengthVal(cx);
      if (!JS_GetProperty(cx, arg, "length", lengthVal.addr()) ||
          !jsvalToSize(cx, lengthVal.value(), false, &length)) {
        JS_ReportError(cx, "argument must be an array object or length");
        return JS_FALSE;
      }

    } else if (JSVAL_IS_STRING(argv[0])) {
      
      
      JSString* sourceString = JSVAL_TO_STRING(argv[0]);
      const jschar* sourceChars = JS_GetStringChars(sourceString);
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

  JSObject* result = CData::Create(cx, obj, NULL, NULL);
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
ArrayType::GetBaseType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_array);

  jsval type;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_ELEMENT_T, &type));
  return JSVAL_TO_OBJECT(type);
}

bool
ArrayType::GetSafeLength(JSContext* cx, JSObject* obj, size_t* result)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_array);

  jsval length;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_LENGTH, &length));

  
  
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
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_LENGTH, &length));

  JS_ASSERT(!JSVAL_IS_VOID(length));

  
  
  
  if (JSVAL_IS_INT(length))
    return JSVAL_TO_INT(length);
  return Convert<size_t>(*JSVAL_TO_DOUBLE(length));
}

JSBool
ArrayType::ElementTypeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_ELEMENT_T, vp));
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp));
  return JS_TRUE;
}

JSBool
ArrayType::LengthGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  
  
  if (CData::IsCData(cx, obj))
    obj = CData::GetCType(cx, obj);

  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_LENGTH, vp));
  JS_ASSERT(JSVAL_IS_NUMBER(*vp) || JSVAL_IS_VOID(*vp));
  return JS_TRUE;
}

JSBool
ArrayType::Getter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  
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
  
  if (!CData::IsCData(cx, obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  
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

  
  JSObject* result = CData::Create(cx, pointerType, NULL, NULL);
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







static JSBool
ExtractStructField(JSContext* cx, jsval val, FieldInfo* field)
{
  if (JSVAL_IS_PRIMITIVE(val)) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return false;
  }

  JSObject* obj = JSVAL_TO_OBJECT(val);
  JSObject* iter = JS_NewPropertyIterator(cx, obj);
  if (!iter)
    return false;
  JSAutoTempValueRooter iterroot(cx, iter);

  jsid id;
  if (!JS_NextProperty(cx, iter, &id))
    return false;

  JSAutoTempValueRooter nameVal(cx);
  if (!JS_IdToValue(cx, id, nameVal.addr()))
    return false;
  if (!JSVAL_IS_STRING(nameVal.value())) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return false;
  }

  
  if (!JS_NextProperty(cx, iter, &id))
    return false;
  if (!JSVAL_IS_VOID(id)) {
    JS_ReportError(cx, "struct field descriptors must contain one property");
    return false;
  }

  JSString* name = JSVAL_TO_STRING(nameVal.value());
  const jschar* nameChars = JS_GetStringChars(name);
  size_t namelen = JS_GetStringLength(name);
  field->mName.Assign(reinterpret_cast<const PRUnichar*>(nameChars), namelen);

  JSAutoTempValueRooter propVal(cx);
  if (!JS_GetUCProperty(cx, obj, nameChars, namelen, propVal.addr()))
    return false;

  if (JSVAL_IS_PRIMITIVE(propVal.value()) ||
      !CType::IsCType(cx, JSVAL_TO_OBJECT(propVal.value()))) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return false;
  }

  
  
  
  field->mType = JSVAL_TO_OBJECT(propVal.value());
  size_t size;
  if (!CType::GetSafeSize(cx, field->mType, &size) || size == 0) {
    JS_ReportError(cx, "struct field types must have defined and nonzero size");
    return false;
  }

  return true;
}



static JSBool
AddFieldToArray(JSContext* cx,
                JSObject* arrayObj,
                jsuint index,
                const nsString& name,
                JSObject* typeObj)
{
  JSObject* fieldObj = JS_NewObject(cx, NULL, NULL, arrayObj);
  if (!fieldObj)
    return false;

  if (!JS_DefineElement(cx, arrayObj, index, OBJECT_TO_JSVAL(fieldObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (!JS_DefineUCProperty(cx, fieldObj,
         reinterpret_cast<const jschar*>(name.get()), name.Length(),
         OBJECT_TO_JSVAL(typeObj), NULL, NULL,
         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return JS_SealObject(cx, fieldObj, JS_FALSE);
}

JSBool
StructType::Create(JSContext* cx, uintN argc, jsval* vp)
{
  
  if (argc != 2) {
    JS_ReportError(cx, "StructType takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  jsval name = argv[0];
  if (!JSVAL_IS_STRING(name)) {
    JS_ReportError(cx, "first argument must be a string");
    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(argv[1]) ||
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
  ASSERT_OK(JS_GetArrayLength(cx, fieldsObj, &len));

  nsAutoPtr<ffi_type> ffiType(new ffi_type);
  if (!ffiType) {
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
  }
  ffiType->type = FFI_TYPE_STRUCT;

  
  
  
  nsAutoPtr< nsTArray<FieldInfo> > fields(new nsTArray<FieldInfo>());
  nsAutoTArray<PropertySpec, 16> instanceProps;
  if (!fields ||
      !fields->SetCapacity(len) ||
      !instanceProps.SetCapacity(len + 1)) {
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
  }
  nsAutoArrayPtr<ffi_type*> elements;

  size_t structSize = 0, structAlign = 0;
  if (len != 0) {
    elements = new ffi_type*[len + 1];
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return JS_FALSE;
    }
    elements[len] = NULL;

    for (jsuint i = 0; i < len; ++i) {
      JSAutoTempValueRooter item(cx);
      if (!JS_GetElement(cx, fieldsObj, i, item.addr()))
        return JS_FALSE;

      FieldInfo* info = fields->AppendElement();
      if (!ExtractStructField(cx, item.value(), info))
        return JS_FALSE;

      
      for (PRUint32 j = 0; j < fields->Length() - 1; ++j) {
        if (fields->ElementAt(j).mName == info->mName) {
          JS_ReportError(cx, "struct fields must have unique names");
          return JS_FALSE;
        }
      }

      
      if (!AddFieldToArray(cx, fieldsProp, i, info->mName, info->mType))
        return JS_FALSE;

      
      PropertySpec* instanceProp = instanceProps.AppendElement();
      instanceProp->name = reinterpret_cast<const jschar*>(info->mName.get());
      instanceProp->namelen = info->mName.Length();
      instanceProp->flags = JSPROP_SHARED | JSPROP_ENUMERATE | JSPROP_PERMANENT;
      instanceProp->getter = StructType::FieldGetter;
      instanceProp->setter = StructType::FieldSetter;

      elements[i] = CType::GetFFIType(cx, info->mType);

      size_t fieldSize = CType::GetSize(cx, info->mType);
      size_t fieldAlign = CType::GetAlignment(cx, info->mType);
      size_t fieldOffset = Align(structSize, fieldAlign);
      
      
      
      if (fieldOffset + fieldSize < structSize) {
        JS_ReportError(cx, "size overflow");
        return JS_FALSE;
      }
      info->mOffset = fieldOffset;
      structSize = fieldOffset + fieldSize;

      if (fieldAlign > structAlign)
        structAlign = fieldAlign;
    }

    
    size_t structTail = Align(structSize, structAlign);
    if (structTail < structSize) {
      JS_ReportError(cx, "size overflow");
      return JS_FALSE;
    }
    structSize = structTail;

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

  
  instanceProps.AppendElement()->name = NULL;

  jsval sizeVal;
  if (!SizeTojsval(cx, structSize, &sizeVal))
    return JS_FALSE;

  
  
  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  JSObject* typeProto = CType::GetProtoFromCtor(cx, callee, SLOT_STRUCTPROTO);
  JSObject* dataProto = CType::GetProtoFromCtor(cx, callee, SLOT_STRUCTDATAPROTO);

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_struct,
                        JSVAL_TO_STRING(name), sizeVal,
                        INT_TO_JSVAL(structAlign), ffiType,
                        instanceProps.Elements());
  if (!typeObj)
    return JS_FALSE;
  ffiType.forget();
  elements.forget();

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(typeObj));

  
  
  if (!JS_SealObject(cx, fieldsProp, JS_FALSE) ||
      !JS_SetReservedSlot(cx, typeObj, SLOT_FIELDS, OBJECT_TO_JSVAL(fieldsProp)))
    return JS_FALSE;

  
  if (!JS_SetReservedSlot(cx, typeObj, SLOT_FIELDINFO,
         PRIVATE_TO_JSVAL(fields.get())))
    return JS_FALSE;
  fields.forget();

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

  JSObject* result = CData::Create(cx, obj, NULL, NULL);
  if (!result)
    return JS_FALSE;

  *rval = OBJECT_TO_JSVAL(result);

  if (argc == 0)
    return JS_TRUE;

  char* buffer = static_cast<char*>(CData::GetData(cx, result));
  nsTArray<FieldInfo>* fields = GetFieldInfo(cx, obj);

  if (argc == 1) {
    
    
    
    
    
    
    

    
    if (ExplicitConvert(cx, argv[0], obj, buffer))
      return JS_TRUE;

    if (fields->Length() != 1)
      return JS_FALSE;

    
    
    if (!JS_IsExceptionPending(cx))
      return JS_FALSE;

    
    
    JS_ClearPendingException(cx);

    
  }

  
  
  if (argc == fields->Length()) {
    for (PRUint32 i = 0; i < fields->Length(); ++i) {
      FieldInfo& field = fields->ElementAt(i);
      if (!ImplicitConvert(cx, argv[i], field.mType, buffer + field.mOffset,
             false, NULL))
        return JS_FALSE;
    }

    return JS_TRUE;
  }

  JS_ReportError(cx, "constructor takes 0, 1, or %u arguments",
    fields->Length());
  return JS_FALSE;
}

nsTArray<FieldInfo>*
StructType::GetFieldInfo(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_struct);

  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_FIELDINFO, &slot));
  JS_ASSERT(!JSVAL_IS_VOID(slot) && JSVAL_TO_PRIVATE(slot));

  return static_cast<nsTArray<FieldInfo>*>(JSVAL_TO_PRIVATE(slot));
}

FieldInfo*
StructType::LookupField(JSContext* cx, JSObject* obj, jsval idval)
{
  JS_ASSERT(CType::IsCType(cx, obj));
  JS_ASSERT(CType::GetTypeCode(cx, obj) == TYPE_struct);

  nsTArray<FieldInfo>* fields = GetFieldInfo(cx, obj);

  JSString* nameStr = JSVAL_TO_STRING(idval);
  const nsDependentString name(GetString(nameStr));

  for (PRUint32 i = 0; i < fields->Length(); ++i) {
    if (fields->ElementAt(i).mName.Equals(name))
      return &fields->ElementAt(i);
  }

  JS_ReportError(cx, "%s does not name a field",
    NS_LossyConvertUTF16toASCII(name).get());
  return NULL;
}

JSBool
StructType::FieldsArrayGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp)
{
  if (!CType::IsCType(cx, obj) || CType::GetTypeCode(cx, obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_FIELDS, vp));
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp) &&
            JS_IsArrayObject(cx, JSVAL_TO_OBJECT(*vp)));
  return JS_TRUE;
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
  if (!field)
    return JS_FALSE;

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
  if (!field)
    return JS_FALSE;

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
  if (!field)
    return JS_FALSE;

  JSObject* baseType = field->mType;
  JSObject* pointerType = PointerType::CreateInternal(cx, NULL, baseType, NULL);
  if (!pointerType)
    return JS_FALSE;
  JSAutoTempValueRooter root(cx, pointerType);

  
  JSObject* result = CData::Create(cx, pointerType, NULL, NULL);
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

  
  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, typeObj, SLOT_PROTO, &slot));
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(slot));

  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JSObject* parent = JS_GetParent(cx, typeObj);
  JS_ASSERT(parent);

  JSObject* dataObj = JS_NewObject(cx, &sCDataClass, proto, parent);
  if (!dataObj)
    return NULL;
  JSAutoTempValueRooter root(cx, dataObj);

  
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
  ASSERT_OK(JS_GetReservedSlot(cx, dataObj, SLOT_CTYPE, &slot));
  JSObject* typeObj = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(CType::IsCType(cx, typeObj));
  return typeObj;
}

void*
CData::GetData(JSContext* cx, JSObject* dataObj)
{
  JS_ASSERT(CData::IsCData(cx, dataObj));

  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, dataObj, SLOT_DATA, &slot));

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

  
  JSObject* result = CData::Create(cx, pointerType, NULL, NULL);
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
  if (JSVAL_IS_PRIMITIVE(argv[0]) ||
      !CData::IsCData(cx, JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "first argument must be a CData");
    return JS_FALSE;
  }
  JSObject* sourceData = JSVAL_TO_OBJECT(argv[0]);
  JSObject* sourceType = CData::GetCType(cx, sourceData);

  if (JSVAL_IS_PRIMITIVE(argv[1]) ||
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

  
  
  void* data = CData::GetData(cx, sourceData);
  JSObject* result = CData::Create(cx, targetType, sourceData, data);
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

    result = NewUCString(cx, NS_ConvertUTF8toUTF16(string));
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

  
  
  
  
  
  
  nsAutoString source = BuildTypeSource(cx, typeObj, true);
  source.Append('(');
  source.Append(BuildDataSource(cx, typeObj, data, false));
  source.Append(')');

  JSString* result = NewUCString(cx, source);
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
  JSObject* result = JS_NewObject(cx, clasp, proto, JS_GetParent(cx, proto));
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

  delete static_cast<PRUint64*>(JSVAL_TO_PRIVATE(slot));
}

PRUint64
Int64Base::GetInt(JSContext* cx, JSObject* obj) {
  JS_ASSERT(Int64::IsInt64(cx, obj) || UInt64::IsUInt64(cx, obj));

  jsval slot;
  ASSERT_OK(JS_GetReservedSlot(cx, obj, SLOT_INT64, &slot));
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

  nsAutoString intString;
  if (isUnsigned) {
    intString = IntegerToString(GetInt(cx, obj), radix);
  } else {
    intString = IntegerToString(static_cast<PRInt64>(GetInt(cx, obj)), radix);
  }

  JSString *result = NewUCString(cx, intString);
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

  
  nsAutoString source;
  if (isUnsigned) {
    source.Append(NS_LITERAL_STRING("ctypes.UInt64(\""));
    source.Append(IntegerToString(GetInt(cx, obj), 10));
  } else {
    source.Append(NS_LITERAL_STRING("ctypes.Int64(\""));
    source.Append(IntegerToString(static_cast<PRInt64>(GetInt(cx, obj)), 10));
  }
  source.Append(NS_LITERAL_STRING("\")"));

  JSString *result = NewUCString(cx, source);
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
  ASSERT_OK(JS_GetProperty(cx, JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv)),
    "prototype", &slot));
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sInt64ProtoClass);

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
      JSVAL_IS_PRIMITIVE(argv[0]) ||
      JSVAL_IS_PRIMITIVE(argv[1]) ||
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
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !Int64::IsInt64(cx, JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "lo takes one Int64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
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
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !Int64::IsInt64(cx, JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "hi takes one Int64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
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
  ASSERT_OK(JS_GetReservedSlot(cx, callee, SLOT_FN_INT64PROTO, &slot));
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sInt64ProtoClass);

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
  ASSERT_OK(JS_GetProperty(cx, JSVAL_TO_OBJECT(JS_ARGV_CALLEE(argv)),
    "prototype", &slot));
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sUInt64ProtoClass);

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
      JSVAL_IS_PRIMITIVE(argv[0]) ||
      JSVAL_IS_PRIMITIVE(argv[1]) ||
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
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !UInt64::IsUInt64(cx, JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "lo takes one UInt64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
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
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !UInt64::IsUInt64(cx, JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "hi takes one UInt64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
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
  ASSERT_OK(JS_GetReservedSlot(cx, callee, SLOT_FN_INT64PROTO, &slot));
  JSObject* proto = JSVAL_TO_OBJECT(slot);
  JS_ASSERT(JS_GET_CLASS(cx, proto) == &sUInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

}
}

