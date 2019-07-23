





































#ifndef CTYPES_H
#define CTYPES_H

#include "jsapi.h"
#include "nsString.h"
#include "nsTArray.h"
#include "prlink.h"
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
  TYPE_function,
  TYPE_array,
  TYPE_struct
};

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



struct FunctionInfo
{
  
  
  
  ffi_cif mCIF;

  
  
  JSObject* mABI;                

  
  JSObject* mReturnType;

  
  
  nsTArray<JSObject*> mArgTypes; 

  
  
  
  nsTArray<ffi_type*> mFFITypes;

  
  
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
#ifdef DEBUG
  PRThread* thread;      
#endif
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
  nsTArray<FieldInfo>* GetFieldInfo(JSContext* cx, JSObject* obj);
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
