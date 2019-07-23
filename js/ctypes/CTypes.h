





































#ifndef CTYPES_H
#define CTYPES_H

#include "jsapi.h"
#include "nsString.h"
#include "ffi.h"

namespace mozilla {
namespace ctypes {


enum ErrorNum {
#define MSG_DEF(name, number, count, exception, format) \
  name = number,
#include "ctypes.msg"
#undef MSG_DEF
  CTYPESERR_LIMIT
};

const JSErrorFormatString*
GetErrorMessage(void* userRef, const char* locale, const uintN errorNumber);
JSBool TypeError(JSContext* cx, const char* expected, jsval actual);







enum ABICode {
  ABI_DEFAULT,
  ABI_STDCALL,
  INVALID_ABI
};

enum TypeCode {
  TYPE_void_t,
#define DEFINE_TYPE(name, type, ffiType) TYPE_##name,
#include "typedefs.h"
  TYPE_pointer,
  TYPE_array,
  TYPE_struct
};

ABICode GetABICode(JSContext* cx, JSObject* obj);

struct FieldInfo
{
  nsString  mName;
  JSObject* mType;
  size_t    mOffset;
};


struct PropertySpec
{
  const jschar* name;
  size_t namelen;
  uint8 flags;
  JSPropertyOp getter;
  JSPropertyOp setter;
};

JSBool InitTypeClasses(JSContext* cx, JSObject* parent);

JSBool ConvertToJS(JSContext* cx, JSObject* typeObj, JSObject* dataObj, void* data, bool wantPrimitive, jsval* result);
JSBool ImplicitConvert(JSContext* cx, jsval val, JSObject* targetType, void* buffer, bool isArgument, bool* freePointer);
JSBool ExplicitConvert(JSContext* cx, jsval val, JSObject* targetType, void* buffer);



enum CABISlot {
  SLOT_ABICODE = 0, 
  CABI_SLOTS
};

enum CTypeProtoSlot {
  SLOT_POINTERPROTO = 0, 
  SLOT_ARRAYPROTO   = 1, 
  SLOT_STRUCTPROTO  = 2, 
  SLOT_INT64PROTO   = 3, 
  SLOT_UINT64PROTO  = 4, 
  CTYPEPROTO_SLOTS
};

enum CTypeSlot {
  SLOT_PROTO     = 0, 
  SLOT_TYPECODE  = 1, 
  SLOT_FFITYPE   = 2, 
  SLOT_NAME      = 3, 
  SLOT_SIZE      = 4, 
  SLOT_ALIGN     = 5, 
  SLOT_PTR       = 6, 
  
  
  SLOT_TARGET_T  = 7, 
  SLOT_ELEMENT_T = 7, 
  SLOT_LENGTH    = 8, 
  SLOT_FIELDS    = 7, 
  SLOT_FIELDINFO = 8, 
  CTYPE_SLOTS
};

enum CDataSlot {
  SLOT_CTYPE    = 0, 
  SLOT_REFERENT = 1, 
  SLOT_DATA     = 2, 
  CDATA_SLOTS
};

enum Int64Slot {
  SLOT_INT64 = 0, 
  INT64_SLOTS
};

enum Int64FunctionSlot {
  SLOT_FN_INT64PROTO = 0 
  
};

class CType {
public:
  static JSObject* Create(JSContext* cx, JSObject* proto, TypeCode type, JSString* name, jsval size, jsval align, ffi_type* ffiType, JSFunctionSpec* fs, PropertySpec* ps);
  static JSObject* DefineBuiltin(JSContext* cx, JSObject* parent, const char* propName, JSObject* proto, const char* name, TypeCode type, jsval size, jsval align, ffi_type* ffiType);
  static void Finalize(JSContext* cx, JSObject* obj);

