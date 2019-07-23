





































#ifndef CTYPES_H
#define CTYPES_H

#include "jscntxt.h"
#include "jsapi.h"
#include "prlink.h"
#include "ffi.h"

namespace js {
namespace ctypes {





template<class T>
class OperatorDelete
{
public:
  static void destroy(T* ptr) { delete ptr; }
};

template<class T>
class OperatorArrayDelete
{
public:
  static void destroy(T* ptr) { delete[] ptr; }
};



template<class T, class DeleteTraits = OperatorDelete<T> >
class AutoPtr {
private:
  typedef AutoPtr<T, DeleteTraits> self_type;

public:
  
  typedef AutoPtr<T, OperatorArrayDelete<T> > Array;

  AutoPtr() : mPtr(NULL) { }
  explicit AutoPtr(T* ptr) : mPtr(ptr) { }
  ~AutoPtr() { DeleteTraits::destroy(mPtr); }

  T*   operator->()         { return mPtr; }
  bool operator!()          { return mPtr == NULL; }
  T&   operator[](size_t i) { return *(mPtr + i); }
  
  
  

  T*   get()         { return mPtr; }
  void set(T* other) { JS_ASSERT(mPtr == NULL); mPtr = other; }
  T*   forget()      { T* result = mPtr; mPtr = NULL; return result; }

  self_type& operator=(T* rhs) { mPtr = rhs; return *this; }

private:
  
  template<class U> AutoPtr(AutoPtr<T, U>&);
  template<class U> self_type& operator=(AutoPtr<T, U>& rhs);

  T* mPtr;
};


template<class T, size_t N = 0>
class Array : public Vector<T, N, SystemAllocPolicy>
{
};


typedef Vector<jschar,  0, SystemAllocPolicy> String;
typedef Vector<jschar, 64, SystemAllocPolicy> AutoString;


template <class T, size_t N, class AP, size_t ArrayLength>
void
AppendString(Vector<T, N, AP> &v, const char (&array)[ArrayLength])
{
  
  size_t alen = ArrayLength - 1;
  size_t vlen = v.length();
  if (!v.resize(vlen + alen))
    return;

  for (size_t i = 0; i < alen; ++i)
    v[i + vlen] = array[i];
}

template <class T, size_t N, size_t M, class AP>
void
AppendString(Vector<T, N, AP> &v, Vector<T, M, AP> &w)
{
  v.append(w.begin(), w.length());
}

template <size_t N, class AP>
void
AppendString(Vector<jschar, N, AP> &v, JSString* str)
{
  JS_ASSERT(str);
  const jschar* chars = JS_GetStringChars(str);
  size_t length = JS_GetStringLength(str);
  v.append(chars, length);
}

template <class T, size_t N, class AP, size_t ArrayLength>
void
PrependString(Vector<T, N, AP> &v, const char (&array)[ArrayLength])
{
  
  size_t alen = ArrayLength - 1;
  size_t vlen = v.length();
  if (!v.resize(vlen + alen))
    return;

  
  memmove(v.begin() + alen, v.begin(), vlen * sizeof(T));

  
  for (size_t i = 0; i < alen; ++i)
    v[i] = array[i];
}

template <size_t N, class AP>
void
PrependString(Vector<jschar, N, AP> &v, JSString* str)
{
  JS_ASSERT(str);
  size_t vlen = v.length();
  size_t alen = JS_GetStringLength(str);
  if (!v.resize(vlen + alen))
    return;

  
  memmove(v.begin() + alen, v.begin(), vlen * sizeof(jschar));

  
  memcpy(v.begin(), JS_GetStringChars(str), alen * sizeof(jschar));
}

template <class T, size_t N, size_t M, class AP>
bool
StringsEqual(Vector<T, N, AP> &v, Vector<T, M, AP> &w)
{
  if (v.length() != w.length())
    return false;

  return memcmp(v.begin(), w.begin(), v.length() * sizeof(T)) == 0;
}

template <size_t N, class AP>
bool
StringsEqual(Vector<jschar, N, AP> &v, JSString* str)
{
  JS_ASSERT(str);
  size_t length = JS_GetStringLength(str);
  if (v.length() != length)
    return false;

  const jschar* chars = JS_GetStringChars(str);
  return memcmp(v.begin(), chars, length * sizeof(jschar)) == 0;
}






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
  TYPE_function,
  TYPE_array,
  TYPE_struct
};

struct FieldInfo
{
  
  FieldInfo() {}
  FieldInfo(const FieldInfo& other)
  {
    JS_NOT_REACHED("shouldn't be copy constructing FieldInfo");
  }

  String    mName;
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



struct FunctionInfo
{
  
  
  
  ffi_cif mCIF;

  
  
  JSObject* mABI;                

  
  JSObject* mReturnType;

  
  
  Array<JSObject*> mArgTypes; 

  
  
  
  Array<ffi_type*> mFFITypes;

  
  
