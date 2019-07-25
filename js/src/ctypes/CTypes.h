




#ifndef CTYPES_H
#define CTYPES_H

#include "jscntxt.h"
#include "jsapi.h"
#include "prlink.h"
#include "ffi.h"

#include "js/HashTable.h"

namespace js {
namespace ctypes {







template<class T>
class AutoPtr {
private:
  typedef AutoPtr<T> self_type;

public:
  AutoPtr() : mPtr(NULL) { }
  explicit AutoPtr(T* ptr) : mPtr(ptr) { }
  ~AutoPtr() { js_delete(mPtr); }

  T*   operator->()         { return mPtr; }
  bool operator!()          { return mPtr == NULL; }
  T&   operator[](size_t i) { return *(mPtr + i); }
  
  
  

  T*   get()         { return mPtr; }
  void set(T* other) { JS_ASSERT(mPtr == NULL); mPtr = other; }
  T*   forget()      { T* result = mPtr; mPtr = NULL; return result; }

  self_type& operator=(T* rhs) { mPtr = rhs; return *this; }

private:
  
  AutoPtr(AutoPtr<T>&);
  self_type& operator=(AutoPtr<T>& rhs);

  T* mPtr;
};


template<class T, size_t N = 0>
class Array : public Vector<T, N, SystemAllocPolicy>
{
};


typedef Vector<jschar,  0, SystemAllocPolicy> String;
typedef Vector<jschar, 64, SystemAllocPolicy> AutoString;
typedef Vector<char,    0, SystemAllocPolicy> CString;
typedef Vector<char,   64, SystemAllocPolicy> AutoCString;


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
  const jschar *chars = str->getChars(NULL);
  if (!chars)
    return;
  v.append(chars, str->length());
}

template <size_t N, class AP>
void
AppendString(Vector<char, N, AP> &v, JSString* str)
{
  JS_ASSERT(str);
  size_t vlen = v.length();
  size_t alen = str->length();
  if (!v.resize(vlen + alen))
    return;

  const jschar *chars = str->getChars(NULL);
  if (!chars)
    return;

  for (size_t i = 0; i < alen; ++i)
    v[i + vlen] = char(chars[i]);
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
  size_t alen = str->length();
  if (!v.resize(vlen + alen))
    return;

  const jschar *chars = str->getChars(NULL);
  if (!chars)
    return;

  
  memmove(v.begin() + alen, v.begin(), vlen * sizeof(jschar));

  
  memcpy(v.begin(), chars, alen * sizeof(jschar));
}





JS_ALWAYS_INLINE void
ASSERT_OK(JSBool ok)
{
  JS_ASSERT(ok);
}


enum ErrorNum {
#define MSG_DEF(name, number, count, exception, format) \
  name = number,
#include "ctypes.msg"
#undef MSG_DEF
  CTYPESERR_LIMIT
};

const JSErrorFormatString*
GetErrorMessage(void* userRef, const char* locale, const unsigned errorNumber);
JSBool TypeError(JSContext* cx, const char* expected, jsval actual);








enum ABICode {
  ABI_DEFAULT,
  ABI_STDCALL,
  ABI_WINAPI,
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
  JSObject* mType;    
  size_t    mIndex;   
  size_t    mOffset;  
};


struct FieldHashPolicy
{
  typedef JSFlatString* Key;
  typedef Key Lookup;

  static uint32_t hash(const Lookup &l) {
    const jschar* s = l->chars();
    size_t n = l->length();
    uint32_t hash = 0;
    for (; n > 0; s++, n--)
      hash = hash * 33 + *s;
    return hash;
  }

  static JSBool match(const Key &k, const Lookup &l) {
    if (k == l)
      return true;

    if (k->length() != l->length())
      return false;

    return memcmp(k->chars(), l->chars(), k->length() * sizeof(jschar)) == 0;
  }
};

typedef HashMap<JSFlatString*, FieldInfo, FieldHashPolicy, SystemAllocPolicy> FieldInfoHash;



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
  JSRuntime* rt;         
                         
  JSObject* closureObj;  
  JSObject* typeObj;     
  JSObject* thisObj;     
  JSObject* jsfnObj;     
  void* errResult;       
  ffi_closure* closure;  

  
  
  ClosureInfo(JSRuntime* runtime)
    : rt(runtime)
    , errResult(NULL)
    , closure(NULL)
  {}

  ~ClosureInfo() {
    if (closure)
      ffi_closure_free(closure);
    if (errResult)
      js_free(errResult);
  };
};

bool IsCTypesGlobal(JSObject* obj);

JSCTypesCallbacks* GetCallbacks(JSObject* obj);

JSBool InitTypeClasses(JSContext* cx, JSHandleObject parent);