  static JSBool ConstructAbstract(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
  static JSBool ConstructData(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
  static JSBool ConstructBasic(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

  static bool IsCType(JSContext* cx, JSObject* obj);
  static TypeCode GetTypeCode(JSContext* cx, JSObject* typeObj);
  static bool TypesEqual(JSContext* cx, JSObject* t1, JSObject* t2);
  static size_t GetSize(JSContext* cx, JSObject* obj);
  static bool GetSafeSize(JSContext* cx, JSObject* obj, size_t* result);
  static bool IsSizeDefined(JSContext* cx, JSObject* obj);
  static size_t GetAlignment(JSContext* cx, JSObject* obj);
  static ffi_type* GetFFIType(JSContext* cx, JSObject* obj);
  static JSString* GetName(JSContext* cx, JSObject* obj);
  static JSObject* GetProtoFromCtor(JSContext* cx, JSObject* obj);
  static JSObject* GetProtoFromType(JSContext* cx, JSObject* obj, CTypeProtoSlot slot);

  static JSBool PrototypeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool NameGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool SizeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool PtrGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool Array(JSContext* cx, uintN argc, jsval* vp);
  static JSBool ToString(JSContext* cx, uintN argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, uintN argc, jsval* vp);
};

class PointerType {
public:
  static JSBool Create(JSContext* cx, uintN argc, jsval* vp);
  static JSObject* CreateInternal(JSContext* cx, JSObject* ctor, JSObject* baseType, JSString* name);

  static JSBool ConstructData(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

  static JSObject* GetBaseType(JSContext* cx, JSObject* obj);

  static JSBool TargetTypeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool ContentsGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool ContentsSetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
};

class ArrayType {
public:
  static JSBool Create(JSContext* cx, uintN argc, jsval* vp);
  static JSObject* CreateInternal(JSContext* cx, JSObject* baseType, size_t length, bool lengthDefined);

  static JSBool ConstructData(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

  static JSObject* GetBaseType(JSContext* cx, JSObject* obj);
  static size_t GetLength(JSContext* cx, JSObject* obj);
  static bool GetSafeLength(JSContext* cx, JSObject* obj, size_t* result);

  static JSBool ElementTypeGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool LengthGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool Getter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool Setter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool AddressOfElement(JSContext* cx, uintN argc, jsval* vp);
};

class StructType {
public:
  static JSBool Create(JSContext* cx, uintN argc, jsval* vp);

  static JSBool ConstructData(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

  static nsTArray<FieldInfo>* GetFieldInfo(JSContext* cx, JSObject* obj);
  static FieldInfo* LookupField(JSContext* cx, JSObject* obj, jsval idval);

  static JSBool FieldsArrayGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool FieldGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool FieldSetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool AddressOfField(JSContext* cx, uintN argc, jsval* vp);
};

class CData {
public:
  static JSObject* Create(JSContext* cx, JSObject* type, JSObject* base, void* data);
  static void Finalize(JSContext* cx, JSObject* obj);

  static JSObject* GetCType(JSContext* cx, JSObject* dataObj);
  static void* GetData(JSContext* cx, JSObject* dataObj);
  static bool IsCData(JSContext* cx, JSObject* obj);

  static JSBool ValueGetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool ValueSetter(JSContext* cx, JSObject* obj, jsval idval, jsval* vp);
  static JSBool Address(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Cast(JSContext* cx, uintN argc, jsval* vp);
  static JSBool ReadString(JSContext* cx, uintN argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, uintN argc, jsval* vp);
};

class Int64Base {
public:
  static JSObject* Construct(JSContext* cx, JSObject* proto, PRUint64 data, bool isUnsigned);
  static void Finalize(JSContext* cx, JSObject* obj);

  static PRUint64 GetInt(JSContext* cx, JSObject* obj);

  static JSBool ToString(JSContext* cx, JSObject* obj, uintN argc, jsval* vp, bool isUnsigned);
  static JSBool ToSource(JSContext* cx, JSObject* obj, uintN argc, jsval* vp, bool isUnsigned);
};

class Int64 : public Int64Base {
public:
  static JSBool Construct(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

  static bool IsInt64(JSContext* cx, JSObject* obj);

  static JSBool ToString(JSContext* cx, uintN argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, uintN argc, jsval* vp);

  
  static JSBool Compare(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Lo(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Hi(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Join(JSContext* cx, uintN argc, jsval* vp);
};

class UInt64 : public Int64Base {
public:
  static JSBool Construct(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

  static bool IsUInt64(JSContext* cx, JSObject* obj);

  static JSBool ToString(JSContext* cx, uintN argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, uintN argc, jsval* vp);

  
  static JSBool Compare(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Lo(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Hi(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Join(JSContext* cx, uintN argc, jsval* vp);
};

}
}

#endif
