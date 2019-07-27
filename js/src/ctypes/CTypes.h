




#ifndef ctypes_CTypes_h
#define ctypes_CTypes_h

#include "mozilla/UniquePtr.h"

#include "ffi.h"
#include "jsalloc.h"
#include "jsprf.h"
#include "prlink.h"

#include "ctypes/typedefs.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/String.h"

namespace js {
namespace ctypes {






template<class T, size_t N = 0>
class Array : public Vector<T, N, SystemAllocPolicy>
{
  static_assert(!mozilla::IsSame<T, JS::Value>::value,
                "use JS::AutoValueVector instead");
};


typedef Vector<char16_t,  0, SystemAllocPolicy> String;
typedef Vector<char16_t, 64, SystemAllocPolicy> AutoString;
typedef Vector<char,      0, SystemAllocPolicy> CString;
typedef Vector<char,     64, SystemAllocPolicy> AutoCString;


template <class T, size_t N, class AP, size_t ArrayLength>
void
AppendString(Vector<T, N, AP>& v, const char (&array)[ArrayLength])
{
  
  size_t alen = ArrayLength - 1;
  size_t vlen = v.length();
  if (!v.resize(vlen + alen))
    return;

  for (size_t i = 0; i < alen; ++i)
    v[i + vlen] = array[i];
}

template <class T, size_t N, class AP>
void
AppendChars(Vector<T, N, AP>& v, const char c, size_t count)
{
  size_t vlen = v.length();
  if (!v.resize(vlen + count))
    return;

  for (size_t i = 0; i < count; ++i)
    v[i + vlen] = c;
}

template <class T, size_t N, class AP>
void
AppendUInt(Vector<T, N, AP>& v, unsigned n)
{
  char array[16];
  size_t alen = JS_snprintf(array, 16, "%u", n);
  size_t vlen = v.length();
  if (!v.resize(vlen + alen))
    return;

  for (size_t i = 0; i < alen; ++i)
    v[i + vlen] = array[i];
}

template <class T, size_t N, size_t M, class AP>
void
AppendString(Vector<T, N, AP>& v, Vector<T, M, AP>& w)
{
  v.append(w.begin(), w.length());
}

template <size_t N, class AP>
void
AppendString(Vector<char16_t, N, AP>& v, JSString* str)
{
  MOZ_ASSERT(str);
  JSLinearString* linear = str->ensureLinear(nullptr);
  if (!linear)
    return;
  JS::AutoCheckCannotGC nogc;
  if (linear->hasLatin1Chars())
    v.append(linear->latin1Chars(nogc), linear->length());
  else
    v.append(linear->twoByteChars(nogc), linear->length());
}

template <size_t N, class AP>
void
AppendString(Vector<char, N, AP>& v, JSString* str)
{
  MOZ_ASSERT(str);
  size_t vlen = v.length();
  size_t alen = str->length();
  if (!v.resize(vlen + alen))
    return;

  JSLinearString* linear = str->ensureLinear(nullptr);
  if (!linear)
    return;

  JS::AutoCheckCannotGC nogc;
  if (linear->hasLatin1Chars()) {
    const Latin1Char* chars = linear->latin1Chars(nogc);
    for (size_t i = 0; i < alen; ++i)
      v[i + vlen] = char(chars[i]);
  } else {
    const char16_t* chars = linear->twoByteChars(nogc);
    for (size_t i = 0; i < alen; ++i)
      v[i + vlen] = char(chars[i]);
  }
}

template <class T, size_t N, class AP, size_t ArrayLength>
void
PrependString(Vector<T, N, AP>& v, const char (&array)[ArrayLength])
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
PrependString(Vector<char16_t, N, AP>& v, JSString* str)
{
  MOZ_ASSERT(str);
  size_t vlen = v.length();
  size_t alen = str->length();
  if (!v.resize(vlen + alen))
    return;

  JSLinearString* linear = str->ensureLinear(nullptr);
  if (!linear)
    return;

  
  memmove(v.begin() + alen, v.begin(), vlen * sizeof(char16_t));

  
  JS::AutoCheckCannotGC nogc;
  if (linear->hasLatin1Chars()) {
    const Latin1Char* chars = linear->latin1Chars(nogc);
    for (size_t i = 0; i < alen; i++)
      v[i] = chars[i];
  } else {
    memcpy(v.begin(), linear->twoByteChars(nogc), alen * sizeof(char16_t));
  }
}

template <typename CharT>
extern size_t
GetDeflatedUTF8StringLength(JSContext* maybecx, const CharT* chars,
                            size_t charsLength);

template <typename CharT>
bool
DeflateStringToUTF8Buffer(JSContext* maybecx, const CharT* src, size_t srclen,
                          char* dst, size_t* dstlenp);






MOZ_ALWAYS_INLINE void
ASSERT_OK(bool ok)
{
  MOZ_ASSERT(ok);
}


enum ErrorNum {
#define MSG_DEF(name, count, exception, format) \
  name,
#include "ctypes/ctypes.msg"
#undef MSG_DEF
  CTYPESERR_LIMIT
};








enum ABICode {
  ABI_DEFAULT,
  ABI_STDCALL,
  ABI_WINAPI,
  INVALID_ABI
};

enum TypeCode {
  TYPE_void_t,
#define DEFINE_TYPE(name, type, ffiType) TYPE_##name,
  CTYPES_FOR_EACH_TYPE(DEFINE_TYPE)
#undef DEFINE_TYPE
  TYPE_pointer,
  TYPE_function,
  TYPE_array,
  TYPE_struct
};



struct FieldInfo
{
  JS::Heap<JSObject*> mType;    
  size_t              mIndex;   
  size_t              mOffset;  
};

struct UnbarrieredFieldInfo
{
  JSObject*           mType;    
  size_t              mIndex;   
  size_t              mOffset;  
};
static_assert(sizeof(UnbarrieredFieldInfo) == sizeof(FieldInfo),
              "UnbarrieredFieldInfo should be the same as FieldInfo but with unbarriered mType");


struct FieldHashPolicy : DefaultHasher<JSFlatString*>
{
  typedef JSFlatString* Key;
  typedef Key Lookup;