  bool mIsVariadic;
};


struct ClosureInfo
{
  JSContext* cx;         
  JSObject* closureObj;  
  JSObject* typeObj;     
  JSObject* thisObj;     
  JSObject* jsfnObj;     
  ffi_closure* closure;  
};

JSBool InitTypeClasses(JSContext* cx, JSObject* parent);

JSBool ConvertToJS(JSContext* cx, JSObject* typeObj, JSObject* dataObj,
  void* data, bool wantPrimitive, bool ownResult, jsval* result);

JSBool ImplicitConvert(JSContext* cx, jsval val, JSObject* targetType,
  void* buffer, bool isArgument, bool* freePointer);

JSBool ExplicitConvert(JSContext* cx, jsval val, JSObject* targetType,
  void* buffer);





enum CABISlot {
  SLOT_ABICODE = 0, 
  CABI_SLOTS
};

enum CTypeProtoSlot {
  SLOT_POINTERPROTO      = 0,  
  SLOT_ARRAYPROTO        = 1,  
  SLOT_STRUCTPROTO       = 2,  
  SLOT_FUNCTIONPROTO     = 3,  
  SLOT_CDATAPROTO        = 4,  
  SLOT_POINTERDATAPROTO  = 5,  
  SLOT_ARRAYDATAPROTO    = 6,  
  SLOT_STRUCTDATAPROTO   = 7,  
  SLOT_FUNCTIONDATAPROTO = 8,  
  SLOT_INT64PROTO        = 9,  
  SLOT_UINT64PROTO       = 10, 
  SLOT_CLOSURECX         = 11, 
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
  SLOT_FNINFO    = 7, 
  SLOT_ARGS_T    = 8, 
  CTYPE_SLOTS
};

enum CDataSlot {
  SLOT_CTYPE    = 0, 
  SLOT_REFERENT = 1, 
  SLOT_DATA     = 2, 
  SLOT_OWNS     = 3, 
  CDATA_SLOTS
};

enum CClosureSlot {
  SLOT_CLOSUREINFO = 0, 
  CCLOSURE_SLOTS
};

enum TypeCtorSlot {
  SLOT_FN_CTORPROTO = 0 
  
};

enum Int64Slot {
  SLOT_INT64 = 0, 
  INT64_SLOTS
};

enum Int64FunctionSlot {
  SLOT_FN_INT64PROTO = 0 
  
};





namespace CType {
  JSObject* Create(JSContext* cx, JSObject* typeProto, JSObject* dataProto,
    TypeCode type, JSString* name, jsval size, jsval align, ffi_type* ffiType, 
    PropertySpec* ps);

  JSObject* DefineBuiltin(JSContext* cx, JSObject* parent, const char* propName,
    JSObject* typeProto, JSObject* dataProto, const char* name, TypeCode type,
    jsval size, jsval align, ffi_type* ffiType);

  bool IsCType(JSContext* cx, JSObject* obj);
  TypeCode GetTypeCode(JSContext* cx, JSObject* typeObj);
  bool TypesEqual(JSContext* cx, JSObject* t1, JSObject* t2);
  size_t GetSize(JSContext* cx, JSObject* obj);
  bool GetSafeSize(JSContext* cx, JSObject* obj, size_t* result);
  bool IsSizeDefined(JSContext* cx, JSObject* obj);
  size_t GetAlignment(JSContext* cx, JSObject* obj);
  ffi_type* GetFFIType(JSContext* cx, JSObject* obj);
  JSString* GetName(JSContext* cx, JSObject* obj);
  JSObject* GetProtoFromCtor(JSContext* cx, JSObject* obj, CTypeProtoSlot slot);
  JSObject* GetProtoFromType(JSContext* cx, JSObject* obj, CTypeProtoSlot slot);
}

namespace PointerType {
  JSObject* CreateInternal(JSContext* cx, JSObject* ctor, JSObject* baseType,
    JSString* name);

  JSObject* GetBaseType(JSContext* cx, JSObject* obj);
}

namespace ArrayType {
  JSObject* CreateInternal(JSContext* cx, JSObject* baseType, size_t length,
    bool lengthDefined);

  JSObject* GetBaseType(JSContext* cx, JSObject* obj);
  size_t GetLength(JSContext* cx, JSObject* obj);
  bool GetSafeLength(JSContext* cx, JSObject* obj, size_t* result);
}

namespace StructType {
  Array<FieldInfo>* GetFieldInfo(JSContext* cx, JSObject* obj);
  FieldInfo* LookupField(JSContext* cx, JSObject* obj, jsval idval);
}

namespace FunctionType {
  JSObject* CreateInternal(JSContext* cx, jsval abi, jsval rtype,
    jsval* argtypes, jsuint arglen);

  JSObject* ConstructWithObject(JSContext* cx, JSObject* typeObj,
    JSObject* refObj, PRFuncPtr fnptr, JSObject* result);

  FunctionInfo* GetFunctionInfo(JSContext* cx, JSObject* obj);
  JSObject* GetLibrary(JSContext* cx, JSObject* obj);
}

namespace CClosure {
  JSObject* Create(JSContext* cx, JSObject* typeObj, JSObject* fnObj,
    JSObject* thisObj, PRFuncPtr* fnptr);
}

namespace CData {
  JSObject* Create(JSContext* cx, JSObject* typeObj, JSObject* refObj,
    void* data, bool ownResult);

  JSObject* GetCType(JSContext* cx, JSObject* dataObj);
  void* GetData(JSContext* cx, JSObject* dataObj);
  bool IsCData(JSContext* cx, JSObject* obj);

  
  JSBool Cast(JSContext* cx, uintN argc, jsval* vp);
}

namespace Int64 {
  bool IsInt64(JSContext* cx, JSObject* obj);
}

namespace UInt64 {
  bool IsUInt64(JSContext* cx, JSObject* obj);
}

}
}

#endif
