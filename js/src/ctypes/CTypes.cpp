





#include "ctypes/CTypes.h"

#include "mozilla/FloatingPoint.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/NumericLimits.h"

#include <math.h>
#include <stdint.h>

#if defined(XP_WIN)
#include <float.h>
#endif

#if defined(SOLARIS)
#include <ieeefp.h>
#endif

#ifdef HAVE_SSIZE_T
#include <sys/types.h>
#endif

#if defined(XP_UNIX)
#include <errno.h>
#elif defined(XP_WIN)
#include <windows.h>
#endif

#include "jscntxt.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsprf.h"

#include "builtin/TypedObject.h"
#include "ctypes/Library.h"

using namespace std;
using mozilla::NumericLimits;

namespace js {
namespace ctypes {

size_t
GetDeflatedUTF8StringLength(JSContext *maybecx, const jschar *chars,
                            size_t nchars)
{
    size_t nbytes;
    const jschar *end;
    unsigned c, c2;
    char buffer[10];

    nbytes = nchars;
    for (end = chars + nchars; chars != end; chars++) {
        c = *chars;
        if (c < 0x80)
            continue;
        if (0xD800 <= c && c <= 0xDFFF) {
            
            chars++;

            
            nbytes--;
            if (c >= 0xDC00 || chars == end)
                goto bad_surrogate;
            c2 = *chars;
            if (c2 < 0xDC00 || c2 > 0xDFFF)
                goto bad_surrogate;
            c = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        c >>= 11;
        nbytes++;
        while (c) {
            c >>= 5;
            nbytes++;
        }
    }
    return nbytes;

  bad_surrogate:
    if (maybecx) {
        JS_snprintf(buffer, 10, "0x%x", c);
        JS_ReportErrorFlagsAndNumber(maybecx, JSREPORT_ERROR, js_GetErrorMessage,
                                     nullptr, JSMSG_BAD_SURROGATE_CHAR, buffer);
    }
    return (size_t) -1;
}

bool
DeflateStringToUTF8Buffer(JSContext *maybecx, const jschar *src, size_t srclen,
                              char *dst, size_t *dstlenp)
{
    size_t i, utf8Len;
    jschar c, c2;
    uint32_t v;
    uint8_t utf8buf[6];

    size_t dstlen = *dstlenp;
    size_t origDstlen = dstlen;

    while (srclen) {
        c = *src++;
        srclen--;
        if (c >= 0xDC00 && c <= 0xDFFF)
            goto badSurrogate;
        if (c < 0xD800 || c > 0xDBFF) {
            v = c;
        } else {
            if (srclen < 1)
                goto badSurrogate;
            c2 = *src;
            if ((c2 < 0xDC00) || (c2 > 0xDFFF))
                goto badSurrogate;
            src++;
            srclen--;
            v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        if (v < 0x0080) {
            
            if (dstlen == 0)
                goto bufferTooSmall;
            *dst++ = (char) v;
            utf8Len = 1;
        } else {
            utf8Len = js_OneUcs4ToUtf8Char(utf8buf, v);
            if (utf8Len > dstlen)
                goto bufferTooSmall;
            for (i = 0; i < utf8Len; i++)
                *dst++ = (char) utf8buf[i];
        }
        dstlen -= utf8Len;
    }
    *dstlenp = (origDstlen - dstlen);
    return true;

badSurrogate:
    *dstlenp = (origDstlen - dstlen);
    
    if (maybecx)
        GetDeflatedUTF8StringLength(maybecx, src - 1, srclen + 1);
    return false;

bufferTooSmall:
    *dstlenp = (origDstlen - dstlen);
    if (maybecx) {
        JS_ReportErrorNumber(maybecx, js_GetErrorMessage, nullptr,
                             JSMSG_BUFFER_TOO_SMALL);
    }
    return false;
}









template<JS::IsAcceptableThis Test, JS::NativeImpl Impl>
struct Property
{
  static bool
  Fun(JSContext* cx, unsigned argc, JS::Value* vp)
  {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    return JS::CallNonGenericMethod<Test, Impl>(cx, args);
  }
};

static bool ConstructAbstract(JSContext* cx, unsigned argc, jsval* vp);

namespace CType {
  static bool ConstructData(JSContext* cx, unsigned argc, jsval* vp);
  static bool ConstructBasic(JSContext* cx, HandleObject obj, const CallArgs& args);

  static void Trace(JSTracer* trc, JSObject* obj);
  static void Finalize(JSFreeOp *fop, JSObject* obj);

  bool IsCType(HandleValue v);
  bool IsCTypeOrProto(HandleValue v);

  bool PrototypeGetter(JSContext* cx, JS::CallArgs args);
  bool NameGetter(JSContext* cx, JS::CallArgs args);
  bool SizeGetter(JSContext* cx, JS::CallArgs args);
  bool PtrGetter(JSContext* cx, JS::CallArgs args);

  static bool CreateArray(JSContext* cx, unsigned argc, jsval* vp);
  static bool ToString(JSContext* cx, unsigned argc, jsval* vp);
  static bool ToSource(JSContext* cx, unsigned argc, jsval* vp);
  static bool HasInstance(JSContext* cx, HandleObject obj, MutableHandleValue v, bool* bp);


  






  static JSObject* GetGlobalCTypes(JSContext* cx, JSObject* obj);

}

namespace ABI {
  bool IsABI(JSObject* obj);
  static bool ToSource(JSContext* cx, unsigned argc, jsval* vp);
}

namespace PointerType {
  static bool Create(JSContext* cx, unsigned argc, jsval* vp);
  static bool ConstructData(JSContext* cx, HandleObject obj, const CallArgs& args);

  bool IsPointerType(HandleValue v);
  bool IsPointer(HandleValue v);

  bool TargetTypeGetter(JSContext* cx, JS::CallArgs args);
  bool ContentsGetter(JSContext* cx, JS::CallArgs args);
  bool ContentsSetter(JSContext* cx, JS::CallArgs args);

  static bool IsNull(JSContext* cx, unsigned argc, jsval* vp);
  static bool Increment(JSContext* cx, unsigned argc, jsval* vp);
  static bool Decrement(JSContext* cx, unsigned argc, jsval* vp);
  
  
  static bool OffsetBy(JSContext* cx, const CallArgs& args, int offset);
}

namespace ArrayType {
  bool IsArrayType(HandleValue v);
  bool IsArrayOrArrayType(HandleValue v);

  static bool Create(JSContext* cx, unsigned argc, jsval* vp);
  static bool ConstructData(JSContext* cx, HandleObject obj, const CallArgs& args);

  bool ElementTypeGetter(JSContext* cx, JS::CallArgs args);
  bool LengthGetter(JSContext* cx, JS::CallArgs args);

  static bool Getter(JSContext* cx, HandleObject obj, HandleId idval, MutableHandleValue vp);
  static bool Setter(JSContext* cx, HandleObject obj, HandleId idval, bool strict, MutableHandleValue vp);
  static bool AddressOfElement(JSContext* cx, unsigned argc, jsval* vp);
}

namespace StructType {
  bool IsStruct(HandleValue v);

  static bool Create(JSContext* cx, unsigned argc, jsval* vp);
  static bool ConstructData(JSContext* cx, HandleObject obj, const CallArgs& args);

  bool FieldsArrayGetter(JSContext* cx, JS::CallArgs args);

  static bool FieldGetter(JSContext* cx, HandleObject obj, HandleId idval,
    MutableHandleValue vp);
  static bool FieldSetter(JSContext* cx, HandleObject obj, HandleId idval, bool strict,
                            MutableHandleValue vp);
  static bool AddressOfField(JSContext* cx, unsigned argc, jsval* vp);
  static bool Define(JSContext* cx, unsigned argc, jsval* vp);
}

namespace FunctionType {
  static bool Create(JSContext* cx, unsigned argc, jsval* vp);
  static bool ConstructData(JSContext* cx, HandleObject typeObj,
    HandleObject dataObj, HandleObject fnObj, HandleObject thisObj, jsval errVal);

  static bool Call(JSContext* cx, unsigned argc, jsval* vp);

  bool IsFunctionType(HandleValue v);

  bool ArgTypesGetter(JSContext* cx, JS::CallArgs args);
  bool ReturnTypeGetter(JSContext* cx, JS::CallArgs args);
  bool ABIGetter(JSContext* cx, JS::CallArgs args);
  bool IsVariadicGetter(JSContext* cx, JS::CallArgs args);
}

namespace CClosure {
  static void Trace(JSTracer* trc, JSObject* obj);
  static void Finalize(JSFreeOp *fop, JSObject* obj);

  
  static void ClosureStub(ffi_cif* cif, void* result, void** args,
    void* userData);
}

namespace CData {
  static void Finalize(JSFreeOp *fop, JSObject* obj);

  bool ValueGetter(JSContext* cx, JS::CallArgs args);
  bool ValueSetter(JSContext* cx, JS::CallArgs args);

  static bool Address(JSContext* cx, unsigned argc, jsval* vp);
  static bool ReadString(JSContext* cx, unsigned argc, jsval* vp);
  static bool ReadStringReplaceMalformed(JSContext* cx, unsigned argc, jsval* vp);
  static bool ToSource(JSContext* cx, unsigned argc, jsval* vp);
  static JSString *GetSourceString(JSContext *cx, HandleObject typeObj,
                                   void *data);

  bool ErrnoGetter(JSContext* cx, JS::CallArgs args);

#if defined(XP_WIN)
  bool LastErrorGetter(JSContext* cx, JS::CallArgs args);
#endif 
}

namespace CDataFinalizer {
  











  static bool Construct(JSContext* cx, unsigned argc, jsval *vp);

  








  struct Private {
    



    void *cargs;

    


    size_t cargs_size;

    



    ffi_cif CIF;

    



    uintptr_t code;

    



    void *rvalue;
  };

  


  namespace Methods {
    static bool Dispose(JSContext* cx, unsigned argc, jsval *vp);
    static bool Forget(JSContext* cx, unsigned argc, jsval *vp);
    static bool ToSource(JSContext* cx, unsigned argc, jsval *vp);
    static bool ToString(JSContext* cx, unsigned argc, jsval *vp);
  }

  




  static bool IsCDataFinalizer(JSObject *obj);

  










  static void Cleanup(Private *p, JSObject *obj);

  


  static void CallFinalizer(CDataFinalizer::Private *p,
                            int* errnoStatus,
                            int32_t* lastErrorStatus);

  



  static JSObject *GetCType(JSContext *cx, JSObject *obj);

  


  static void Finalize(JSFreeOp *fop, JSObject *obj);

  




  static bool GetValue(JSContext *cx, JSObject *obj, jsval *result);

  static JSObject* GetCData(JSContext *cx, JSObject *obj);
 }



namespace Int64Base {
  JSObject* Construct(JSContext* cx, HandleObject proto, uint64_t data,
    bool isUnsigned);

  uint64_t GetInt(JSObject* obj);

  bool ToString(JSContext* cx, JSObject* obj, const CallArgs& args,
                bool isUnsigned);

  bool ToSource(JSContext* cx, JSObject* obj, const CallArgs& args,
                bool isUnsigned);

  static void Finalize(JSFreeOp *fop, JSObject* obj);
}

namespace Int64 {
  static bool Construct(JSContext* cx, unsigned argc, jsval* vp);

  static bool ToString(JSContext* cx, unsigned argc, jsval* vp);
  static bool ToSource(JSContext* cx, unsigned argc, jsval* vp);

  static bool Compare(JSContext* cx, unsigned argc, jsval* vp);
  static bool Lo(JSContext* cx, unsigned argc, jsval* vp);
  static bool Hi(JSContext* cx, unsigned argc, jsval* vp);
  static bool Join(JSContext* cx, unsigned argc, jsval* vp);
}

namespace UInt64 {
  static bool Construct(JSContext* cx, unsigned argc, jsval* vp);

  static bool ToString(JSContext* cx, unsigned argc, jsval* vp);
  static bool ToSource(JSContext* cx, unsigned argc, jsval* vp);

  static bool Compare(JSContext* cx, unsigned argc, jsval* vp);
  static bool Lo(JSContext* cx, unsigned argc, jsval* vp);
  static bool Hi(JSContext* cx, unsigned argc, jsval* vp);
  static bool Join(JSContext* cx, unsigned argc, jsval* vp);
}







static const JSClass sCTypesGlobalClass = {
  "ctypes",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPESGLOBAL_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static const JSClass sCABIClass = {
  "CABI",
  JSCLASS_HAS_RESERVED_SLOTS(CABI_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};




static const JSClass sCTypeProtoClass = {
  "CType",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPEPROTO_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
  ConstructAbstract, nullptr, ConstructAbstract
};



static const JSClass sCDataProtoClass = {
  "CData",
  0,
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static const JSClass sCTypeClass = {
  "CType",
  JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_HAS_RESERVED_SLOTS(CTYPE_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CType::Finalize,
  CType::ConstructData, CType::HasInstance, CType::ConstructData,
  CType::Trace
};

static const JSClass sCDataClass = {
  "CData",
  JSCLASS_HAS_RESERVED_SLOTS(CDATA_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, ArrayType::Getter, ArrayType::Setter,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CData::Finalize,
  FunctionType::Call, nullptr, FunctionType::Call
};

static const JSClass sCClosureClass = {
  "CClosure",
  JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_HAS_RESERVED_SLOTS(CCLOSURE_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CClosure::Finalize,
  nullptr, nullptr, nullptr, CClosure::Trace
};




static const JSClass sCDataFinalizerProtoClass = {
  "CDataFinalizer",
  0,
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};







static const JSClass sCDataFinalizerClass = {
  "CDataFinalizer",
  JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(CDATAFINALIZER_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CDataFinalizer::Finalize,
};


#define CTYPESFN_FLAGS \
  (JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

#define CTYPESCTOR_FLAGS \
  (CTYPESFN_FLAGS | JSFUN_CONSTRUCTOR)

#define CTYPESACC_FLAGS \
  (JSPROP_ENUMERATE | JSPROP_PERMANENT)

#define CABIFN_FLAGS \
  (JSPROP_READONLY | JSPROP_PERMANENT)

#define CDATAFN_FLAGS \
  (JSPROP_READONLY | JSPROP_PERMANENT)

#define CDATAFINALIZERFN_FLAGS \
  (JSPROP_READONLY | JSPROP_PERMANENT)

static const JSPropertySpec sCTypeProps[] = {
  JS_PSG("name",
         (Property<CType::IsCType, CType::NameGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("size",
         (Property<CType::IsCType, CType::SizeGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("ptr",
         (Property<CType::IsCType, CType::PtrGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("prototype",
         (Property<CType::IsCTypeOrProto, CType::PrototypeGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PS_END
};

static const JSFunctionSpec sCTypeFunctions[] = {
  JS_FN("array", CType::CreateArray, 0, CTYPESFN_FLAGS),
  JS_FN("toString", CType::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", CType::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sCABIFunctions[] = {
  JS_FN("toSource", ABI::ToSource, 0, CABIFN_FLAGS),
  JS_FN("toString", ABI::ToSource, 0, CABIFN_FLAGS),
  JS_FS_END
};

static const JSPropertySpec sCDataProps[] = {
  JS_PSGS("value",
          (Property<CData::IsCData, CData::ValueGetter>::Fun),
          (Property<CData::IsCData, CData::ValueSetter>::Fun),
          JSPROP_PERMANENT),
  JS_PS_END
};

static const JSFunctionSpec sCDataFunctions[] = {
  JS_FN("address", CData::Address, 0, CDATAFN_FLAGS),
  JS_FN("readString", CData::ReadString, 0, CDATAFN_FLAGS),
  JS_FN("readStringReplaceMalformed", CData::ReadStringReplaceMalformed, 0, CDATAFN_FLAGS),
  JS_FN("toSource", CData::ToSource, 0, CDATAFN_FLAGS),
  JS_FN("toString", CData::ToSource, 0, CDATAFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sCDataFinalizerFunctions[] = {
  JS_FN("dispose",  CDataFinalizer::Methods::Dispose,  0, CDATAFINALIZERFN_FLAGS),
  JS_FN("forget",   CDataFinalizer::Methods::Forget,   0, CDATAFINALIZERFN_FLAGS),
  JS_FN("readString",CData::ReadString, 0, CDATAFINALIZERFN_FLAGS),
  JS_FN("toString", CDataFinalizer::Methods::ToString, 0, CDATAFINALIZERFN_FLAGS),
  JS_FN("toSource", CDataFinalizer::Methods::ToSource, 0, CDATAFINALIZERFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sPointerFunction =
  JS_FN("PointerType", PointerType::Create, 1, CTYPESCTOR_FLAGS);

static const JSPropertySpec sPointerProps[] = {
  JS_PSG("targetType",
         (Property<PointerType::IsPointerType, PointerType::TargetTypeGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PS_END
};

static const JSFunctionSpec sPointerInstanceFunctions[] = {
  JS_FN("isNull", PointerType::IsNull, 0, CTYPESFN_FLAGS),
  JS_FN("increment", PointerType::Increment, 0, CTYPESFN_FLAGS),
  JS_FN("decrement", PointerType::Decrement, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static const JSPropertySpec sPointerInstanceProps[] = {
  JS_PSGS("contents",
         (Property<PointerType::IsPointer, PointerType::ContentsGetter>::Fun),
         (Property<PointerType::IsPointer, PointerType::ContentsSetter>::Fun),
          JSPROP_PERMANENT),
  JS_PS_END
};

static const JSFunctionSpec sArrayFunction =
  JS_FN("ArrayType", ArrayType::Create, 1, CTYPESCTOR_FLAGS);

static const JSPropertySpec sArrayProps[] = {
  JS_PSG("elementType",
         (Property<ArrayType::IsArrayType, ArrayType::ElementTypeGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("length",
         (Property<ArrayType::IsArrayOrArrayType, ArrayType::LengthGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PS_END
};

static const JSFunctionSpec sArrayInstanceFunctions[] = {
  JS_FN("addressOfElement", ArrayType::AddressOfElement, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static const JSPropertySpec sArrayInstanceProps[] = {
  JS_PSG("length",
         (Property<ArrayType::IsArrayOrArrayType, ArrayType::LengthGetter>::Fun),
         JSPROP_PERMANENT),
  JS_PS_END
};

static const JSFunctionSpec sStructFunction =
  JS_FN("StructType", StructType::Create, 2, CTYPESCTOR_FLAGS);

static const JSPropertySpec sStructProps[] = {
  JS_PSG("fields",
         (Property<StructType::IsStruct, StructType::FieldsArrayGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PS_END
};

static const JSFunctionSpec sStructFunctions[] = {
  JS_FN("define", StructType::Define, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sStructInstanceFunctions[] = {
  JS_FN("addressOfField", StructType::AddressOfField, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sFunctionFunction =
  JS_FN("FunctionType", FunctionType::Create, 2, CTYPESCTOR_FLAGS);

static const JSPropertySpec sFunctionProps[] = {
  JS_PSG("argTypes",
         (Property<FunctionType::IsFunctionType, FunctionType::ArgTypesGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("returnType",
         (Property<FunctionType::IsFunctionType, FunctionType::ReturnTypeGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("abi",
         (Property<FunctionType::IsFunctionType, FunctionType::ABIGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PSG("isVariadic",
         (Property<FunctionType::IsFunctionType, FunctionType::IsVariadicGetter>::Fun),
         CTYPESACC_FLAGS),
  JS_PS_END
};

static const JSFunctionSpec sFunctionInstanceFunctions[] = {
  JS_FN("call", js_fun_call, 1, CDATAFN_FLAGS),
  JS_FN("apply", js_fun_apply, 2, CDATAFN_FLAGS),
  JS_FS_END
};

static const JSClass sInt64ProtoClass = {
  "Int64",
  0,
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static const JSClass sUInt64ProtoClass = {
  "UInt64",
  0,
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static const JSClass sInt64Class = {
  "Int64",
  JSCLASS_HAS_RESERVED_SLOTS(INT64_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Int64Base::Finalize
};

static const JSClass sUInt64Class = {
  "UInt64",
  JSCLASS_HAS_RESERVED_SLOTS(INT64_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Int64Base::Finalize
};

static const JSFunctionSpec sInt64StaticFunctions[] = {
  JS_FN("compare", Int64::Compare, 2, CTYPESFN_FLAGS),
  JS_FN("lo", Int64::Lo, 1, CTYPESFN_FLAGS),
  JS_FN("hi", Int64::Hi, 1, CTYPESFN_FLAGS),
  JS_FN("join", Int64::Join, 2, CTYPESFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sUInt64StaticFunctions[] = {
  JS_FN("compare", UInt64::Compare, 2, CTYPESFN_FLAGS),
  JS_FN("lo", UInt64::Lo, 1, CTYPESFN_FLAGS),
  JS_FN("hi", UInt64::Hi, 1, CTYPESFN_FLAGS),
  JS_FN("join", UInt64::Join, 2, CTYPESFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sInt64Functions[] = {
  JS_FN("toString", Int64::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", Int64::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static const JSFunctionSpec sUInt64Functions[] = {
  JS_FN("toString", UInt64::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", UInt64::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static const JSPropertySpec sModuleProps[] = {
  JS_PSG("errno",
         (Property<IsCTypesGlobal, CData::ErrnoGetter>::Fun),
         JSPROP_PERMANENT),
#if defined(XP_WIN)
  JS_PSG("winLastError",
         (Property<IsCTypesGlobal, CData::LastErrorGetter>::Fun),
         JSPROP_PERMANENT),
#endif 
  JS_PS_END
};

static const JSFunctionSpec sModuleFunctions[] = {
  JS_FN("CDataFinalizer", CDataFinalizer::Construct, 2, CTYPESFN_FLAGS),
  JS_FN("open", Library::Open, 1, CTYPESFN_FLAGS),
  JS_FN("cast", CData::Cast, 2, CTYPESFN_FLAGS),
  JS_FN("getRuntime", CData::GetRuntime, 1, CTYPESFN_FLAGS),
  JS_FN("libraryName", Library::Name, 1, CTYPESFN_FLAGS),
  JS_FS_END
};

static MOZ_ALWAYS_INLINE JSString*
NewUCString(JSContext* cx, const AutoString& from)
{
  return JS_NewUCStringCopyN(cx, from.begin(), from.length());
}






static MOZ_ALWAYS_INLINE size_t
Align(size_t val, size_t align)
{
  
  MOZ_ASSERT(align != 0 && (align & (align - 1)) == 0);
  return ((val - 1) | (align - 1)) + 1;
}

static ABICode
GetABICode(JSObject* obj)
{
  
  
  if (JS_GetClass(obj) != &sCABIClass)
    return INVALID_ABI;

  jsval result = JS_GetReservedSlot(obj, SLOT_ABICODE);
  return ABICode(result.toInt32());
}

static const JSErrorFormatString ErrorFormatString[CTYPESERR_LIMIT] = {
#define MSG_DEF(name, number, count, exception, format) \
  { format, count, exception } ,
#include "ctypes/ctypes.msg"
#undef MSG_DEF
};

static const JSErrorFormatString*
GetErrorMessage(void* userRef, const char* locale, const unsigned errorNumber)
{
  if (0 < errorNumber && errorNumber < CTYPESERR_LIMIT)
    return &ErrorFormatString[errorNumber];
  return nullptr;
}

static bool
TypeError(JSContext* cx, const char* expected, HandleValue actual)
{
  JSString* str = JS_ValueToSource(cx, actual);
  JSAutoByteString bytes;

  const char* src;
  if (str) {
    src = bytes.encodeLatin1(cx, str);
    if (!src)
      return false;
  } else {
    JS_ClearPendingException(cx);
    src = "<<error converting value to string>>";
  }
  JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                       CTYPESMSG_TYPE_ERROR, expected, src);
  return false;
}

static JSObject*
InitCTypeClass(JSContext* cx, HandleObject parent)
{
  JSFunction *fun = JS_DefineFunction(cx, parent, "CType", ConstructAbstract, 0,
                                      CTYPESCTOR_FLAGS);
  if (!fun)
    return nullptr;

  RootedObject ctor(cx, JS_GetFunctionObject(fun));
  RootedObject fnproto(cx);
  if (!JS_GetPrototype(cx, ctor, &fnproto))
    return nullptr;
  JS_ASSERT(ctor);
  JS_ASSERT(fnproto);

  
  RootedObject prototype(cx, JS_NewObject(cx, &sCTypeProtoClass, fnproto, parent));
  if (!prototype)
    return nullptr;

  if (!JS_DefineProperty(cx, ctor, "prototype", prototype,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return nullptr;

  if (!JS_DefineProperty(cx, prototype, "constructor", ctor,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return nullptr;

  
  if (!JS_DefineProperties(cx, prototype, sCTypeProps) ||
      !JS_DefineFunctions(cx, prototype, sCTypeFunctions))
    return nullptr;

  if (!JS_FreezeObject(cx, ctor) || !JS_FreezeObject(cx, prototype))
    return nullptr;

  return prototype;
}

static JSObject*
InitABIClass(JSContext* cx, JSObject* parent)
{
  RootedObject obj(cx, JS_NewObject(cx, nullptr, NullPtr(), NullPtr()));

  if (!obj)
    return nullptr;

  if (!JS_DefineFunctions(cx, obj, sCABIFunctions))
    return nullptr;

  return obj;
}


static JSObject*
InitCDataClass(JSContext* cx, HandleObject parent, HandleObject CTypeProto)
{
  JSFunction* fun = JS_DefineFunction(cx, parent, "CData", ConstructAbstract, 0,
                      CTYPESCTOR_FLAGS);
  if (!fun)
    return nullptr;

  RootedObject ctor(cx, JS_GetFunctionObject(fun));
  JS_ASSERT(ctor);

  
  
  
  if (!JS_SetPrototype(cx, ctor, CTypeProto))
    return nullptr;

  
  RootedObject prototype(cx, JS_NewObject(cx, &sCDataProtoClass, NullPtr(), parent));
  if (!prototype)
    return nullptr;

  if (!JS_DefineProperty(cx, ctor, "prototype", prototype,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return nullptr;

  if (!JS_DefineProperty(cx, prototype, "constructor", ctor,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return nullptr;

  
  if (!JS_DefineProperties(cx, prototype, sCDataProps) ||
      !JS_DefineFunctions(cx, prototype, sCDataFunctions))
    return nullptr;

  if (
      !JS_FreezeObject(cx, ctor))
    return nullptr;

  return prototype;
}

static bool
DefineABIConstant(JSContext* cx,
                  HandleObject parent,
                  const char* name,
                  ABICode code,
                  HandleObject prototype)
{
  RootedObject obj(cx, JS_DefineObject(cx, parent, name, &sCABIClass, prototype,
                                       JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));
  if (!obj)
    return false;
  JS_SetReservedSlot(obj, SLOT_ABICODE, INT_TO_JSVAL(code));
  return JS_FreezeObject(cx, obj);
}



static bool
InitTypeConstructor(JSContext* cx,
                    HandleObject parent,
                    HandleObject CTypeProto,
                    HandleObject CDataProto,
                    const JSFunctionSpec spec,
                    const JSFunctionSpec* fns,
                    const JSPropertySpec* props,
                    const JSFunctionSpec* instanceFns,
                    const JSPropertySpec* instanceProps,
                    MutableHandleObject typeProto,
                    MutableHandleObject dataProto)
{
  JSFunction* fun = js::DefineFunctionWithReserved(cx, parent, spec.name, spec.call.op,
                      spec.nargs, spec.flags);
  if (!fun)
    return false;

  RootedObject obj(cx, JS_GetFunctionObject(fun));
  if (!obj)
    return false;

  
  typeProto.set(JS_NewObject(cx, &sCTypeProtoClass, CTypeProto, parent));
  if (!typeProto)
    return false;

  
  if (!JS_DefineProperty(cx, obj, "prototype", typeProto,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (fns && !JS_DefineFunctions(cx, typeProto, fns))
    return false;

  if (!JS_DefineProperties(cx, typeProto, props))
    return false;

  if (!JS_DefineProperty(cx, typeProto, "constructor", obj,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  
  
  js::SetFunctionNativeReserved(obj, SLOT_FN_CTORPROTO, OBJECT_TO_JSVAL(typeProto));

  
  
  
  
  dataProto.set(JS_NewObject(cx, &sCDataProtoClass, CDataProto, parent));
  if (!dataProto)
    return false;

  
  
  
  if (instanceFns && !JS_DefineFunctions(cx, dataProto, instanceFns))
    return false;

  if (instanceProps && !JS_DefineProperties(cx, dataProto, instanceProps))
    return false;

  
  JS_SetReservedSlot(typeProto, SLOT_OURDATAPROTO, OBJECT_TO_JSVAL(dataProto));

  if (!JS_FreezeObject(cx, obj) ||
      
      !JS_FreezeObject(cx, typeProto))
    return false;

  return true;
}

static JSObject*
InitInt64Class(JSContext* cx,
               HandleObject parent,
               const JSClass* clasp,
               JSNative construct,
               const JSFunctionSpec* fs,
               const JSFunctionSpec* static_fs)
{
  
  RootedObject prototype(cx, JS_InitClass(cx, parent, js::NullPtr(), clasp, construct,
                                          0, nullptr, fs, nullptr, static_fs));
  if (!prototype)
    return nullptr;

  RootedObject ctor(cx, JS_GetConstructor(cx, prototype));
  if (!ctor)
    return nullptr;
  if (!JS_FreezeObject(cx, ctor))
    return nullptr;

  
  
  JS_ASSERT(clasp == &sInt64ProtoClass || clasp == &sUInt64ProtoClass);
  JSNative native = (clasp == &sInt64ProtoClass) ? Int64::Join : UInt64::Join;
  JSFunction* fun = js::DefineFunctionWithReserved(cx, ctor, "join", native,
                      2, CTYPESFN_FLAGS);
  if (!fun)
    return nullptr;

  js::SetFunctionNativeReserved(fun, SLOT_FN_INT64PROTO,
    OBJECT_TO_JSVAL(prototype));

  if (!JS_FreezeObject(cx, prototype))
    return nullptr;

  return prototype;
}

static void
AttachProtos(JSObject* proto, const AutoObjectVector& protos)
{
  
  
  
  for (uint32_t i = 0; i <= SLOT_CTYPES; ++i)
    JS_SetReservedSlot(proto, i, OBJECT_TO_JSVAL(protos[i]));
}

static bool
InitTypeClasses(JSContext* cx, HandleObject parent)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  RootedObject CTypeProto(cx, InitCTypeClass(cx, parent));
  if (!CTypeProto)
    return false;

  
  
  
  
  
  
  
  
  
  
  
  RootedObject CDataProto(cx, InitCDataClass(cx, parent, CTypeProto));
  if (!CDataProto)
    return false;

  
  JS_SetReservedSlot(CTypeProto, SLOT_OURDATAPROTO, OBJECT_TO_JSVAL(CDataProto));

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  AutoObjectVector protos(cx);
  protos.resize(CTYPEPROTO_SLOTS);
  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sPointerFunction, nullptr, sPointerProps,
         sPointerInstanceFunctions, sPointerInstanceProps,
         protos[SLOT_POINTERPROTO], protos[SLOT_POINTERDATAPROTO]))
    return false;

  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sArrayFunction, nullptr, sArrayProps,
         sArrayInstanceFunctions, sArrayInstanceProps,
         protos[SLOT_ARRAYPROTO], protos[SLOT_ARRAYDATAPROTO]))
    return false;

  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sStructFunction, sStructFunctions, sStructProps,
         sStructInstanceFunctions, nullptr,
         protos[SLOT_STRUCTPROTO], protos[SLOT_STRUCTDATAPROTO]))
    return false;

  if (!InitTypeConstructor(cx, parent, CTypeProto, protos[SLOT_POINTERDATAPROTO],
         sFunctionFunction, nullptr, sFunctionProps, sFunctionInstanceFunctions, nullptr,
         protos[SLOT_FUNCTIONPROTO], protos[SLOT_FUNCTIONDATAPROTO]))
    return false;

  protos[SLOT_CDATAPROTO].set(CDataProto);

  
  
  
  
  
  
  
  
  protos[SLOT_INT64PROTO].set(InitInt64Class(cx, parent, &sInt64ProtoClass,
    Int64::Construct, sInt64Functions, sInt64StaticFunctions));
  if (!protos[SLOT_INT64PROTO])
    return false;
  protos[SLOT_UINT64PROTO].set(InitInt64Class(cx, parent, &sUInt64ProtoClass,
    UInt64::Construct, sUInt64Functions, sUInt64StaticFunctions));
  if (!protos[SLOT_UINT64PROTO])
    return false;

  
  
  protos[SLOT_CTYPES].set(parent);

  
  
  
  AttachProtos(CTypeProto, protos);
  AttachProtos(protos[SLOT_POINTERPROTO], protos);
  AttachProtos(protos[SLOT_ARRAYPROTO], protos);
  AttachProtos(protos[SLOT_STRUCTPROTO], protos);
  AttachProtos(protos[SLOT_FUNCTIONPROTO], protos);

  RootedObject ABIProto(cx, InitABIClass(cx, parent));
  if (!ABIProto)
    return false;

  
  if (!DefineABIConstant(cx, parent, "default_abi", ABI_DEFAULT, ABIProto) ||
      !DefineABIConstant(cx, parent, "stdcall_abi", ABI_STDCALL, ABIProto) ||
      !DefineABIConstant(cx, parent, "winapi_abi", ABI_WINAPI, ABIProto))
    return false;

  
  
  
  
  
  
  
  
  
  
#define DEFINE_TYPE(name, type, ffiType)                                       \
  RootedObject typeObj_##name(cx,                                              \
    CType::DefineBuiltin(cx, parent, #name, CTypeProto, CDataProto, #name,     \
      TYPE_##name, INT_TO_JSVAL(sizeof(type)),                                 \
      INT_TO_JSVAL(ffiType.alignment), &ffiType));                             \
  if (!typeObj_##name)                                                         \
    return false;
#include "ctypes/typedefs.h"

  
  
  if (!JS_DefineProperty(cx, parent, "unsigned", typeObj_unsigned_int,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  
  RootedObject typeObj(cx,
    CType::DefineBuiltin(cx, parent, "void_t", CTypeProto, CDataProto, "void",
                         TYPE_void_t, JSVAL_VOID, JSVAL_VOID, &ffi_type_void));
  if (!typeObj)
    return false;

  typeObj = PointerType::CreateInternal(cx, typeObj);
  if (!typeObj)
    return false;
  if (!JS_DefineProperty(cx, parent, "voidptr_t", typeObj,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return true;
}

bool
IsCTypesGlobal(JSObject* obj)
{
  return JS_GetClass(obj) == &sCTypesGlobalClass;
}

bool
IsCTypesGlobal(HandleValue v)
{
  return v.isObject() && IsCTypesGlobal(&v.toObject());
}


JSCTypesCallbacks*
GetCallbacks(JSObject* obj)
{
  JS_ASSERT(IsCTypesGlobal(obj));

  jsval result = JS_GetReservedSlot(obj, SLOT_CALLBACKS);
  if (result.isUndefined())
    return nullptr;

  return static_cast<JSCTypesCallbacks*>(JSVAL_TO_PRIVATE(result));
}




static bool GetObjectProperty(JSContext *cx, HandleObject obj,
                              const char *property, MutableHandleObject result)
{
  RootedValue val(cx);
  if (!JS_GetProperty(cx, obj, property, &val)) {
    return false;
  }

  if (val.isPrimitive()) {
    JS_ReportError(cx, "missing or non-object field");
    return false;
  }

  result.set(val.toObjectOrNull());
  return true;
}

} 
} 

using namespace js;
using namespace js::ctypes;

JS_PUBLIC_API(bool)
JS_InitCTypesClass(JSContext* cx, HandleObject global)
{
  
  RootedObject ctypes(cx, JS_NewObject(cx, &sCTypesGlobalClass, NullPtr(), NullPtr()));
  if (!ctypes)
    return false;

  if (!JS_DefineProperty(cx, global, "ctypes", ctypes, JSPROP_READONLY | JSPROP_PERMANENT,
                         JS_PropertyStub, JS_StrictPropertyStub)){
    return false;
  }

  if (!InitTypeClasses(cx, ctypes))
    return false;

  
  if (!JS_DefineFunctions(cx, ctypes, sModuleFunctions) ||
      !JS_DefineProperties(cx, ctypes, sModuleProps))
    return false;

  
  RootedObject ctor(cx);
  if (!GetObjectProperty(cx, ctypes, "CDataFinalizer", &ctor))
    return false;

  RootedObject prototype(cx, JS_NewObject(cx, &sCDataFinalizerProtoClass, NullPtr(), ctypes));
  if (!prototype)
    return false;

  if (!JS_DefineFunctions(cx, prototype, sCDataFinalizerFunctions))
    return false;

  if (!JS_DefineProperty(cx, ctor, "prototype", prototype,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (!JS_DefineProperty(cx, prototype, "constructor", ctor,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;


  
  return JS_FreezeObject(cx, ctypes);
}

JS_PUBLIC_API(void)
JS_SetCTypesCallbacks(JSObject *ctypesObj, JSCTypesCallbacks* callbacks)
{
  JS_ASSERT(callbacks);
  JS_ASSERT(IsCTypesGlobal(ctypesObj));

  
  JS_SetReservedSlot(ctypesObj, SLOT_CALLBACKS, PRIVATE_TO_JSVAL(callbacks));
}

namespace js {

JS_FRIEND_API(size_t)
SizeOfDataIfCDataObject(mozilla::MallocSizeOf mallocSizeOf, JSObject *obj)
{
    if (!CData::IsCData(obj))
        return 0;

    size_t n = 0;
    jsval slot = JS_GetReservedSlot(obj, ctypes::SLOT_OWNS);
    if (!slot.isUndefined()) {
        bool owns = slot.toBoolean();
        slot = JS_GetReservedSlot(obj, ctypes::SLOT_DATA);
        if (!slot.isUndefined()) {
            char** buffer = static_cast<char**>(JSVAL_TO_PRIVATE(slot));
            n += mallocSizeOf(buffer);
            if (owns)
                n += mallocSizeOf(*buffer);
        }
    }
    return n;
}

namespace ctypes {










JS_STATIC_ASSERT(sizeof(bool) == 1 || sizeof(bool) == 4);
JS_STATIC_ASSERT(sizeof(char) == 1);
JS_STATIC_ASSERT(sizeof(short) == 2);
JS_STATIC_ASSERT(sizeof(int) == 4);
JS_STATIC_ASSERT(sizeof(unsigned) == 4);
JS_STATIC_ASSERT(sizeof(long) == 4 || sizeof(long) == 8);
JS_STATIC_ASSERT(sizeof(long long) == 8);
JS_STATIC_ASSERT(sizeof(size_t) == sizeof(uintptr_t));
JS_STATIC_ASSERT(sizeof(float) == 4);
JS_STATIC_ASSERT(sizeof(PRFuncPtr) == sizeof(void*));
JS_STATIC_ASSERT(NumericLimits<double>::is_signed);



template<class TargetType, class FromType>
struct ConvertImpl {
  static MOZ_ALWAYS_INLINE TargetType Convert(FromType d) {
    return TargetType(d);
  }
};

#ifdef _MSC_VER


template<>
struct ConvertImpl<uint64_t, double> {
  static MOZ_ALWAYS_INLINE uint64_t Convert(double d) {
    return d > 0x7fffffffffffffffui64 ?
           uint64_t(d - 0x8000000000000000ui64) + 0x8000000000000000ui64 :
           uint64_t(d);
  }
};
#endif





#if defined(SPARC) || defined(__powerpc__)

template<>
struct ConvertImpl<uint64_t, double> {
  static MOZ_ALWAYS_INLINE uint64_t Convert(double d) {
    return d >= 0xffffffffffffffff ?
           0x8000000000000000 : uint64_t(d);
  }
};

template<>
struct ConvertImpl<int64_t, double> {
  static MOZ_ALWAYS_INLINE int64_t Convert(double d) {
    return d >= 0x7fffffffffffffff ?
           0x8000000000000000 : int64_t(d);
  }
};
#endif

template<class TargetType, class FromType>
static MOZ_ALWAYS_INLINE TargetType Convert(FromType d)
{
  return ConvertImpl<TargetType, FromType>::Convert(d);
}

template<class TargetType, class FromType>
static MOZ_ALWAYS_INLINE bool IsAlwaysExact()
{
  
  
  
  
  
  
  
  
  
  if (NumericLimits<TargetType>::digits < NumericLimits<FromType>::digits)
    return false;

  if (NumericLimits<FromType>::is_signed &&
      !NumericLimits<TargetType>::is_signed)
    return false;

  if (!NumericLimits<FromType>::is_exact &&
      NumericLimits<TargetType>::is_exact)
    return false;

  return true;
}



template<class TargetType, class FromType, bool TargetSigned, bool FromSigned>
struct IsExactImpl {
  static MOZ_ALWAYS_INLINE bool Test(FromType i, TargetType j) {
    JS_STATIC_ASSERT(NumericLimits<TargetType>::is_exact);
    return FromType(j) == i;
  }
};


template<class TargetType, class FromType>
struct IsExactImpl<TargetType, FromType, false, true> {
  static MOZ_ALWAYS_INLINE bool Test(FromType i, TargetType j) {
    JS_STATIC_ASSERT(NumericLimits<TargetType>::is_exact);
    return i >= 0 && FromType(j) == i;
  }
};


template<class TargetType, class FromType>
struct IsExactImpl<TargetType, FromType, true, false> {
  static MOZ_ALWAYS_INLINE bool Test(FromType i, TargetType j) {
    JS_STATIC_ASSERT(NumericLimits<TargetType>::is_exact);
    return TargetType(i) >= 0 && FromType(j) == i;
  }
};



template<class TargetType, class FromType>
static MOZ_ALWAYS_INLINE bool ConvertExact(FromType i, TargetType* result)
{
  
  JS_STATIC_ASSERT(NumericLimits<TargetType>::is_exact);

  *result = Convert<TargetType>(i);

  
  if (IsAlwaysExact<TargetType, FromType>())
    return true;

  
  return IsExactImpl<TargetType,
                     FromType,
                     NumericLimits<TargetType>::is_signed,
                     NumericLimits<FromType>::is_signed>::Test(i, *result);
}



template<class Type, bool IsSigned>
struct IsNegativeImpl {
  static MOZ_ALWAYS_INLINE bool Test(Type i) {
    return false;
  }
};


template<class Type>
struct IsNegativeImpl<Type, true> {
  static MOZ_ALWAYS_INLINE bool Test(Type i) {
    return i < 0;
  }
};


template<class Type>
static MOZ_ALWAYS_INLINE bool IsNegative(Type i)
{
  return IsNegativeImpl<Type, NumericLimits<Type>::is_signed>::Test(i);
}



static bool
jsvalToBool(JSContext* cx, jsval val, bool* result)
{
  if (val.isBoolean()) {
    *result = val.toBoolean();
    return true;
  }
  if (val.isInt32()) {
    int32_t i = val.toInt32();
    *result = i != 0;
    return i == 0 || i == 1;
  }
  if (val.isDouble()) {
    double d = val.toDouble();
    *result = d != 0;
    
    return d == 1 || d == 0;
  }
  
  return false;
}




template<class IntegerType>
static bool
jsvalToInteger(JSContext* cx, jsval val, IntegerType* result)
{
  JS_STATIC_ASSERT(NumericLimits<IntegerType>::is_exact);

  if (val.isInt32()) {
    
    
    int32_t i = val.toInt32();
    return ConvertExact(i, result);
  }
  if (val.isDouble()) {
    
    
    double d = val.toDouble();
    return ConvertExact(d, result);
  }
  if (val.isObject()) {
    JSObject* obj = &val.toObject();
    if (CData::IsCData(obj)) {
      JSObject* typeObj = CData::GetCType(obj);
      void* data = CData::GetData(obj);

      
      
      switch (CType::GetTypeCode(typeObj)) {
#define DEFINE_INT_TYPE(name, fromType, ffiType)                               \
      case TYPE_##name:                                                        \
        if (!IsAlwaysExact<IntegerType, fromType>())                           \
          return false;                                                        \
        *result = IntegerType(*static_cast<fromType*>(data));                  \
        return true;
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "ctypes/typedefs.h"
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
      case TYPE_function:
      case TYPE_array:
      case TYPE_struct:
        
        return false;
      }
    }

    if (Int64::IsInt64(obj)) {
      
      int64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (UInt64::IsUInt64(obj)) {
      
      uint64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (CDataFinalizer::IsCDataFinalizer(obj)) {
      RootedValue innerData(cx);
      if (!CDataFinalizer::GetValue(cx, obj, innerData.address())) {
        return false; 
      }
      return jsvalToInteger(cx, innerData, result);
    }

    return false;
  }
  if (val.isBoolean()) {
    
    *result = val.toBoolean();
    JS_ASSERT(*result == 0 || *result == 1);
    return true;
  }
  
  return false;
}




template<class FloatType>
static bool
jsvalToFloat(JSContext *cx, jsval val, FloatType* result)
{
  JS_STATIC_ASSERT(!NumericLimits<FloatType>::is_exact);

  
  
  
  
  if (val.isInt32()) {
    *result = FloatType(val.toInt32());
    return true;
  }
  if (val.isDouble()) {
    *result = FloatType(val.toDouble());
    return true;
  }
  if (val.isObject()) {
    JSObject* obj = &val.toObject();
    if (CData::IsCData(obj)) {
      JSObject* typeObj = CData::GetCType(obj);
      void* data = CData::GetData(obj);

      
      
      switch (CType::GetTypeCode(typeObj)) {
#define DEFINE_FLOAT_TYPE(name, fromType, ffiType)                             \
      case TYPE_##name:                                                        \
        if (!IsAlwaysExact<FloatType, fromType>())                             \
          return false;                                                        \
        *result = FloatType(*static_cast<fromType*>(data));                    \
        return true;
#define DEFINE_INT_TYPE(x, y, z) DEFINE_FLOAT_TYPE(x, y, z)
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "ctypes/typedefs.h"
      case TYPE_void_t:
      case TYPE_bool:
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char:
      case TYPE_jschar:
      case TYPE_pointer:
      case TYPE_function:
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
StringToInteger(JSContext* cx, JSString* string, IntegerType* result)
{
  JS_STATIC_ASSERT(NumericLimits<IntegerType>::is_exact);

  const jschar* cp = string->getChars(nullptr);
  if (!cp)
    return false;

  const jschar* end = cp + string->length();
  if (cp == end)
    return false;

  IntegerType sign = 1;
  if (cp[0] == '-') {
    if (!NumericLimits<IntegerType>::is_signed)
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




template<class IntegerType>
static bool
jsvalToBigInteger(JSContext* cx,
                  jsval val,
                  bool allowString,
                  IntegerType* result)
{
  JS_STATIC_ASSERT(NumericLimits<IntegerType>::is_exact);

  if (val.isInt32()) {
    
    
    int32_t i = val.toInt32();
    return ConvertExact(i, result);
  }
  if (val.isDouble()) {
    
    
    double d = val.toDouble();
    return ConvertExact(d, result);
  }
  if (allowString && val.isString()) {
    
    
    
    
    return StringToInteger(cx, val.toString(), result);
  }
  if (val.isObject()) {
    
    JSObject* obj = &val.toObject();

    if (UInt64::IsUInt64(obj)) {
      
      uint64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (Int64::IsInt64(obj)) {
      
      int64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (CDataFinalizer::IsCDataFinalizer(obj)) {
      RootedValue innerData(cx);
      if (!CDataFinalizer::GetValue(cx, obj, innerData.address())) {
        return false; 
      }
      return jsvalToBigInteger(cx, innerData, allowString, result);
    }

  }
  return false;
}



static bool
jsvalToSize(JSContext* cx, jsval val, bool allowString, size_t* result)
{
  if (!jsvalToBigInteger(cx, val, allowString, result))
    return false;

  
  return Convert<size_t>(double(*result)) == *result;
}




template<class IntegerType>
static bool
jsidToBigInteger(JSContext* cx,
                  jsid val,
                  bool allowString,
                  IntegerType* result)
{
  JS_STATIC_ASSERT(NumericLimits<IntegerType>::is_exact);

  if (JSID_IS_INT(val)) {
    
    
    int32_t i = JSID_TO_INT(val);
    return ConvertExact(i, result);
  }
  if (allowString && JSID_IS_STRING(val)) {
    
    
    
    
    return StringToInteger(cx, JSID_TO_STRING(val), result);
  }
  if (JSID_IS_OBJECT(val)) {
    
    JSObject* obj = JSID_TO_OBJECT(val);

    if (UInt64::IsUInt64(obj)) {
      
      uint64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (Int64::IsInt64(obj)) {
      
      int64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }
  }
  return false;
}



static bool
jsidToSize(JSContext* cx, jsid val, bool allowString, size_t* result)
{
  if (!jsidToBigInteger(cx, val, allowString, result))
    return false;

  
  return Convert<size_t>(double(*result)) == *result;
}



static bool
SizeTojsval(JSContext* cx, size_t size, jsval* result)
{
  if (Convert<size_t>(double(size)) != size) {
    JS_ReportError(cx, "size overflow");
    return false;
  }

  *result = JS_NumberValue(double(size));
  return true;
}


template<class IntegerType>
static bool
jsvalToIntegerExplicit(jsval val, IntegerType* result)
{
  JS_STATIC_ASSERT(NumericLimits<IntegerType>::is_exact);

  if (val.isDouble()) {
    
    double d = val.toDouble();
    *result = mozilla::IsFinite(d) ? IntegerType(d) : 0;
    return true;
  }
  if (val.isObject()) {
    
    JSObject* obj = &val.toObject();
    if (Int64::IsInt64(obj)) {
      int64_t i = Int64Base::GetInt(obj);
      *result = IntegerType(i);
      return true;
    }
    if (UInt64::IsUInt64(obj)) {
      uint64_t i = Int64Base::GetInt(obj);
      *result = IntegerType(i);
      return true;
    }
  }
  return false;
}


static bool
jsvalToPtrExplicit(JSContext* cx, jsval val, uintptr_t* result)
{
  if (val.isInt32()) {
    
    
    int32_t i = val.toInt32();
    *result = i < 0 ? uintptr_t(intptr_t(i)) : uintptr_t(i);
    return true;
  }
  if (val.isDouble()) {
    double d = val.toDouble();
    if (d < 0) {
      
      intptr_t i = Convert<intptr_t>(d);
      if (double(i) != d)
        return false;

      *result = uintptr_t(i);
      return true;
    }

    
    
    *result = Convert<uintptr_t>(d);
    return double(*result) == d;
  }
  if (val.isObject()) {
    JSObject* obj = &val.toObject();
    if (Int64::IsInt64(obj)) {
      int64_t i = Int64Base::GetInt(obj);
      intptr_t p = intptr_t(i);

      
      if (int64_t(p) != i)
        return false;
      *result = uintptr_t(p);
      return true;
    }

    if (UInt64::IsUInt64(obj)) {
      uint64_t i = Int64Base::GetInt(obj);

      
      *result = uintptr_t(i);
      return uint64_t(*result) == i;
    }
  }
  return false;
}

template<class IntegerType, class CharType, size_t N, class AP>
void
IntegerToString(IntegerType i, int radix, Vector<CharType, N, AP>& result)
{
  JS_STATIC_ASSERT(NumericLimits<IntegerType>::is_exact);

  
  
  CharType buffer[sizeof(IntegerType) * 8 + 1];
  CharType* end = buffer + sizeof(buffer) / sizeof(CharType);
  CharType* cp = end;

  
  
  const bool isNegative = IsNegative(i);
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
  result.append(cp, end);
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












static bool
ConvertToJS(JSContext* cx,
            HandleObject typeObj,
            HandleObject parentObj,
            void* data,
            bool wantPrimitive,
            bool ownResult,
            jsval* result)
{
  JS_ASSERT(!parentObj || CData::IsCData(parentObj));
  JS_ASSERT(!parentObj || !ownResult);
  JS_ASSERT(!wantPrimitive || !ownResult);

  TypeCode typeCode = CType::GetTypeCode(typeObj);

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
      *result = INT_TO_JSVAL(int32_t(value));                                  \
    else                                                                       \
      *result = JS_NumberValue(double(value));                                 \
    break;                                                                     \
  }
#define DEFINE_WRAPPED_INT_TYPE(name, type, ffiType)                           \
  case TYPE_##name: {                                                          \
    /* Return an Int64 or UInt64 object - do not convert to a JS number. */    \
    uint64_t value;                                                            \
    RootedObject proto(cx);                                                    \
    if (!NumericLimits<type>::is_signed) {                                     \
      value = *static_cast<type*>(data);                                       \
      /* Get ctypes.UInt64.prototype from ctypes.CType.prototype. */           \
      proto = CType::GetProtoFromType(cx, typeObj, SLOT_UINT64PROTO);          \
      if (!proto)                                                              \
        return false;                                                          \
    } else {                                                                   \
      value = int64_t(*static_cast<type*>(data));                              \
      /* Get ctypes.Int64.prototype from ctypes.CType.prototype. */            \
      proto = CType::GetProtoFromType(cx, typeObj, SLOT_INT64PROTO);           \
      if (!proto)                                                              \
        return false;                                                          \
    }                                                                          \
                                                                               \
    JSObject* obj = Int64Base::Construct(cx, proto, value,                     \
      !NumericLimits<type>::is_signed);                                        \
    if (!obj)                                                                  \
      return false;                                                            \
    *result = OBJECT_TO_JSVAL(obj);                                            \
    break;                                                                     \
  }
#define DEFINE_FLOAT_TYPE(name, type, ffiType)                                 \
  case TYPE_##name: {                                                          \
    type value = *static_cast<type*>(data);                                    \
    *result = JS_NumberValue(double(value));                                   \
    break;                                                                     \
  }
#define DEFINE_CHAR_TYPE(name, type, ffiType)                                  \
  case TYPE_##name:                                                            \
    /* Convert to an integer. We have no idea what character encoding to */    \
    /* use, if any. */                                                         \
    *result = INT_TO_JSVAL(*static_cast<type*>(data));                         \
    break;
#include "ctypes/typedefs.h"
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

    JSObject* obj = CData::Create(cx, typeObj, parentObj, data, ownResult);
    if (!obj)
      return false;

    *result = OBJECT_TO_JSVAL(obj);
    break;
  }
  case TYPE_function:
    MOZ_ASSUME_UNREACHABLE("cannot return a FunctionType");
  }

  return true;
}




bool CanConvertTypedArrayItemTo(JSObject *baseType, JSObject *valObj, JSContext *cx) {
  TypeCode baseTypeCode = CType::GetTypeCode(baseType);
  if (baseTypeCode == TYPE_void_t) {
    return true;
  }
  TypeCode elementTypeCode;
  switch (JS_GetArrayBufferViewType(valObj)) {
  case ScalarTypeDescr::TYPE_INT8:
    elementTypeCode = TYPE_int8_t;
    break;
  case ScalarTypeDescr::TYPE_UINT8:
  case ScalarTypeDescr::TYPE_UINT8_CLAMPED:
    elementTypeCode = TYPE_uint8_t;
    break;
  case ScalarTypeDescr::TYPE_INT16:
    elementTypeCode = TYPE_int16_t;
    break;
  case ScalarTypeDescr::TYPE_UINT16:
    elementTypeCode = TYPE_uint16_t;
    break;
  case ScalarTypeDescr::TYPE_INT32:
    elementTypeCode = TYPE_int32_t;
    break;
  case ScalarTypeDescr::TYPE_UINT32:
    elementTypeCode = TYPE_uint32_t;
    break;
  case ScalarTypeDescr::TYPE_FLOAT32:
    elementTypeCode = TYPE_float32_t;
    break;
  case ScalarTypeDescr::TYPE_FLOAT64:
    elementTypeCode = TYPE_float64_t;
    break;
  default:
    return false;
  }
  return elementTypeCode == baseTypeCode;
}












static bool
ImplicitConvert(JSContext* cx,
                HandleValue val,
                JSObject* targetType_,
                void* buffer,
                bool isArgument,
                bool* freePointer)
{
  RootedObject targetType(cx, targetType_);
  JS_ASSERT(CType::IsSizeDefined(targetType));

  
  
  JSObject* sourceData = nullptr;
  JSObject* sourceType = nullptr;
  RootedObject valObj(cx, nullptr);
  if (val.isObject()) {
    valObj = &val.toObject();
    if (CData::IsCData(valObj)) {
      sourceData = valObj;
      sourceType = CData::GetCType(sourceData);

      
      
      if (CType::TypesEqual(sourceType, targetType)) {
        size_t size = CType::GetSize(sourceType);
        memmove(buffer, CData::GetData(sourceData), size);
        return true;
      }
    } else if (CDataFinalizer::IsCDataFinalizer(valObj)) {
      sourceData = valObj;
      sourceType = CDataFinalizer::GetCType(cx, sourceData);

      CDataFinalizer::Private *p = (CDataFinalizer::Private *)
        JS_GetPrivate(sourceData);

      if (!p) {
        
        JS_ReportError(cx, "Attempting to convert an empty CDataFinalizer");
        return false;
      }

      
      if (CType::TypesEqual(sourceType, targetType)) {
        memmove(buffer, p->cargs, p->cargs_size);
        return true;
      }
    }
  }

  TypeCode targetCode = CType::GetTypeCode(targetType);

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
    if (val.isString()) {                                                \
      JSString* str = val.toString();                                    \
      if (str->length() != 1)                                                  \
        return TypeError(cx, #name, val);                                      \
      const jschar *chars = str->getChars(cx);                                 \
      if (!chars)                                                              \
        return false;                                                          \
      result = chars[0];                                                       \
    } else if (!jsvalToInteger(cx, val, &result)) {                            \
      return TypeError(cx, #name, val);                                        \
    }                                                                          \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#include "ctypes/typedefs.h"
  case TYPE_pointer: {
    if (val.isNull()) {
      
      *static_cast<void**>(buffer) = nullptr;
      break;
    }

    JS::Rooted<JSObject*> baseType(cx, PointerType::GetBaseType(targetType));
    if (sourceData) {
      
      TypeCode sourceCode = CType::GetTypeCode(sourceType);
      void* sourceBuffer = CData::GetData(sourceData);
      bool voidptrTarget = CType::GetTypeCode(baseType) == TYPE_void_t;

      if (sourceCode == TYPE_pointer && voidptrTarget) {
        
        *static_cast<void**>(buffer) = *static_cast<void**>(sourceBuffer);
        break;
      }
      if (sourceCode == TYPE_array) {
        
        
        JSObject* elementType = ArrayType::GetBaseType(sourceType);
        if (voidptrTarget || CType::TypesEqual(baseType, elementType)) {
          *static_cast<void**>(buffer) = sourceBuffer;
          break;
        }
      }

    } else if (isArgument && val.isString()) {
      
      
      
      JSString* sourceString = val.toString();
      size_t sourceLength = sourceString->length();
      const jschar* sourceChars = sourceString->getChars(cx);
      if (!sourceChars)
        return false;

      switch (CType::GetTypeCode(baseType)) {
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char: {
        
        size_t nbytes =
          GetDeflatedUTF8StringLength(cx, sourceChars, sourceLength);
        if (nbytes == (size_t) -1)
          return false;

        char** charBuffer = static_cast<char**>(buffer);
        *charBuffer = cx->pod_malloc<char>(nbytes + 1);
        if (!*charBuffer) {
          JS_ReportAllocationOverflow(cx);
          return false;
        }

        ASSERT_OK(DeflateStringToUTF8Buffer(cx, sourceChars, sourceLength,
                    *charBuffer, &nbytes));
        (*charBuffer)[nbytes] = 0;
        *freePointer = true;
        break;
      }
      case TYPE_jschar: {
        
        
        
        jschar** jscharBuffer = static_cast<jschar**>(buffer);
        *jscharBuffer = cx->pod_malloc<jschar>(sourceLength + 1);
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
        return TypeError(cx, "string pointer", val);
      }
      break;
    } else if (val.isObject() && JS_IsArrayBufferObject(valObj)) {
      
      
      
      void* p = JS_GetStableArrayBufferData(cx, valObj);
      if (!p)
          return false;
      *static_cast<void**>(buffer) = p;
      break;
    } if (val.isObject() && JS_IsTypedArrayObject(valObj)) {
      if(!CanConvertTypedArrayItemTo(baseType, valObj, cx)) {
        return TypeError(cx, "typed array with the appropriate type", val);
      }

      
      
      
      *static_cast<void**>(buffer) = JS_GetArrayBufferViewData(valObj);
      break;
    }
    return TypeError(cx, "pointer", val);
  }
  case TYPE_array: {
    RootedObject baseType(cx, ArrayType::GetBaseType(targetType));
    size_t targetLength = ArrayType::GetLength(targetType);

    if (val.isString()) {
      JSString* sourceString = val.toString();
      size_t sourceLength = sourceString->length();
      const jschar* sourceChars = sourceString->getChars(cx);
      if (!sourceChars)
        return false;

      switch (CType::GetTypeCode(baseType)) {
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char: {
        
        size_t nbytes =
          GetDeflatedUTF8StringLength(cx, sourceChars, sourceLength);
        if (nbytes == (size_t) -1)
          return false;

        if (targetLength < nbytes) {
          JS_ReportError(cx, "ArrayType has insufficient length");
          return false;
        }

        char* charBuffer = static_cast<char*>(buffer);
        ASSERT_OK(DeflateStringToUTF8Buffer(cx, sourceChars, sourceLength,
                    charBuffer, &nbytes));

        if (targetLength > nbytes)
          charBuffer[nbytes] = 0;

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

    } else if (val.isObject() && JS_IsArrayObject(cx, valObj)) {
      
      uint32_t sourceLength;
      if (!JS_GetArrayLength(cx, valObj, &sourceLength) ||
          targetLength != size_t(sourceLength)) {
        JS_ReportError(cx, "ArrayType length does not match source array length");
        return false;
      }

      
      size_t elementSize = CType::GetSize(baseType);
      size_t arraySize = elementSize * targetLength;
      AutoPtr<char> intermediate(cx->pod_malloc<char>(arraySize));
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      for (uint32_t i = 0; i < sourceLength; ++i) {
        RootedValue item(cx);
        if (!JS_GetElement(cx, valObj, i, &item))
          return false;

        char* data = intermediate.get() + elementSize * i;
        if (!ImplicitConvert(cx, item, baseType, data, false, nullptr))
          return false;
      }

      memcpy(buffer, intermediate.get(), arraySize);

    } else if (val.isObject() && JS_IsArrayBufferObject(valObj)) {
      
      
      uint32_t sourceLength = JS_GetArrayBufferByteLength(valObj);
      size_t elementSize = CType::GetSize(baseType);
      size_t arraySize = elementSize * targetLength;
      if (arraySize != size_t(sourceLength)) {
        JS_ReportError(cx, "ArrayType length does not match source ArrayBuffer length");
        return false;
      }
      memcpy(buffer, JS_GetArrayBufferData(valObj), sourceLength);
      break;
    } else if (val.isObject() && JS_IsTypedArrayObject(valObj)) {
      
      
      if(!CanConvertTypedArrayItemTo(baseType, valObj, cx)) {
        return TypeError(cx, "typed array with the appropriate type", val);
      }

      uint32_t sourceLength = JS_GetTypedArrayByteLength(valObj);
      size_t elementSize = CType::GetSize(baseType);
      size_t arraySize = elementSize * targetLength;
      if (arraySize != size_t(sourceLength)) {
        JS_ReportError(cx, "typed array length does not match source TypedArray length");
        return false;
      }
      memcpy(buffer, JS_GetArrayBufferViewData(valObj), sourceLength);
      break;
    } else {
      
      
      return TypeError(cx, "array", val);
    }
    break;
  }
  case TYPE_struct: {
    if (val.isObject() && !sourceData) {
      
      
      RootedObject iter(cx, JS_NewPropertyIterator(cx, valObj));
      if (!iter)
        return false;

      
      size_t structSize = CType::GetSize(targetType);
      AutoPtr<char> intermediate(cx->pod_malloc<char>(structSize));
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      RootedId id(cx);
      size_t i = 0;
      while (1) {
        if (!JS_NextProperty(cx, iter, id.address()))
          return false;
        if (JSID_IS_VOID(id))
          break;

        if (!JSID_IS_STRING(id)) {
          JS_ReportError(cx, "property name is not a string");
          return false;
        }

        JSFlatString *name = JSID_TO_FLAT_STRING(id);
        const FieldInfo* field = StructType::LookupField(cx, targetType, name);
        if (!field)
          return false;

        RootedValue prop(cx);
        if (!JS_GetPropertyById(cx, valObj, id, &prop))
          return false;

        
        char* fieldData = intermediate.get() + field->mOffset;
        if (!ImplicitConvert(cx, prop, field->mType, fieldData, false, nullptr))
          return false;

        ++i;
      }

      const FieldInfoHash* fields = StructType::GetFieldInfo(targetType);
      if (i != fields->count()) {
        JS_ReportError(cx, "missing fields");
        return false;
      }

      memcpy(buffer, intermediate.get(), structSize);
      break;
    }

    return TypeError(cx, "struct", val);
  }
  case TYPE_void_t:
  case TYPE_function:
    MOZ_ASSUME_UNREACHABLE("invalid type");
  }

  return true;
}




static bool
ExplicitConvert(JSContext* cx, HandleValue val, HandleObject targetType, void* buffer)
{
  
  if (ImplicitConvert(cx, val, targetType, buffer, false, nullptr))
    return true;

  
  
  
  RootedValue ex(cx);
  if (!JS_GetPendingException(cx, &ex))
    return false;

  
  
  JS_ClearPendingException(cx);

  TypeCode type = CType::GetTypeCode(targetType);

  switch (type) {
  case TYPE_bool: {
    *static_cast<bool*>(buffer) = ToBoolean(val);
    break;
  }
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name: {                                                          \
    /* Convert numeric values with a C-style cast, and */                      \
    /* allow conversion from a base-10 or base-16 string. */                   \
    type result;                                                               \
    if (!jsvalToIntegerExplicit(val, &result) &&                               \
        (!val.isString() ||                                              \
         !StringToInteger(cx, val.toString(), &result)))                 \
      return TypeError(cx, #name, val);                                        \
    *static_cast<type*>(buffer) = result;                                      \
    break;                                                                     \
  }
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_JSCHAR_TYPE(x, y, z) DEFINE_CHAR_TYPE(x, y, z)
#include "ctypes/typedefs.h"
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
  case TYPE_function:
    MOZ_ASSUME_UNREACHABLE("invalid type");
  }
  return true;
}





static JSString*
BuildTypeName(JSContext* cx, JSObject* typeObj_)
{
  AutoString result;
  RootedObject typeObj(cx, typeObj_);

  
  
  
  
  
  
  TypeCode prevGrouping = CType::GetTypeCode(typeObj), currentGrouping;
  while (1) {
    currentGrouping = CType::GetTypeCode(typeObj);
    switch (currentGrouping) {
    case TYPE_pointer: {
      
      PrependString(result, "*");

      typeObj = PointerType::GetBaseType(typeObj);
      prevGrouping = currentGrouping;
      continue;
    }
    case TYPE_array: {
      if (prevGrouping == TYPE_pointer) {
        
        PrependString(result, "(");
        AppendString(result, ")");
      }

      
      AppendString(result, "[");
      size_t length;
      if (ArrayType::GetSafeLength(typeObj, &length))
        IntegerToString(length, 10, result);

      AppendString(result, "]");

      typeObj = ArrayType::GetBaseType(typeObj);
      prevGrouping = currentGrouping;
      continue;
    }
    case TYPE_function: {
      FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);

      
      
      
      
      
      ABICode abi = GetABICode(fninfo->mABI);
      if (abi == ABI_STDCALL)
        PrependString(result, "__stdcall");
      else if (abi == ABI_WINAPI)
        PrependString(result, "WINAPI");

      
      
      
      
      if (prevGrouping == TYPE_pointer) {
        PrependString(result, "(");
        AppendString(result, ")");
      }

      
      AppendString(result, "(");
      for (size_t i = 0; i < fninfo->mArgTypes.length(); ++i) {
        RootedObject argType(cx, fninfo->mArgTypes[i]);
        JSString* argName = CType::GetName(cx, argType);
        AppendString(result, argName);
        if (i != fninfo->mArgTypes.length() - 1 ||
            fninfo->mIsVariadic)
          AppendString(result, ", ");
      }
      if (fninfo->mIsVariadic)
        AppendString(result, "...");
      AppendString(result, ")");

      
      
      
      typeObj = fninfo->mReturnType;
      continue;
    }
    default:
      
      break;
    }
    break;
  }

  
  
  if (('a' <= result[0] && result[0] <= 'z') ||
      ('A' <= result[0] && result[0] <= 'Z') ||
      (result[0] == '_'))
    PrependString(result, " ");

  
  JSString* baseName = CType::GetName(cx, typeObj);
  PrependString(result, baseName);
  return NewUCString(cx, result);
}








static void
BuildTypeSource(JSContext* cx,
                JSObject* typeObj_,
                bool makeShort,
                AutoString& result)
{
  RootedObject typeObj(cx, typeObj_);

  
  switch (CType::GetTypeCode(typeObj)) {
  case TYPE_void_t:
#define DEFINE_TYPE(name, type, ffiType)  \
  case TYPE_##name:
#include "ctypes/typedefs.h"
  {
    AppendString(result, "ctypes.");
    JSString* nameStr = CType::GetName(cx, typeObj);
    AppendString(result, nameStr);
    break;
  }
  case TYPE_pointer: {
    RootedObject baseType(cx, PointerType::GetBaseType(typeObj));

    
    if (CType::GetTypeCode(baseType) == TYPE_void_t) {
      AppendString(result, "ctypes.voidptr_t");
      break;
    }

    
    BuildTypeSource(cx, baseType, makeShort, result);
    AppendString(result, ".ptr");
    break;
  }
  case TYPE_function: {
    FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);

    AppendString(result, "ctypes.FunctionType(");

    switch (GetABICode(fninfo->mABI)) {
    case ABI_DEFAULT:
      AppendString(result, "ctypes.default_abi, ");
      break;
    case ABI_STDCALL:
      AppendString(result, "ctypes.stdcall_abi, ");
      break;
    case ABI_WINAPI:
      AppendString(result, "ctypes.winapi_abi, ");
      break;
    case INVALID_ABI:
      MOZ_ASSUME_UNREACHABLE("invalid abi");
    }

    
    
    BuildTypeSource(cx, fninfo->mReturnType, true, result);

    if (fninfo->mArgTypes.length() > 0) {
      AppendString(result, ", [");
      for (size_t i = 0; i < fninfo->mArgTypes.length(); ++i) {
        BuildTypeSource(cx, fninfo->mArgTypes[i], true, result);
        if (i != fninfo->mArgTypes.length() - 1 ||
            fninfo->mIsVariadic)
          AppendString(result, ", ");
      }
      if (fninfo->mIsVariadic)
        AppendString(result, "\"...\"");
      AppendString(result, "]");
    }

    AppendString(result, ")");
    break;
  }
  case TYPE_array: {
    
    
    
    JSObject* baseType = ArrayType::GetBaseType(typeObj);
    BuildTypeSource(cx, baseType, makeShort, result);
    AppendString(result, ".array(");

    size_t length;
    if (ArrayType::GetSafeLength(typeObj, &length))
      IntegerToString(length, 10, result);

    AppendString(result, ")");
    break;
  }
  case TYPE_struct: {
    JSString* name = CType::GetName(cx, typeObj);

    if (makeShort) {
      
      
      AppendString(result, name);
      break;
    }

    
    AppendString(result, "ctypes.StructType(\"");
    AppendString(result, name);
    AppendString(result, "\"");

    
    if (!CType::IsSizeDefined(typeObj)) {
      AppendString(result, ")");
      break;
    }

    AppendString(result, ", [");

    const FieldInfoHash* fields = StructType::GetFieldInfo(typeObj);
    size_t length = fields->count();
    Array<const FieldInfoHash::Entry*, 64> fieldsArray;
    if (!fieldsArray.resize(length))
      break;

    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront())
      fieldsArray[r.front().value().mIndex] = &r.front();

    for (size_t i = 0; i < length; ++i) {
      const FieldInfoHash::Entry* entry = fieldsArray[i];
      AppendString(result, "{ \"");
      AppendString(result, entry->key());
      AppendString(result, "\": ");
      BuildTypeSource(cx, entry->value().mType, true, result);
      AppendString(result, " }");
      if (i != length - 1)
        AppendString(result, ", ");
    }

    AppendString(result, "])");
    break;
  }
  }
}











static bool
BuildDataSource(JSContext* cx,
                HandleObject typeObj,
                void* data,
                bool isImplicit,
                AutoString& result)
{
  TypeCode type = CType::GetTypeCode(typeObj);
  switch (type) {
  case TYPE_bool:
    if (*static_cast<bool*>(data))
      AppendString(result, "true");
    else
      AppendString(result, "false");
    break;
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name:                                                            \
    /* Serialize as a primitive decimal integer. */                            \
    IntegerToString(*static_cast<type*>(data), 10, result);                    \
    break;
#define DEFINE_WRAPPED_INT_TYPE(name, type, ffiType)                           \
  case TYPE_##name:                                                            \
    /* Serialize as a wrapped decimal integer. */                              \
    if (!NumericLimits<type>::is_signed)                                       \
      AppendString(result, "ctypes.UInt64(\"");                                \
    else                                                                       \
      AppendString(result, "ctypes.Int64(\"");                                 \
                                                                               \
    IntegerToString(*static_cast<type*>(data), 10, result);                    \
    AppendString(result, "\")");                                               \
    break;
#define DEFINE_FLOAT_TYPE(name, type, ffiType)                                 \
  case TYPE_##name: {                                                          \
    /* Serialize as a primitive double. */                                     \
    double fp = *static_cast<type*>(data);                                     \
    ToCStringBuf cbuf;                                                         \
    char* str = NumberToCString(cx, &cbuf, fp);                                \
    if (!str) {                                                                \
      JS_ReportOutOfMemory(cx);                                                \
      return false;                                                            \
    }                                                                          \
                                                                               \
    result.append(str, strlen(str));                                           \
    break;                                                                     \
  }
#define DEFINE_CHAR_TYPE(name, type, ffiType)                                  \
  case TYPE_##name:                                                            \
    /* Serialize as an integer. */                                             \
    IntegerToString(*static_cast<type*>(data), 10, result);                    \
    break;
#include "ctypes/typedefs.h"
  case TYPE_jschar: {
    
    JSString* str = JS_NewUCStringCopyN(cx, static_cast<jschar*>(data), 1);
    if (!str)
      return false;

    
    RootedValue valStr(cx, StringValue(str));
    JSString* src = JS_ValueToSource(cx, valStr);
    if (!src)
      return false;

    AppendString(result, src);
    break;
  }
  case TYPE_pointer:
  case TYPE_function: {
    if (isImplicit) {
      
      
      BuildTypeSource(cx, typeObj, true, result);
      AppendString(result, "(");
    }

    
    uintptr_t ptr = *static_cast<uintptr_t*>(data);
    AppendString(result, "ctypes.UInt64(\"0x");
    IntegerToString(ptr, 16, result);
    AppendString(result, "\")");

    if (isImplicit)
      AppendString(result, ")");

    break;
  }
  case TYPE_array: {
    
    
    RootedObject baseType(cx, ArrayType::GetBaseType(typeObj));
    AppendString(result, "[");

    size_t length = ArrayType::GetLength(typeObj);
    size_t elementSize = CType::GetSize(baseType);
    for (size_t i = 0; i < length; ++i) {
      char* element = static_cast<char*>(data) + elementSize * i;
      if (!BuildDataSource(cx, baseType, element, true, result))
        return false;

      if (i + 1 < length)
        AppendString(result, ", ");
    }
    AppendString(result, "]");
    break;
  }
  case TYPE_struct: {
    if (isImplicit) {
      
      
      
      AppendString(result, "{");
    }

    
    
    const FieldInfoHash* fields = StructType::GetFieldInfo(typeObj);
    size_t length = fields->count();
    Array<const FieldInfoHash::Entry*, 64> fieldsArray;
    if (!fieldsArray.resize(length))
      return false;

    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront())
      fieldsArray[r.front().value().mIndex] = &r.front();

    for (size_t i = 0; i < length; ++i) {
      const FieldInfoHash::Entry* entry = fieldsArray[i];

      if (isImplicit) {
        AppendString(result, "\"");
        AppendString(result, entry->key());
        AppendString(result, "\": ");
      }

      char* fieldData = static_cast<char*>(data) + entry->value().mOffset;
      RootedObject entryType(cx, entry->value().mType);
      if (!BuildDataSource(cx, entryType, fieldData, true, result))
        return false;

      if (i + 1 != length)
        AppendString(result, ", ");
    }

    if (isImplicit)
      AppendString(result, "}");

    break;
  }
  case TYPE_void_t:
    MOZ_ASSUME_UNREACHABLE("invalid type");
  }

  return true;
}





bool
ConstructAbstract(JSContext* cx,
                  unsigned argc,
                  jsval* vp)
{
  
  JS_ReportError(cx, "cannot construct from abstract type");
  return false;
}





bool
CType::ConstructData(JSContext* cx,
                     unsigned argc,
                     jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  RootedObject obj(cx, &args.callee());
  if (!CType::IsCType(obj)) {
    JS_ReportError(cx, "not a CType");
    return false;
  }

  
  
  
  
  switch (GetTypeCode(obj)) {
  case TYPE_void_t:
    JS_ReportError(cx, "cannot construct from void_t");
    return false;
  case TYPE_function:
    JS_ReportError(cx, "cannot construct from FunctionType; use FunctionType.ptr instead");
    return false;
  case TYPE_pointer:
    return PointerType::ConstructData(cx, obj, args);
  case TYPE_array:
    return ArrayType::ConstructData(cx, obj, args);
  case TYPE_struct:
    return StructType::ConstructData(cx, obj, args);
  default:
    return ConstructBasic(cx, obj, args);
  }
}

bool
CType::ConstructBasic(JSContext* cx,
                      HandleObject obj,
                      const CallArgs& args)
{
  if (args.length() > 1) {
    JS_ReportError(cx, "CType constructor takes zero or one argument");
    return false;
  }

  
  RootedObject result(cx, CData::Create(cx, obj, NullPtr(), nullptr, true));
  if (!result)
    return false;

  if (args.length() == 1) {
    if (!ExplicitConvert(cx, args[0], obj, CData::GetData(result)))
      return false;
  }

  args.rval().setObject(*result);
  return true;
}

JSObject*
CType::Create(JSContext* cx,
              HandleObject typeProto,
              HandleObject dataProto,
              TypeCode type,
              JSString* name_,
              jsval size_,
              jsval align_,
              ffi_type* ffiType)
{
  RootedString name(cx, name_);
  RootedValue size(cx, size_);
  RootedValue align(cx, align_);
  RootedObject parent(cx, JS_GetParent(typeProto));
  JS_ASSERT(parent);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  RootedObject typeObj(cx, JS_NewObject(cx, &sCTypeClass, typeProto, parent));
  if (!typeObj)
    return nullptr;

  
  JS_SetReservedSlot(typeObj, SLOT_TYPECODE, INT_TO_JSVAL(type));
  if (ffiType)
    JS_SetReservedSlot(typeObj, SLOT_FFITYPE, PRIVATE_TO_JSVAL(ffiType));
  if (name)
    JS_SetReservedSlot(typeObj, SLOT_NAME, STRING_TO_JSVAL(name));
  JS_SetReservedSlot(typeObj, SLOT_SIZE, size);
  JS_SetReservedSlot(typeObj, SLOT_ALIGN, align);

  if (dataProto) {
    
    RootedObject prototype(cx, JS_NewObject(cx, &sCDataProtoClass, dataProto, parent));
    if (!prototype)
      return nullptr;

    if (!JS_DefineProperty(cx, prototype, "constructor", typeObj,
                           JSPROP_READONLY | JSPROP_PERMANENT))
      return nullptr;

    
    
    
    JS_SetReservedSlot(typeObj, SLOT_PROTO, OBJECT_TO_JSVAL(prototype));
  }

  if (!JS_FreezeObject(cx, typeObj))
    return nullptr;

  
  
  JS_ASSERT_IF(IsSizeDefined(typeObj),
               GetSize(typeObj) % GetAlignment(typeObj) == 0);

  return typeObj;
}

JSObject*
CType::DefineBuiltin(JSContext* cx,
                     JSObject* parent_,
                     const char* propName,
                     JSObject* typeProto_,
                     JSObject* dataProto_,
                     const char* name,
                     TypeCode type,
                     jsval size_,
                     jsval align_,
                     ffi_type* ffiType)
{
  RootedObject parent(cx, parent_);
  RootedObject typeProto(cx, typeProto_);
  RootedObject dataProto(cx, dataProto_);
  RootedValue size(cx, size_);
  RootedValue align(cx, align_);

  RootedString nameStr(cx, JS_NewStringCopyZ(cx, name));
  if (!nameStr)
    return nullptr;

  
  RootedObject typeObj(cx, Create(cx, typeProto, dataProto, type, nameStr, size, align, ffiType));
  if (!typeObj)
    return nullptr;

  
  if (!JS_DefineProperty(cx, parent, propName, typeObj,
                         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return nullptr;

  return typeObj;
}

void
CType::Finalize(JSFreeOp *fop, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_TYPECODE);
  if (slot.isUndefined())
    return;

  
  switch (TypeCode(slot.toInt32())) {
  case TYPE_function: {
    
    slot = JS_GetReservedSlot(obj, SLOT_FNINFO);
    if (!slot.isUndefined())
      FreeOp::get(fop)->delete_(static_cast<FunctionInfo*>(JSVAL_TO_PRIVATE(slot)));
    break;
  }

  case TYPE_struct: {
    
    slot = JS_GetReservedSlot(obj, SLOT_FIELDINFO);
    if (!slot.isUndefined()) {
      void* info = JSVAL_TO_PRIVATE(slot);
      FreeOp::get(fop)->delete_(static_cast<FieldInfoHash*>(info));
    }
  }

    
  case TYPE_array: {
    
    slot = JS_GetReservedSlot(obj, SLOT_FFITYPE);
    if (!slot.isUndefined()) {
      ffi_type* ffiType = static_cast<ffi_type*>(JSVAL_TO_PRIVATE(slot));
      FreeOp::get(fop)->free_(ffiType->elements);
      FreeOp::get(fop)->delete_(ffiType);
    }

    break;
  }
  default:
    
    break;
  }
}

void
CType::Trace(JSTracer* trc, JSObject* obj)
{
  
  jsval slot = obj->getSlot(SLOT_TYPECODE);
  if (slot.isUndefined())
    return;

  
  switch (TypeCode(slot.toInt32())) {
  case TYPE_struct: {
    slot = obj->getReservedSlot(SLOT_FIELDINFO);
    if (slot.isUndefined())
      return;

    FieldInfoHash* fields =
      static_cast<FieldInfoHash*>(JSVAL_TO_PRIVATE(slot));
    for (FieldInfoHash::Enum e(*fields); !e.empty(); e.popFront()) {
      JSString *key = e.front().key();
      JS_CallStringTracer(trc, &key, "fieldName");
      if (key != e.front().key())
          e.rekeyFront(JS_ASSERT_STRING_IS_FLAT(key));
      JS_CallHeapObjectTracer(trc, &e.front().value().mType, "fieldType");
    }

    break;
  }
  case TYPE_function: {
    
    slot = obj->getReservedSlot(SLOT_FNINFO);
    if (slot.isUndefined())
      return;

    FunctionInfo* fninfo = static_cast<FunctionInfo*>(JSVAL_TO_PRIVATE(slot));
    JS_ASSERT(fninfo);

    
    JS_CallHeapObjectTracer(trc, &fninfo->mABI, "abi");
    JS_CallHeapObjectTracer(trc, &fninfo->mReturnType, "returnType");
    for (size_t i = 0; i < fninfo->mArgTypes.length(); ++i)
      JS_CallHeapObjectTracer(trc, &fninfo->mArgTypes[i], "argType");

    break;
  }
  default:
    
    break;
  }
}

bool
CType::IsCType(JSObject* obj)
{
  return JS_GetClass(obj) == &sCTypeClass;
}

bool
CType::IsCTypeProto(JSObject* obj)
{
  return JS_GetClass(obj) == &sCTypeProtoClass;
}

TypeCode
CType::GetTypeCode(JSObject* typeObj)
{
  JS_ASSERT(IsCType(typeObj));

  jsval result = JS_GetReservedSlot(typeObj, SLOT_TYPECODE);
  return TypeCode(result.toInt32());
}

bool
CType::TypesEqual(JSObject* t1, JSObject* t2)
{
  JS_ASSERT(IsCType(t1) && IsCType(t2));

  
  if (t1 == t2)
    return true;

  
  TypeCode c1 = GetTypeCode(t1);
  TypeCode c2 = GetTypeCode(t2);
  if (c1 != c2)
    return false;

  
  switch (c1) {
  case TYPE_pointer: {
    
    JSObject* b1 = PointerType::GetBaseType(t1);
    JSObject* b2 = PointerType::GetBaseType(t2);
    return TypesEqual(b1, b2);
  }
  case TYPE_function: {
    FunctionInfo* f1 = FunctionType::GetFunctionInfo(t1);
    FunctionInfo* f2 = FunctionType::GetFunctionInfo(t2);

    
    if (f1->mABI != f2->mABI)
      return false;

    if (!TypesEqual(f1->mReturnType, f2->mReturnType))
      return false;

    if (f1->mArgTypes.length() != f2->mArgTypes.length())
      return false;

    if (f1->mIsVariadic != f2->mIsVariadic)
      return false;

    for (size_t i = 0; i < f1->mArgTypes.length(); ++i) {
      if (!TypesEqual(f1->mArgTypes[i], f2->mArgTypes[i]))
        return false;
    }

    return true;
  }
  case TYPE_array: {
    
    
    size_t s1 = 0, s2 = 0;
    bool d1 = ArrayType::GetSafeLength(t1, &s1);
    bool d2 = ArrayType::GetSafeLength(t2, &s2);
    if (d1 != d2 || (d1 && s1 != s2))
      return false;

    JSObject* b1 = ArrayType::GetBaseType(t1);
    JSObject* b2 = ArrayType::GetBaseType(t2);
    return TypesEqual(b1, b2);
  }
  case TYPE_struct:
    
    return false;
  default:
    
    return true;
  }
}

bool
CType::GetSafeSize(JSObject* obj, size_t* result)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval size = JS_GetReservedSlot(obj, SLOT_SIZE);

  
  
  if (size.isInt32()) {
    *result = size.toInt32();
    return true;
  }
  if (size.isDouble()) {
    *result = Convert<size_t>(size.toDouble());
    return true;
  }

  JS_ASSERT(size.isUndefined());
  return false;
}

size_t
CType::GetSize(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval size = JS_GetReservedSlot(obj, SLOT_SIZE);

  JS_ASSERT(!size.isUndefined());

  
  
  
  if (size.isInt32())
    return size.toInt32();
  return Convert<size_t>(size.toDouble());
}

bool
CType::IsSizeDefined(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval size = JS_GetReservedSlot(obj, SLOT_SIZE);

  
  
  JS_ASSERT(size.isInt32() || size.isDouble() || size.isUndefined());
  return !size.isUndefined();
}

size_t
CType::GetAlignment(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_ALIGN);
  return static_cast<size_t>(slot.toInt32());
}

ffi_type*
CType::GetFFIType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_FFITYPE);

  if (!slot.isUndefined()) {
    return static_cast<ffi_type*>(JSVAL_TO_PRIVATE(slot));
  }

  AutoPtr<ffi_type> result;
  switch (CType::GetTypeCode(obj)) {
  case TYPE_array:
    result = ArrayType::BuildFFIType(cx, obj);
    break;

  case TYPE_struct:
    result = StructType::BuildFFIType(cx, obj);
    break;

  default:
    MOZ_ASSUME_UNREACHABLE("simple types must have an ffi_type");
  }

  if (!result)
    return nullptr;
  JS_SetReservedSlot(obj, SLOT_FFITYPE, PRIVATE_TO_JSVAL(result.get()));
  return result.forget();
}

JSString*
CType::GetName(JSContext* cx, HandleObject obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval string = JS_GetReservedSlot(obj, SLOT_NAME);
  if (!string.isUndefined())
    return string.toString();

  
  JSString* name = BuildTypeName(cx, obj);
  if (!name)
    return nullptr;
  JS_SetReservedSlot(obj, SLOT_NAME, STRING_TO_JSVAL(name));
  return name;
}

JSObject*
CType::GetProtoFromCtor(JSObject* obj, CTypeProtoSlot slot)
{
  
  
  jsval protoslot = js::GetFunctionNativeReserved(obj, SLOT_FN_CTORPROTO);
  JSObject* proto = &protoslot.toObject();
  JS_ASSERT(proto);
  JS_ASSERT(CType::IsCTypeProto(proto));

  
  jsval result = JS_GetReservedSlot(proto, slot);
  return &result.toObject();
}

JSObject*
CType::GetProtoFromType(JSContext* cx, JSObject* objArg, CTypeProtoSlot slot)
{
  JS_ASSERT(IsCType(objArg));
  RootedObject obj(cx, objArg);

  
  RootedObject proto(cx);
  if (!JS_GetPrototype(cx, obj, &proto))
    return nullptr;
  JS_ASSERT(proto);
  JS_ASSERT(CType::IsCTypeProto(proto));

  
  jsval result = JS_GetReservedSlot(proto, slot);
  JS_ASSERT(result.isObject());
  return &result.toObject();
}

bool
CType::IsCTypeOrProto(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();
  return CType::IsCType(obj) || CType::IsCTypeProto(obj);
}

bool
CType::PrototypeGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  unsigned slot = CType::IsCTypeProto(obj) ? (unsigned) SLOT_OURDATAPROTO
                                           : (unsigned) SLOT_PROTO;
  args.rval().set(JS_GetReservedSlot(obj, slot));
  MOZ_ASSERT(args.rval().isObject() || args.rval().isUndefined());
  return true;
}

bool
CType::IsCType(HandleValue v)
{
  return v.isObject() && CType::IsCType(&v.toObject());
}

bool
CType::NameGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  JSString* name = CType::GetName(cx, obj);
  if (!name)
    return false;

  args.rval().setString(name);
  return true;
}

bool
CType::SizeGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  args.rval().set(JS_GetReservedSlot(obj, SLOT_SIZE));
  MOZ_ASSERT(args.rval().isNumber() || args.rval().isUndefined());
  return true;
}

bool
CType::PtrGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  JSObject* pointerType = PointerType::CreateInternal(cx, obj);
  if (!pointerType)
    return false;

  args.rval().setObject(*pointerType);
  return true;
}

bool
CType::CreateArray(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject baseType(cx, JS_THIS_OBJECT(cx, vp));
  if (!baseType)
    return false;
  if (!CType::IsCType(baseType)) {
    JS_ReportError(cx, "not a CType");
    return false;
  }

  
  if (args.length() > 1) {
    JS_ReportError(cx, "array takes zero or one argument");
    return false;
  }

  
  size_t length = 0;
  if (args.length() == 1 && !jsvalToSize(cx, args[0], false, &length)) {
    JS_ReportError(cx, "argument must be a nonnegative integer");
    return false;
  }

  JSObject* result = ArrayType::CreateInternal(cx, baseType, length, args.length() == 1);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

bool
CType::ToString(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!CType::IsCType(obj) && !CType::IsCTypeProto(obj)) {
    JS_ReportError(cx, "not a CType");
    return false;
  }

  
  
  JSString* result;
  if (CType::IsCType(obj)) {
    AutoString type;
    AppendString(type, "type ");
    AppendString(type, GetName(cx, obj));
    result = NewUCString(cx, type);
  }
  else {
    result = JS_NewStringCopyZ(cx, "[CType proto object]");
  }
  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

bool
CType::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!CType::IsCType(obj) && !CType::IsCTypeProto(obj))
  {
    JS_ReportError(cx, "not a CType");
    return false;
  }

  
  
  JSString* result;
  if (CType::IsCType(obj)) {
    AutoString source;
    BuildTypeSource(cx, obj, false, source);
    result = NewUCString(cx, source);
  } else {
    result = JS_NewStringCopyZ(cx, "[CType proto object]");
  }
  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

bool
CType::HasInstance(JSContext* cx, HandleObject obj, MutableHandleValue v, bool* bp)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_PROTO);
  JS::Rooted<JSObject*> prototype(cx, &slot.toObject());
  JS_ASSERT(prototype);
  JS_ASSERT(CData::IsCDataProto(prototype));

  *bp = false;
  if (v.isPrimitive())
    return true;

  RootedObject proto(cx, &v.toObject());
  for (;;) {
    if (!JS_GetPrototype(cx, proto, &proto))
      return false;
    if (!proto)
      break;
    if (proto == prototype) {
      *bp = true;
      break;
    }
  }
  return true;
}

static JSObject*
CType::GetGlobalCTypes(JSContext* cx, JSObject* objArg)
{
  JS_ASSERT(CType::IsCType(objArg));

  RootedObject obj(cx, objArg);
  RootedObject objTypeProto(cx);
  if (!JS_GetPrototype(cx, obj, &objTypeProto))
    return nullptr;
  JS_ASSERT(objTypeProto);
  JS_ASSERT(CType::IsCTypeProto(objTypeProto));

  jsval valCTypes = JS_GetReservedSlot(objTypeProto, SLOT_CTYPES);
  JS_ASSERT(valCTypes.isObject());
  return &valCTypes.toObject();
}





bool
ABI::IsABI(JSObject* obj)
{
  return JS_GetClass(obj) == &sCABIClass;
}

bool
ABI::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return false;
  }

  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!ABI::IsABI(obj)) {
    JS_ReportError(cx, "not an ABI");
    return false;
  }

  JSString* result;
  switch (GetABICode(obj)) {
    case ABI_DEFAULT:
      result = JS_NewStringCopyZ(cx, "ctypes.default_abi");
      break;
    case ABI_STDCALL:
      result = JS_NewStringCopyZ(cx, "ctypes.stdcall_abi");
      break;
    case ABI_WINAPI:
      result = JS_NewStringCopyZ(cx, "ctypes.winapi_abi");
      break;
    default:
      JS_ReportError(cx, "not a valid ABICode");
      return false;
  }
  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}






bool
PointerType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  if (args.length() != 1) {
    JS_ReportError(cx, "PointerType takes one argument");
    return false;
  }

  jsval arg = args[0];
  RootedObject obj(cx);
  if (arg.isPrimitive() || !CType::IsCType(obj = &arg.toObject())) {
    JS_ReportError(cx, "first argument must be a CType");
    return false;
  }

  JSObject* result = CreateInternal(cx, obj);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

JSObject*
PointerType::CreateInternal(JSContext* cx, HandleObject baseType)
{
  
  jsval slot = JS_GetReservedSlot(baseType, SLOT_PTR);
  if (!slot.isUndefined())
    return &slot.toObject();

  
  
  CTypeProtoSlot slotId = CType::GetTypeCode(baseType) == TYPE_function ?
    SLOT_FUNCTIONDATAPROTO : SLOT_POINTERDATAPROTO;
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, baseType, slotId));
  if (!dataProto)
    return nullptr;
  RootedObject typeProto(cx, CType::GetProtoFromType(cx, baseType, SLOT_POINTERPROTO));
  if (!typeProto)
    return nullptr;

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_pointer,
                        nullptr, INT_TO_JSVAL(sizeof(void*)),
                        INT_TO_JSVAL(ffi_type_pointer.alignment),
                        &ffi_type_pointer);
  if (!typeObj)
    return nullptr;

  
  JS_SetReservedSlot(typeObj, SLOT_TARGET_T, OBJECT_TO_JSVAL(baseType));

  
  JS_SetReservedSlot(baseType, SLOT_PTR, OBJECT_TO_JSVAL(typeObj));

  return typeObj;
}

bool
PointerType::ConstructData(JSContext* cx,
                           HandleObject obj,
                           const CallArgs& args)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return false;
  }

  if (args.length() > 3) {
    JS_ReportError(cx, "constructor takes 0, 1, 2, or 3 arguments");
    return false;
  }

  RootedObject result(cx, CData::Create(cx, obj, NullPtr(), nullptr, true));
  if (!result)
    return false;

  
  args.rval().setObject(*result);

  
  
  
  
  
  
  

  
  
  
  if (args.length() == 0)
    return true;

  
  RootedObject baseObj(cx, PointerType::GetBaseType(obj));
  bool looksLikeClosure = CType::GetTypeCode(baseObj) == TYPE_function &&
                          args[0].isObject() &&
                          JS_ObjectIsCallable(cx, &args[0].toObject());

  
  
  
  if (!looksLikeClosure) {
    if (args.length() != 1) {
      JS_ReportError(cx, "first argument must be a function");
      return false;
    }
    return ExplicitConvert(cx, args[0], obj, CData::GetData(result));
  }

  
  
  

  
  
  
  RootedObject thisObj(cx, nullptr);
  if (args.length() >= 2) {
    if (args[1].isNull()) {
      thisObj = nullptr;
    } else if (args[1].isObject()) {
      thisObj = &args[1].toObject();
    } else if (!JS_ValueToObject(cx, args[1], &thisObj)) {
      return false;
    }
  }

  
  
  
  jsval errVal = JSVAL_VOID;
  if (args.length() == 3)
    errVal = args[2];

  RootedObject fnObj(cx, &args[0].toObject());
  return FunctionType::ConstructData(cx, baseObj, result, fnObj, thisObj, errVal);
}

JSObject*
PointerType::GetBaseType(JSObject* obj)
{
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_pointer);

  jsval type = JS_GetReservedSlot(obj, SLOT_TARGET_T);
  JS_ASSERT(!type.isNull());
  return &type.toObject();
}

bool
PointerType::IsPointerType(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();
  return CType::IsCType(obj) && CType::GetTypeCode(obj) == TYPE_pointer;
}

bool
PointerType::IsPointer(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();
  return CData::IsCData(obj) && CType::GetTypeCode(CData::GetCType(obj)) == TYPE_pointer;
}

bool
PointerType::TargetTypeGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  args.rval().set(JS_GetReservedSlot(obj, SLOT_TARGET_T));
  MOZ_ASSERT(args.rval().isObject());
  return true;
}

bool
PointerType::IsNull(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return false;
  }

  void* data = *static_cast<void**>(CData::GetData(obj));
  args.rval().setBoolean(data == nullptr);
  return true;
}

bool
PointerType::OffsetBy(JSContext* cx, const CallArgs& args, int offset)
{
  JSObject* obj = JS_THIS_OBJECT(cx, args.base());
  if (!obj)
    return false;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return false;
  }

  RootedObject baseType(cx, PointerType::GetBaseType(typeObj));
  if (!CType::IsSizeDefined(baseType)) {
    JS_ReportError(cx, "cannot modify pointer of undefined size");
    return false;
  }

  size_t elementSize = CType::GetSize(baseType);
  char* data = static_cast<char*>(*static_cast<void**>(CData::GetData(obj)));
  void* address = data + offset * elementSize;

  
  JSObject* result = CData::Create(cx, typeObj, NullPtr(), &address, true);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

bool
PointerType::Increment(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  return OffsetBy(cx, args, 1);
}

bool
PointerType::Decrement(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  return OffsetBy(cx, args, -1);
}

bool
PointerType::ContentsGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  RootedObject baseType(cx, GetBaseType(CData::GetCType(obj)));
  if (!CType::IsSizeDefined(baseType)) {
    JS_ReportError(cx, "cannot get contents of undefined size");
    return false;
  }

  void* data = *static_cast<void**>(CData::GetData(obj));
  if (data == nullptr) {
    JS_ReportError(cx, "cannot read contents of null pointer");
    return false;
  }

  RootedValue result(cx);
  if (!ConvertToJS(cx, baseType, NullPtr(), data, false, false, result.address()))
    return false;

  args.rval().set(result);
  return true;
}

bool
PointerType::ContentsSetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  RootedObject baseType(cx, GetBaseType(CData::GetCType(obj)));
  if (!CType::IsSizeDefined(baseType)) {
    JS_ReportError(cx, "cannot set contents of undefined size");
    return false;
  }

  void* data = *static_cast<void**>(CData::GetData(obj));
  if (data == nullptr) {
    JS_ReportError(cx, "cannot write contents to null pointer");
    return false;
  }

  args.rval().setUndefined();
  return ImplicitConvert(cx, args.get(0), baseType, data, false, nullptr);
}





bool
ArrayType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  if (args.length() < 1 || args.length() > 2) {
    JS_ReportError(cx, "ArrayType takes one or two arguments");
    return false;
  }

  if (args[0].isPrimitive() ||
      !CType::IsCType(&args[0].toObject())) {
    JS_ReportError(cx, "first argument must be a CType");
    return false;
  }

  
  size_t length = 0;
  if (args.length() == 2 && !jsvalToSize(cx, args[1], false, &length)) {
    JS_ReportError(cx, "second argument must be a nonnegative integer");
    return false;
  }

  RootedObject baseType(cx, &args[0].toObject());
  JSObject* result = CreateInternal(cx, baseType, length, args.length() == 2);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

JSObject*
ArrayType::CreateInternal(JSContext* cx,
                          HandleObject baseType,
                          size_t length,
                          bool lengthDefined)
{
  
  
  RootedObject typeProto(cx, CType::GetProtoFromType(cx, baseType, SLOT_ARRAYPROTO));
  if (!typeProto)
    return nullptr;
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, baseType, SLOT_ARRAYDATAPROTO));
  if (!dataProto)
    return nullptr;

  
  
  
  size_t baseSize;
  if (!CType::GetSafeSize(baseType, &baseSize)) {
    JS_ReportError(cx, "base size must be defined");
    return nullptr;
  }

  RootedValue sizeVal(cx, JSVAL_VOID);
  RootedValue lengthVal(cx, JSVAL_VOID);
  if (lengthDefined) {
    
    size_t size = length * baseSize;
    if (length > 0 && size / length != baseSize) {
      JS_ReportError(cx, "size overflow");
      return nullptr;
    }
    if (!SizeTojsval(cx, size, sizeVal.address()) ||
        !SizeTojsval(cx, length, lengthVal.address()))
      return nullptr;
  }

  size_t align = CType::GetAlignment(baseType);

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_array, nullptr,
                        sizeVal, INT_TO_JSVAL(align), nullptr);
  if (!typeObj)
    return nullptr;

  
  JS_SetReservedSlot(typeObj, SLOT_ELEMENT_T, OBJECT_TO_JSVAL(baseType));

  
  JS_SetReservedSlot(typeObj, SLOT_LENGTH, lengthVal);

  return typeObj;
}

bool
ArrayType::ConstructData(JSContext* cx,
                         HandleObject obj_,
                         const CallArgs& args)
{
  RootedObject obj(cx, obj_); 

  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return false;
  }

  
  
  bool convertObject = args.length() == 1;

  
  
  if (CType::IsSizeDefined(obj)) {
    if (args.length() > 1) {
      JS_ReportError(cx, "constructor takes zero or one argument");
      return false;
    }

  } else {
    if (args.length() != 1) {
      JS_ReportError(cx, "constructor takes one argument");
      return false;
    }

    RootedObject baseType(cx, GetBaseType(obj));

    size_t length;
    if (jsvalToSize(cx, args[0], false, &length)) {
      
      convertObject = false;

    } else if (args[0].isObject()) {
      
      
      RootedObject arg(cx, &args[0].toObject());
      RootedValue lengthVal(cx);
      if (!JS_GetProperty(cx, arg, "length", &lengthVal) ||
          !jsvalToSize(cx, lengthVal, false, &length)) {
        JS_ReportError(cx, "argument must be an array object or length");
        return false;
      }

    } else if (args[0].isString()) {
      
      
      JSString* sourceString = args[0].toString();
      size_t sourceLength = sourceString->length();
      const jschar* sourceChars = sourceString->getChars(cx);
      if (!sourceChars)
        return false;

      switch (CType::GetTypeCode(baseType)) {
      case TYPE_char:
      case TYPE_signed_char:
      case TYPE_unsigned_char: {
        
        length = GetDeflatedUTF8StringLength(cx, sourceChars, sourceLength);
        if (length == (size_t) -1)
          return false;

        ++length;
        break;
      }
      case TYPE_jschar:
        length = sourceLength + 1;
        break;
      default:
        return TypeError(cx, "array", args[0]);
      }

    } else {
      JS_ReportError(cx, "argument must be an array object or length");
      return false;
    }

    
    obj = CreateInternal(cx, baseType, length, true);
    if (!obj)
      return false;
  }

  JSObject* result = CData::Create(cx, obj, NullPtr(), nullptr, true);
  if (!result)
    return false;

  args.rval().setObject(*result);

  if (convertObject) {
    if (!ExplicitConvert(cx, args[0], obj, CData::GetData(result)))
      return false;
  }

  return true;
}

JSObject*
ArrayType::GetBaseType(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_array);

  jsval type = JS_GetReservedSlot(obj, SLOT_ELEMENT_T);
  JS_ASSERT(!type.isNull());
  return &type.toObject();
}

bool
ArrayType::GetSafeLength(JSObject* obj, size_t* result)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_array);

  jsval length = JS_GetReservedSlot(obj, SLOT_LENGTH);

  
  
  if (length.isInt32()) {
    *result = length.toInt32();;
    return true;
  }
  if (length.isDouble()) {
    *result = Convert<size_t>(length.toDouble());
    return true;
  }

  JS_ASSERT(length.isUndefined());
  return false;
}

size_t
ArrayType::GetLength(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_array);

  jsval length = JS_GetReservedSlot(obj, SLOT_LENGTH);

  JS_ASSERT(!length.isUndefined());

  
  
  
  if (length.isInt32())
    return length.toInt32();;
  return Convert<size_t>(length.toDouble());
}

ffi_type*
ArrayType::BuildFFIType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_array);
  JS_ASSERT(CType::IsSizeDefined(obj));

  JSObject* baseType = ArrayType::GetBaseType(obj);
  ffi_type* ffiBaseType = CType::GetFFIType(cx, baseType);
  if (!ffiBaseType)
    return nullptr;

  size_t length = ArrayType::GetLength(obj);

  
  
  
  
  
  
  
  AutoPtr<ffi_type> ffiType(cx->new_<ffi_type>());
  if (!ffiType) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }

  ffiType->type = FFI_TYPE_STRUCT;
  ffiType->size = CType::GetSize(obj);
  ffiType->alignment = CType::GetAlignment(obj);
  ffiType->elements = cx->pod_malloc<ffi_type*>(length + 1);
  if (!ffiType->elements) {
    JS_ReportAllocationOverflow(cx);
    return nullptr;
  }

  for (size_t i = 0; i < length; ++i)
    ffiType->elements[i] = ffiBaseType;
  ffiType->elements[length] = nullptr;

  return ffiType.forget();
}

bool
ArrayType::IsArrayType(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();
  return CType::IsCType(obj) && CType::GetTypeCode(obj) == TYPE_array;
}

bool
ArrayType::IsArrayOrArrayType(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();

   
   
  if (CData::IsCData(obj)) {
    obj = CData::GetCType(obj);
  }
  return CType::IsCType(obj) && CType::GetTypeCode(obj) == TYPE_array;
}

bool
ArrayType::ElementTypeGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  args.rval().set(JS_GetReservedSlot(obj, SLOT_ELEMENT_T));
  MOZ_ASSERT(args.rval().isObject());
  return true;
}

bool
ArrayType::LengthGetter(JSContext* cx, JS::CallArgs args)
{
  JSObject *obj = &args.thisv().toObject();

  
  
  if (CData::IsCData(obj))
    obj = CData::GetCType(obj);

  args.rval().set(JS_GetReservedSlot(obj, SLOT_LENGTH));
  JS_ASSERT(args.rval().isNumber() || args.rval().isUndefined());
  return true;
}

bool
ArrayType::Getter(JSContext* cx, HandleObject obj, HandleId idval, MutableHandleValue vp)
{
  
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  
  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_array)
    return true;

  
  size_t index;
  size_t length = GetLength(typeObj);
  bool ok = jsidToSize(cx, idval, true, &index);
  int32_t dummy;
  if (!ok && JSID_IS_STRING(idval) && !StringToInteger(cx, JSID_TO_STRING(idval), &dummy)) {
    
    
    return true;
  }
  if (!ok || index >= length) {
    JS_ReportError(cx, "invalid index");
    return false;
  }

  RootedObject baseType(cx, GetBaseType(typeObj));
  size_t elementSize = CType::GetSize(baseType);
  char* data = static_cast<char*>(CData::GetData(obj)) + elementSize * index;
  return ConvertToJS(cx, baseType, obj, data, false, false, vp.address());
}

bool
ArrayType::Setter(JSContext* cx, HandleObject obj, HandleId idval, bool strict, MutableHandleValue vp)
{
  
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  
  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_array)
    return true;

  
  size_t index;
  size_t length = GetLength(typeObj);
  bool ok = jsidToSize(cx, idval, true, &index);
  int32_t dummy;
  if (!ok && JSID_IS_STRING(idval) && !StringToInteger(cx, JSID_TO_STRING(idval), &dummy)) {
    
    
    return true;
  }
  if (!ok || index >= length) {
    JS_ReportError(cx, "invalid index");
    return false;
  }

  JSObject* baseType = GetBaseType(typeObj);
  size_t elementSize = CType::GetSize(baseType);
  char* data = static_cast<char*>(CData::GetData(obj)) + elementSize * index;
  return ImplicitConvert(cx, vp, baseType, data, false, nullptr);
}

bool
ArrayType::AddressOfElement(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  if (CType::GetTypeCode(typeObj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return false;
  }

  if (args.length() != 1) {
    JS_ReportError(cx, "addressOfElement takes one argument");
    return false;
  }

  RootedObject baseType(cx, GetBaseType(typeObj));
  RootedObject pointerType(cx, PointerType::CreateInternal(cx, baseType));
  if (!pointerType)
    return false;

  
  RootedObject result(cx, CData::Create(cx, pointerType, NullPtr(), nullptr, true));
  if (!result)
    return false;

  args.rval().setObject(*result);

  
  size_t index;
  size_t length = GetLength(typeObj);
  if (!jsvalToSize(cx, args[0], false, &index) ||
      index >= length) {
    JS_ReportError(cx, "invalid index");
    return false;
  }

  
  void** data = static_cast<void**>(CData::GetData(result));
  size_t elementSize = CType::GetSize(baseType);
  *data = static_cast<char*>(CData::GetData(obj)) + elementSize * index;
  return true;
}







static JSFlatString*
ExtractStructField(JSContext* cx, jsval val, JSObject** typeObj)
{
  if (val.isPrimitive()) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return nullptr;
  }

  RootedObject obj(cx, val.toObjectOrNull());
  RootedObject iter(cx, JS_NewPropertyIterator(cx, obj));
  if (!iter)
    return nullptr;

  RootedId nameid(cx);
  if (!JS_NextProperty(cx, iter, nameid.address()))
    return nullptr;
  if (JSID_IS_VOID(nameid)) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return nullptr;
  }

  if (!JSID_IS_STRING(nameid)) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return nullptr;
  }

  
  jsid id;
  if (!JS_NextProperty(cx, iter, &id))
    return nullptr;
  if (!JSID_IS_VOID(id)) {
    JS_ReportError(cx, "struct field descriptors must contain one property");
    return nullptr;
  }

  RootedValue propVal(cx);
  if (!JS_GetPropertyById(cx, obj, nameid, &propVal))
    return nullptr;

  if (propVal.isPrimitive() || !CType::IsCType(&propVal.toObject())) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return nullptr;
  }

  
  
  
  *typeObj = &propVal.toObject();
  size_t size;
  if (!CType::GetSafeSize(*typeObj, &size) || size == 0) {
    JS_ReportError(cx, "struct field types must have defined and nonzero size");
    return nullptr;
  }

  return JSID_TO_FLAT_STRING(nameid);
}



static bool
AddFieldToArray(JSContext* cx,
                jsval* element,
                JSFlatString* name_,
                JSObject* typeObj_)
{
  RootedObject typeObj(cx, typeObj_);
  Rooted<JSFlatString*> name(cx, name_);
  RootedObject fieldObj(cx, JS_NewObject(cx, nullptr, NullPtr(), NullPtr()));
  if (!fieldObj)
    return false;

  *element = OBJECT_TO_JSVAL(fieldObj);

  if (!JS_DefineUCProperty(cx, fieldObj,
         name->chars(), name->length(),
         OBJECT_TO_JSVAL(typeObj), nullptr, nullptr,
         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return JS_FreezeObject(cx, fieldObj);
}

bool
StructType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);

  
  if (args.length() < 1 || args.length() > 2) {
    JS_ReportError(cx, "StructType takes one or two arguments");
    return false;
  }

  jsval name = args[0];
  if (!name.isString()) {
    JS_ReportError(cx, "first argument must be a string");
    return false;
  }

  
  RootedObject typeProto(cx, CType::GetProtoFromCtor(&args.callee(), SLOT_STRUCTPROTO));

  
  
  
  RootedObject result(cx, CType::Create(cx, typeProto, NullPtr(), TYPE_struct,
                                        name.toString(), JSVAL_VOID, JSVAL_VOID, nullptr));
  if (!result)
    return false;

  if (args.length() == 2) {
    RootedObject arr(cx, args[1].isPrimitive() ? nullptr : &args[1].toObject());
    if (!arr || !JS_IsArrayObject(cx, arr)) {
      JS_ReportError(cx, "second argument must be an array");
      return false;
    }

    
    if (!DefineInternal(cx, result, arr))
      return false;
  }

  args.rval().setObject(*result);
  return true;
}

static void
PostBarrierCallback(JSTracer *trc, JSString *key, void *data)
{
    typedef HashMap<JSFlatString*,
                    UnbarrieredFieldInfo,
                    FieldHashPolicy,
                    SystemAllocPolicy> UnbarrieredFieldInfoHash;

    UnbarrieredFieldInfoHash *table = reinterpret_cast<UnbarrieredFieldInfoHash*>(data);
    JSString *prior = key;
    JS_CallStringTracer(trc, &key, "CType fieldName");
    table->rekeyIfMoved(JS_ASSERT_STRING_IS_FLAT(prior), JS_ASSERT_STRING_IS_FLAT(key));
}

bool
StructType::DefineInternal(JSContext* cx, JSObject* typeObj_, JSObject* fieldsObj_)
{
  RootedObject typeObj(cx, typeObj_);
  RootedObject fieldsObj(cx, fieldsObj_);

  uint32_t len;
  ASSERT_OK(JS_GetArrayLength(cx, fieldsObj, &len));

  
  
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, typeObj, SLOT_STRUCTDATAPROTO));
  if (!dataProto)
    return false;

  
  
  
  RootedObject prototype(cx, JS_NewObject(cx, &sCDataProtoClass, dataProto, NullPtr()));
  if (!prototype)
    return false;

  if (!JS_DefineProperty(cx, prototype, "constructor", typeObj,
                         JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  
  
  
  
  AutoPtr<FieldInfoHash> fields(cx->new_<FieldInfoHash>());
  if (!fields || !fields->init(len)) {
    JS_ReportOutOfMemory(cx);
    return false;
  }
  JS::AutoValueVector fieldRoots(cx);
  if (!fieldRoots.resize(len)) {
    JS_ReportOutOfMemory(cx);
    return false;
  }

  
  size_t structSize, structAlign;
  if (len != 0) {
    structSize = 0;
    structAlign = 0;

    for (uint32_t i = 0; i < len; ++i) {
      RootedValue item(cx);
      if (!JS_GetElement(cx, fieldsObj, i, &item))
        return false;

      RootedObject fieldType(cx, nullptr);
      Rooted<JSFlatString*> name(cx, ExtractStructField(cx, item, fieldType.address()));
      if (!name)
        return false;
      fieldRoots[i].setObject(*fieldType);

      
      FieldInfoHash::AddPtr entryPtr = fields->lookupForAdd(name);
      if (entryPtr) {
        JS_ReportError(cx, "struct fields must have unique names");
        return false;
      }

      
      if (!JS_DefineUCProperty(cx, prototype,
             name->chars(), name->length(), JSVAL_VOID,
             StructType::FieldGetter, StructType::FieldSetter,
             JSPROP_SHARED | JSPROP_ENUMERATE | JSPROP_PERMANENT))
        return false;

      size_t fieldSize = CType::GetSize(fieldType);
      size_t fieldAlign = CType::GetAlignment(fieldType);
      size_t fieldOffset = Align(structSize, fieldAlign);
      
      
      
      if (fieldOffset + fieldSize < structSize) {
        JS_ReportError(cx, "size overflow");
        return false;
      }

      
      FieldInfo info;
      info.mType = fieldType;
      info.mIndex = i;
      info.mOffset = fieldOffset;
      ASSERT_OK(fields->add(entryPtr, name, info));
      JS_StoreStringPostBarrierCallback(cx, PostBarrierCallback, name, fields.get());

      structSize = fieldOffset + fieldSize;

      if (fieldAlign > structAlign)
        structAlign = fieldAlign;
    }

    
    size_t structTail = Align(structSize, structAlign);
    if (structTail < structSize) {
      JS_ReportError(cx, "size overflow");
      return false;
    }
    structSize = structTail;

  } else {
    
    
    
    
    structSize = 1;
    structAlign = 1;
  }

  RootedValue sizeVal(cx);
  if (!SizeTojsval(cx, structSize, sizeVal.address()))
    return false;

  JS_SetReservedSlot(typeObj, SLOT_FIELDINFO, PRIVATE_TO_JSVAL(fields.forget()));

  JS_SetReservedSlot(typeObj, SLOT_SIZE, sizeVal);
  JS_SetReservedSlot(typeObj, SLOT_ALIGN, INT_TO_JSVAL(structAlign));
  
  
  JS_SetReservedSlot(typeObj, SLOT_PROTO, OBJECT_TO_JSVAL(prototype));
  return true;
}

ffi_type*
StructType::BuildFFIType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);
  JS_ASSERT(CType::IsSizeDefined(obj));

  const FieldInfoHash* fields = GetFieldInfo(obj);
  size_t len = fields->count();

  size_t structSize = CType::GetSize(obj);
  size_t structAlign = CType::GetAlignment(obj);

  AutoPtr<ffi_type> ffiType(cx->new_<ffi_type>());
  if (!ffiType) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }
  ffiType->type = FFI_TYPE_STRUCT;

  AutoPtr<ffi_type*> elements;
  if (len != 0) {
    elements = cx->pod_malloc<ffi_type*>(len + 1);
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return nullptr;
    }
    elements[len] = nullptr;

    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
      const FieldInfoHash::Entry& entry = r.front();
      ffi_type* fieldType = CType::GetFFIType(cx, entry.value().mType);
      if (!fieldType)
        return nullptr;
      elements[entry.value().mIndex] = fieldType;
    }

  } else {
    
    JS_ASSERT(structSize == 1);
    JS_ASSERT(structAlign == 1);
    elements = cx->pod_malloc<ffi_type*>(2);
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return nullptr;
    }
    elements[0] = &ffi_type_uint8;
    elements[1] = nullptr;
  }

  ffiType->elements = elements.get();

#ifdef DEBUG
  
  
  
  ffi_cif cif;
  ffiType->size = 0;
  ffiType->alignment = 0;
  ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, ffiType.get(), nullptr);
  JS_ASSERT(status == FFI_OK);
  JS_ASSERT(structSize == ffiType->size);
  JS_ASSERT(structAlign == ffiType->alignment);
#else
  
  
  
  
  ffiType->size = structSize;
  ffiType->alignment = structAlign;
#endif

  elements.forget();
  return ffiType.forget();
}

bool
StructType::Define(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!CType::IsCType(obj) ||
      CType::GetTypeCode(obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return false;
  }

  if (CType::IsSizeDefined(obj)) {
    JS_ReportError(cx, "StructType has already been defined");
    return false;
  }

  if (args.length() != 1) {
    JS_ReportError(cx, "define takes one argument");
    return false;
  }

  jsval arg = args[0];
  if (arg.isPrimitive()) {
    JS_ReportError(cx, "argument must be an array");
    return false;
  }
  RootedObject arr(cx, arg.toObjectOrNull());
  if (!JS_IsArrayObject(cx, arr)) {
    JS_ReportError(cx, "argument must be an array");
    return false;
  }

  return DefineInternal(cx, obj, arr);
}

bool
StructType::ConstructData(JSContext* cx,
                          HandleObject obj,
                          const CallArgs& args)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return false;
  }

  if (!CType::IsSizeDefined(obj)) {
    JS_ReportError(cx, "cannot construct an opaque StructType");
    return false;
  }

  JSObject* result = CData::Create(cx, obj, NullPtr(), nullptr, true);
  if (!result)
    return false;

  args.rval().setObject(*result);

  if (args.length() == 0)
    return true;

  char* buffer = static_cast<char*>(CData::GetData(result));
  const FieldInfoHash* fields = GetFieldInfo(obj);

  if (args.length() == 1) {
    
    
    
    
    
    
    

    
    if (ExplicitConvert(cx, args[0], obj, buffer))
      return true;

    if (fields->count() != 1)
      return false;

    
    
    if (!JS_IsExceptionPending(cx))
      return false;

    
    
    JS_ClearPendingException(cx);

    
  }

  
  
  if (args.length() == fields->count()) {
    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
      const FieldInfo& field = r.front().value();
      STATIC_ASSUME(field.mIndex < fields->count());  
      if (!ImplicitConvert(cx, args[field.mIndex], field.mType,
             buffer + field.mOffset,
             false, nullptr))
        return false;
    }

    return true;
  }

  JS_ReportError(cx, "constructor takes 0, 1, or %u arguments",
    fields->count());
  return false;
}

const FieldInfoHash*
StructType::GetFieldInfo(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);

  jsval slot = JS_GetReservedSlot(obj, SLOT_FIELDINFO);
  JS_ASSERT(!slot.isUndefined() && JSVAL_TO_PRIVATE(slot));

  return static_cast<const FieldInfoHash*>(JSVAL_TO_PRIVATE(slot));
}

const FieldInfo*
StructType::LookupField(JSContext* cx, JSObject* obj, JSFlatString *name)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);

  FieldInfoHash::Ptr ptr = GetFieldInfo(obj)->lookup(name);
  if (ptr)
    return &ptr->value();

  JSAutoByteString bytes(cx, name);
  if (!bytes)
    return nullptr;

  JS_ReportError(cx, "%s does not name a field", bytes.ptr());
  return nullptr;
}

JSObject*
StructType::BuildFieldsArray(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);
  JS_ASSERT(CType::IsSizeDefined(obj));

  const FieldInfoHash* fields = GetFieldInfo(obj);
  size_t len = fields->count();

  
  JS::AutoValueVector fieldsVec(cx);
  if (!fieldsVec.resize(len))
    return nullptr;

  for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
    const FieldInfoHash::Entry& entry = r.front();
    
    if (!AddFieldToArray(cx, fieldsVec[entry.value().mIndex].address(),
                         entry.key(), entry.value().mType))
      return nullptr;
  }

  RootedObject fieldsProp(cx, JS_NewArrayObject(cx, fieldsVec));
  if (!fieldsProp)
    return nullptr;

  
  if (!JS_FreezeObject(cx, fieldsProp))
    return nullptr;

  return fieldsProp;
}

 bool
StructType::IsStruct(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();
  return CType::IsCType(obj) && CType::GetTypeCode(obj) == TYPE_struct;
}

bool
StructType::FieldsArrayGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());

  args.rval().set(JS_GetReservedSlot(obj, SLOT_FIELDS));

  if (!CType::IsSizeDefined(obj)) {
    MOZ_ASSERT(args.rval().isUndefined());
    return true;
  }

  if (args.rval().isUndefined()) {
    
    JSObject* fields = BuildFieldsArray(cx, obj);
    if (!fields)
      return false;
    JS_SetReservedSlot(obj, SLOT_FIELDS, OBJECT_TO_JSVAL(fields));

    args.rval().setObject(*fields);
  }

  MOZ_ASSERT(args.rval().isObject());
  MOZ_ASSERT(JS_IsArrayObject(cx, args.rval()));
  return true;
}

bool
StructType::FieldGetter(JSContext* cx, HandleObject obj, HandleId idval, MutableHandleValue vp)
{
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return false;
  }

  const FieldInfo* field = LookupField(cx, typeObj, JSID_TO_FLAT_STRING(idval));
  if (!field)
    return false;

  char* data = static_cast<char*>(CData::GetData(obj)) + field->mOffset;
  RootedObject fieldType(cx, field->mType);
  return ConvertToJS(cx, fieldType, obj, data, false, false, vp.address());
}

bool
StructType::FieldSetter(JSContext* cx, HandleObject obj, HandleId idval, bool strict, MutableHandleValue vp)
{
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return false;
  }

  const FieldInfo* field = LookupField(cx, typeObj, JSID_TO_FLAT_STRING(idval));
  if (!field)
    return false;

  char* data = static_cast<char*>(CData::GetData(obj)) + field->mOffset;
  return ImplicitConvert(cx, vp, field->mType, data, false, nullptr);
}

bool
StructType::AddressOfField(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return false;
  }

  if (args.length() != 1) {
    JS_ReportError(cx, "addressOfField takes one argument");
    return false;
  }

  JSFlatString *str = JS_FlattenString(cx, args[0].toString());
  if (!str)
    return false;

  const FieldInfo* field = LookupField(cx, typeObj, str);
  if (!field)
    return false;

  RootedObject baseType(cx, field->mType);
  RootedObject pointerType(cx, PointerType::CreateInternal(cx, baseType));
  if (!pointerType)
    return false;

  
  JSObject* result = CData::Create(cx, pointerType, NullPtr(), nullptr, true);
  if (!result)
    return false;

  args.rval().setObject(*result);

  
  void** data = static_cast<void**>(CData::GetData(result));
  *data = static_cast<char*>(CData::GetData(obj)) + field->mOffset;
  return true;
}






struct AutoValue
{
  AutoValue() : mData(nullptr) { }

  ~AutoValue()
  {
    js_free(mData);
  }

  bool SizeToType(JSContext* cx, JSObject* type)
  {
    
    size_t size = Align(CType::GetSize(type), sizeof(ffi_arg));
    mData = js_malloc(size);
    if (mData)
      memset(mData, 0, size);
    return mData != nullptr;
  }

  void* mData;
};

static bool
GetABI(JSContext* cx, jsval abiType, ffi_abi* result)
{
  if (abiType.isPrimitive())
    return false;

  ABICode abi = GetABICode(abiType.toObjectOrNull());

  
  
  
  switch (abi) {
  case ABI_DEFAULT:
    *result = FFI_DEFAULT_ABI;
    return true;
  case ABI_STDCALL:
  case ABI_WINAPI:
#if (defined(_WIN32) && !defined(_WIN64)) || defined(_OS2)
    *result = FFI_STDCALL;
    return true;
#elif (defined(_WIN64))
    
    
    *result = FFI_WIN64;
    return true;
#endif
  case INVALID_ABI:
    break;
  }
  return false;
}

static JSObject*
PrepareType(JSContext* cx, jsval type)
{
  if (type.isPrimitive() || !CType::IsCType(type.toObjectOrNull())) {
    JS_ReportError(cx, "not a ctypes type");
    return nullptr;
  }

  JSObject* result = type.toObjectOrNull();
  TypeCode typeCode = CType::GetTypeCode(result);

  if (typeCode == TYPE_array) {
    
    
    RootedObject baseType(cx, ArrayType::GetBaseType(result));
    result = PointerType::CreateInternal(cx, baseType);
    if (!result)
      return nullptr;

  } else if (typeCode == TYPE_void_t || typeCode == TYPE_function) {
    
    JS_ReportError(cx, "Cannot have void or function argument type");
    return nullptr;
  }

  if (!CType::IsSizeDefined(result)) {
    JS_ReportError(cx, "Argument type must have defined size");
    return nullptr;
  }

  
  JS_ASSERT(CType::GetSize(result) != 0);

  return result;
}

static JSObject*
PrepareReturnType(JSContext* cx, jsval type)
{
  if (type.isPrimitive() || !CType::IsCType(type.toObjectOrNull())) {
    JS_ReportError(cx, "not a ctypes type");
    return nullptr;
  }

  JSObject* result = type.toObjectOrNull();
  TypeCode typeCode = CType::GetTypeCode(result);

  
  if (typeCode == TYPE_array || typeCode == TYPE_function) {
    JS_ReportError(cx, "Return type cannot be an array or function");
    return nullptr;
  }

  if (typeCode != TYPE_void_t && !CType::IsSizeDefined(result)) {
    JS_ReportError(cx, "Return type must have defined size");
    return nullptr;
  }

  
  JS_ASSERT(typeCode == TYPE_void_t || CType::GetSize(result) != 0);

  return result;
}

static MOZ_ALWAYS_INLINE bool
IsEllipsis(JSContext* cx, jsval v, bool* isEllipsis)
{
  *isEllipsis = false;
  if (!v.isString())
    return true;
  JSString* str = v.toString();
  if (str->length() != 3)
    return true;
  const jschar* chars = str->getChars(cx);
  if (!chars)
    return false;
  jschar dot = '.';
  *isEllipsis = (chars[0] == dot &&
                 chars[1] == dot &&
                 chars[2] == dot);
  return true;
}

static bool
PrepareCIF(JSContext* cx,
           FunctionInfo* fninfo)
{
  ffi_abi abi;
  if (!GetABI(cx, OBJECT_TO_JSVAL(fninfo->mABI), &abi)) {
    JS_ReportError(cx, "Invalid ABI specification");
    return false;
  }

  ffi_type* rtype = CType::GetFFIType(cx, fninfo->mReturnType);
  if (!rtype)
    return false;

  ffi_status status =
    ffi_prep_cif(&fninfo->mCIF,
                 abi,
                 fninfo->mFFITypes.length(),
                 rtype,
                 fninfo->mFFITypes.begin());

  switch (status) {
  case FFI_OK:
    return true;
  case FFI_BAD_ABI:
    JS_ReportError(cx, "Invalid ABI specification");
    return false;
  case FFI_BAD_TYPEDEF:
    JS_ReportError(cx, "Invalid type specification");
    return false;
  default:
    JS_ReportError(cx, "Unknown libffi error");
    return false;
  }
}

void
FunctionType::BuildSymbolName(JSString* name,
                              JSObject* typeObj,
                              AutoCString& result)
{
  FunctionInfo* fninfo = GetFunctionInfo(typeObj);

  switch (GetABICode(fninfo->mABI)) {
  case ABI_DEFAULT:
  case ABI_WINAPI:
    
    AppendString(result, name);
    break;

  case ABI_STDCALL: {
#if (defined(_WIN32) && !defined(_WIN64)) || defined(_OS2)
    
    
    
    
    AppendString(result, "_");
    AppendString(result, name);
    AppendString(result, "@");

    
    size_t size = 0;
    for (size_t i = 0; i < fninfo->mArgTypes.length(); ++i) {
      JSObject* argType = fninfo->mArgTypes[i];
      size += Align(CType::GetSize(argType), sizeof(ffi_arg));
    }

    IntegerToString(size, 10, result);
#elif defined(_WIN64)
    
    
    AppendString(result, name);
#endif
    break;
  }

  case INVALID_ABI:
    MOZ_ASSUME_UNREACHABLE("invalid abi");
  }
}

static FunctionInfo*
NewFunctionInfo(JSContext* cx,
                jsval abiType,
                jsval returnType,
                jsval* argTypes,
                unsigned argLength)
{
  AutoPtr<FunctionInfo> fninfo(cx->new_<FunctionInfo>());
  if (!fninfo) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }

  ffi_abi abi;
  if (!GetABI(cx, abiType, &abi)) {
    JS_ReportError(cx, "Invalid ABI specification");
    return nullptr;
  }
  fninfo->mABI = abiType.toObjectOrNull();

  
  fninfo->mReturnType = PrepareReturnType(cx, returnType);
  if (!fninfo->mReturnType)
    return nullptr;

  
  if (!fninfo->mArgTypes.reserve(argLength) ||
      !fninfo->mFFITypes.reserve(argLength)) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }

  fninfo->mIsVariadic = false;

  for (uint32_t i = 0; i < argLength; ++i) {
    bool isEllipsis;
    if (!IsEllipsis(cx, argTypes[i], &isEllipsis))
      return nullptr;
    if (isEllipsis) {
      fninfo->mIsVariadic = true;
      if (i < 1) {
        JS_ReportError(cx, "\"...\" may not be the first and only parameter "
                       "type of a variadic function declaration");
        return nullptr;
      }
      if (i < argLength - 1) {
        JS_ReportError(cx, "\"...\" must be the last parameter type of a "
                       "variadic function declaration");
        return nullptr;
      }
      if (GetABICode(fninfo->mABI) != ABI_DEFAULT) {
        JS_ReportError(cx, "Variadic functions must use the __cdecl calling "
                       "convention");
        return nullptr;
      }
      break;
    }

    JSObject* argType = PrepareType(cx, argTypes[i]);
    if (!argType)
      return nullptr;

    ffi_type* ffiType = CType::GetFFIType(cx, argType);
    if (!ffiType)
      return nullptr;

    fninfo->mArgTypes.infallibleAppend(argType);
    fninfo->mFFITypes.infallibleAppend(ffiType);
  }

  if (fninfo->mIsVariadic)
    
    return fninfo.forget();

  if (!PrepareCIF(cx, fninfo.get()))
    return nullptr;

  return fninfo.forget();
}

bool
FunctionType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() < 2 || args.length() > 3) {
    JS_ReportError(cx, "FunctionType takes two or three arguments");
    return false;
  }

  AutoValueVector argTypes(cx);
  RootedObject arrayObj(cx, nullptr);

  if (args.length() == 3) {
    
    if (args[2].isObject())
      arrayObj = &args[2].toObject();
    if (!arrayObj || !JS_IsArrayObject(cx, arrayObj)) {
      JS_ReportError(cx, "third argument must be an array");
      return false;
    }

    uint32_t len;
    ASSERT_OK(JS_GetArrayLength(cx, arrayObj, &len));

    if (!argTypes.resize(len)) {
      JS_ReportOutOfMemory(cx);
      return false;
    }
  }

  
  JS_ASSERT_IF(argTypes.length(), arrayObj);
  for (uint32_t i = 0; i < argTypes.length(); ++i) {
    if (!JS_GetElement(cx, arrayObj, i, argTypes[i]))
      return false;
  }

  JSObject* result = CreateInternal(cx, args[0], args[1],
      argTypes.begin(), argTypes.length());
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

JSObject*
FunctionType::CreateInternal(JSContext* cx,
                             jsval abi,
                             jsval rtype,
                             jsval* argtypes,
                             unsigned arglen)
{
  
  AutoPtr<FunctionInfo> fninfo(NewFunctionInfo(cx, abi, rtype, argtypes, arglen));
  if (!fninfo)
    return nullptr;

  
  
  RootedObject typeProto(cx, CType::GetProtoFromType(cx, fninfo->mReturnType,
                                                     SLOT_FUNCTIONPROTO));
  if (!typeProto)
    return nullptr;
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, fninfo->mReturnType,
                                                     SLOT_FUNCTIONDATAPROTO));
  if (!dataProto)
    return nullptr;

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_function,
                        nullptr, JSVAL_VOID, JSVAL_VOID, nullptr);
  if (!typeObj)
    return nullptr;

  
  JS_SetReservedSlot(typeObj, SLOT_FNINFO, PRIVATE_TO_JSVAL(fninfo.forget()));

  return typeObj;
}




bool
FunctionType::ConstructData(JSContext* cx,
                            HandleObject typeObj,
                            HandleObject dataObj,
                            HandleObject fnObj,
                            HandleObject thisObj,
                            jsval errVal)
{
  JS_ASSERT(CType::GetTypeCode(typeObj) == TYPE_function);

  PRFuncPtr* data = static_cast<PRFuncPtr*>(CData::GetData(dataObj));

  FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);
  if (fninfo->mIsVariadic) {
    JS_ReportError(cx, "Can't declare a variadic callback function");
    return false;
  }
  if (GetABICode(fninfo->mABI) == ABI_WINAPI) {
    JS_ReportError(cx, "Can't declare a ctypes.winapi_abi callback function, "
                   "use ctypes.stdcall_abi instead");
    return false;
  }

  RootedObject closureObj(cx, CClosure::Create(cx, typeObj, fnObj, thisObj, errVal, data));
  if (!closureObj)
    return false;

  
  JS_SetReservedSlot(dataObj, SLOT_REFERENT, OBJECT_TO_JSVAL(closureObj));

  
  
  
  
  
  
  return JS_FreezeObject(cx, dataObj);
}

typedef Array<AutoValue, 16> AutoValueAutoArray;

static bool
ConvertArgument(JSContext* cx,
                HandleValue arg,
                JSObject* type,
                AutoValue* value,
                AutoValueAutoArray* strings)
{
  if (!value->SizeToType(cx, type)) {
    JS_ReportAllocationOverflow(cx);
    return false;
  }

  bool freePointer = false;
  if (!ImplicitConvert(cx, arg, type, value->mData, true, &freePointer))
    return false;

  if (freePointer) {
    
    
    if (!strings->growBy(1)) {
      JS_ReportOutOfMemory(cx);
      return false;
    }
    strings->back().mData = *static_cast<char**>(value->mData);
  }

  return true;
}

bool
FunctionType::Call(JSContext* cx,
                   unsigned argc,
                   jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  RootedObject obj(cx, &args.callee());
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a FunctionType.ptr");
    return false;
  }

  typeObj = PointerType::GetBaseType(typeObj);
  if (CType::GetTypeCode(typeObj) != TYPE_function) {
    JS_ReportError(cx, "not a FunctionType.ptr");
    return false;
  }

  FunctionInfo* fninfo = GetFunctionInfo(typeObj);
  uint32_t argcFixed = fninfo->mArgTypes.length();

  if ((!fninfo->mIsVariadic && args.length() != argcFixed) ||
      (fninfo->mIsVariadic && args.length() < argcFixed)) {
    JS_ReportError(cx, "Number of arguments does not match declaration");
    return false;
  }

  
  jsval slot = JS_GetReservedSlot(obj, SLOT_REFERENT);
  if (!slot.isUndefined() && Library::IsLibrary(&slot.toObject())) {
    PRLibrary* library = Library::GetLibrary(&slot.toObject());
    if (!library) {
      JS_ReportError(cx, "library is not open");
      return false;
    }
  }

  
  AutoValueAutoArray values;
  AutoValueAutoArray strings;
  if (!values.resize(args.length())) {
    JS_ReportOutOfMemory(cx);
    return false;
  }

  for (unsigned i = 0; i < argcFixed; ++i)
    if (!ConvertArgument(cx, args[i], fninfo->mArgTypes[i], &values[i], &strings))
      return false;

  if (fninfo->mIsVariadic) {
    if (!fninfo->mFFITypes.resize(args.length())) {
      JS_ReportOutOfMemory(cx);
      return false;
    }

    RootedObject obj(cx);  
    RootedObject type(cx); 

    for (uint32_t i = argcFixed; i < args.length(); ++i) {
      if (args[i].isPrimitive() ||
          !CData::IsCData(obj = &args[i].toObject())) {
        
        
        JS_ReportError(cx, "argument %d of type %s is not a CData object",
                       i, JS_GetTypeName(cx, JS_TypeOfValue(cx, args[i])));
        return false;
      }
      if (!(type = CData::GetCType(obj)) ||
          !(type = PrepareType(cx, OBJECT_TO_JSVAL(type))) ||
          
          
          !ConvertArgument(cx, args[i], type, &values[i], &strings) ||
          !(fninfo->mFFITypes[i] = CType::GetFFIType(cx, type))) {
        
        return false;
      }
    }
    if (!PrepareCIF(cx, fninfo))
      return false;
  }

  
  AutoValue returnValue;
  TypeCode typeCode = CType::GetTypeCode(fninfo->mReturnType);
  if (typeCode != TYPE_void_t &&
      !returnValue.SizeToType(cx, fninfo->mReturnType)) {
    JS_ReportAllocationOverflow(cx);
    return false;
  }

  
  js::AutoCTypesActivityCallback autoCallback(cx, js::CTYPES_CALL_BEGIN, js::CTYPES_CALL_END);

  uintptr_t fn = *reinterpret_cast<uintptr_t*>(CData::GetData(obj));

#if defined(XP_WIN)
  int32_t lastErrorStatus; 
  int32_t savedLastError = GetLastError();
  SetLastError(0);
#endif 
  int errnoStatus;         
  int savedErrno = errno;
  errno = 0;

  ffi_call(&fninfo->mCIF, FFI_FN(fn), returnValue.mData,
           reinterpret_cast<void**>(values.begin()));

  
  
  
  

  errnoStatus = errno;
#if defined(XP_WIN)
  lastErrorStatus = GetLastError();
  SetLastError(savedLastError);
#endif 

  errno = savedErrno;

  
  autoCallback.DoEndCallback();

  
  JSObject *objCTypes = CType::GetGlobalCTypes(cx, typeObj);
  if (!objCTypes)
    return false;

  JS_SetReservedSlot(objCTypes, SLOT_ERRNO, INT_TO_JSVAL(errnoStatus));
#if defined(XP_WIN)
  JS_SetReservedSlot(objCTypes, SLOT_LASTERROR, INT_TO_JSVAL(lastErrorStatus));
#endif 

  
  
  switch (typeCode) {
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name:                                                            \
    if (sizeof(type) < sizeof(ffi_arg)) {                                      \
      ffi_arg data = *static_cast<ffi_arg*>(returnValue.mData);                \
      *static_cast<type*>(returnValue.mData) = static_cast<type>(data);        \
    }                                                                          \
    break;
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_BOOL_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_JSCHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "ctypes/typedefs.h"
  default:
    break;
  }

  
  RootedObject returnType(cx, fninfo->mReturnType);
  return ConvertToJS(cx, returnType, NullPtr(), returnValue.mData, false, true, vp);
}

FunctionInfo*
FunctionType::GetFunctionInfo(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_function);

  jsval slot = JS_GetReservedSlot(obj, SLOT_FNINFO);
  JS_ASSERT(!slot.isUndefined() && JSVAL_TO_PRIVATE(slot));

  return static_cast<FunctionInfo*>(JSVAL_TO_PRIVATE(slot));
}

bool
FunctionType::IsFunctionType(HandleValue v)
{
  if (!v.isObject())
    return false;
  JSObject* obj = &v.toObject();
  return CType::IsCType(obj) && CType::GetTypeCode(obj) == TYPE_function;
}

bool
FunctionType::ArgTypesGetter(JSContext* cx, JS::CallArgs args)
{
  JS::Rooted<JSObject*> obj(cx, &args.thisv().toObject());

  args.rval().set(JS_GetReservedSlot(obj, SLOT_ARGS_T));
  if (!args.rval().isUndefined())
    return true;

  FunctionInfo* fninfo = GetFunctionInfo(obj);
  size_t len = fninfo->mArgTypes.length();

  
  JS::Rooted<JSObject*> argTypes(cx);
  {
      JS::AutoValueVector vec(cx);
      if (!vec.resize(len))
        return false;

      for (size_t i = 0; i < len; ++i)
        vec[i].setObject(*fninfo->mArgTypes[i]);

      argTypes = JS_NewArrayObject(cx, vec);
      if (!argTypes)
        return false;
  }

  
  if (!JS_FreezeObject(cx, argTypes))
    return false;
  JS_SetReservedSlot(obj, SLOT_ARGS_T, JS::ObjectValue(*argTypes));

  args.rval().setObject(*argTypes);
  return true;
}

bool
FunctionType::ReturnTypeGetter(JSContext* cx, JS::CallArgs args)
{
  
  args.rval().setObject(*GetFunctionInfo(&args.thisv().toObject())->mReturnType);
  return true;
}

bool
FunctionType::ABIGetter(JSContext* cx, JS::CallArgs args)
{
  
  args.rval().setObject(*GetFunctionInfo(&args.thisv().toObject())->mABI);
  return true;
}

bool
FunctionType::IsVariadicGetter(JSContext* cx, JS::CallArgs args)
{
  args.rval().setBoolean(GetFunctionInfo(&args.thisv().toObject())->mIsVariadic);
  return true;
}





JSObject*
CClosure::Create(JSContext* cx,
                 HandleObject typeObj,
                 HandleObject fnObj,
                 HandleObject thisObj,
                 jsval errVal_,
                 PRFuncPtr* fnptr)
{
  RootedValue errVal(cx, errVal_);
  JS_ASSERT(fnObj);

  RootedObject result(cx, JS_NewObject(cx, &sCClosureClass, NullPtr(), NullPtr()));
  if (!result)
    return nullptr;

  
  FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);
  JS_ASSERT(!fninfo->mIsVariadic);
  JS_ASSERT(GetABICode(fninfo->mABI) != ABI_WINAPI);

  AutoPtr<ClosureInfo> cinfo(cx->new_<ClosureInfo>(JS_GetRuntime(cx)));
  if (!cinfo) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }

  
  
  RootedObject proto(cx);
  if (!JS_GetPrototype(cx, typeObj, &proto))
    return nullptr;
  JS_ASSERT(proto);
  JS_ASSERT(CType::IsCTypeProto(proto));

  
  cinfo->cx = js::DefaultJSContext(JS_GetRuntime(cx));

  
  
  
  
  if (!errVal.isUndefined()) {

    
    if (CType::GetTypeCode(fninfo->mReturnType) == TYPE_void_t) {
      JS_ReportError(cx, "A void callback can't pass an error sentinel");
      return nullptr;
    }

    
    
    JS_ASSERT(CType::IsSizeDefined(fninfo->mReturnType));

    
    size_t rvSize = CType::GetSize(fninfo->mReturnType);
    cinfo->errResult = cx->malloc_(rvSize);
    if (!cinfo->errResult)
      return nullptr;

    
    if (!ImplicitConvert(cx, errVal, fninfo->mReturnType, cinfo->errResult,
                         false, nullptr))
      return nullptr;
  } else {
    cinfo->errResult = nullptr;
  }

  
  cinfo->closureObj = result;
  cinfo->typeObj = typeObj;
  cinfo->thisObj = thisObj;
  cinfo->jsfnObj = fnObj;

  
  void* code;
  cinfo->closure =
    static_cast<ffi_closure*>(ffi_closure_alloc(sizeof(ffi_closure), &code));
  if (!cinfo->closure || !code) {
    JS_ReportError(cx, "couldn't create closure - libffi error");
    return nullptr;
  }

  ffi_status status = ffi_prep_closure_loc(cinfo->closure, &fninfo->mCIF,
    CClosure::ClosureStub, cinfo.get(), code);
  if (status != FFI_OK) {
    JS_ReportError(cx, "couldn't create closure - libffi error");
    return nullptr;
  }

  
  JS_SetReservedSlot(result, SLOT_CLOSUREINFO, PRIVATE_TO_JSVAL(cinfo.forget()));

  
  
  *fnptr = reinterpret_cast<PRFuncPtr>(reinterpret_cast<uintptr_t>(code));
  return result;
}

void
CClosure::Trace(JSTracer* trc, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_CLOSUREINFO);
  if (slot.isUndefined())
    return;

  ClosureInfo* cinfo = static_cast<ClosureInfo*>(JSVAL_TO_PRIVATE(slot));

  
  
  JS_CallHeapObjectTracer(trc, &cinfo->typeObj, "typeObj");
  JS_CallHeapObjectTracer(trc, &cinfo->jsfnObj, "jsfnObj");
  if (cinfo->thisObj)
    JS_CallHeapObjectTracer(trc, &cinfo->thisObj, "thisObj");
}

void
CClosure::Finalize(JSFreeOp *fop, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_CLOSUREINFO);
  if (slot.isUndefined())
    return;

  ClosureInfo* cinfo = static_cast<ClosureInfo*>(JSVAL_TO_PRIVATE(slot));
  FreeOp::get(fop)->delete_(cinfo);
}

void
CClosure::ClosureStub(ffi_cif* cif, void* result, void** args, void* userData)
{
  JS_ASSERT(cif);
  JS_ASSERT(result);
  JS_ASSERT(args);
  JS_ASSERT(userData);

  
  ClosureInfo* cinfo = static_cast<ClosureInfo*>(userData);
  JSContext* cx = cinfo->cx;

  
  
  js::AutoCTypesActivityCallback autoCallback(cx, js::CTYPES_CALLBACK_BEGIN,
                                              js::CTYPES_CALLBACK_END);

  RootedObject typeObj(cx, cinfo->typeObj);
  RootedObject thisObj(cx, cinfo->thisObj);
  RootedValue jsfnVal(cx, ObjectValue(*cinfo->jsfnObj));

  JS_AbortIfWrongThread(JS_GetRuntime(cx));

  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, cinfo->jsfnObj);

  
  FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);
  JS_ASSERT(cif == &fninfo->mCIF);

  TypeCode typeCode = CType::GetTypeCode(fninfo->mReturnType);

  
  
  
  size_t rvSize = 0;
  if (cif->rtype != &ffi_type_void) {
    rvSize = cif->rtype->size;
    switch (typeCode) {
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
    case TYPE_##name:
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_BOOL_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_JSCHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "ctypes/typedefs.h"
      rvSize = Align(rvSize, sizeof(ffi_arg));
      break;
    default:
      break;
    }
    memset(result, 0, rvSize);
  }

  
  JS::AutoValueVector argv(cx);
  if (!argv.resize(cif->nargs)) {
    JS_ReportOutOfMemory(cx);
    return;
  }

  for (uint32_t i = 0; i < cif->nargs; ++i) {
    
    
    RootedObject argType(cx, fninfo->mArgTypes[i]);
    if (!ConvertToJS(cx, argType, NullPtr(), args[i], false, false, argv[i].address()))
      return;
  }

  
  
  RootedValue rval(cx);
  bool success = JS_CallFunctionValue(cx, thisObj, jsfnVal, argv, &rval);

  
  
  
  
  
  if (success && cif->rtype != &ffi_type_void)
    success = ImplicitConvert(cx, rval, fninfo->mReturnType, result, false,
                              nullptr);

  if (!success) {
    
    
    

    
    
    if (JS_IsExceptionPending(cx))
      JS_ReportPendingException(cx);

    if (cinfo->errResult) {
      
      

      
      
      
      size_t copySize = CType::GetSize(fninfo->mReturnType);
      JS_ASSERT(copySize <= rvSize);
      memcpy(result, cinfo->errResult, copySize);
    } else {
      
      
      
      
      JS_ReportError(cx, "JavaScript callback failed, and an error sentinel "
                         "was not specified.");
      if (JS_IsExceptionPending(cx))
        JS_ReportPendingException(cx);

      return;
    }
  }

  
  
  switch (typeCode) {
#define DEFINE_INT_TYPE(name, type, ffiType)                                   \
  case TYPE_##name:                                                            \
    if (sizeof(type) < sizeof(ffi_arg)) {                                      \
      ffi_arg data = *static_cast<type*>(result);                              \
      *static_cast<ffi_arg*>(result) = data;                                   \
    }                                                                          \
    break;
#define DEFINE_WRAPPED_INT_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_BOOL_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_CHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#define DEFINE_JSCHAR_TYPE(x, y, z) DEFINE_INT_TYPE(x, y, z)
#include "ctypes/typedefs.h"
  default:
    break;
  }
}



























JSObject*
CData::Create(JSContext* cx,
              HandleObject typeObj,
              HandleObject refObj,
              void* source,
              bool ownResult)
{
  JS_ASSERT(typeObj);
  JS_ASSERT(CType::IsCType(typeObj));
  JS_ASSERT(CType::IsSizeDefined(typeObj));
  JS_ASSERT(ownResult || source);
  JS_ASSERT_IF(refObj && CData::IsCData(refObj), !ownResult);

  
  jsval slot = JS_GetReservedSlot(typeObj, SLOT_PROTO);
  JS_ASSERT(slot.isObject());

  RootedObject proto(cx, &slot.toObject());
  RootedObject parent(cx, JS_GetParent(typeObj));
  JS_ASSERT(parent);

  RootedObject dataObj(cx, JS_NewObject(cx, &sCDataClass, proto, parent));
  if (!dataObj)
    return nullptr;

  
  JS_SetReservedSlot(dataObj, SLOT_CTYPE, OBJECT_TO_JSVAL(typeObj));

  
  if (refObj)
    JS_SetReservedSlot(dataObj, SLOT_REFERENT, OBJECT_TO_JSVAL(refObj));

  
  JS_SetReservedSlot(dataObj, SLOT_OWNS, BOOLEAN_TO_JSVAL(ownResult));

  
  
  char** buffer = cx->new_<char*>();
  if (!buffer) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }

  char* data;
  if (!ownResult) {
    data = static_cast<char*>(source);
  } else {
    
    size_t size = CType::GetSize(typeObj);
    data = (char*)cx->malloc_(size);
    if (!data) {
      
      JS_ReportAllocationOverflow(cx);
      js_free(buffer);
      return nullptr;
    }

    if (!source)
      memset(data, 0, size);
    else
      memcpy(data, source, size);
  }

  *buffer = data;
  JS_SetReservedSlot(dataObj, SLOT_DATA, PRIVATE_TO_JSVAL(buffer));

  return dataObj;
}

void
CData::Finalize(JSFreeOp *fop, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_OWNS);
  if (slot.isUndefined())
    return;

  bool owns = slot.toBoolean();

  slot = JS_GetReservedSlot(obj, SLOT_DATA);
  if (slot.isUndefined())
    return;
  char** buffer = static_cast<char**>(JSVAL_TO_PRIVATE(slot));

  if (owns)
    FreeOp::get(fop)->free_(*buffer);
  FreeOp::get(fop)->delete_(buffer);
}

JSObject*
CData::GetCType(JSObject* dataObj)
{
  JS_ASSERT(CData::IsCData(dataObj));

  jsval slot = JS_GetReservedSlot(dataObj, SLOT_CTYPE);
  JSObject* typeObj = slot.toObjectOrNull();
  JS_ASSERT(CType::IsCType(typeObj));
  return typeObj;
}