JSBool ConvertToJS(JSContext* cx, JSHandleObject typeObj, JSHandleObject dataObj,
  void* data, bool wantPrimitive, bool ownResult, jsval* result);

JSBool ImplicitConvert(JSContext* cx, jsval val, JSObject* targetType,
  void* buffer, bool isArgument, bool* freePointer);

JSBool ExplicitConvert(JSContext* cx, jsval val, JSHandleObject targetType,
  void* buffer);





enum CTypesGlobalSlot {
  SLOT_CALLBACKS = 0, 
  SLOT_ERRNO = 1,     
  SLOT_LASTERROR = 2, 
  CTYPESGLOBAL_SLOTS
};

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
  SLOT_CTYPES            = 11, 
  SLOT_OURDATAPROTO      = 12, 
  SLOT_CLOSURECX         = 13, 
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

enum CDataFinalizerSlot {
  
  
  SLOT_DATAFINALIZER_VALTYPE           = 0,
  
  
  SLOT_DATAFINALIZER_CODETYPE          = 1,
  CDATAFINALIZER_SLOTS
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
  JSObject* Create(JSContext* cx, JSHandleObject typeProto, JSHandleObject dataProto,
    TypeCode type, JSString* name, jsval size, jsval align, ffi_type* ffiType);

  JSObject* DefineBuiltin(JSContext* cx, JSObject* parent, const char* propName,
    JSObject* typeProto, JSObject* dataProto, const char* name, TypeCode type,
    jsval size, jsval align, ffi_type* ffiType);

  bool IsCType(JSObject* obj);
  bool IsCTypeProto(JSObject* obj);
  TypeCode GetTypeCode(JSObject* typeObj);
  bool TypesEqual(JSObject* t1, JSObject* t2);
  size_t GetSize(JSObject* obj);
  bool GetSafeSize(JSObject* obj, size_t* result);
  bool IsSizeDefined(JSObject* obj);
  size_t GetAlignment(JSObject* obj);
  ffi_type* GetFFIType(JSContext* cx, JSObject* obj);
  JSString* GetName(JSContext* cx, JSHandleObject obj);
  JSObject* GetProtoFromCtor(JSObject* obj, CTypeProtoSlot slot);
  JSObject* GetProtoFromType(JSObject* obj, CTypeProtoSlot slot);
  JSCTypesCallbacks* GetCallbacksFromType(JSObject* obj);
}

namespace PointerType {
  JSObject* CreateInternal(JSContext* cx, JSHandleObject baseType);

  JSObject* GetBaseType(JSObject* obj);
}

namespace ArrayType {
  JSObject* CreateInternal(JSContext* cx, JSHandleObject baseType, size_t length,
    bool lengthDefined);

  JSObject* GetBaseType(JSObject* obj);
  size_t GetLength(JSObject* obj);
  bool GetSafeLength(JSObject* obj, size_t* result);
  ffi_type* BuildFFIType(JSContext* cx, JSObject* obj);
}

namespace StructType {
  JSBool DefineInternal(JSContext* cx, JSObject* typeObj, JSObject* fieldsObj);

  const FieldInfoHash* GetFieldInfo(JSObject* obj);
  const FieldInfo* LookupField(JSContext* cx, JSObject* obj, JSFlatString *name);
  JSObject* BuildFieldsArray(JSContext* cx, JSObject* obj);
  ffi_type* BuildFFIType(JSContext* cx, JSObject* obj);
}

namespace FunctionType {
  JSObject* CreateInternal(JSContext* cx, jsval abi, jsval rtype,
    jsval* argtypes, unsigned arglen);

  JSObject* ConstructWithObject(JSContext* cx, JSObject* typeObj,
    JSObject* refObj, PRFuncPtr fnptr, JSObject* result);

  FunctionInfo* GetFunctionInfo(JSObject* obj);
  void BuildSymbolName(JSString* name, JSObject* typeObj,
    AutoCString& result);
}

namespace CClosure {
  JSObject* Create(JSContext* cx, JSHandleObject typeObj, JSHandleObject fnObj,
    JSHandleObject thisObj, jsval errVal, PRFuncPtr* fnptr);
}

namespace CData {
  JSObject* Create(JSContext* cx, JSHandleObject typeObj, JSHandleObject refObj,
    void* data, bool ownResult);

  JSObject* GetCType(JSObject* dataObj);
  void* GetData(JSObject* dataObj);
  bool IsCData(JSObject* obj);
  bool IsCDataProto(JSObject* obj);

  
  JSBool Cast(JSContext* cx, unsigned argc, jsval* vp);
  
  JSBool GetRuntime(JSContext* cx, unsigned argc, jsval* vp);
}

namespace Int64 {
  bool IsInt64(JSObject* obj);
}

namespace UInt64 {
  bool IsUInt64(JSObject* obj);
}

}
}

#endif