  template <typename CharT>
  static uint32_t hash(const CharT* s, size_t n) {
    uint32_t hash = 0;
    for (; n > 0; s++, n--)
      hash = hash * 33 + *s;
    return hash;
  }

  static uint32_t hash(const Lookup& l) {
    JS::AutoCheckCannotGC nogc;
    return l->hasLatin1Chars()
           ? hash(l->latin1Chars(nogc), l->length())
           : hash(l->twoByteChars(nogc), l->length());
  }

  static bool match(const Key& k, const Lookup& l) {
    if (k == l)
      return true;

    if (k->length() != l->length())
      return false;

    return EqualChars(k, l);
  }
};

typedef HashMap<JSFlatString*, FieldInfo, FieldHashPolicy, SystemAllocPolicy> FieldInfoHash;



struct FunctionInfo
{
  
  
  
  ffi_cif mCIF;

  
  
  JS::Heap<JSObject*> mABI;

  
  JS::Heap<JSObject*> mReturnType;

  
  
  Array<JS::Heap<JSObject*> > mArgTypes;

  
  
  
  Array<ffi_type*> mFFITypes;

  
  
  bool mIsVariadic;
};


struct ClosureInfo
{
  JSContext* cx;                   
  JSRuntime* rt;                   
                                   
  JS::Heap<JSObject*> closureObj;  
  JS::Heap<JSObject*> typeObj;     
  JS::Heap<JSObject*> thisObj;     
  JS::Heap<JSObject*> jsfnObj;     
  void* errResult;                 
  ffi_closure* closure;            

  
  
  explicit ClosureInfo(JSRuntime* runtime)
    : rt(runtime)
    , errResult(nullptr)
    , closure(nullptr)
  {}

  ~ClosureInfo() {
    if (closure)
      ffi_closure_free(closure);
    js_free(errResult);
  }
};

bool IsCTypesGlobal(HandleValue v);
bool IsCTypesGlobal(JSObject* obj);

const JSCTypesCallbacks* GetCallbacks(JSObject* obj);





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
  SLOT_FUNNAME  = 4, 
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
  JSObject* Create(JSContext* cx, HandleObject typeProto, HandleObject dataProto,
    TypeCode type, JSString* name, jsval size, jsval align, ffi_type* ffiType);

  JSObject* DefineBuiltin(JSContext* cx, HandleObject ctypesObj, const char* propName,
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
  JSString* GetName(JSContext* cx, HandleObject obj);
  JSObject* GetProtoFromCtor(JSObject* obj, CTypeProtoSlot slot);
  JSObject* GetProtoFromType(JSContext* cx, JSObject* obj, CTypeProtoSlot slot);
  const JSCTypesCallbacks* GetCallbacksFromType(JSObject* obj);
}

namespace PointerType {
  JSObject* CreateInternal(JSContext* cx, HandleObject baseType);

  JSObject* GetBaseType(JSObject* obj);
}

typedef mozilla::UniquePtr<ffi_type, JS::DeletePolicy<ffi_type>> UniquePtrFFIType;

namespace ArrayType {
  JSObject* CreateInternal(JSContext* cx, HandleObject baseType, size_t length,
    bool lengthDefined);

  JSObject* GetBaseType(JSObject* obj);
  size_t GetLength(JSObject* obj);
  bool GetSafeLength(JSObject* obj, size_t* result);
  UniquePtrFFIType BuildFFIType(JSContext* cx, JSObject* obj);
}

namespace StructType {
  bool DefineInternal(JSContext* cx, JSObject* typeObj, JSObject* fieldsObj);

  const FieldInfoHash* GetFieldInfo(JSObject* obj);
  const FieldInfo* LookupField(JSContext* cx, JSObject* obj, JSFlatString* name);
  JSObject* BuildFieldsArray(JSContext* cx, JSObject* obj);
  UniquePtrFFIType BuildFFIType(JSContext* cx, JSObject* obj);
}

namespace FunctionType {
  JSObject* CreateInternal(JSContext* cx, HandleValue abi, HandleValue rtype,
    const HandleValueArray& args);

  JSObject* ConstructWithObject(JSContext* cx, JSObject* typeObj,
    JSObject* refObj, PRFuncPtr fnptr, JSObject* result);

  FunctionInfo* GetFunctionInfo(JSObject* obj);
  void BuildSymbolName(JSString* name, JSObject* typeObj,
    AutoCString& result);
}

namespace CClosure {
  JSObject* Create(JSContext* cx, HandleObject typeObj, HandleObject fnObj,
    HandleObject thisObj, jsval errVal, PRFuncPtr* fnptr);
}

namespace CData {
  JSObject* Create(JSContext* cx, HandleObject typeObj, HandleObject refObj,
    void* data, bool ownResult);

  JSObject* GetCType(JSObject* dataObj);
  void* GetData(JSObject* dataObj);
  bool IsCData(JSObject* obj);
  bool IsCData(HandleValue v);
  bool IsCDataProto(JSObject* obj);

  
  bool Cast(JSContext* cx, unsigned argc, jsval* vp);
  
  bool GetRuntime(JSContext* cx, unsigned argc, jsval* vp);
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