void*
CData::GetData(JSObject* dataObj)
{
  JS_ASSERT(CData::IsCData(dataObj));

  jsval slot = JS_GetReservedSlot(dataObj, SLOT_DATA);

  void** buffer = static_cast<void**>(JSVAL_TO_PRIVATE(slot));
  JS_ASSERT(buffer);
  JS_ASSERT(*buffer);
  return *buffer;
}

bool
CData::IsCData(JSObject* obj)
{
  return JS_GetClass(obj) == &sCDataClass;
}

bool
CData::IsCData(HandleValue v)
{
  return v.isObject() && CData::IsCData(&v.toObject());
}

bool
CData::IsCDataProto(JSObject* obj)
{
  return JS_GetClass(obj) == &sCDataProtoClass;
}

bool
CData::ValueGetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());

  
  RootedObject ctype(cx, GetCType(obj));
  return ConvertToJS(cx, ctype, NullPtr(), GetData(obj), true, false, args.rval().address());
}

bool
CData::ValueSetter(JSContext* cx, JS::CallArgs args)
{
  RootedObject obj(cx, &args.thisv().toObject());
  args.rval().setUndefined();
  return ImplicitConvert(cx, args.get(0), GetCType(obj), GetData(obj), false, nullptr);
}

bool
CData::Address(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "address takes zero arguments");
    return false;
  }

  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  RootedObject pointerType(cx, PointerType::CreateInternal(cx, typeObj));
  if (!pointerType)
    return false;

  
  JSObject* result = CData::Create(cx, pointerType, NullPtr(), nullptr, true);
  if (!result)
    return false;

  args.rval().setObject(*result);

  
  void** data = static_cast<void**>(GetData(result));
  *data = GetData(obj);
  return true;
}

bool
CData::Cast(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 2) {
    JS_ReportError(cx, "cast takes two arguments");
    return false;
  }

  if (args[0].isPrimitive() || !CData::IsCData(&args[0].toObject())) {
    JS_ReportError(cx, "first argument must be a CData");
    return false;
  }
  RootedObject sourceData(cx, &args[0].toObject());
  JSObject* sourceType = CData::GetCType(sourceData);

  if (args[1].isPrimitive() || !CType::IsCType(&args[1].toObject())) {
    JS_ReportError(cx, "second argument must be a CType");
    return false;
  }

  RootedObject targetType(cx, &args[1].toObject());
  size_t targetSize;
  if (!CType::GetSafeSize(targetType, &targetSize) ||
      targetSize > CType::GetSize(sourceType)) {
    JS_ReportError(cx,
      "target CType has undefined or larger size than source CType");
    return false;
  }

  
  
  void* data = CData::GetData(sourceData);
  JSObject* result = CData::Create(cx, targetType, sourceData, data, false);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

bool
CData::GetRuntime(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 1) {
    JS_ReportError(cx, "getRuntime takes one argument");
    return false;
  }

  if (args[0].isPrimitive() || !CType::IsCType(&args[0].toObject())) {
    JS_ReportError(cx, "first argument must be a CType");
    return false;
  }

  RootedObject targetType(cx, &args[0].toObject());
  size_t targetSize;
  if (!CType::GetSafeSize(targetType, &targetSize) ||
      targetSize != sizeof(void*)) {
    JS_ReportError(cx, "target CType has non-pointer size");
    return false;
  }

  void* data = static_cast<void*>(cx->runtime());
  JSObject* result = CData::Create(cx, targetType, NullPtr(), &data, true);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

typedef JS::TwoByteCharsZ (*InflateUTF8Method)(JSContext *, const JS::UTF8Chars, size_t *);

static bool
ReadStringCommon(JSContext* cx, InflateUTF8Method inflateUTF8, unsigned argc, jsval *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "readString takes zero arguments");
    return false;
  }

  JSObject* obj = CDataFinalizer::GetCData(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj || !CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  
  
  JSObject* baseType;
  JSObject* typeObj = CData::GetCType(obj);
  TypeCode typeCode = CType::GetTypeCode(typeObj);
  void* data;
  size_t maxLength = -1;
  switch (typeCode) {
  case TYPE_pointer:
    baseType = PointerType::GetBaseType(typeObj);
    data = *static_cast<void**>(CData::GetData(obj));
    if (data == nullptr) {
      JS_ReportError(cx, "cannot read contents of null pointer");
      return false;
    }
    break;
  case TYPE_array:
    baseType = ArrayType::GetBaseType(typeObj);
    data = CData::GetData(obj);
    maxLength = ArrayType::GetLength(typeObj);
    break;
  default:
    JS_ReportError(cx, "not a PointerType or ArrayType");
    return false;
  }

  
  
  JSString* result;
  switch (CType::GetTypeCode(baseType)) {
  case TYPE_int8_t:
  case TYPE_uint8_t:
  case TYPE_char:
  case TYPE_signed_char:
  case TYPE_unsigned_char: {
    char* bytes = static_cast<char*>(data);
    size_t length = strnlen(bytes, maxLength);

    
    jschar *dst = inflateUTF8(cx, JS::UTF8Chars(bytes, length), &length).get();
    if (!dst)
      return false;

    result = JS_NewUCString(cx, dst, length);
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
    return false;
  }

  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

bool
CData::ReadString(JSContext* cx, unsigned argc, jsval* vp)
{
  return ReadStringCommon(cx, JS::UTF8CharsToNewTwoByteCharsZ, argc, vp);
}

bool
CData::ReadStringReplaceMalformed(JSContext* cx, unsigned argc, jsval* vp)
{
  return ReadStringCommon(cx, JS::LossyUTF8CharsToNewTwoByteCharsZ, argc, vp);
}

JSString *
CData::GetSourceString(JSContext *cx, HandleObject typeObj, void *data)
{
  
  
  
  
  
  
  AutoString source;
  BuildTypeSource(cx, typeObj, true, source);
  AppendString(source, "(");
  if (!BuildDataSource(cx, typeObj, data, false, source))
    return nullptr;

  AppendString(source, ")");

  return NewUCString(cx, source);
}

bool
CData::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return false;
  }

  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!CData::IsCData(obj) && !CData::IsCDataProto(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  JSString* result;
  if (CData::IsCData(obj)) {
    RootedObject typeObj(cx, CData::GetCType(obj));
    void* data = CData::GetData(obj);

    result = CData::GetSourceString(cx, typeObj, data);
  } else {
    result = JS_NewStringCopyZ(cx, "[CData proto object]");
  }

  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

bool
CData::ErrnoGetter(JSContext* cx, JS::CallArgs args)
{
  args.rval().set(JS_GetReservedSlot(&args.thisv().toObject(), SLOT_ERRNO));
  return true;
}

#if defined(XP_WIN)
bool
CData::LastErrorGetter(JSContext* cx, JS::CallArgs args)
{
  args.rval().set(JS_GetReservedSlot(&args.thisv().toObject(), SLOT_LASTERROR));
  return true;
}
#endif 

bool
CDataFinalizer::Methods::ToSource(JSContext *cx, unsigned argc, jsval *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject objThis(cx, JS_THIS_OBJECT(cx, vp));
  if (!objThis)
    return false;
  if (!CDataFinalizer::IsCDataFinalizer(objThis)) {
    JS_ReportError(cx, "not a CDataFinalizer");
    return false;
  }

  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(objThis);

  JSString *strMessage;
  if (!p) {
    strMessage = JS_NewStringCopyZ(cx, "ctypes.CDataFinalizer()");
  } else {
    RootedObject objType(cx, CDataFinalizer::GetCType(cx, objThis));
    if (!objType) {
      JS_ReportError(cx, "CDataFinalizer has no type");
      return false;
    }

    AutoString source;
    AppendString(source, "ctypes.CDataFinalizer(");
    JSString *srcValue = CData::GetSourceString(cx, objType, p->cargs);
    if (!srcValue) {
      return false;
    }
    AppendString(source, srcValue);
    AppendString(source, ", ");
    jsval valCodePtrType = JS_GetReservedSlot(objThis,
                                              SLOT_DATAFINALIZER_CODETYPE);
    if (valCodePtrType.isPrimitive()) {
      return false;
    }

    RootedObject typeObj(cx, valCodePtrType.toObjectOrNull());
    JSString *srcDispose = CData::GetSourceString(cx, typeObj, &(p->code));
    if (!srcDispose) {
      return false;
    }

    AppendString(source, srcDispose);
    AppendString(source, ")");
    strMessage = NewUCString(cx, source);
  }

  if (!strMessage) {
    
    return false;
  }

  args.rval().setString(strMessage);
  return true;
}

bool
CDataFinalizer::Methods::ToString(JSContext *cx, unsigned argc, jsval *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* objThis = JS_THIS_OBJECT(cx, vp);
  if (!objThis)
    return false;
  if (!CDataFinalizer::IsCDataFinalizer(objThis)) {
    JS_ReportError(cx, "not a CDataFinalizer");
    return false;
  }

  JSString *strMessage;
  RootedValue value(cx);
  if (!JS_GetPrivate(objThis)) {
    
    
    strMessage = JS_NewStringCopyZ(cx, "[CDataFinalizer - empty]");
    if (!strMessage) {
      return false;
    }
  } else if (!CDataFinalizer::GetValue(cx, objThis, value.address())) {
    MOZ_ASSUME_UNREACHABLE("Could not convert an empty CDataFinalizer");
  } else {
    strMessage = ToString(cx, value);
    if (!strMessage) {
      return false;
    }
  }
  args.rval().setString(strMessage);
  return true;
}

bool
CDataFinalizer::IsCDataFinalizer(JSObject *obj)
{
  return JS_GetClass(obj) == &sCDataFinalizerClass;
}


JSObject *
CDataFinalizer::GetCType(JSContext *cx, JSObject *obj)
{
  MOZ_ASSERT(IsCDataFinalizer(obj));

  jsval valData = JS_GetReservedSlot(obj,
                                     SLOT_DATAFINALIZER_VALTYPE);
  if (valData.isUndefined()) {
    return nullptr;
  }

  return valData.toObjectOrNull();
}

JSObject*
CDataFinalizer::GetCData(JSContext *cx, JSObject *obj)
{
  if (!obj) {
    JS_ReportError(cx, "No C data");
    return nullptr;
  }
  if (CData::IsCData(obj)) {
    return obj;
  }
  if (!CDataFinalizer::IsCDataFinalizer(obj)) {
    JS_ReportError(cx, "Not C data");
    return nullptr;
  }
  RootedValue val(cx);
  if (!CDataFinalizer::GetValue(cx, obj, val.address()) || val.isPrimitive()) {
    JS_ReportError(cx, "Empty CDataFinalizer");
    return nullptr;
  }
  return val.toObjectOrNull();
}

bool
CDataFinalizer::GetValue(JSContext *cx, JSObject *obj, jsval *aResult)
{
  MOZ_ASSERT(IsCDataFinalizer(obj));

  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(obj);

  if (!p) {
    JS_ReportError(cx, "Attempting to get the value of an empty CDataFinalizer");
    return false;  
  }

  RootedObject ctype(cx, GetCType(cx, obj));
  return ConvertToJS(cx, ctype, NullPtr(), p -> cargs, false, true, aResult);
}














bool
CDataFinalizer::Construct(JSContext* cx, unsigned argc, jsval *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject objSelf(cx, &args.callee());
  RootedObject objProto(cx);
  if (!GetObjectProperty(cx, objSelf, "prototype", &objProto)) {
    JS_ReportError(cx, "CDataFinalizer.prototype does not exist");
    return false;
  }

  
  if (args.length() == 0) { 
    JSObject *objResult = JS_NewObject(cx, &sCDataFinalizerClass, objProto, NullPtr());
    args.rval().setObject(*objResult);
    return true;
  }

  if (args.length() != 2) {
    JS_ReportError(cx, "CDataFinalizer takes 2 arguments");
    return false;
  }

  JS::HandleValue valCodePtr = args[1];
  if (!valCodePtr.isObject()) {
    return TypeError(cx, "_a CData object_ of a function pointer type",
                     valCodePtr);
  }
  JSObject *objCodePtr = &valCodePtr.toObject();

  
  

  
  if (!CData::IsCData(objCodePtr)) {
    return TypeError(cx, "a _CData_ object of a function pointer type",
                     valCodePtr);
  }
  RootedObject objCodePtrType(cx, CData::GetCType(objCodePtr));
  RootedValue valCodePtrType(cx, ObjectValue(*objCodePtrType));
  MOZ_ASSERT(objCodePtrType);

  TypeCode typCodePtr = CType::GetTypeCode(objCodePtrType);
  if (typCodePtr != TYPE_pointer) {
    return TypeError(cx, "a CData object of a function _pointer_ type",
                     valCodePtrType);
  }

  JSObject *objCodeType = PointerType::GetBaseType(objCodePtrType);
  MOZ_ASSERT(objCodeType);

  TypeCode typCode = CType::GetTypeCode(objCodeType);
  if (typCode != TYPE_function) {
    return TypeError(cx, "a CData object of a _function_ pointer type",
                     valCodePtrType);
  }
  uintptr_t code = *reinterpret_cast<uintptr_t*>(CData::GetData(objCodePtr));
  if (!code) {
    return TypeError(cx, "a CData object of a _non-NULL_ function pointer type",
                     valCodePtrType);
  }

  FunctionInfo* funInfoFinalizer =
    FunctionType::GetFunctionInfo(objCodeType);
  MOZ_ASSERT(funInfoFinalizer);

  if ((funInfoFinalizer->mArgTypes.length() != 1)
      || (funInfoFinalizer->mIsVariadic)) {
    RootedValue valCodeType(cx, ObjectValue(*objCodeType));
    return TypeError(cx, "a function accepting exactly one argument",
                     valCodeType);
  }
  RootedObject objArgType(cx, funInfoFinalizer->mArgTypes[0]);
  RootedObject returnType(cx, funInfoFinalizer->mReturnType);

  
  

  bool freePointer = false;

  

  size_t sizeArg;
  RootedValue valData(cx, args[0]);
  if (!CType::GetSafeSize(objArgType, &sizeArg)) {
    return TypeError(cx, "(an object with known size)", valData);
  }

  ScopedJSFreePtr<void> cargs(malloc(sizeArg));

  if (!ImplicitConvert(cx, valData, objArgType, cargs.get(),
                       false, &freePointer)) {
    RootedValue valArgType(cx, ObjectValue(*objArgType));
    return TypeError(cx, "(an object that can be converted to the following type)",
                     valArgType);
  }
  if (freePointer) {
    
    JS_ReportError(cx, "Internal Error during CDataFinalizer. Object cannot be represented");
    return false;
  }

  

  ScopedJSFreePtr<void> rvalue;
  if (CType::GetTypeCode(returnType) != TYPE_void_t) {
    rvalue = malloc(Align(CType::GetSize(returnType),
                          sizeof(ffi_arg)));
  } 

  

  JSObject *objResult = JS_NewObject(cx, &sCDataFinalizerClass, objProto, NullPtr());
  if (!objResult) {
    return false;
  }

  
  
  
  JSObject *objBestArgType = objArgType;
  if (valData.isObject()) {
    JSObject *objData = &valData.toObject();
    if (CData::IsCData(objData)) {
      objBestArgType = CData::GetCType(objData);
      size_t sizeBestArg;
      if (!CType::GetSafeSize(objBestArgType, &sizeBestArg)) {
        MOZ_ASSUME_UNREACHABLE("object with unknown size");
      }
      if (sizeBestArg != sizeArg) {
        return TypeError(cx, "(an object with the same size as that expected by the C finalization function)", valData);
      }
    }
  }

  
  JS_SetReservedSlot(objResult,
                     SLOT_DATAFINALIZER_VALTYPE,
                     OBJECT_TO_JSVAL(objBestArgType));

  
  JS_SetReservedSlot(objResult,
                     SLOT_DATAFINALIZER_CODETYPE,
                     OBJECT_TO_JSVAL(objCodePtrType));

  ffi_abi abi;
  if (!GetABI(cx, OBJECT_TO_JSVAL(funInfoFinalizer->mABI), &abi)) {
    JS_ReportError(cx, "Internal Error: "
                   "Invalid ABI specification in CDataFinalizer");
    return false;
  }

  ffi_type* rtype = CType::GetFFIType(cx, funInfoFinalizer->mReturnType);
  if (!rtype) {
    JS_ReportError(cx, "Internal Error: "
                   "Could not access ffi type of CDataFinalizer");
    return false;
  }

  
  ScopedJSFreePtr<CDataFinalizer::Private>
    p((CDataFinalizer::Private*)malloc(sizeof(CDataFinalizer::Private)));

  memmove(&p->CIF, &funInfoFinalizer->mCIF, sizeof(ffi_cif));

  p->cargs = cargs.forget();
  p->rvalue = rvalue.forget();
  p->cargs_size = sizeArg;
  p->code = code;


  JS_SetPrivate(objResult, p.forget());
  args.rval().setObject(*objResult);
  return true;
}
















void
CDataFinalizer::CallFinalizer(CDataFinalizer::Private *p,
                              int* errnoStatus,
                              int32_t* lastErrorStatus)
{
  int savedErrno = errno;
  errno = 0;
#if defined(XP_WIN)
  int32_t savedLastError = GetLastError();
  SetLastError(0);
#endif 

  ffi_call(&p->CIF, FFI_FN(p->code), p->rvalue, &p->cargs);

  if (errnoStatus) {
    *errnoStatus = errno;
  }
  errno = savedErrno;
#if defined(XP_WIN)
  if (lastErrorStatus) {
    *lastErrorStatus = GetLastError();
  }
  SetLastError(savedLastError);
#endif 
}











bool
CDataFinalizer::Methods::Forget(JSContext* cx, unsigned argc, jsval *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "CDataFinalizer.prototype.forget takes no arguments");
    return false;
  }

  JS::Rooted<JSObject*> obj(cx, args.thisv().toObjectOrNull());
  if (!obj)
    return false;
  if (!CDataFinalizer::IsCDataFinalizer(obj)) {
    RootedValue val(cx, ObjectValue(*obj));
    return TypeError(cx, "a CDataFinalizer", val);
  }

  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(obj);

  if (!p) {
    JS_ReportError(cx, "forget called on an empty CDataFinalizer");
    return false;
  }

  RootedValue valJSData(cx);
  RootedObject ctype(cx, GetCType(cx, obj));
  if (!ConvertToJS(cx, ctype, NullPtr(), p->cargs, false, true, valJSData.address())) {
    JS_ReportError(cx, "CDataFinalizer value cannot be represented");
    return false;
  }

  CDataFinalizer::Cleanup(p, obj);

  args.rval().set(valJSData);
  return true;
}











bool
CDataFinalizer::Methods::Dispose(JSContext* cx, unsigned argc, jsval *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "CDataFinalizer.prototype.dispose takes no arguments");
    return false;
  }

  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!CDataFinalizer::IsCDataFinalizer(obj)) {
    RootedValue val(cx, ObjectValue(*obj));
    return TypeError(cx, "a CDataFinalizer", val);
  }

  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(obj);

  if (!p) {
    JS_ReportError(cx, "dispose called on an empty CDataFinalizer.");
    return false;
  }

  jsval valType = JS_GetReservedSlot(obj, SLOT_DATAFINALIZER_VALTYPE);
  JS_ASSERT(valType.isObject());

  JSObject *objCTypes = CType::GetGlobalCTypes(cx, &valType.toObject());
  if (!objCTypes)
    return false;

  jsval valCodePtrType = JS_GetReservedSlot(obj, SLOT_DATAFINALIZER_CODETYPE);
  JS_ASSERT(valCodePtrType.isObject());
  JSObject *objCodePtrType = &valCodePtrType.toObject();

  JSObject *objCodeType = PointerType::GetBaseType(objCodePtrType);
  JS_ASSERT(objCodeType);
  JS_ASSERT(CType::GetTypeCode(objCodeType) == TYPE_function);

  RootedObject resultType(cx, FunctionType::GetFunctionInfo(objCodeType)->mReturnType);
  RootedValue result(cx, JSVAL_VOID);

  int errnoStatus;
#if defined(XP_WIN)
  int32_t lastErrorStatus;
  CDataFinalizer::CallFinalizer(p, &errnoStatus, &lastErrorStatus);
#else
  CDataFinalizer::CallFinalizer(p, &errnoStatus, nullptr);
#endif 

  JS_SetReservedSlot(objCTypes, SLOT_ERRNO, INT_TO_JSVAL(errnoStatus));
#if defined(XP_WIN)
  JS_SetReservedSlot(objCTypes, SLOT_LASTERROR, INT_TO_JSVAL(lastErrorStatus));
#endif 

  if (ConvertToJS(cx, resultType, NullPtr(), p->rvalue, false, true, result.address())) {
    CDataFinalizer::Cleanup(p, obj);
    args.rval().set(result);
    return true;
  }
  CDataFinalizer::Cleanup(p, obj);
  return false;
}











void
CDataFinalizer::Finalize(JSFreeOp* fop, JSObject* obj)
{
  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(obj);

  if (!p) {
    return;
  }

  CDataFinalizer::CallFinalizer(p, nullptr, nullptr);
  CDataFinalizer::Cleanup(p, nullptr);
}












void
CDataFinalizer::Cleanup(CDataFinalizer::Private *p, JSObject *obj)
{
  if (!p) {
    return;  
  }

  free(p->cargs);
  free(p->rvalue);
  free(p);

  if (!obj) {
    return;  
  }

  JS_ASSERT(CDataFinalizer::IsCDataFinalizer(obj));

  JS_SetPrivate(obj, nullptr);
  for (int i = 0; i < CDATAFINALIZER_SLOTS; ++i) {
    JS_SetReservedSlot(obj, i, JSVAL_NULL);
  }
}






JSObject*
Int64Base::Construct(JSContext* cx,
                     HandleObject proto,
                     uint64_t data,
                     bool isUnsigned)
{
  const JSClass* clasp = isUnsigned ? &sUInt64Class : &sInt64Class;
  RootedObject parent(cx, JS_GetParent(proto));
  RootedObject result(cx, JS_NewObject(cx, clasp, proto, parent));
  if (!result)
    return nullptr;

  
  uint64_t* buffer = cx->new_<uint64_t>(data);
  if (!buffer) {
    JS_ReportOutOfMemory(cx);
    return nullptr;
  }

  JS_SetReservedSlot(result, SLOT_INT64, PRIVATE_TO_JSVAL(buffer));

  if (!JS_FreezeObject(cx, result))
    return nullptr;

  return result;
}

void
Int64Base::Finalize(JSFreeOp *fop, JSObject* obj)
{
  jsval slot = JS_GetReservedSlot(obj, SLOT_INT64);
  if (slot.isUndefined())
    return;

  FreeOp::get(fop)->delete_(static_cast<uint64_t*>(JSVAL_TO_PRIVATE(slot)));
}

uint64_t
Int64Base::GetInt(JSObject* obj) {
  JS_ASSERT(Int64::IsInt64(obj) || UInt64::IsUInt64(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_INT64);
  return *static_cast<uint64_t*>(JSVAL_TO_PRIVATE(slot));
}

bool
Int64Base::ToString(JSContext* cx,
                    JSObject* obj,
                    const CallArgs& args,
                    bool isUnsigned)
{
  if (args.length() > 1) {
    JS_ReportError(cx, "toString takes zero or one argument");
    return false;
  }

  int radix = 10;
  if (args.length() == 1) {
    jsval arg = args[0];
    if (arg.isInt32())
      radix = arg.toInt32();
    if (!arg.isInt32() || radix < 2 || radix > 36) {
      JS_ReportError(cx, "radix argument must be an integer between 2 and 36");
      return false;
    }
  }

  AutoString intString;
  if (isUnsigned) {
    IntegerToString(GetInt(obj), radix, intString);
  } else {
    IntegerToString(static_cast<int64_t>(GetInt(obj)), radix, intString);
  }

  JSString *result = NewUCString(cx, intString);
  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

bool
Int64Base::ToSource(JSContext* cx,
                    JSObject* obj,
                    const CallArgs& args,
                    bool isUnsigned)
{
  if (args.length() != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return false;
  }

  
  AutoString source;
  if (isUnsigned) {
    AppendString(source, "ctypes.UInt64(\"");
    IntegerToString(GetInt(obj), 10, source);
  } else {
    AppendString(source, "ctypes.Int64(\"");
    IntegerToString(static_cast<int64_t>(GetInt(obj)), 10, source);
  }
  AppendString(source, "\")");

  JSString *result = NewUCString(cx, source);
  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

bool
Int64::Construct(JSContext* cx,
                 unsigned argc,
                 jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);

  
  if (args.length() != 1) {
    JS_ReportError(cx, "Int64 takes one argument");
    return false;
  }

  int64_t i = 0;
  if (!jsvalToBigInteger(cx, args[0], true, &i))
    return TypeError(cx, "int64", args[0]);

  
  RootedValue slot(cx);
  RootedObject callee(cx, &args.callee());
  ASSERT_OK(JS_GetProperty(cx, callee, "prototype", &slot));
  RootedObject proto(cx, slot.toObjectOrNull());
  JS_ASSERT(JS_GetClass(proto) == &sInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, i, false);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

bool
Int64::IsInt64(JSObject* obj)
{
  return JS_GetClass(obj) == &sInt64Class;
}

bool
Int64::ToString(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!Int64::IsInt64(obj)) {
    JS_ReportError(cx, "not an Int64");
    return false;
  }

  return Int64Base::ToString(cx, obj, args, false);
}

bool
Int64::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!Int64::IsInt64(obj)) {
    JS_ReportError(cx, "not an Int64");
    return false;
  }

  return Int64Base::ToSource(cx, obj, args, false);
}

bool
Int64::Compare(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 2 ||
      args[0].isPrimitive() ||
      args[1].isPrimitive() ||
      !Int64::IsInt64(&args[0].toObject()) ||
      !Int64::IsInt64(&args[1].toObject())) {
    JS_ReportError(cx, "compare takes two Int64 arguments");
    return false;
  }

  JSObject* obj1 = &args[0].toObject();
  JSObject* obj2 = &args[1].toObject();

  int64_t i1 = Int64Base::GetInt(obj1);
  int64_t i2 = Int64Base::GetInt(obj2);

  if (i1 == i2)
    args.rval().setInt32(0);
  else if (i1 < i2)
    args.rval().setInt32(-1);
  else
    args.rval().setInt32(1);

  return true;
}

#define LO_MASK ((uint64_t(1) << 32) - 1)
#define INT64_LO(i) ((i) & LO_MASK)
#define INT64_HI(i) ((i) >> 32)

bool
Int64::Lo(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 1 || args[0].isPrimitive() ||
      !Int64::IsInt64(&args[0].toObject())) {
    JS_ReportError(cx, "lo takes one Int64 argument");
    return false;
  }

  JSObject* obj = &args[0].toObject();
  int64_t u = Int64Base::GetInt(obj);
  double d = uint32_t(INT64_LO(u));

  args.rval().setNumber(d);
  return true;
}

bool
Int64::Hi(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 1 || args[0].isPrimitive() ||
      !Int64::IsInt64(&args[0].toObject())) {
    JS_ReportError(cx, "hi takes one Int64 argument");
    return false;
  }

  JSObject* obj = &args[0].toObject();
  int64_t u = Int64Base::GetInt(obj);
  double d = int32_t(INT64_HI(u));

  args.rval().setDouble(d);
  return true;
}

bool
Int64::Join(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 2) {
    JS_ReportError(cx, "join takes two arguments");
    return false;
  }

  int32_t hi;
  uint32_t lo;
  if (!jsvalToInteger(cx, args[0], &hi))
    return TypeError(cx, "int32", args[0]);
  if (!jsvalToInteger(cx, args[1], &lo))
    return TypeError(cx, "uint32", args[1]);

  int64_t i = (int64_t(hi) << 32) + int64_t(lo);

  
  JSObject* callee = &args.callee();

  jsval slot = js::GetFunctionNativeReserved(callee, SLOT_FN_INT64PROTO);
  RootedObject proto(cx, &slot.toObject());
  JS_ASSERT(JS_GetClass(proto) == &sInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, i, false);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

bool
UInt64::Construct(JSContext* cx,
                  unsigned argc,
                  jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);

  
  if (args.length() != 1) {
    JS_ReportError(cx, "UInt64 takes one argument");
    return false;
  }

  uint64_t u = 0;
  if (!jsvalToBigInteger(cx, args[0], true, &u))
    return TypeError(cx, "uint64", args[0]);

  
  RootedValue slot(cx);
  RootedObject callee(cx, &args.callee());
  ASSERT_OK(JS_GetProperty(cx, callee, "prototype", &slot));
  RootedObject proto(cx, &slot.toObject());
  JS_ASSERT(JS_GetClass(proto) == &sUInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

bool
UInt64::IsUInt64(JSObject* obj)
{
  return JS_GetClass(obj) == &sUInt64Class;
}

bool
UInt64::ToString(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!UInt64::IsUInt64(obj)) {
    JS_ReportError(cx, "not a UInt64");
    return false;
  }

  return Int64Base::ToString(cx, obj, args, true);
}

bool
UInt64::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!UInt64::IsUInt64(obj)) {
    JS_ReportError(cx, "not a UInt64");
    return false;
  }

  return Int64Base::ToSource(cx, obj, args, true);
}

bool
UInt64::Compare(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 2 ||
      args[0].isPrimitive() ||
      args[1].isPrimitive() ||
      !UInt64::IsUInt64(&args[0].toObject()) ||
      !UInt64::IsUInt64(&args[1].toObject())) {
    JS_ReportError(cx, "compare takes two UInt64 arguments");
    return false;
  }

  JSObject* obj1 = &args[0].toObject();
  JSObject* obj2 = &args[1].toObject();

  uint64_t u1 = Int64Base::GetInt(obj1);
  uint64_t u2 = Int64Base::GetInt(obj2);

  if (u1 == u2)
    args.rval().setInt32(0);
  else if (u1 < u2)
    args.rval().setInt32(-1);
  else
    args.rval().setInt32(1);

  return true;
}

bool
UInt64::Lo(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 1 || args[0].isPrimitive() ||
      !UInt64::IsUInt64(&args[0].toObject())) {
    JS_ReportError(cx, "lo takes one UInt64 argument");
    return false;
  }

  JSObject* obj = &args[0].toObject();
  uint64_t u = Int64Base::GetInt(obj);
  double d = uint32_t(INT64_LO(u));

  args.rval().setDouble(d);
  return true;
}

bool
UInt64::Hi(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 1 || args[0].isPrimitive() ||
      !UInt64::IsUInt64(&args[0].toObject())) {
    JS_ReportError(cx, "hi takes one UInt64 argument");
    return false;
  }

  JSObject* obj = &args[0].toObject();
  uint64_t u = Int64Base::GetInt(obj);
  double d = uint32_t(INT64_HI(u));

  args.rval().setDouble(d);
  return true;
}

bool
UInt64::Join(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 2) {
    JS_ReportError(cx, "join takes two arguments");
    return false;
  }

  uint32_t hi;
  uint32_t lo;
  if (!jsvalToInteger(cx, args[0], &hi))
    return TypeError(cx, "uint32_t", args[0]);
  if (!jsvalToInteger(cx, args[1], &lo))
    return TypeError(cx, "uint32_t", args[1]);

  uint64_t u = (uint64_t(hi) << 32) + uint64_t(lo);

  
  JSObject* callee = &args.callee();

  jsval slot = js::GetFunctionNativeReserved(callee, SLOT_FN_INT64PROTO);
  RootedObject proto(cx, &slot.toObject());
  JS_ASSERT(JS_GetClass(proto) == &sUInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return false;

  args.rval().setObject(*result);
  return true;
}

}
}
