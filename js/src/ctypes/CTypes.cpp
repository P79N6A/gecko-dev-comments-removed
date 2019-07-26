




#include "mozilla/FloatingPoint.h"

#include "CTypes.h"
#include "Library.h"
#include "jsnum.h"
#include "jscompartment.h"
#include "jsobjinlines.h"
#include <limits>

#include <math.h>
#if defined(XP_WIN) || defined(XP_OS2)
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

#include "mozilla/StandardInteger.h"

using namespace std;

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
                                     NULL, JSMSG_BAD_SURROGATE_CHAR, buffer);
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
    return JS_TRUE;

badSurrogate:
    *dstlenp = (origDstlen - dstlen);
    
    if (maybecx)
        GetDeflatedUTF8StringLength(maybecx, src - 1, srclen + 1);
    return JS_FALSE;

bufferTooSmall:
    *dstlenp = (origDstlen - dstlen);
    if (maybecx) {
        JS_ReportErrorNumber(maybecx, js_GetErrorMessage, NULL,
                             JSMSG_BUFFER_TOO_SMALL);
    }
    return JS_FALSE;
}





static JSBool ConstructAbstract(JSContext* cx, unsigned argc, jsval* vp);

namespace CType {
  static JSBool ConstructData(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ConstructBasic(JSContext* cx, JSHandleObject obj, const CallArgs& args);

  static void Trace(JSTracer* trc, JSObject* obj);
  static void Finalize(JSFreeOp *fop, JSObject* obj);
  static void FinalizeProtoClass(JSFreeOp *fop, JSObject* obj);

  static JSBool PrototypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool NameGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool SizeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool PtrGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp);
  static JSBool CreateArray(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ToString(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool HasInstance(JSContext* cx, JSHandleObject obj, JSMutableHandleValue v, JSBool* bp);


  






  static JSObject* GetGlobalCTypes(JSContext* cx, JSObject* obj);

}

namespace ABI {
  bool IsABI(JSObject* obj);
  static JSBool ToSource(JSContext* cx, unsigned argc, jsval* vp);
}

namespace PointerType {
  static JSBool Create(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ConstructData(JSContext* cx, JSHandleObject obj, const CallArgs& args);

  static JSBool TargetTypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool ContentsGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool ContentsSetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, JSBool strict,
    MutableHandleValue vp);
  static JSBool IsNull(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Increment(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Decrement(JSContext* cx, unsigned argc, jsval* vp);
  
  
  static JSBool OffsetBy(JSContext* cx, const CallArgs& args, int offset);
}

namespace ArrayType {
  static JSBool Create(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ConstructData(JSContext* cx, JSHandleObject obj, const CallArgs& args);

  static JSBool ElementTypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool LengthGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool Getter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp);
  static JSBool Setter(JSContext* cx, JSHandleObject obj, JSHandleId idval, JSBool strict, MutableHandleValue vp);
  static JSBool AddressOfElement(JSContext* cx, unsigned argc, jsval* vp);
}

namespace StructType {
  static JSBool Create(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ConstructData(JSContext* cx, JSHandleObject obj, const CallArgs& args);

  static JSBool FieldsArrayGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool FieldGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool FieldSetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, JSBool strict,
                            MutableHandleValue vp);
  static JSBool AddressOfField(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Define(JSContext* cx, unsigned argc, jsval* vp);
}

namespace FunctionType {
  static JSBool Create(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ConstructData(JSContext* cx, JSHandleObject typeObj,
    JSHandleObject dataObj, JSHandleObject fnObj, JSHandleObject thisObj, jsval errVal);

  static JSBool Call(JSContext* cx, unsigned argc, jsval* vp);

  static JSBool ArgTypesGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool ReturnTypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
  static JSBool ABIGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp);
  static JSBool IsVariadicGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
    MutableHandleValue vp);
}

namespace CClosure {
  static void Trace(JSTracer* trc, JSObject* obj);
  static void Finalize(JSFreeOp *fop, JSObject* obj);

  
  static void ClosureStub(ffi_cif* cif, void* result, void** args,
    void* userData);
}

namespace CData {
  static void Finalize(JSFreeOp *fop, JSObject* obj);

  static JSBool ValueGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
                            MutableHandleValue vp);
  static JSBool ValueSetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
                            JSBool strict, MutableHandleValue vp);
  static JSBool Address(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ReadString(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ReadStringReplaceMalformed(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, unsigned argc, jsval* vp);
  static JSString *GetSourceString(JSContext *cx, JSHandleObject typeObj,
                                   void *data);
  static JSBool ErrnoGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
                            MutableHandleValue vp);

#if defined(XP_WIN)
  static JSBool LastErrorGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval,
                                MutableHandleValue vp);
#endif 
}

namespace CDataFinalizer {
  











  static JSBool Construct(JSContext* cx, unsigned argc, jsval *vp);

  








  struct Private {
    



    void *cargs;

    


    size_t cargs_size;

    



    ffi_cif CIF;

    



    uintptr_t code;

    



    void *rvalue;
  };

  


  namespace Methods {
    static JSBool Dispose(JSContext* cx, unsigned argc, jsval *vp);
    static JSBool Forget(JSContext* cx, unsigned argc, jsval *vp);
    static JSBool ToSource(JSContext* cx, unsigned argc, jsval *vp);
    static JSBool ToString(JSContext* cx, unsigned argc, jsval *vp);
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

  JSBool ToString(JSContext* cx, JSObject* obj, unsigned argc, jsval* vp,
    bool isUnsigned);

  JSBool ToSource(JSContext* cx, JSObject* obj, unsigned argc, jsval* vp,
    bool isUnsigned);

  static void Finalize(JSFreeOp *fop, JSObject* obj);
}

namespace Int64 {
  static JSBool Construct(JSContext* cx, unsigned argc, jsval* vp);

  static JSBool ToString(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, unsigned argc, jsval* vp);

  static JSBool Compare(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Lo(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Hi(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Join(JSContext* cx, unsigned argc, jsval* vp);
}

namespace UInt64 {
  static JSBool Construct(JSContext* cx, unsigned argc, jsval* vp);

  static JSBool ToString(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool ToSource(JSContext* cx, unsigned argc, jsval* vp);

  static JSBool Compare(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Lo(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Hi(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Join(JSContext* cx, unsigned argc, jsval* vp);
}







static JSClass sCTypesGlobalClass = {
  "ctypes",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPESGLOBAL_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static JSClass sCABIClass = {
  "CABI",
  JSCLASS_HAS_RESERVED_SLOTS(CABI_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};




static JSClass sCTypeProtoClass = {
  "CType",
  JSCLASS_HAS_RESERVED_SLOTS(CTYPEPROTO_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CType::FinalizeProtoClass,
  NULL, ConstructAbstract, NULL, ConstructAbstract
};



static JSClass sCDataProtoClass = {
  "CData",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static JSClass sCTypeClass = {
  "CType",
  JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_HAS_RESERVED_SLOTS(CTYPE_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CType::Finalize,
  NULL, CType::ConstructData, CType::HasInstance, CType::ConstructData,
  CType::Trace
};

static JSClass sCDataClass = {
  "CData",
  JSCLASS_HAS_RESERVED_SLOTS(CDATA_SLOTS),
  JS_PropertyStub, JS_PropertyStub, ArrayType::Getter, ArrayType::Setter,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CData::Finalize,
  NULL, FunctionType::Call, NULL, FunctionType::Call
};

static JSClass sCClosureClass = {
  "CClosure",
  JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_HAS_RESERVED_SLOTS(CCLOSURE_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CClosure::Finalize,
  NULL, NULL, NULL, NULL, CClosure::Trace
};




static JSClass sCDataFinalizerProtoClass = {
  "CDataFinalizer",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};







static JSClass sCDataFinalizerClass = {
  "CDataFinalizer",
  JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(CDATAFINALIZER_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CDataFinalizer::Finalize,
};


#define CTYPESFN_FLAGS \
  (JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

#define CTYPESCTOR_FLAGS \
  (CTYPESFN_FLAGS | JSFUN_CONSTRUCTOR)

#define CTYPESPROP_FLAGS \
  (JSPROP_SHARED | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

#define CABIFN_FLAGS \
  (JSPROP_READONLY | JSPROP_PERMANENT)

#define CDATAFN_FLAGS \
  (JSPROP_READONLY | JSPROP_PERMANENT)

#define CDATAFINALIZERFN_FLAGS \
  (JSPROP_READONLY | JSPROP_PERMANENT)

static JSPropertySpec sCTypeProps[] = {
  { "name", 0, CTYPESPROP_FLAGS, JSOP_WRAPPER(CType::NameGetter), JSOP_NULLWRAPPER },
  { "size", 0, CTYPESPROP_FLAGS, JSOP_WRAPPER(CType::SizeGetter), JSOP_NULLWRAPPER },
  { "ptr", 0, CTYPESPROP_FLAGS, JSOP_WRAPPER(CType::PtrGetter), JSOP_NULLWRAPPER },
  { "prototype", 0, CTYPESPROP_FLAGS, JSOP_WRAPPER(CType::PrototypeGetter), JSOP_NULLWRAPPER },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sCTypeFunctions[] = {
  JS_FN("array", CType::CreateArray, 0, CTYPESFN_FLAGS),
  JS_FN("toString", CType::ToString, 0, CTYPESFN_FLAGS),
  JS_FN("toSource", CType::ToSource, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sCABIFunctions[] = {
  JS_FN("toSource", ABI::ToSource, 0, CABIFN_FLAGS),
  JS_FN("toString", ABI::ToSource, 0, CABIFN_FLAGS),
  JS_FS_END
};

static JSPropertySpec sCDataProps[] = {
  { "value", 0, JSPROP_SHARED | JSPROP_PERMANENT,
    JSOP_WRAPPER(CData::ValueGetter), JSOP_WRAPPER(CData::ValueSetter) },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sCDataFunctions[] = {
  JS_FN("address", CData::Address, 0, CDATAFN_FLAGS),
  JS_FN("readString", CData::ReadString, 0, CDATAFN_FLAGS),
  JS_FN("readStringReplaceMalformed", CData::ReadStringReplaceMalformed, 0, CDATAFN_FLAGS),
  JS_FN("toSource", CData::ToSource, 0, CDATAFN_FLAGS),
  JS_FN("toString", CData::ToSource, 0, CDATAFN_FLAGS),
  JS_FS_END
};

static JSPropertySpec sCDataFinalizerProps[] = {
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sCDataFinalizerFunctions[] = {
  JS_FN("dispose",  CDataFinalizer::Methods::Dispose,  0, CDATAFINALIZERFN_FLAGS),
  JS_FN("forget",   CDataFinalizer::Methods::Forget,   0, CDATAFINALIZERFN_FLAGS),
  JS_FN("readString",CData::ReadString, 0, CDATAFINALIZERFN_FLAGS),
  JS_FN("toString", CDataFinalizer::Methods::ToString, 0, CDATAFINALIZERFN_FLAGS),
  JS_FN("toSource", CDataFinalizer::Methods::ToSource, 0, CDATAFINALIZERFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sPointerFunction =
  JS_FN("PointerType", PointerType::Create, 1, CTYPESCTOR_FLAGS);

static JSPropertySpec sPointerProps[] = {
  { "targetType", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(PointerType::TargetTypeGetter), JSOP_NULLWRAPPER },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sPointerInstanceFunctions[] = {
  JS_FN("isNull", PointerType::IsNull, 0, CTYPESFN_FLAGS),
  JS_FN("increment", PointerType::Increment, 0, CTYPESFN_FLAGS),
  JS_FN("decrement", PointerType::Decrement, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

static JSPropertySpec sPointerInstanceProps[] = {
  { "contents", 0, JSPROP_SHARED | JSPROP_PERMANENT,
    JSOP_WRAPPER(PointerType::ContentsGetter),
    JSOP_WRAPPER(PointerType::ContentsSetter) },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sArrayFunction =
  JS_FN("ArrayType", ArrayType::Create, 1, CTYPESCTOR_FLAGS);

static JSPropertySpec sArrayProps[] = {
  { "elementType", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(ArrayType::ElementTypeGetter), JSOP_NULLWRAPPER },
  { "length", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(ArrayType::LengthGetter), JSOP_NULLWRAPPER },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sArrayInstanceFunctions[] = {
  JS_FN("addressOfElement", ArrayType::AddressOfElement, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static JSPropertySpec sArrayInstanceProps[] = {
  { "length", 0, JSPROP_SHARED | JSPROP_READONLY | JSPROP_PERMANENT,
    JSOP_WRAPPER(ArrayType::LengthGetter), JSOP_NULLWRAPPER },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sStructFunction =
  JS_FN("StructType", StructType::Create, 2, CTYPESCTOR_FLAGS);

static JSPropertySpec sStructProps[] = {
  { "fields", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(StructType::FieldsArrayGetter), JSOP_NULLWRAPPER },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sStructFunctions[] = {
  JS_FN("define", StructType::Define, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sStructInstanceFunctions[] = {
  JS_FN("addressOfField", StructType::AddressOfField, 1, CDATAFN_FLAGS),
  JS_FS_END
};

static JSFunctionSpec sFunctionFunction =
  JS_FN("FunctionType", FunctionType::Create, 2, CTYPESCTOR_FLAGS);

static JSPropertySpec sFunctionProps[] = {
  { "argTypes", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(FunctionType::ArgTypesGetter), JSOP_NULLWRAPPER },
  { "returnType", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(FunctionType::ReturnTypeGetter), JSOP_NULLWRAPPER },
  { "abi", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(FunctionType::ABIGetter), JSOP_NULLWRAPPER },
  { "isVariadic", 0, CTYPESPROP_FLAGS,
    JSOP_WRAPPER(FunctionType::IsVariadicGetter), JSOP_NULLWRAPPER },
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sFunctionInstanceFunctions[] = {
  JS_FN("call", js_fun_call, 1, CDATAFN_FLAGS),
  JS_FN("apply", js_fun_apply, 2, CDATAFN_FLAGS),
  JS_FS_END
};

static JSClass sInt64ProtoClass = {
  "Int64",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static JSClass sUInt64ProtoClass = {
  "UInt64",
  0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static JSClass sInt64Class = {
  "Int64",
  JSCLASS_HAS_RESERVED_SLOTS(INT64_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Int64Base::Finalize
};

static JSClass sUInt64Class = {
  "UInt64",
  JSCLASS_HAS_RESERVED_SLOTS(INT64_SLOTS),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Int64Base::Finalize
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

static JSPropertySpec sModuleProps[] = {
  { "errno", 0, JSPROP_SHARED | JSPROP_PERMANENT,
    JSOP_WRAPPER(CData::ErrnoGetter), JSOP_NULLWRAPPER },
#if defined(XP_WIN)
  { "winLastError", 0, JSPROP_SHARED | JSPROP_PERMANENT,
    JSOP_WRAPPER(CData::LastErrorGetter), JSOP_NULLWRAPPER },
#endif 
  { 0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER }
};

static JSFunctionSpec sModuleFunctions[] = {
  JS_FN("CDataFinalizer", CDataFinalizer::Construct, 2, CTYPESFN_FLAGS),
  JS_FN("open", Library::Open, 1, CTYPESFN_FLAGS),
  JS_FN("cast", CData::Cast, 2, CTYPESFN_FLAGS),
  JS_FN("getRuntime", CData::GetRuntime, 1, CTYPESFN_FLAGS),
  JS_FN("libraryName", Library::Name, 1, CTYPESFN_FLAGS),
  JS_FS_END
};

JS_ALWAYS_INLINE JSString*
NewUCString(JSContext* cx, const AutoString& from)
{
  return JS_NewUCStringCopyN(cx, from.begin(), from.length());
}






JS_ALWAYS_INLINE size_t
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
  return ABICode(JSVAL_TO_INT(result));
}

JSErrorFormatString ErrorFormatString[CTYPESERR_LIMIT] = {
#define MSG_DEF(name, number, count, exception, format) \
  { format, count, exception } ,
#include "ctypes.msg"
#undef MSG_DEF
};

const JSErrorFormatString*
GetErrorMessage(void* userRef, const char* locale, const unsigned errorNumber)
{
  if (0 < errorNumber && errorNumber < CTYPESERR_LIMIT)
    return &ErrorFormatString[errorNumber];
  return NULL;
}

JSBool
TypeError(JSContext* cx, const char* expected, jsval actual)
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
  JS_ReportErrorNumber(cx, GetErrorMessage, NULL,
                       CTYPESMSG_TYPE_ERROR, expected, src);
  return false;
}

static JSObject*
InitCTypeClass(JSContext* cx, HandleObject parent)
{
  JSFunction *fun = JS_DefineFunction(cx, parent, "CType", ConstructAbstract, 0,
                                      CTYPESCTOR_FLAGS);
  if (!fun)
    return NULL;

  RootedObject ctor(cx, JS_GetFunctionObject(fun));
  RootedObject fnproto(cx);
  if (!JS_GetPrototype(cx, ctor, fnproto.address()))
    return NULL;
  JS_ASSERT(ctor);
  JS_ASSERT(fnproto);

  
  RootedObject prototype(cx, JS_NewObject(cx, &sCTypeProtoClass, fnproto, parent));
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

  if (!JS_FreezeObject(cx, ctor) || !JS_FreezeObject(cx, prototype))
    return NULL;

  return prototype;
}

static JSObject*
InitABIClass(JSContext* cx, JSObject* parent)
{
  RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));
  
  if (!obj)
    return NULL;
    
  if (!JS_DefineFunctions(cx, obj, sCABIFunctions))
    return NULL;
  
  return obj;
}


static JSObject*
InitCDataClass(JSContext* cx, HandleObject parent, HandleObject CTypeProto)
{
  JSFunction* fun = JS_DefineFunction(cx, parent, "CData", ConstructAbstract, 0,
                      CTYPESCTOR_FLAGS);
  if (!fun)
    return NULL;

  RootedObject ctor(cx, JS_GetFunctionObject(fun));
  JS_ASSERT(ctor);

  
  
  
  if (!JS_SetPrototype(cx, ctor, CTypeProto))
    return NULL;

  
  RootedObject prototype(cx, JS_NewObject(cx, &sCDataProtoClass, NULL, parent));
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
      !JS_FreezeObject(cx, ctor))
    return NULL;

  return prototype;
}

static JSBool
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



static JSBool
InitTypeConstructor(JSContext* cx,
                    HandleObject parent,
                    HandleObject CTypeProto,
                    HandleObject CDataProto,
                    JSFunctionSpec spec,
                    JSFunctionSpec* fns,
                    JSPropertySpec* props,
                    JSFunctionSpec* instanceFns,
                    JSPropertySpec* instanceProps,
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

  
  if (!JS_DefineProperty(cx, obj, "prototype", OBJECT_TO_JSVAL(typeProto),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (fns && !JS_DefineFunctions(cx, typeProto, fns))
    return false;

  if (!JS_DefineProperties(cx, typeProto, props))
    return false;

  if (!JS_DefineProperty(cx, typeProto, "constructor", OBJECT_TO_JSVAL(obj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
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

JSObject*
InitInt64Class(JSContext* cx,
               HandleObject parent,
               JSClass* clasp,
               JSNative construct,
               JSFunctionSpec* fs,
               JSFunctionSpec* static_fs)
{
  
  RootedObject prototype(cx, JS_InitClass(cx, parent, NULL, clasp, construct,
                                          0, NULL, fs, NULL, static_fs));
  if (!prototype)
    return NULL;

  RootedObject ctor(cx, JS_GetConstructor(cx, prototype));
  if (!ctor)
    return NULL;
  if (!JS_FreezeObject(cx, ctor))
    return NULL;

  
  
  JS_ASSERT(clasp == &sInt64ProtoClass || clasp == &sUInt64ProtoClass);
  JSNative native = (clasp == &sInt64ProtoClass) ? Int64::Join : UInt64::Join;
  JSFunction* fun = js::DefineFunctionWithReserved(cx, ctor, "join", native,
                      2, CTYPESFN_FLAGS);
  if (!fun)
    return NULL;

  js::SetFunctionNativeReserved(fun, SLOT_FN_INT64PROTO,
    OBJECT_TO_JSVAL(prototype));

  if (!JS_FreezeObject(cx, prototype))
    return NULL;

  return prototype;
}

static void
AttachProtos(JSObject* proto, const AutoObjectVector& protos)
{
  
  
  
  for (uint32_t i = 0; i <= SLOT_CTYPES; ++i)
    JS_SetReservedSlot(proto, i, OBJECT_TO_JSVAL(protos[i]));
}

JSBool
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
         sPointerFunction, NULL, sPointerProps,
         sPointerInstanceFunctions, sPointerInstanceProps,
         protos.handleAt(SLOT_POINTERPROTO), protos.handleAt(SLOT_POINTERDATAPROTO)))
    return false;

  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sArrayFunction, NULL, sArrayProps,
         sArrayInstanceFunctions, sArrayInstanceProps,
         protos.handleAt(SLOT_ARRAYPROTO), protos.handleAt(SLOT_ARRAYDATAPROTO)))
    return false;

  if (!InitTypeConstructor(cx, parent, CTypeProto, CDataProto,
         sStructFunction, sStructFunctions, sStructProps,
         sStructInstanceFunctions, NULL,
         protos.handleAt(SLOT_STRUCTPROTO), protos.handleAt(SLOT_STRUCTDATAPROTO)))
    return false;

  if (!InitTypeConstructor(cx, parent, CTypeProto, protos.handleAt(SLOT_POINTERDATAPROTO),
         sFunctionFunction, NULL, sFunctionProps, sFunctionInstanceFunctions, NULL,
         protos.handleAt(SLOT_FUNCTIONPROTO), protos.handleAt(SLOT_FUNCTIONDATAPROTO)))
    return false;

  protos[SLOT_CDATAPROTO] = CDataProto;

  
  
  
  
  
  
  
  
  protos[SLOT_INT64PROTO] = InitInt64Class(cx, parent, &sInt64ProtoClass,
    Int64::Construct, sInt64Functions, sInt64StaticFunctions);
  if (!protos[SLOT_INT64PROTO])
    return false;
  protos[SLOT_UINT64PROTO] = InitInt64Class(cx, parent, &sUInt64ProtoClass,
    UInt64::Construct, sUInt64Functions, sUInt64StaticFunctions);
  if (!protos[SLOT_UINT64PROTO])
    return false;

  
  
  protos[SLOT_CTYPES] = parent;

  
  
  
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
#include "typedefs.h"

  
  
  if (!JS_DefineProperty(cx, parent, "unsigned",
         OBJECT_TO_JSVAL(typeObj_unsigned_int), NULL, NULL,
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
  if (!JS_DefineProperty(cx, parent, "voidptr_t", OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return true;
}

bool
IsCTypesGlobal(JSObject* obj)
{
  return JS_GetClass(obj) == &sCTypesGlobalClass;
}


JSCTypesCallbacks*
GetCallbacks(JSObject* obj)
{
  JS_ASSERT(IsCTypesGlobal(obj));

  jsval result = JS_GetReservedSlot(obj, SLOT_CALLBACKS);
  if (JSVAL_IS_VOID(result))
    return NULL;

  return static_cast<JSCTypesCallbacks*>(JSVAL_TO_PRIVATE(result));
}




bool GetObjectProperty(JSContext *cx, HandleObject obj,
                       const char *property, MutableHandleObject result)
{
  jsval val;
  if (!JS_GetProperty(cx, obj, property, &val)) {
    return false;
  }

  if (JSVAL_IS_PRIMITIVE(val)) {
    JS_ReportError(cx, "missing or non-object field");
    return false;
  }

  result.set(JSVAL_TO_OBJECT(val));
  return true;
}

} 
} 

using namespace js;
using namespace js::ctypes;

JS_PUBLIC_API(JSBool)
JS_InitCTypesClass(JSContext* cx, JSObject *globalArg)
{
  RootedObject global(cx, globalArg);

  
  RootedObject ctypes(cx, JS_NewObject(cx, &sCTypesGlobalClass, NULL, NULL));
  if (!ctypes)
    return false;

  if (!JS_DefineProperty(cx, global, "ctypes", OBJECT_TO_JSVAL(ctypes),
         JS_PropertyStub, JS_StrictPropertyStub, JSPROP_READONLY | JSPROP_PERMANENT)){
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

  RootedObject prototype(cx, JS_NewObject(cx, &sCDataFinalizerProtoClass, NULL, ctypes));
  if (!prototype)
    return false;

  if (!JS_DefineProperties(cx, prototype, sCDataFinalizerProps) ||
      !JS_DefineFunctions(cx, prototype, sCDataFinalizerFunctions))
    return false;

  if (!JS_DefineProperty(cx, ctor, "prototype", OBJECT_TO_JSVAL(prototype),
                         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(ctor),
                         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;


  
  return JS_FreezeObject(cx, ctypes);
}

JS_PUBLIC_API(void)
JS_SetCTypesCallbacks(JSRawObject ctypesObj, JSCTypesCallbacks* callbacks)
{
  JS_ASSERT(callbacks);
  JS_ASSERT(IsCTypesGlobal(ctypesObj));

  
  JS_SetReservedSlot(ctypesObj, SLOT_CALLBACKS, PRIVATE_TO_JSVAL(callbacks));
}

namespace js {

JS_FRIEND_API(size_t)
SizeOfDataIfCDataObject(JSMallocSizeOfFun mallocSizeOf, JSObject *obj)
{
    if (!CData::IsCData(obj))
        return 0;

    size_t n = 0;
    jsval slot = JS_GetReservedSlot(obj, ctypes::SLOT_OWNS);
    if (!JSVAL_IS_VOID(slot)) {
        JSBool owns = JSVAL_TO_BOOLEAN(slot);
        slot = JS_GetReservedSlot(obj, ctypes::SLOT_DATA);
        if (!JSVAL_IS_VOID(slot)) {
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
JS_STATIC_ASSERT(numeric_limits<double>::is_signed);



template<class TargetType, class FromType>
struct ConvertImpl {
  static JS_ALWAYS_INLINE TargetType Convert(FromType d) {
    return TargetType(d);
  }
};

#ifdef _MSC_VER


template<>
struct ConvertImpl<uint64_t, double> {
  static JS_ALWAYS_INLINE uint64_t Convert(double d) {
    return d > 0x7fffffffffffffffui64 ?
           uint64_t(d - 0x8000000000000000ui64) + 0x8000000000000000ui64 :
           uint64_t(d);
  }
};
#endif





#ifdef SPARC

template<>
struct ConvertImpl<uint64_t, double> {
  static JS_ALWAYS_INLINE uint64_t Convert(double d) {
    return d >= 0xffffffffffffffff ?
           0x8000000000000000 : uint64_t(d);
  }
};

template<>
struct ConvertImpl<int64_t, double> {
  static JS_ALWAYS_INLINE int64_t Convert(double d) {
    return d >= 0x7fffffffffffffff ?
           0x8000000000000000 : int64_t(d);
  }
};
#endif

template<class TargetType, class FromType>
static JS_ALWAYS_INLINE TargetType Convert(FromType d)
{
  return ConvertImpl<TargetType, FromType>::Convert(d);
}

template<class TargetType, class FromType>
static JS_ALWAYS_INLINE bool IsAlwaysExact()
{
  
  
  
  
  
  
  
  
  
  if (numeric_limits<TargetType>::digits < numeric_limits<FromType>::digits)
    return false;

  if (numeric_limits<FromType>::is_signed &&
      !numeric_limits<TargetType>::is_signed)
    return false;

  if (!numeric_limits<FromType>::is_exact &&
      numeric_limits<TargetType>::is_exact)
    return false;

  return true;
}



template<class TargetType, class FromType, bool TargetSigned, bool FromSigned>
struct IsExactImpl {
  static JS_ALWAYS_INLINE bool Test(FromType i, TargetType j) {
    JS_STATIC_ASSERT(numeric_limits<TargetType>::is_exact);
    return FromType(j) == i;
  }
};


template<class TargetType, class FromType>
struct IsExactImpl<TargetType, FromType, false, true> {
  static JS_ALWAYS_INLINE bool Test(FromType i, TargetType j) {
    JS_STATIC_ASSERT(numeric_limits<TargetType>::is_exact);
    return i >= 0 && FromType(j) == i;
  }
};


template<class TargetType, class FromType>
struct IsExactImpl<TargetType, FromType, true, false> {
  static JS_ALWAYS_INLINE bool Test(FromType i, TargetType j) {
    JS_STATIC_ASSERT(numeric_limits<TargetType>::is_exact);
    return TargetType(i) >= 0 && FromType(j) == i;
  }
};



template<class TargetType, class FromType>
static JS_ALWAYS_INLINE bool ConvertExact(FromType i, TargetType* result)
{
  
  JS_STATIC_ASSERT(numeric_limits<TargetType>::is_exact);

  *result = Convert<TargetType>(i);

  
  if (IsAlwaysExact<TargetType, FromType>())
    return true;

  
  return IsExactImpl<TargetType,
                     FromType,
                     numeric_limits<TargetType>::is_signed,
                     numeric_limits<FromType>::is_signed>::Test(i, *result);
}



template<class Type, bool IsSigned>
struct IsNegativeImpl {
  static JS_ALWAYS_INLINE bool Test(Type i) {
    return false;
  }
};


template<class Type>
struct IsNegativeImpl<Type, true> {
  static JS_ALWAYS_INLINE bool Test(Type i) {
    return i < 0;
  }
};


template<class Type>
static JS_ALWAYS_INLINE bool IsNegative(Type i)
{
  return IsNegativeImpl<Type, numeric_limits<Type>::is_signed>::Test(i);
}



static bool
jsvalToBool(JSContext* cx, jsval val, bool* result)
{
  if (JSVAL_IS_BOOLEAN(val)) {
    *result = JSVAL_TO_BOOLEAN(val) != JS_FALSE;
    return true;
  }
  if (JSVAL_IS_INT(val)) {
    int32_t i = JSVAL_TO_INT(val);
    *result = i != 0;
    return i == 0 || i == 1;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    double d = JSVAL_TO_DOUBLE(val);
    *result = d != 0;
    
    return d == 1 || d == 0;
  }
  
  return false;
}




template<class IntegerType>
static bool
jsvalToInteger(JSContext* cx, jsval val, IntegerType* result)
{
  JS_STATIC_ASSERT(numeric_limits<IntegerType>::is_exact);

  if (JSVAL_IS_INT(val)) {
    
    
    int32_t i = JSVAL_TO_INT(val);
    return ConvertExact(i, result);
  }
  if (JSVAL_IS_DOUBLE(val)) {
    
    
    double d = JSVAL_TO_DOUBLE(val);
    return ConvertExact(d, result);
  }
  if (!JSVAL_IS_PRIMITIVE(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
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
      jsval innerData;
      if (!CDataFinalizer::GetValue(cx, obj, &innerData)) {
        return false; 
      }
      return jsvalToInteger(cx, innerData, result);
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
  JS_STATIC_ASSERT(!numeric_limits<FloatType>::is_exact);

  
  
  
  
  if (JSVAL_IS_INT(val)) {
    *result = FloatType(JSVAL_TO_INT(val));
    return true;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    *result = FloatType(JSVAL_TO_DOUBLE(val));
    return true;
  }
  if (!JSVAL_IS_PRIMITIVE(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
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
#include "typedefs.h"
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
  JS_STATIC_ASSERT(numeric_limits<IntegerType>::is_exact);

  const jschar* cp = string->getChars(NULL);
  if (!cp)
    return false;

  const jschar* end = cp + string->length();
  if (cp == end)
    return false;

  IntegerType sign = 1;
  if (cp[0] == '-') {
    if (!numeric_limits<IntegerType>::is_signed)
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
  JS_STATIC_ASSERT(numeric_limits<IntegerType>::is_exact);

  if (JSVAL_IS_INT(val)) {
    
    
    int32_t i = JSVAL_TO_INT(val);
    return ConvertExact(i, result);
  }
  if (JSVAL_IS_DOUBLE(val)) {
    
    
    double d = JSVAL_TO_DOUBLE(val);
    return ConvertExact(d, result);
  }
  if (allowString && JSVAL_IS_STRING(val)) {
    
    
    
    
    return StringToInteger(cx, JSVAL_TO_STRING(val), result);
  }
  if (!JSVAL_IS_PRIMITIVE(val)) {
    
    JSObject* obj = JSVAL_TO_OBJECT(val);

    if (UInt64::IsUInt64(obj)) {
      
      uint64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (Int64::IsInt64(obj)) {
      
      int64_t i = Int64Base::GetInt(obj);
      return ConvertExact(i, result);
    }

    if (CDataFinalizer::IsCDataFinalizer(obj)) {
      jsval innerData;
      if (!CDataFinalizer::GetValue(cx, obj, &innerData)) {
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
  JS_STATIC_ASSERT(numeric_limits<IntegerType>::is_exact);

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



static JSBool
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
  JS_STATIC_ASSERT(numeric_limits<IntegerType>::is_exact);

  if (JSVAL_IS_DOUBLE(val)) {
    
    double d = JSVAL_TO_DOUBLE(val);
    *result = MOZ_DOUBLE_IS_FINITE(d) ? IntegerType(d) : 0;
    return true;
  }
  if (!JSVAL_IS_PRIMITIVE(val)) {
    
    JSObject* obj = JSVAL_TO_OBJECT(val);
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
  if (JSVAL_IS_INT(val)) {
    
    
    int32_t i = JSVAL_TO_INT(val);
    *result = i < 0 ? uintptr_t(intptr_t(i)) : uintptr_t(i);
    return true;
  }
  if (JSVAL_IS_DOUBLE(val)) {
    double d = JSVAL_TO_DOUBLE(val);
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
  if (!JSVAL_IS_PRIMITIVE(val)) {
    JSObject* obj = JSVAL_TO_OBJECT(val);
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
  JS_STATIC_ASSERT(numeric_limits<IntegerType>::is_exact);

  
  
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












JSBool
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
    if (!numeric_limits<type>::is_signed) {                                    \
      value = *static_cast<type*>(data);                                       \
      /* Get ctypes.UInt64.prototype from ctypes.CType.prototype. */           \
      proto = CType::GetProtoFromType(cx, typeObj, SLOT_UINT64PROTO);          \
      if (!proto)                                                              \
        return false;                                                          \
    } else {                                                                   \
      value = int64_t(*static_cast<type*>(data));                              \
      /* Get ctypes.Int64.prototype from ctypes.CType.prototype. */            \
      proto = CType::GetProtoFromType(cx, typeObj, SLOT_INT64PROTO);     \
      if (!proto)                                                              \
        return false;                                                          \
    }                                                                          \
                                                                               \
    JSObject* obj = Int64Base::Construct(cx, proto, value,                     \
      !numeric_limits<type>::is_signed);                                       \
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

    JSObject* obj = CData::Create(cx, typeObj, parentObj, data, ownResult);
    if (!obj)
      return false;

    *result = OBJECT_TO_JSVAL(obj);
    break;
  }
  case TYPE_function:
    JS_NOT_REACHED("cannot return a FunctionType");
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
  case TypedArray::TYPE_INT8:
    elementTypeCode = TYPE_int8_t;
    break;
  case TypedArray::TYPE_UINT8:
  case TypedArray::TYPE_UINT8_CLAMPED:
    elementTypeCode = TYPE_uint8_t;
    break;
  case TypedArray::TYPE_INT16:
    elementTypeCode = TYPE_int16_t;
    break;
  case TypedArray::TYPE_UINT16:
    elementTypeCode = TYPE_uint16_t;
    break;
  case TypedArray::TYPE_INT32:
    elementTypeCode = TYPE_int32_t;
    break;
  case TypedArray::TYPE_UINT32:
    elementTypeCode = TYPE_uint32_t;
    break;
  case TypedArray::TYPE_FLOAT32:
    elementTypeCode = TYPE_float32_t;
    break;
  case TypedArray::TYPE_FLOAT64:
    elementTypeCode = TYPE_float64_t;
    break;
  default:
    return false;
  }
  return elementTypeCode == baseTypeCode;
}












JSBool
ImplicitConvert(JSContext* cx,
                jsval val,
                JSObject* targetType,
                void* buffer,
                bool isArgument,
                bool* freePointer)
{
  JS_ASSERT(CType::IsSizeDefined(targetType));

  
  
  JSObject* sourceData = NULL;
  JSObject* sourceType = NULL;
  RootedObject valObj(cx, NULL);
  if (!JSVAL_IS_PRIMITIVE(val)) {
    valObj = JSVAL_TO_OBJECT(val);
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
        return JS_FALSE;
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
    if (JSVAL_IS_STRING(val)) {                                                \
      JSString* str = JSVAL_TO_STRING(val);                                    \
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
#include "typedefs.h"
  case TYPE_pointer: {
    if (JSVAL_IS_NULL(val)) {
      
      *static_cast<void**>(buffer) = NULL;
      break;
    }

    JSObject* baseType = PointerType::GetBaseType(targetType);
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

    } else if (isArgument && JSVAL_IS_STRING(val)) {
      
      
      
      JSString* sourceString = JSVAL_TO_STRING(val);
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
    } else if (!JSVAL_IS_PRIMITIVE(val) && JS_IsArrayBufferObject(valObj)) {
      
      
      
      *static_cast<void**>(buffer) = JS_GetArrayBufferData(valObj);
      break;
    } if (!JSVAL_IS_PRIMITIVE(val) && JS_IsTypedArrayObject(valObj)) {
      if(!CanConvertTypedArrayItemTo(baseType, valObj, cx)) {
        return TypeError(cx, "typed array with the appropriate type", val);
      }

      
      
      
      *static_cast<void**>(buffer) = JS_GetArrayBufferViewData(valObj);
      break;
    }
    return TypeError(cx, "pointer", val);
  }
  case TYPE_array: {
    JSObject* baseType = ArrayType::GetBaseType(targetType);
    size_t targetLength = ArrayType::GetLength(targetType);

    if (JSVAL_IS_STRING(val)) {
      JSString* sourceString = JSVAL_TO_STRING(val);
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

    } else if (!JSVAL_IS_PRIMITIVE(val) && JS_IsArrayObject(cx, valObj)) {
      
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
        js::AutoValueRooter item(cx);
        if (!JS_GetElement(cx, valObj, i, item.jsval_addr()))
          return false;

        char* data = intermediate.get() + elementSize * i;
        if (!ImplicitConvert(cx, item.jsval_value(), baseType, data, false, NULL))
          return false;
      }

      memcpy(buffer, intermediate.get(), arraySize);

    } else if (!JSVAL_IS_PRIMITIVE(val) &&
               JS_IsArrayBufferObject(valObj)) {
      
      
      uint32_t sourceLength = JS_GetArrayBufferByteLength(valObj);
      size_t elementSize = CType::GetSize(baseType);
      size_t arraySize = elementSize * targetLength;
      if (arraySize != size_t(sourceLength)) {
        JS_ReportError(cx, "ArrayType length does not match source ArrayBuffer length");
        return false;
      }
      memcpy(buffer, JS_GetArrayBufferData(valObj), sourceLength);
      break;
    }  else if (!JSVAL_IS_PRIMITIVE(val) &&
               JS_IsTypedArrayObject(valObj)) {
      
      
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
    if (!JSVAL_IS_PRIMITIVE(val) && !sourceData) {
      
      
      RootedObject iter(cx, JS_NewPropertyIterator(cx, valObj));
      if (!iter)
        return false;

      
      size_t structSize = CType::GetSize(targetType);
      AutoPtr<char> intermediate(cx->pod_malloc<char>(structSize));
      if (!intermediate) {
        JS_ReportAllocationOverflow(cx);
        return false;
      }

      jsid id;
      size_t i = 0;
      while (1) {
        if (!JS_NextProperty(cx, iter, &id))
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

        js::AutoValueRooter prop(cx);
        if (!JS_GetPropertyById(cx, valObj, id, prop.jsval_addr()))
          return false;

        
        char* fieldData = intermediate.get() + field->mOffset;
        if (!ImplicitConvert(cx, prop.jsval_value(), field->mType, fieldData, false, NULL))
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
    JS_NOT_REACHED("invalid type");
    return false;
  }

  return true;
}




JSBool
ExplicitConvert(JSContext* cx, jsval val, HandleObject targetType, void* buffer)
{
  
  if (ImplicitConvert(cx, val, targetType, buffer, false, NULL))
    return true;

  
  
  
  js::AutoValueRooter ex(cx);
  if (!JS_GetPendingException(cx, ex.jsval_addr()))
    return false;

  
  
  JS_ClearPendingException(cx);

  TypeCode type = CType::GetTypeCode(targetType);

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
    if (!jsvalToIntegerExplicit(val, &result) &&                               \
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
    
    JS_SetPendingException(cx, ex.jsval_value());
    return false;
  case TYPE_void_t:
  case TYPE_function:
    JS_NOT_REACHED("invalid type");
    return false;
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
#include "typedefs.h"
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
      JS_NOT_REACHED("invalid abi");
      break;
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
      fieldsArray[r.front().value.mIndex] = &r.front();

    for (size_t i = 0; i < length; ++i) {
      const FieldInfoHash::Entry* entry = fieldsArray[i];
      AppendString(result, "{ \"");
      AppendString(result, entry->key);
      AppendString(result, "\": ");
      BuildTypeSource(cx, entry->value.mType, true, result);
      AppendString(result, " }");
      if (i != length - 1)
        AppendString(result, ", ");
    }

    AppendString(result, "])");
    break;
  }
  }
}











static JSBool
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
    if (!numeric_limits<type>::is_signed)                                      \
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
#include "typedefs.h"
  case TYPE_jschar: {
    
    JSString* str = JS_NewUCStringCopyN(cx, static_cast<jschar*>(data), 1);
    if (!str)
      return false;

    
    JSString* src = JS_ValueToSource(cx, STRING_TO_JSVAL(str));
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
      fieldsArray[r.front().value.mIndex] = &r.front();

    for (size_t i = 0; i < length; ++i) {
      const FieldInfoHash::Entry* entry = fieldsArray[i];

      if (isImplicit) {
        AppendString(result, "\"");
        AppendString(result, entry->key);
        AppendString(result, "\": ");
      }

      char* fieldData = static_cast<char*>(data) + entry->value.mOffset;
      RootedObject entryType(cx, entry->value.mType);
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
    JS_NOT_REACHED("invalid type");
    break;
  }

  return true;
}





JSBool
ConstructAbstract(JSContext* cx,
                  unsigned argc,
                  jsval* vp)
{
  
  JS_ReportError(cx, "cannot construct from abstract type");
  return JS_FALSE;
}





JSBool
CType::ConstructData(JSContext* cx,
                     unsigned argc,
                     jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  RootedObject obj(cx, &args.callee());
  if (!CType::IsCType(obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  
  
  
  
  switch (GetTypeCode(obj)) {
  case TYPE_void_t:
    JS_ReportError(cx, "cannot construct from void_t");
    return JS_FALSE;
  case TYPE_function:
    JS_ReportError(cx, "cannot construct from FunctionType; use FunctionType.ptr instead");
    return JS_FALSE;
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

JSBool
CType::ConstructBasic(JSContext* cx,
                      HandleObject obj,
                      const CallArgs& args)
{
  if (args.length() > 1) {
    JS_ReportError(cx, "CType constructor takes zero or one argument");
    return JS_FALSE;
  }

  
  RootedObject result(cx, CData::Create(cx, obj, NullPtr(), NULL, true));
  if (!result)
    return JS_FALSE;

  if (args.length() == 1) {
    if (!ExplicitConvert(cx, args[0], obj, CData::GetData(result)))
      return JS_FALSE;
  }

  args.rval().setObject(*result);
  return JS_TRUE;
}

JSObject*
CType::Create(JSContext* cx,
              HandleObject typeProto,
              HandleObject dataProto,
              TypeCode type,
              JSString* name_,
              jsval size,
              jsval align,
              ffi_type* ffiType)
{
  RootedString name(cx, name_);
  RootedObject parent(cx, JS_GetParent(typeProto));
  JS_ASSERT(parent);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  RootedObject typeObj(cx, JS_NewObject(cx, &sCTypeClass, typeProto, parent));
  if (!typeObj)
    return NULL;

  
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
      return NULL;

    if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(typeObj),
           NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT))
      return NULL;

    
    
    
    JS_SetReservedSlot(typeObj, SLOT_PROTO, OBJECT_TO_JSVAL(prototype));
  }

  if (!JS_FreezeObject(cx, typeObj))
    return NULL;

  
  
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
                     jsval size,
                     jsval align,
                     ffi_type* ffiType)
{
  RootedObject parent(cx, parent_);
  RootedObject typeProto(cx, typeProto_);
  RootedObject dataProto(cx, dataProto_);

  RootedString nameStr(cx, JS_NewStringCopyZ(cx, name));
  if (!nameStr)
    return NULL;

  
  RootedObject typeObj(cx, Create(cx, typeProto, dataProto, type, nameStr, size, align, ffiType));
  if (!typeObj)
    return NULL;

  
  if (!JS_DefineProperty(cx, parent, propName, OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return NULL;

  return typeObj;
}

void
CType::Finalize(JSFreeOp *fop, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_TYPECODE);
  if (JSVAL_IS_VOID(slot))
    return;

  
  switch (TypeCode(JSVAL_TO_INT(slot))) {
  case TYPE_function: {
    
    slot = JS_GetReservedSlot(obj, SLOT_FNINFO);
    if (!JSVAL_IS_VOID(slot))
      FreeOp::get(fop)->delete_(static_cast<FunctionInfo*>(JSVAL_TO_PRIVATE(slot)));
    break;
  }

  case TYPE_struct: {
    
    slot = JS_GetReservedSlot(obj, SLOT_FIELDINFO);
    if (!JSVAL_IS_VOID(slot)) {
      void* info = JSVAL_TO_PRIVATE(slot);
      FreeOp::get(fop)->delete_(static_cast<FieldInfoHash*>(info));
    }
  }

    
  case TYPE_array: {
    
    slot = JS_GetReservedSlot(obj, SLOT_FFITYPE);
    if (!JSVAL_IS_VOID(slot)) {
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
CType::FinalizeProtoClass(JSFreeOp *fop, JSObject* obj)
{
  
  
  
  
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_CLOSURECX);
  if (JSVAL_IS_VOID(slot))
    return;

  JSContext* closureCx = static_cast<JSContext*>(JSVAL_TO_PRIVATE(slot));
  JS_DestroyContextNoGC(closureCx);
}

void
CType::Trace(JSTracer* trc, JSObject* obj)
{
  
  jsval slot = obj->getSlot(SLOT_TYPECODE);
  if (JSVAL_IS_VOID(slot))
    return;

  
  switch (TypeCode(JSVAL_TO_INT(slot))) {
  case TYPE_struct: {
    slot = obj->getReservedSlot(SLOT_FIELDINFO);
    if (JSVAL_IS_VOID(slot))
      return;

    FieldInfoHash* fields =
      static_cast<FieldInfoHash*>(JSVAL_TO_PRIVATE(slot));
    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
      JS_CallStringTracer(trc, r.front().key, "fieldName");
      JS_CallObjectTracer(trc, r.front().value.mType, "fieldType");
    }

    break;
  }
  case TYPE_function: {
    
    slot = obj->getReservedSlot(SLOT_FNINFO);
    if (JSVAL_IS_VOID(slot))
      return;

    FunctionInfo* fninfo = static_cast<FunctionInfo*>(JSVAL_TO_PRIVATE(slot));
    JS_ASSERT(fninfo);

    
    JS_CallObjectTracer(trc, fninfo->mABI, "abi");
    JS_CallObjectTracer(trc, fninfo->mReturnType, "returnType");
    for (size_t i = 0; i < fninfo->mArgTypes.length(); ++i)
      JS_CallObjectTracer(trc, fninfo->mArgTypes[i], "argType");

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
  return TypeCode(JSVAL_TO_INT(result));
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

  
  
  if (JSVAL_IS_INT(size)) {
    *result = JSVAL_TO_INT(size);
    return true;
  }
  if (JSVAL_IS_DOUBLE(size)) {
    *result = Convert<size_t>(JSVAL_TO_DOUBLE(size));
    return true;
  }

  JS_ASSERT(JSVAL_IS_VOID(size));
  return false;
}

size_t
CType::GetSize(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval size = JS_GetReservedSlot(obj, SLOT_SIZE);

  JS_ASSERT(!JSVAL_IS_VOID(size));

  
  
  
  if (JSVAL_IS_INT(size))
    return JSVAL_TO_INT(size);
  return Convert<size_t>(JSVAL_TO_DOUBLE(size));
}

bool
CType::IsSizeDefined(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval size = JS_GetReservedSlot(obj, SLOT_SIZE);

  
  
  JS_ASSERT(JSVAL_IS_INT(size) || JSVAL_IS_DOUBLE(size) || JSVAL_IS_VOID(size));
  return !JSVAL_IS_VOID(size);
}

size_t
CType::GetAlignment(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_ALIGN);
  return static_cast<size_t>(JSVAL_TO_INT(slot));
}

ffi_type*
CType::GetFFIType(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_FFITYPE);

  if (!JSVAL_IS_VOID(slot)) {
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
    JS_NOT_REACHED("simple types must have an ffi_type");
  }

  if (!result)
    return NULL;
  JS_SetReservedSlot(obj, SLOT_FFITYPE, PRIVATE_TO_JSVAL(result.get()));
  return result.forget();
}

JSString*
CType::GetName(JSContext* cx, JSHandleObject obj)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval string = JS_GetReservedSlot(obj, SLOT_NAME);
  if (!JSVAL_IS_VOID(string))
    return JSVAL_TO_STRING(string);

  
  JSString* name = BuildTypeName(cx, obj);
  if (!name)
    return NULL;
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
CType::GetProtoFromType(JSContext* cx, JSObject* obj, CTypeProtoSlot slot)
{
  JS_ASSERT(IsCType(obj));

  
  JSObject* proto;
  if (!JS_GetPrototype(cx, obj, &proto))
    return NULL;
  JS_ASSERT(proto);
  JS_ASSERT(CType::IsCTypeProto(proto));

  
  jsval result = JS_GetReservedSlot(proto, slot);
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(result));
  return JSVAL_TO_OBJECT(result);
}

JSBool
CType::PrototypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!(CType::IsCType(obj) || CType::IsCTypeProto(obj))) {
    JS_ReportError(cx, "not a CType or CTypeProto");
    return JS_FALSE;
  }

  unsigned slot = CType::IsCTypeProto(obj) ? (unsigned) SLOT_OURDATAPROTO
                                           : (unsigned) SLOT_PROTO;
  vp.set(JS_GetReservedSlot(obj, slot));
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp) || JSVAL_IS_VOID(vp));
  return JS_TRUE;
}

JSBool
CType::NameGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CType::IsCType(obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  JSString* name = CType::GetName(cx, obj);
  if (!name)
    return JS_FALSE;

  vp.setString(name);
  return JS_TRUE;
}

JSBool
CType::SizeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CType::IsCType(obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_SIZE));
  JS_ASSERT(JSVAL_IS_NUMBER(vp) || JSVAL_IS_VOID(vp));
  return JS_TRUE;
}

JSBool
CType::PtrGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CType::IsCType(obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  JSObject* pointerType = PointerType::CreateInternal(cx, obj);
  if (!pointerType)
    return JS_FALSE;

  vp.setObject(*pointerType);
  return JS_TRUE;
}

JSBool
CType::CreateArray(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject baseType(cx, JS_THIS_OBJECT(cx, vp));
  if (!baseType)
    return JS_FALSE;
  if (!CType::IsCType(baseType)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
  }

  
  if (args.length() > 1) {
    JS_ReportError(cx, "array takes zero or one argument");
    return JS_FALSE;
  }

  
  size_t length = 0;
  if (args.length() == 1 && !jsvalToSize(cx, args[0], false, &length)) {
    JS_ReportError(cx, "argument must be a nonnegative integer");
    return JS_FALSE;
  }

  JSObject* result = ArrayType::CreateInternal(cx, baseType, length, args.length() == 1);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);
  return JS_TRUE;
}

JSBool
CType::ToString(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return JS_FALSE;
  if (!CType::IsCType(obj) && !CType::IsCTypeProto(obj)) {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
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
    return JS_FALSE;

  args.rval().setString(result);
  return JS_TRUE;
}

JSBool
CType::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CType::IsCType(obj) && !CType::IsCTypeProto(obj))
  {
    JS_ReportError(cx, "not a CType");
    return JS_FALSE;
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
    return JS_FALSE;

  args.rval().setString(result);
  return JS_TRUE;
}

JSBool
CType::HasInstance(JSContext* cx, JSHandleObject obj, JSMutableHandleValue v, JSBool* bp)
{
  JS_ASSERT(CType::IsCType(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_PROTO);
  JSObject* prototype = &slot.toObject();
  JS_ASSERT(prototype);
  JS_ASSERT(CData::IsCDataProto(prototype));

  *bp = JS_FALSE;
  if (JSVAL_IS_PRIMITIVE(v))
    return JS_TRUE;

  JSObject* proto = &v.toObject();
  for (;;) {
    if (!JS_GetPrototype(cx, proto, &proto))
      return JS_FALSE;
    if (!proto)
      break;
    if (proto == prototype) {
      *bp = JS_TRUE;
      break;
    }
  }
  return JS_TRUE;
}

static JSObject*
CType::GetGlobalCTypes(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));

  JSObject *objTypeProto;
  if (!JS_GetPrototype(cx, obj, &objTypeProto))
    return NULL;
  JS_ASSERT(objTypeProto);
  JS_ASSERT(CType::IsCTypeProto(objTypeProto));

  jsval valCTypes = JS_GetReservedSlot(objTypeProto, SLOT_CTYPES);
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(valCTypes));

  JS_ASSERT(!JSVAL_IS_PRIMITIVE(valCTypes));
  return &valCTypes.toObject();
}





bool
ABI::IsABI(JSObject* obj)
{
  return JS_GetClass(obj) == &sCABIClass;
}

JSBool
ABI::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return JS_FALSE;
  }
  
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!ABI::IsABI(obj)) {
    JS_ReportError(cx, "not an ABI");
    return JS_FALSE;
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
      return JS_FALSE;
  }
  if (!result)
    return JS_FALSE;
  
  args.rval().setString(result);
  return JS_TRUE;
}






JSBool
PointerType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  if (args.length() != 1) {
    JS_ReportError(cx, "PointerType takes one argument");
    return JS_FALSE;
  }

  jsval arg = args[0];
  RootedObject obj(cx);
  if (JSVAL_IS_PRIMITIVE(arg) || !CType::IsCType(obj = &arg.toObject())) {
    JS_ReportError(cx, "first argument must be a CType");
    return JS_FALSE;
  }

  JSObject* result = CreateInternal(cx, obj);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);
  return JS_TRUE;
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
    return NULL;
  RootedObject typeProto(cx, CType::GetProtoFromType(cx, baseType, SLOT_POINTERPROTO));
  if (!typeProto)
    return NULL;

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_pointer,
                        NULL, INT_TO_JSVAL(sizeof(void*)),
                        INT_TO_JSVAL(ffi_type_pointer.alignment),
                        &ffi_type_pointer);
  if (!typeObj)
    return NULL;

  
  JS_SetReservedSlot(typeObj, SLOT_TARGET_T, OBJECT_TO_JSVAL(baseType));

  
  JS_SetReservedSlot(baseType, SLOT_PTR, OBJECT_TO_JSVAL(typeObj));

  return typeObj;
}

JSBool
PointerType::ConstructData(JSContext* cx,
                           JSHandleObject obj,
                           const CallArgs& args)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  if (args.length() > 3) {
    JS_ReportError(cx, "constructor takes 0, 1, 2, or 3 arguments");
    return JS_FALSE;
  }

  RootedObject result(cx, CData::Create(cx, obj, NullPtr(), NULL, true));
  if (!result)
    return JS_FALSE;

  
  args.rval().setObject(*result);

  
  
  
  
  
  
  

  
  
  
  if (args.length() == 0)
    return JS_TRUE;

  
  RootedObject baseObj(cx, PointerType::GetBaseType(obj));
  bool looksLikeClosure = CType::GetTypeCode(baseObj) == TYPE_function &&
                          args[0].isObject() &&
                          JS_ObjectIsCallable(cx, &args[0].toObject());

  
  
  
  if (!looksLikeClosure) {
    if (args.length() != 1) {
      JS_ReportError(cx, "first argument must be a function");
      return JS_FALSE;
    }
    return ExplicitConvert(cx, args[0], obj, CData::GetData(result));
  }

  
  
  

  
  
  
  RootedObject thisObj(cx, NULL);
  if (args.length() >= 2) {
    if (args[1].isNull()) {
      thisObj = NULL;
    } else if (!JSVAL_IS_PRIMITIVE(args[1])) {
      thisObj = &args[1].toObject();
    } else if (!JS_ValueToObject(cx, args[1], thisObj.address())) {
      return JS_FALSE;
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

JSBool
PointerType::TargetTypeGetter(JSContext* cx,
                              JSHandleObject obj,
                              JSHandleId idval,
                              MutableHandleValue vp)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_TARGET_T));
  JS_ASSERT(vp.isObject());
  return JS_TRUE;
}

JSBool
PointerType::IsNull(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  void* data = *static_cast<void**>(CData::GetData(obj));
  args.rval().setBoolean(data == NULL);
  return JS_TRUE;
}

JSBool
PointerType::OffsetBy(JSContext* cx, const CallArgs& args, int offset)
{
  JSObject* obj = JS_THIS_OBJECT(cx, args.base());
  if (!obj)
    return JS_FALSE;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  RootedObject baseType(cx, PointerType::GetBaseType(typeObj));
  if (!CType::IsSizeDefined(baseType)) {
    JS_ReportError(cx, "cannot modify pointer of undefined size");
    return JS_FALSE;
  }

  size_t elementSize = CType::GetSize(baseType);
  char* data = static_cast<char*>(*static_cast<void**>(CData::GetData(obj)));
  void* address = data + offset * elementSize;

  
  JSObject* result = CData::Create(cx, typeObj, NullPtr(), &address, true);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);
  return JS_TRUE;
}

JSBool
PointerType::Increment(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  return OffsetBy(cx, args, 1);
}

JSBool
PointerType::Decrement(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  return OffsetBy(cx, args, -1);
}

JSBool
PointerType::ContentsGetter(JSContext* cx,
                            JSHandleObject obj,
                            JSHandleId idval,
                            MutableHandleValue vp)
{
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  RootedObject baseType(cx, GetBaseType(typeObj));
  if (!CType::IsSizeDefined(baseType)) {
    JS_ReportError(cx, "cannot get contents of undefined size");
    return JS_FALSE;
  }

  void* data = *static_cast<void**>(CData::GetData(obj));
  if (data == NULL) {
    JS_ReportError(cx, "cannot read contents of null pointer");
    return JS_FALSE;
  }

  jsval result;
  if (!ConvertToJS(cx, baseType, NullPtr(), data, false, false, &result))
    return JS_FALSE;

  vp.set(result);
  return JS_TRUE;
}

JSBool
PointerType::ContentsSetter(JSContext* cx,
                            JSHandleObject obj,
                            JSHandleId idval,
                            JSBool strict,
                            MutableHandleValue vp)
{
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_pointer) {
    JS_ReportError(cx, "not a PointerType");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(typeObj);
  if (!CType::IsSizeDefined(baseType)) {
    JS_ReportError(cx, "cannot set contents of undefined size");
    return JS_FALSE;
  }

  void* data = *static_cast<void**>(CData::GetData(obj));
  if (data == NULL) {
    JS_ReportError(cx, "cannot write contents to null pointer");
    return JS_FALSE;
  }

  return ImplicitConvert(cx, vp, baseType, data, false, NULL);
}





JSBool
ArrayType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  
  if (args.length() < 1 || args.length() > 2) {
    JS_ReportError(cx, "ArrayType takes one or two arguments");
    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(args[0]) ||
      !CType::IsCType(&args[0].toObject())) {
    JS_ReportError(cx, "first argument must be a CType");
    return JS_FALSE;
  }

  
  size_t length = 0;
  if (args.length() == 2 && !jsvalToSize(cx, args[1], false, &length)) {
    JS_ReportError(cx, "second argument must be a nonnegative integer");
    return JS_FALSE;
  }

  RootedObject baseType(cx, &args[0].toObject());
  JSObject* result = CreateInternal(cx, baseType, length, args.length() == 2);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);
  return JS_TRUE;
}

JSObject*
ArrayType::CreateInternal(JSContext* cx,
                          HandleObject baseType,
                          size_t length,
                          bool lengthDefined)
{
  
  
  RootedObject typeProto(cx, CType::GetProtoFromType(cx, baseType, SLOT_ARRAYPROTO));
  if (!typeProto)
    return NULL;
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, baseType, SLOT_ARRAYDATAPROTO));
  if (!dataProto)
    return NULL;

  
  
  
  size_t baseSize;
  if (!CType::GetSafeSize(baseType, &baseSize)) {
    JS_ReportError(cx, "base size must be defined");
    return NULL;
  }

  jsval sizeVal = JSVAL_VOID;
  jsval lengthVal = JSVAL_VOID;
  if (lengthDefined) {
    
    size_t size = length * baseSize;
    if (length > 0 && size / length != baseSize) {
      JS_ReportError(cx, "size overflow");
      return NULL;
    }
    if (!SizeTojsval(cx, size, &sizeVal) ||
        !SizeTojsval(cx, length, &lengthVal))
      return NULL;
  }

  size_t align = CType::GetAlignment(baseType);

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_array, NULL,
                        sizeVal, INT_TO_JSVAL(align), NULL);
  if (!typeObj)
    return NULL;

  
  JS_SetReservedSlot(typeObj, SLOT_ELEMENT_T, OBJECT_TO_JSVAL(baseType));

  
  JS_SetReservedSlot(typeObj, SLOT_LENGTH, lengthVal);

  return typeObj;
}

JSBool
ArrayType::ConstructData(JSContext* cx,
                         JSHandleObject obj_,
                         const CallArgs& args)
{
  RootedObject obj(cx, obj_); 

  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  
  
  bool convertObject = args.length() == 1;

  
  
  if (CType::IsSizeDefined(obj)) {
    if (args.length() > 1) {
      JS_ReportError(cx, "constructor takes zero or one argument");
      return JS_FALSE;
    }

  } else {
    if (args.length() != 1) {
      JS_ReportError(cx, "constructor takes one argument");
      return JS_FALSE;
    }

    RootedObject baseType(cx, GetBaseType(obj));

    size_t length;
    if (jsvalToSize(cx, args[0], false, &length)) {
      
      convertObject = false;

    } else if (!JSVAL_IS_PRIMITIVE(args[0])) {
      
      
      RootedObject arg(cx, &args[0].toObject());
      js::AutoValueRooter lengthVal(cx);
      if (!JS_GetProperty(cx, arg, "length", lengthVal.jsval_addr()) ||
          !jsvalToSize(cx, lengthVal.jsval_value(), false, &length)) {
        JS_ReportError(cx, "argument must be an array object or length");
        return JS_FALSE;
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
      return JS_FALSE;
    }

    
    obj = CreateInternal(cx, baseType, length, true);
    if (!obj)
      return JS_FALSE;
  }

  
  js::AutoObjectRooter root(cx, obj);

  JSObject* result = CData::Create(cx, obj, NullPtr(), NULL, true);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);

  if (convertObject) {
    if (!ExplicitConvert(cx, args[0], obj, CData::GetData(result)))
      return JS_FALSE;
  }

  return JS_TRUE;
}

JSObject*
ArrayType::GetBaseType(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_array);

  jsval type = JS_GetReservedSlot(obj, SLOT_ELEMENT_T);
  JS_ASSERT(!JSVAL_IS_NULL(type));
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
    return NULL;

  size_t length = ArrayType::GetLength(obj);

  
  
  
  
  
  
  
  AutoPtr<ffi_type> ffiType(cx->new_<ffi_type>());
  if (!ffiType) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }

  ffiType->type = FFI_TYPE_STRUCT;
  ffiType->size = CType::GetSize(obj);
  ffiType->alignment = CType::GetAlignment(obj);
  ffiType->elements = cx->pod_malloc<ffi_type*>(length + 1);
  if (!ffiType->elements) {
    JS_ReportAllocationOverflow(cx);
    return NULL;
  }

  for (size_t i = 0; i < length; ++i)
    ffiType->elements[i] = ffiBaseType;
  ffiType->elements[length] = NULL;

  return ffiType.forget();
}

JSBool
ArrayType::ElementTypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_ELEMENT_T));
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp));
  return JS_TRUE;
}

JSBool
ArrayType::LengthGetter(JSContext* cx, JSHandleObject obj_, JSHandleId idval, MutableHandleValue vp)
{
  JSObject *obj = obj_;

  
  
  if (CData::IsCData(obj))
    obj = CData::GetCType(obj);

  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_LENGTH));
  JS_ASSERT(vp.isNumber() || vp.isUndefined());
  return JS_TRUE;
}

JSBool
ArrayType::Getter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_array)
    return JS_TRUE;

  
  size_t index;
  size_t length = GetLength(typeObj);
  bool ok = jsidToSize(cx, idval, true, &index);
  int32_t dummy;
  if (!ok && JSID_IS_STRING(idval) && !StringToInteger(cx, JSID_TO_STRING(idval), &dummy)) {
    
    
    return JS_TRUE;
  }
  if (!ok || index >= length) {
    JS_ReportError(cx, "invalid index");
    return JS_FALSE;
  }

  RootedObject baseType(cx, GetBaseType(typeObj));
  size_t elementSize = CType::GetSize(baseType);
  char* data = static_cast<char*>(CData::GetData(obj)) + elementSize * index;
  return ConvertToJS(cx, baseType, obj, data, false, false, vp.address());
}

JSBool
ArrayType::Setter(JSContext* cx, JSHandleObject obj, JSHandleId idval, JSBool strict, MutableHandleValue vp)
{
  
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  
  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_array)
    return JS_TRUE;

  
  size_t index;
  size_t length = GetLength(typeObj);
  bool ok = jsidToSize(cx, idval, true, &index);
  int32_t dummy;
  if (!ok && JSID_IS_STRING(idval) && !StringToInteger(cx, JSID_TO_STRING(idval), &dummy)) {
    
    
    return JS_TRUE;
  }
  if (!ok || index >= length) {
    JS_ReportError(cx, "invalid index");
    return JS_FALSE;
  }

  JSObject* baseType = GetBaseType(typeObj);
  size_t elementSize = CType::GetSize(baseType);
  char* data = static_cast<char*>(CData::GetData(obj)) + elementSize * index;
  return ImplicitConvert(cx, vp, baseType, data, false, NULL);
}

JSBool
ArrayType::AddressOfElement(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  if (CType::GetTypeCode(typeObj) != TYPE_array) {
    JS_ReportError(cx, "not an ArrayType");
    return JS_FALSE;
  }

  if (args.length() != 1) {
    JS_ReportError(cx, "addressOfElement takes one argument");
    return JS_FALSE;
  }

  RootedObject baseType(cx, GetBaseType(typeObj));
  RootedObject pointerType(cx, PointerType::CreateInternal(cx, baseType));
  if (!pointerType)
    return JS_FALSE;

  
  JSObject* result = CData::Create(cx, pointerType, NullPtr(), NULL, true);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);

  
  size_t index;
  size_t length = GetLength(typeObj);
  if (!jsvalToSize(cx, args[0], false, &index) ||
      index >= length) {
    JS_ReportError(cx, "invalid index");
    return JS_FALSE;
  }

  
  void** data = static_cast<void**>(CData::GetData(result));
  size_t elementSize = CType::GetSize(baseType);
  *data = static_cast<char*>(CData::GetData(obj)) + elementSize * index;
  return JS_TRUE;
}







static JSFlatString*
ExtractStructField(JSContext* cx, jsval val, JSObject** typeObj)
{
  if (JSVAL_IS_PRIMITIVE(val)) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return NULL;
  }

  RootedObject obj(cx, JSVAL_TO_OBJECT(val));
  RootedObject iter(cx, JS_NewPropertyIterator(cx, obj));
  if (!iter)
    return NULL;
  js::AutoObjectRooter iterroot(cx, iter);

  jsid nameid;
  if (!JS_NextProperty(cx, iter, &nameid))
    return NULL;
  if (JSID_IS_VOID(nameid)) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return NULL;
  }

  if (!JSID_IS_STRING(nameid)) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return NULL;
  }

  
  jsid id;
  if (!JS_NextProperty(cx, iter, &id))
    return NULL;
  if (!JSID_IS_VOID(id)) {
    JS_ReportError(cx, "struct field descriptors must contain one property");
    return NULL;
  }

  js::AutoValueRooter propVal(cx);
  if (!JS_GetPropertyById(cx, obj, nameid, propVal.jsval_addr()))
    return NULL;

  if (propVal.value().isPrimitive() ||
      !CType::IsCType(JSVAL_TO_OBJECT(propVal.jsval_value()))) {
    JS_ReportError(cx, "struct field descriptors require a valid name and type");
    return NULL;
  }

  
  
  
  *typeObj = JSVAL_TO_OBJECT(propVal.jsval_value());
  size_t size;
  if (!CType::GetSafeSize(*typeObj, &size) || size == 0) {
    JS_ReportError(cx, "struct field types must have defined and nonzero size");
    return NULL;
  }

  return JSID_TO_FLAT_STRING(nameid);
}



static JSBool
AddFieldToArray(JSContext* cx,
                jsval* element,
                JSFlatString* name_,
                JSObject* typeObj_)
{
  RootedObject typeObj(cx, typeObj_);
  Rooted<JSFlatString*> name(cx, name_);
  RootedObject fieldObj(cx, JS_NewObject(cx, NULL, NULL, NULL));
  if (!fieldObj)
    return false;

  *element = OBJECT_TO_JSVAL(fieldObj);

  if (!JS_DefineUCProperty(cx, fieldObj,
         name->chars(), name->length(),
         OBJECT_TO_JSVAL(typeObj), NULL, NULL,
         JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  return JS_FreezeObject(cx, fieldObj);
}

JSBool
StructType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);

  
  if (args.length() < 1 || args.length() > 2) {
    JS_ReportError(cx, "StructType takes one or two arguments");
    return JS_FALSE;
  }

  jsval name = args[0];
  if (!name.isString()) {
    JS_ReportError(cx, "first argument must be a string");
    return JS_FALSE;
  }

  
  RootedObject typeProto(cx, CType::GetProtoFromCtor(&args.callee(), SLOT_STRUCTPROTO));

  
  
  
  RootedObject result(cx, CType::Create(cx, typeProto, NullPtr(), TYPE_struct,
                                        JSVAL_TO_STRING(name), JSVAL_VOID, JSVAL_VOID, NULL));
  if (!result)
    return JS_FALSE;

  if (args.length() == 2) {
    RootedObject arr(cx, JSVAL_IS_PRIMITIVE(args[1]) ? NULL : &args[1].toObject());
    if (!arr || !JS_IsArrayObject(cx, arr)) {
      JS_ReportError(cx, "second argument must be an array");
      return JS_FALSE;
    }

    
    if (!DefineInternal(cx, result, arr))
      return JS_FALSE;
  }

  args.rval().setObject(*result);
  return JS_TRUE;
}

JSBool
StructType::DefineInternal(JSContext* cx, JSObject* typeObj_, JSObject* fieldsObj_)
{
  RootedObject typeObj(cx, typeObj_);
  RootedObject fieldsObj(cx, fieldsObj_);

  uint32_t len;
  ASSERT_OK(JS_GetArrayLength(cx, fieldsObj, &len));

  
  
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, typeObj, SLOT_STRUCTDATAPROTO));
  if (!dataProto)
    return JS_FALSE;

  
  
  
  RootedObject prototype(cx, JS_NewObject(cx, &sCDataProtoClass, dataProto, NULL));
  if (!prototype)
    return JS_FALSE;

  if (!JS_DefineProperty(cx, prototype, "constructor", OBJECT_TO_JSVAL(typeObj),
         NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT))
    return JS_FALSE;

  
  
  
  
  AutoPtr<FieldInfoHash> fields(cx->new_<FieldInfoHash>());
  Array<jsval, 16> fieldRootsArray;
  if (!fields || !fields->init(len) || !fieldRootsArray.appendN(JSVAL_VOID, len)) {
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
  }
  js::AutoArrayRooter fieldRoots(cx, fieldRootsArray.length(), 
    fieldRootsArray.begin());

  
  size_t structSize, structAlign;
  if (len != 0) {
    structSize = 0;
    structAlign = 0;

    for (uint32_t i = 0; i < len; ++i) {
      js::AutoValueRooter item(cx);
      if (!JS_GetElement(cx, fieldsObj, i, item.jsval_addr()))
        return JS_FALSE;

      RootedObject fieldType(cx, NULL);
      JSFlatString* flat = ExtractStructField(cx, item.jsval_value(), fieldType.address());
      if (!flat)
        return JS_FALSE;
      Rooted<JSStableString*> name(cx, flat->ensureStable(cx));
      if (!name)
        return JS_FALSE;
      fieldRootsArray[i] = OBJECT_TO_JSVAL(fieldType);

      
      FieldInfoHash::AddPtr entryPtr = fields->lookupForAdd(name);
      if (entryPtr) {
        JS_ReportError(cx, "struct fields must have unique names");
        return JS_FALSE;
      }
      ASSERT_OK(fields->add(entryPtr, name, FieldInfo()));
      FieldInfo& info = entryPtr->value;
      info.mType = fieldType;
      info.mIndex = i;

      
      if (!JS_DefineUCProperty(cx, prototype,
             name->chars().get(), name->length(), JSVAL_VOID,
             StructType::FieldGetter, StructType::FieldSetter,
             JSPROP_SHARED | JSPROP_ENUMERATE | JSPROP_PERMANENT))
        return JS_FALSE;

      size_t fieldSize = CType::GetSize(fieldType);
      size_t fieldAlign = CType::GetAlignment(fieldType);
      size_t fieldOffset = Align(structSize, fieldAlign);
      
      
      
      if (fieldOffset + fieldSize < structSize) {
        JS_ReportError(cx, "size overflow");
        return JS_FALSE;
      }
      info.mOffset = fieldOffset;
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
  }

  jsval sizeVal;
  if (!SizeTojsval(cx, structSize, &sizeVal))
    return JS_FALSE;

  JS_SetReservedSlot(typeObj, SLOT_FIELDINFO, PRIVATE_TO_JSVAL(fields.forget()));

  JS_SetReservedSlot(typeObj, SLOT_SIZE, sizeVal);
  JS_SetReservedSlot(typeObj, SLOT_ALIGN, INT_TO_JSVAL(structAlign));
  
  
  JS_SetReservedSlot(typeObj, SLOT_PROTO, OBJECT_TO_JSVAL(prototype));
  return JS_TRUE;
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
    return NULL;
  }
  ffiType->type = FFI_TYPE_STRUCT;

  AutoPtr<ffi_type*> elements;
  if (len != 0) {
    elements = cx->pod_malloc<ffi_type*>(len + 1);
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return NULL;
    }
    elements[len] = NULL;

    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
      const FieldInfoHash::Entry& entry = r.front();
      ffi_type* fieldType = CType::GetFFIType(cx, entry.value.mType);
      if (!fieldType)
        return NULL;
      elements[entry.value.mIndex] = fieldType;
    }

  } else {
    
    JS_ASSERT(structSize == 1);
    JS_ASSERT(structAlign == 1);
    elements = cx->pod_malloc<ffi_type*>(2);
    if (!elements) {
      JS_ReportOutOfMemory(cx);
      return NULL;
    }
    elements[0] = &ffi_type_uint8;
    elements[1] = NULL;
  }

  ffiType->elements = elements.get();

#ifdef DEBUG
  
  
  
  ffi_cif cif;
  ffiType->size = 0;
  ffiType->alignment = 0;
  ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, ffiType.get(), NULL);
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

JSBool
StructType::Define(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CType::IsCType(obj) ||
      CType::GetTypeCode(obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  if (CType::IsSizeDefined(obj)) {
    JS_ReportError(cx, "StructType has already been defined");
    return JS_FALSE;
  }

  if (args.length() != 1) {
    JS_ReportError(cx, "define takes one argument");
    return JS_FALSE;
  }

  jsval arg = args[0];
  if (JSVAL_IS_PRIMITIVE(arg)) {
    JS_ReportError(cx, "argument must be an array");
    return JS_FALSE;
  }
  RootedObject arr(cx, JSVAL_TO_OBJECT(arg));
  if (!JS_IsArrayObject(cx, arr)) {
    JS_ReportError(cx, "argument must be an array");
    return JS_FALSE;
  }

  return DefineInternal(cx, obj, arr);
}

JSBool
StructType::ConstructData(JSContext* cx,
                          HandleObject obj,
                          const CallArgs& args)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  if (!CType::IsSizeDefined(obj)) {
    JS_ReportError(cx, "cannot construct an opaque StructType");
    return JS_FALSE;
  }

  JSObject* result = CData::Create(cx, obj, NullPtr(), NULL, true);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);

  if (args.length() == 0)
    return JS_TRUE;

  char* buffer = static_cast<char*>(CData::GetData(result));
  const FieldInfoHash* fields = GetFieldInfo(obj);

  if (args.length() == 1) {
    
    
    
    
    
    
    

    
    if (ExplicitConvert(cx, args[0], obj, buffer))
      return JS_TRUE;

    if (fields->count() != 1)
      return JS_FALSE;

    
    
    if (!JS_IsExceptionPending(cx))
      return JS_FALSE;

    
    
    JS_ClearPendingException(cx);

    
  }

  
  
  if (args.length() == fields->count()) {
    for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
      const FieldInfo& field = r.front().value;
      STATIC_ASSUME(field.mIndex < fields->count());  
      if (!ImplicitConvert(cx, args[field.mIndex], field.mType,
             buffer + field.mOffset,
             false, NULL))
        return JS_FALSE;
    }

    return JS_TRUE;
  }

  JS_ReportError(cx, "constructor takes 0, 1, or %u arguments",
    fields->count());
  return JS_FALSE;
}

const FieldInfoHash*
StructType::GetFieldInfo(JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);

  jsval slot = JS_GetReservedSlot(obj, SLOT_FIELDINFO);
  JS_ASSERT(!JSVAL_IS_VOID(slot) && JSVAL_TO_PRIVATE(slot));

  return static_cast<const FieldInfoHash*>(JSVAL_TO_PRIVATE(slot));
}

const FieldInfo*
StructType::LookupField(JSContext* cx, JSObject* obj, JSFlatString *name)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);

  FieldInfoHash::Ptr ptr = GetFieldInfo(obj)->lookup(name);
  if (ptr)
    return &ptr->value;

  JSAutoByteString bytes(cx, name);
  if (!bytes)
    return NULL;

  JS_ReportError(cx, "%s does not name a field", bytes.ptr());
  return NULL;
}

JSObject*
StructType::BuildFieldsArray(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(CType::IsCType(obj));
  JS_ASSERT(CType::GetTypeCode(obj) == TYPE_struct);
  JS_ASSERT(CType::IsSizeDefined(obj));

  const FieldInfoHash* fields = GetFieldInfo(obj);
  size_t len = fields->count();

  
  Array<jsval, 16> fieldsVec;
  if (!fieldsVec.appendN(JSVAL_VOID, len))
    return NULL;
  js::AutoArrayRooter root(cx, fieldsVec.length(), fieldsVec.begin());

  for (FieldInfoHash::Range r = fields->all(); !r.empty(); r.popFront()) {
    const FieldInfoHash::Entry& entry = r.front();
    
    if (!AddFieldToArray(cx, &fieldsVec[entry.value.mIndex],
                         entry.key, entry.value.mType))
      return NULL;
  }

  RootedObject fieldsProp(cx, JS_NewArrayObject(cx, len, fieldsVec.begin()));
  if (!fieldsProp)
    return NULL;

  
  if (!JS_FreezeObject(cx, fieldsProp))
    return NULL;

  return fieldsProp;
}

JSBool
StructType::FieldsArrayGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_FIELDS));

  if (!CType::IsSizeDefined(obj)) {
    JS_ASSERT(JSVAL_IS_VOID(vp));
    return JS_TRUE;
  }

  if (JSVAL_IS_VOID(vp)) {
    
    JSObject* fields = BuildFieldsArray(cx, obj);
    if (!fields)
      return JS_FALSE;
    JS_SetReservedSlot(obj, SLOT_FIELDS, OBJECT_TO_JSVAL(fields));

    vp.setObject(*fields);
  }

  JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp) &&
            JS_IsArrayObject(cx, JSVAL_TO_OBJECT(vp)));
  return JS_TRUE;
}

JSBool
StructType::FieldGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  const FieldInfo* field = LookupField(cx, typeObj, JSID_TO_FLAT_STRING(idval));
  if (!field)
    return JS_FALSE;

  char* data = static_cast<char*>(CData::GetData(obj)) + field->mOffset;
  RootedObject fieldType(cx, field->mType);
  return ConvertToJS(cx, fieldType, obj, data, false, false, vp.address());
}

JSBool
StructType::FieldSetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, JSBool strict, MutableHandleValue vp)
{
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  const FieldInfo* field = LookupField(cx, typeObj, JSID_TO_FLAT_STRING(idval));
  if (!field)
    return JS_FALSE;

  char* data = static_cast<char*>(CData::GetData(obj)) + field->mOffset;
  return ImplicitConvert(cx, vp, field->mType, data, false, NULL);
}

JSBool
StructType::AddressOfField(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  JSObject* typeObj = CData::GetCType(obj);
  if (CType::GetTypeCode(typeObj) != TYPE_struct) {
    JS_ReportError(cx, "not a StructType");
    return JS_FALSE;
  }

  if (args.length() != 1) {
    JS_ReportError(cx, "addressOfField takes one argument");
    return JS_FALSE;
  }

  JSFlatString *str = JS_FlattenString(cx, args[0].toString());
  if (!str)
    return JS_FALSE;

  const FieldInfo* field = LookupField(cx, typeObj, str);
  if (!field)
    return JS_FALSE;

  RootedObject baseType(cx, field->mType);
  RootedObject pointerType(cx, PointerType::CreateInternal(cx, baseType));
  if (!pointerType)
    return JS_FALSE;

  
  JSObject* result = CData::Create(cx, pointerType, NullPtr(), NULL, true);
  if (!result)
    return JS_FALSE;

  args.rval().setObject(*result);

  
  void** data = static_cast<void**>(CData::GetData(result));
  *data = static_cast<char*>(CData::GetData(obj)) + field->mOffset;
  return JS_TRUE;
}






struct AutoValue
{
  AutoValue() : mData(NULL) { }

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
    return mData != NULL;
  }

  void* mData;
};

static bool
GetABI(JSContext* cx, jsval abiType, ffi_abi* result)
{
  if (JSVAL_IS_PRIMITIVE(abiType))
    return false;

  ABICode abi = GetABICode(JSVAL_TO_OBJECT(abiType));

  
  
  
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
  if (JSVAL_IS_PRIMITIVE(type) ||
      !CType::IsCType(JSVAL_TO_OBJECT(type))) {
    JS_ReportError(cx, "not a ctypes type");
    return NULL;
  }

  JSObject* result = JSVAL_TO_OBJECT(type);
  TypeCode typeCode = CType::GetTypeCode(result);

  if (typeCode == TYPE_array) {
    
    
    RootedObject baseType(cx, ArrayType::GetBaseType(result));
    result = PointerType::CreateInternal(cx, baseType);
    if (!result)
      return NULL;

  } else if (typeCode == TYPE_void_t || typeCode == TYPE_function) {
    
    JS_ReportError(cx, "Cannot have void or function argument type");
    return NULL;
  }

  if (!CType::IsSizeDefined(result)) {
    JS_ReportError(cx, "Argument type must have defined size");
    return NULL;
  }

  
  JS_ASSERT(CType::GetSize(result) != 0);

  return result;
}

static JSObject*
PrepareReturnType(JSContext* cx, jsval type)
{
  if (JSVAL_IS_PRIMITIVE(type) ||
      !CType::IsCType(JSVAL_TO_OBJECT(type))) {
    JS_ReportError(cx, "not a ctypes type");
    return NULL;
  }

  JSObject* result = JSVAL_TO_OBJECT(type);
  TypeCode typeCode = CType::GetTypeCode(result);

  
  if (typeCode == TYPE_array || typeCode == TYPE_function) {
    JS_ReportError(cx, "Return type cannot be an array or function");
    return NULL;
  }

  if (typeCode != TYPE_void_t && !CType::IsSizeDefined(result)) {
    JS_ReportError(cx, "Return type must have defined size");
    return NULL;
  }

  
  JS_ASSERT(typeCode == TYPE_void_t || CType::GetSize(result) != 0);

  return result;
}

static JS_ALWAYS_INLINE JSBool
IsEllipsis(JSContext* cx, jsval v, bool* isEllipsis)
{
  *isEllipsis = false;
  if (!JSVAL_IS_STRING(v))
    return true;
  JSString* str = JSVAL_TO_STRING(v);
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

static JSBool
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
    JS_NOT_REACHED("invalid abi");
    break;
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
    return NULL;
  }

  ffi_abi abi;
  if (!GetABI(cx, abiType, &abi)) {
    JS_ReportError(cx, "Invalid ABI specification");
    return NULL;
  }
  fninfo->mABI = JSVAL_TO_OBJECT(abiType);

  
  fninfo->mReturnType = PrepareReturnType(cx, returnType);
  if (!fninfo->mReturnType)
    return NULL;

  
  if (!fninfo->mArgTypes.reserve(argLength) ||
      !fninfo->mFFITypes.reserve(argLength)) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }

  fninfo->mIsVariadic = false;

  for (uint32_t i = 0; i < argLength; ++i) {
    bool isEllipsis;
    if (!IsEllipsis(cx, argTypes[i], &isEllipsis))
      return NULL;
    if (isEllipsis) {
      fninfo->mIsVariadic = true;
      if (i < 1) {
        JS_ReportError(cx, "\"...\" may not be the first and only parameter "
                       "type of a variadic function declaration");
        return NULL;
      }
      if (i < argLength - 1) {
        JS_ReportError(cx, "\"...\" must be the last parameter type of a "
                       "variadic function declaration");
        return NULL;
      }
      if (GetABICode(fninfo->mABI) != ABI_DEFAULT) {
        JS_ReportError(cx, "Variadic functions must use the __cdecl calling "
                       "convention");
        return NULL;
      }
      break;
    }

    JSObject* argType = PrepareType(cx, argTypes[i]);
    if (!argType)
      return NULL;

    ffi_type* ffiType = CType::GetFFIType(cx, argType);
    if (!ffiType)
      return NULL;

    fninfo->mArgTypes.infallibleAppend(argType);
    fninfo->mFFITypes.infallibleAppend(ffiType);
  }

  if (fninfo->mIsVariadic)
    
    return fninfo.forget();

  if (!PrepareCIF(cx, fninfo.get()))
    return NULL;

  return fninfo.forget();
}

JSBool
FunctionType::Create(JSContext* cx, unsigned argc, jsval* vp)
{
  
  if (argc < 2 || argc > 3) {
    JS_ReportError(cx, "FunctionType takes two or three arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  Array<jsval, 16> argTypes;
  RootedObject arrayObj(cx, NULL);

  if (argc == 3) {
    
    if (!JSVAL_IS_PRIMITIVE(argv[2]))
      arrayObj = JSVAL_TO_OBJECT(argv[2]);
    if (!arrayObj || !JS_IsArrayObject(cx, arrayObj)) {
      JS_ReportError(cx, "third argument must be an array");
      return JS_FALSE;
    }

    uint32_t len;
    ASSERT_OK(JS_GetArrayLength(cx, arrayObj, &len));

    if (!argTypes.appendN(JSVAL_VOID, len)) {
      JS_ReportOutOfMemory(cx);
      return JS_FALSE;
    }
  }

  
  JS_ASSERT(!argTypes.length() || arrayObj);
  js::AutoArrayRooter items(cx, argTypes.length(), argTypes.begin());
  for (uint32_t i = 0; i < argTypes.length(); ++i) {
    if (!JS_GetElement(cx, arrayObj, i, &argTypes[i]))
      return JS_FALSE;
  }

  JSObject* result = CreateInternal(cx, argv[0], argv[1],
      argTypes.begin(), argTypes.length());
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
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
    return NULL;

  
  
  RootedObject typeProto(cx, CType::GetProtoFromType(cx, fninfo->mReturnType,
                                                     SLOT_FUNCTIONPROTO));
  if (!typeProto)
    return NULL;
  RootedObject dataProto(cx, CType::GetProtoFromType(cx, fninfo->mReturnType,
                                                     SLOT_FUNCTIONDATAPROTO));
  if (!dataProto)
    return NULL;

  
  JSObject* typeObj = CType::Create(cx, typeProto, dataProto, TYPE_function,
                        NULL, JSVAL_VOID, JSVAL_VOID, NULL);
  if (!typeObj)
    return NULL;
  js::AutoObjectRooter root(cx, typeObj);

  
  JS_SetReservedSlot(typeObj, SLOT_FNINFO, PRIVATE_TO_JSVAL(fninfo.forget()));

  return typeObj;
}




JSBool
FunctionType::ConstructData(JSContext* cx,
                            JSHandleObject typeObj,
                            JSHandleObject dataObj,
                            JSHandleObject fnObj,
                            JSHandleObject thisObj,
                            jsval errVal)
{
  JS_ASSERT(CType::GetTypeCode(typeObj) == TYPE_function);

  PRFuncPtr* data = static_cast<PRFuncPtr*>(CData::GetData(dataObj));

  FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);
  if (fninfo->mIsVariadic) {
    JS_ReportError(cx, "Can't declare a variadic callback function");
    return JS_FALSE;
  }
  if (GetABICode(fninfo->mABI) == ABI_WINAPI) {
    JS_ReportError(cx, "Can't declare a ctypes.winapi_abi callback function, "
                   "use ctypes.stdcall_abi instead");
    return JS_FALSE;
  }

  JSObject* closureObj = CClosure::Create(cx, typeObj, fnObj, thisObj, errVal, data);
  if (!closureObj)
    return JS_FALSE;
  js::AutoObjectRooter root(cx, closureObj);

  
  JS_SetReservedSlot(dataObj, SLOT_REFERENT, OBJECT_TO_JSVAL(closureObj));

  
  
  
  
  
  
  return JS_FreezeObject(cx, dataObj);
}

typedef Array<AutoValue, 16> AutoValueAutoArray;

static JSBool
ConvertArgument(JSContext* cx,
                jsval arg,
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

JSBool
FunctionType::Call(JSContext* cx,
                   unsigned argc,
                   jsval* vp)
{
  
  JSObject* obj = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  if (!CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return false;
  }

  JSObject* typeObj = CData::GetCType(obj);
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

  if ((!fninfo->mIsVariadic && argc != argcFixed) ||
      (fninfo->mIsVariadic && argc < argcFixed)) {
    JS_ReportError(cx, "Number of arguments does not match declaration");
    return false;
  }

  
  jsval slot = JS_GetReservedSlot(obj, SLOT_REFERENT);
  if (!JSVAL_IS_VOID(slot) && Library::IsLibrary(JSVAL_TO_OBJECT(slot))) {
    PRLibrary* library = Library::GetLibrary(JSVAL_TO_OBJECT(slot));
    if (!library) {
      JS_ReportError(cx, "library is not open");
      return false;
    }
  }

  
  AutoValueAutoArray values;
  AutoValueAutoArray strings;
  if (!values.resize(argc)) {
    JS_ReportOutOfMemory(cx);
    return false;
  }

  jsval* argv = JS_ARGV(cx, vp);
  for (unsigned i = 0; i < argcFixed; ++i)
    if (!ConvertArgument(cx, argv[i], fninfo->mArgTypes[i], &values[i], &strings))
      return false;

  if (fninfo->mIsVariadic) {
    if (!fninfo->mFFITypes.resize(argc)) {
      JS_ReportOutOfMemory(cx);
      return false;
    }

    JSObject* obj;  
    JSObject* type; 

    for (uint32_t i = argcFixed; i < argc; ++i) {
      if (JSVAL_IS_PRIMITIVE(argv[i]) ||
          !CData::IsCData(obj = JSVAL_TO_OBJECT(argv[i]))) {
        
        
        JS_ReportError(cx, "argument %d of type %s is not a CData object",
                       i, JS_GetTypeName(cx, JS_TypeOfValue(cx, argv[i])));
        return false;
      }
      if (!(type = CData::GetCType(obj)) ||
          !(type = PrepareType(cx, OBJECT_TO_JSVAL(type))) ||
          
          
          !ConvertArgument(cx, argv[i], type, &values[i], &strings) ||
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
#include "typedefs.h"
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
  JS_ASSERT(!JSVAL_IS_VOID(slot) && JSVAL_TO_PRIVATE(slot));

  return static_cast<FunctionInfo*>(JSVAL_TO_PRIVATE(slot));
}

static JSBool
CheckFunctionType(JSContext* cx, JSObject* obj)
{
  if (!CType::IsCType(obj) || CType::GetTypeCode(obj) != TYPE_function) {
    JS_ReportError(cx, "not a FunctionType");
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool
FunctionType::ArgTypesGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CheckFunctionType(cx, obj))
    return JS_FALSE;

  
  vp.set(JS_GetReservedSlot(obj, SLOT_ARGS_T));
  if (!JSVAL_IS_VOID(vp))
    return JS_TRUE;

  FunctionInfo* fninfo = GetFunctionInfo(obj);
  size_t len = fninfo->mArgTypes.length();

  
  Array<jsval, 16> vec;
  if (!vec.resize(len))
    return JS_FALSE;

  for (size_t i = 0; i < len; ++i)
    vec[i] = OBJECT_TO_JSVAL(fninfo->mArgTypes[i]);

  RootedObject argTypes(cx, JS_NewArrayObject(cx, len, vec.begin()));
  if (!argTypes)
    return JS_FALSE;

  
  if (!JS_FreezeObject(cx, argTypes))
    return JS_FALSE;
  JS_SetReservedSlot(obj, SLOT_ARGS_T, OBJECT_TO_JSVAL(argTypes));

  vp.set(OBJECT_TO_JSVAL(argTypes));
  return JS_TRUE;
}

JSBool
FunctionType::ReturnTypeGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CheckFunctionType(cx, obj))
    return JS_FALSE;

  
  vp.set(OBJECT_TO_JSVAL(GetFunctionInfo(obj)->mReturnType));
  return JS_TRUE;
}

JSBool
FunctionType::ABIGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CheckFunctionType(cx, obj))
    return JS_FALSE;

  
  vp.set(OBJECT_TO_JSVAL(GetFunctionInfo(obj)->mABI));
  return JS_TRUE;
}

JSBool
FunctionType::IsVariadicGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!CheckFunctionType(cx, obj))
    return JS_FALSE;

  vp.set(BOOLEAN_TO_JSVAL(GetFunctionInfo(obj)->mIsVariadic));
  return JS_TRUE;
}





JSObject*
CClosure::Create(JSContext* cx,
                 HandleObject typeObj,
                 HandleObject fnObj,
                 HandleObject thisObj,
                 jsval errVal,
                 PRFuncPtr* fnptr)
{
  JS_ASSERT(fnObj);

  RootedObject result(cx, JS_NewObject(cx, &sCClosureClass, NULL, NULL));
  if (!result)
    return NULL;

  
  FunctionInfo* fninfo = FunctionType::GetFunctionInfo(typeObj);
  JS_ASSERT(!fninfo->mIsVariadic);
  JS_ASSERT(GetABICode(fninfo->mABI) != ABI_WINAPI);

  AutoPtr<ClosureInfo> cinfo(cx->new_<ClosureInfo>(JS_GetRuntime(cx)));
  if (!cinfo) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }

  
  
  JSObject* proto;
  if (!JS_GetPrototype(cx, typeObj, &proto))
    return NULL;
  JS_ASSERT(proto);
  JS_ASSERT(CType::IsCTypeProto(proto));

  
  jsval slot = JS_GetReservedSlot(proto, SLOT_CLOSURECX);
  if (!JSVAL_IS_VOID(slot)) {
    
    cinfo->cx = static_cast<JSContext*>(JSVAL_TO_PRIVATE(slot));
    JS_ASSERT(cinfo->cx);
  } else {
    
    
    JSRuntime* runtime = JS_GetRuntime(cx);
    cinfo->cx = JS_NewContext(runtime, 8192);
    if (!cinfo->cx) {
      JS_ReportOutOfMemory(cx);
      return NULL;
    }

    JS_SetReservedSlot(proto, SLOT_CLOSURECX, PRIVATE_TO_JSVAL(cinfo->cx));
  }

  
  
  
  
  if (!JSVAL_IS_VOID(errVal)) {

    
    if (CType::GetTypeCode(fninfo->mReturnType) == TYPE_void_t) {
      JS_ReportError(cx, "A void callback can't pass an error sentinel");
      return NULL;
    }

    
    
    JS_ASSERT(CType::IsSizeDefined(fninfo->mReturnType));

    
    size_t rvSize = CType::GetSize(fninfo->mReturnType);
    cinfo->errResult = cx->malloc_(rvSize);
    if (!cinfo->errResult)
      return NULL;

    
    if (!ImplicitConvert(cx, errVal, fninfo->mReturnType, cinfo->errResult,
                         false, NULL))
      return NULL;
  } else {
    cinfo->errResult = NULL;
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
    return NULL;
  }

  ffi_status status = ffi_prep_closure_loc(cinfo->closure, &fninfo->mCIF,
    CClosure::ClosureStub, cinfo.get(), code);
  if (status != FFI_OK) {
    JS_ReportError(cx, "couldn't create closure - libffi error");
    return NULL;
  }

  
  JS_SetReservedSlot(result, SLOT_CLOSUREINFO, PRIVATE_TO_JSVAL(cinfo.forget()));

  
  
  *fnptr = reinterpret_cast<PRFuncPtr>(reinterpret_cast<uintptr_t>(code));
  return result;
}

void
CClosure::Trace(JSTracer* trc, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_CLOSUREINFO);
  if (JSVAL_IS_VOID(slot))
    return;

  ClosureInfo* cinfo = static_cast<ClosureInfo*>(JSVAL_TO_PRIVATE(slot));

  
  
  JS_CallObjectTracer(trc, cinfo->typeObj, "typeObj");
  JS_CallObjectTracer(trc, cinfo->jsfnObj, "jsfnObj");
  if (cinfo->thisObj)
    JS_CallObjectTracer(trc, cinfo->thisObj, "thisObj");
}

void
CClosure::Finalize(JSFreeOp *fop, JSObject* obj)
{
  
  jsval slot = JS_GetReservedSlot(obj, SLOT_CLOSUREINFO);
  if (JSVAL_IS_VOID(slot))
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
  RootedObject jsfnObj(cx, cinfo->jsfnObj);

  JS_AbortIfWrongThread(JS_GetRuntime(cx));

  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, jsfnObj);

  
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
#include "typedefs.h"
      rvSize = Align(rvSize, sizeof(ffi_arg));
      break;
    default:
      break;
    }
    memset(result, 0, rvSize);
  }

  
  js::AutoObjectRooter root(cx, cinfo->closureObj);

  
  Array<jsval, 16> argv;
  if (!argv.appendN(JSVAL_VOID, cif->nargs)) {
    JS_ReportOutOfMemory(cx);
    return;
  }

  js::AutoArrayRooter roots(cx, argv.length(), argv.begin());
  for (uint32_t i = 0; i < cif->nargs; ++i) {
    
    
    RootedObject argType(cx, fninfo->mArgTypes[i]);
    if (!ConvertToJS(cx, argType, NullPtr(), args[i], false, false, &argv[i]))
      return;
  }

  
  
  jsval rval;
  JSBool success = JS_CallFunctionValue(cx, thisObj, OBJECT_TO_JSVAL(jsfnObj),
                                        cif->nargs, argv.begin(), &rval);

  
  
  
  
  
  if (success && cif->rtype != &ffi_type_void)
    success = ImplicitConvert(cx, rval, fninfo->mReturnType, result, false,
                              NULL);

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
#include "typedefs.h"
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
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(slot));

  RootedObject proto(cx, JSVAL_TO_OBJECT(slot));
  RootedObject parent(cx, JS_GetParent(typeObj));
  JS_ASSERT(parent);

  RootedObject dataObj(cx, JS_NewObject(cx, &sCDataClass, proto, parent));
  if (!dataObj)
    return NULL;

  
  JS_SetReservedSlot(dataObj, SLOT_CTYPE, OBJECT_TO_JSVAL(typeObj));

  
  if (refObj)
    JS_SetReservedSlot(dataObj, SLOT_REFERENT, OBJECT_TO_JSVAL(refObj));

  
  JS_SetReservedSlot(dataObj, SLOT_OWNS, BOOLEAN_TO_JSVAL(ownResult));

  
  
  char** buffer = cx->new_<char*>();
  if (!buffer) {
    JS_ReportOutOfMemory(cx);
    return NULL;
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
      return NULL;
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
  if (JSVAL_IS_VOID(slot))
    return;

  JSBool owns = JSVAL_TO_BOOLEAN(slot);

  slot = JS_GetReservedSlot(obj, SLOT_DATA);
  if (JSVAL_IS_VOID(slot))
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
  JSObject* typeObj = JSVAL_TO_OBJECT(slot);
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
CData::IsCDataProto(JSObject* obj)
{
  return JS_GetClass(obj) == &sCDataProtoClass;
}

JSBool
CData::ValueGetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, MutableHandleValue vp)
{
  if (!IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  
  RootedObject ctype(cx, GetCType(obj));
  if (!ConvertToJS(cx, ctype, NullPtr(), GetData(obj), true, false, vp.address()))
    return JS_FALSE;

  return JS_TRUE;
}

JSBool
CData::ValueSetter(JSContext* cx, JSHandleObject obj, JSHandleId idval, JSBool strict, MutableHandleValue vp)
{
  if (!IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  return ImplicitConvert(cx, vp, GetCType(obj), GetData(obj), false, NULL);
}

JSBool
CData::Address(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "address takes zero arguments");
    return JS_FALSE;
  }

  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return JS_FALSE;
  if (!IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
  }

  RootedObject typeObj(cx, CData::GetCType(obj));
  RootedObject pointerType(cx, PointerType::CreateInternal(cx, typeObj));
  if (!pointerType)
    return JS_FALSE;

  
  JSObject* result = CData::Create(cx, pointerType, NullPtr(), NULL, true);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));

  
  void** data = static_cast<void**>(GetData(result));
  *data = GetData(obj);
  return JS_TRUE;
}

JSBool
CData::Cast(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 2) {
    JS_ReportError(cx, "cast takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (JSVAL_IS_PRIMITIVE(argv[0]) ||
      !CData::IsCData(JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "first argument must be a CData");
    return JS_FALSE;
  }
  RootedObject sourceData(cx, JSVAL_TO_OBJECT(argv[0]));
  JSObject* sourceType = CData::GetCType(sourceData);

  if (JSVAL_IS_PRIMITIVE(argv[1]) ||
      !CType::IsCType(JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "second argument must be a CType");
    return JS_FALSE;
  }

  RootedObject targetType(cx, JSVAL_TO_OBJECT(argv[1]));
  size_t targetSize;
  if (!CType::GetSafeSize(targetType, &targetSize) ||
      targetSize > CType::GetSize(sourceType)) {
    JS_ReportError(cx,
      "target CType has undefined or larger size than source CType");
    return JS_FALSE;
  }

  
  
  void* data = CData::GetData(sourceData);
  JSObject* result = CData::Create(cx, targetType, sourceData, data, false);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CData::GetRuntime(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 1) {
    JS_ReportError(cx, "getRuntime takes one argument");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (JSVAL_IS_PRIMITIVE(argv[0]) ||
      !CType::IsCType(JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "first argument must be a CType");
    return JS_FALSE;
  }

  RootedObject targetType(cx, JSVAL_TO_OBJECT(argv[0]));
  size_t targetSize;
  if (!CType::GetSafeSize(targetType, &targetSize) ||
      targetSize != sizeof(void*)) {
    JS_ReportError(cx, "target CType has non-pointer size");
    return JS_FALSE;
  }

  void* data = static_cast<void*>(cx->runtime);
  JSObject* result = CData::Create(cx, targetType, NullPtr(), &data, true);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

typedef bool (*InflateUTF8Method)(JSContext *, const char *, size_t,
                                  jschar *, size_t *);

template <InflateUTF8Method InflateUTF8>
static JSBool
ReadStringCommon(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "readString takes zero arguments");
    return JS_FALSE;
  }

  JSObject* obj = CDataFinalizer::GetCData(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj || !CData::IsCData(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
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
    if (data == NULL) {
      JS_ReportError(cx, "cannot read contents of null pointer");
      return JS_FALSE;
    }
    break;
  case TYPE_array:
    baseType = ArrayType::GetBaseType(typeObj);
    data = CData::GetData(obj);
    maxLength = ArrayType::GetLength(typeObj);
    break;
  default:
    JS_ReportError(cx, "not a PointerType or ArrayType");
    return JS_FALSE;
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

    
    size_t dstlen;
    if (!InflateUTF8(cx, bytes, length, NULL, &dstlen))
      return JS_FALSE;

    jschar* dst =
      static_cast<jschar*>(JS_malloc(cx, (dstlen + 1) * sizeof(jschar)));
    if (!dst)
      return JS_FALSE;

    ASSERT_OK(InflateUTF8(cx, bytes, length, dst, &dstlen));
    dst[dstlen] = 0;

    result = JS_NewUCString(cx, dst, dstlen);
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
CData::ReadString(JSContext* cx, unsigned argc, jsval* vp)
{
  return ReadStringCommon<InflateUTF8StringToBuffer>(cx, argc, vp);
}

JSBool
CData::ReadStringReplaceMalformed(JSContext* cx, unsigned argc, jsval* vp)
{
  return ReadStringCommon<InflateUTF8StringToBufferReplaceInvalid>(cx, argc, vp);
}

JSString *
CData::GetSourceString(JSContext *cx, JSHandleObject typeObj, void *data)
{
  
  
  
  
  
  
  AutoString source;
  BuildTypeSource(cx, typeObj, true, source);
  AppendString(source, "(");
  if (!BuildDataSource(cx, typeObj, data, false, source))
    return NULL;

  AppendString(source, ")");

  return NewUCString(cx, source);
}

JSBool
CData::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return JS_FALSE;
  }

  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CData::IsCData(obj) && !CData::IsCDataProto(obj)) {
    JS_ReportError(cx, "not a CData");
    return JS_FALSE;
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
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
CData::ErrnoGetter(JSContext* cx, JSHandleObject obj, JSHandleId, MutableHandleValue vp)
{
  if (!IsCTypesGlobal(obj)) {
    JS_ReportError(cx, "this is not not global object ctypes");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_ERRNO));
  return JS_TRUE;
}

#if defined(XP_WIN)
JSBool
CData::LastErrorGetter(JSContext* cx, JSHandleObject obj, JSHandleId, MutableHandleValue vp)
{
  if (!IsCTypesGlobal(obj)) {
    JS_ReportError(cx, "not global object ctypes");
    return JS_FALSE;
  }

  vp.set(JS_GetReservedSlot(obj, SLOT_LASTERROR));
  return JS_TRUE;
}
#endif 

JSBool
CDataFinalizer::Methods::ToSource(JSContext *cx, unsigned argc, jsval *vp)
{
  RootedObject objThis(cx, JS_THIS_OBJECT(cx, vp));
  if (!objThis)
    return JS_FALSE;
  if (!CDataFinalizer::IsCDataFinalizer(objThis)) {
    JS_ReportError(cx, "not a CDataFinalizer");
    return JS_FALSE;
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
      return JS_FALSE;
    }

    AutoString source;
    AppendString(source, "ctypes.CDataFinalizer(");
    JSString *srcValue = CData::GetSourceString(cx, objType, p->cargs);
    if (!srcValue) {
      return JS_FALSE;
    }
    AppendString(source, srcValue);
    AppendString(source, ", ");
    jsval valCodePtrType = JS_GetReservedSlot(objThis,
                                              SLOT_DATAFINALIZER_CODETYPE);
    if (JSVAL_IS_PRIMITIVE(valCodePtrType)) {
      return JS_FALSE;
    }

    RootedObject typeObj(cx, JSVAL_TO_OBJECT(valCodePtrType));
    JSString *srcDispose = CData::GetSourceString(cx, typeObj, &(p->code));
    if (!srcDispose) {
      return JS_FALSE;
    }

    AppendString(source, srcDispose);
    AppendString(source, ")");
    strMessage = NewUCString(cx, source);
  }

  if (!strMessage) {
    
    return JS_FALSE;
  }

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(strMessage));
  return JS_TRUE;
}

JSBool
CDataFinalizer::Methods::ToString(JSContext *cx, unsigned argc, jsval *vp)
{
  JSObject* objThis = JS_THIS_OBJECT(cx, vp);
  if (!objThis)
    return JS_FALSE;
  if (!CDataFinalizer::IsCDataFinalizer(objThis)) {
    JS_ReportError(cx, "not a CDataFinalizer");
    return JS_FALSE;
  }

  JSString *strMessage;
  jsval value;
  if (!JS_GetPrivate(objThis)) {
    
    
    strMessage = JS_NewStringCopyZ(cx, "[CDataFinalizer - empty]");
    if (!strMessage) {
      return JS_FALSE;
    }
  } else if (!CDataFinalizer::GetValue(cx, objThis, &value)) {
    JS_NOT_REACHED("Could not convert an empty CDataFinalizer");
  } else {
    strMessage = JS_ValueToString(cx, value);
    if (!strMessage) {
      return JS_FALSE;
    }
  }
  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(strMessage));
  return JS_TRUE;
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
  if (JSVAL_IS_VOID(valData)) {
    return NULL;
  }

  return JSVAL_TO_OBJECT(valData);
}

JSObject*
CDataFinalizer::GetCData(JSContext *cx, JSObject *obj)
{
  if (!obj) {
    JS_ReportError(cx, "No C data");
    return NULL;
  }
  if (CData::IsCData(obj)) {
    return obj;
  }
  if (!CDataFinalizer::IsCDataFinalizer(obj)) {
    JS_ReportError(cx, "Not C data");
    return NULL;
  }
  jsval val;
  if (!CDataFinalizer::GetValue(cx, obj, &val) || JSVAL_IS_PRIMITIVE(val)) {
    JS_ReportError(cx, "Empty CDataFinalizer");
    return NULL;
  }
  return JSVAL_TO_OBJECT(val);
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














JSBool
CDataFinalizer::Construct(JSContext* cx, unsigned argc, jsval *vp)
{
  RootedObject objSelf(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
  RootedObject objProto(cx);
  if (!GetObjectProperty(cx, objSelf, "prototype", &objProto)) {
    JS_ReportError(cx, "CDataFinalizer.prototype does not exist");
    return JS_FALSE;
  }

  
  if (argc == 0) { 
    JSObject *objResult = JS_NewObject(cx, &sCDataFinalizerClass, objProto, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(objResult));
    return JS_TRUE;
  }

  if (argc != 2) {
    JS_ReportError(cx, "CDataFinalizer takes 2 arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  JS::Value valCodePtr = argv[1];
  if (!valCodePtr.isObject()) {
    return TypeError(cx, "_a CData object_ of a function pointer type",
                     valCodePtr);
  }
  JSObject *objCodePtr = &valCodePtr.toObject();

  
  

  
  if (!CData::IsCData(objCodePtr)) {
    return TypeError(cx, "a _CData_ object of a function pointer type",
                     valCodePtr);
  }
  JSObject *objCodePtrType = CData::GetCType(objCodePtr);
  MOZ_ASSERT(objCodePtrType);

  TypeCode typCodePtr = CType::GetTypeCode(objCodePtrType);
  if (typCodePtr != TYPE_pointer) {
    return TypeError(cx, "a CData object of a function _pointer_ type",
                     OBJECT_TO_JSVAL(objCodePtrType));
  }

  JSObject *objCodeType = PointerType::GetBaseType(objCodePtrType);
  MOZ_ASSERT(objCodeType);

  TypeCode typCode = CType::GetTypeCode(objCodeType);
  if (typCode != TYPE_function) {
    return TypeError(cx, "a CData object of a _function_ pointer type",
                     OBJECT_TO_JSVAL(objCodePtrType));
  }
  uintptr_t code = *reinterpret_cast<uintptr_t*>(CData::GetData(objCodePtr));
  if (!code) {
    return TypeError(cx, "a CData object of a _non-NULL_ function pointer type",
                     OBJECT_TO_JSVAL(objCodePtrType));
  }

  FunctionInfo* funInfoFinalizer =
    FunctionType::GetFunctionInfo(objCodeType);
  MOZ_ASSERT(funInfoFinalizer);

  if ((funInfoFinalizer->mArgTypes.length() != 1)
      || (funInfoFinalizer->mIsVariadic)) {
    return TypeError(cx, "a function accepting exactly one argument",
                     OBJECT_TO_JSVAL(objCodeType));
  }
  RootedObject objArgType(cx, funInfoFinalizer->mArgTypes[0]);
  RootedObject returnType(cx, funInfoFinalizer->mReturnType);

  
  

  bool freePointer = false;

  

  size_t sizeArg;
  jsval valData = argv[0];
  if (!CType::GetSafeSize(objArgType, &sizeArg)) {
    return TypeError(cx, "(an object with known size)", valData);
  }

  ScopedJSFreePtr<void> cargs(malloc(sizeArg));

  if (!ImplicitConvert(cx, valData, objArgType, cargs.get(),
                       false, &freePointer)) {
    return TypeError(cx, "(an object that can be converted to the following type)",
                     OBJECT_TO_JSVAL(objArgType));
  }
  if (freePointer) {
    
    JS_ReportError(cx, "Internal Error during CDataFinalizer. Object cannot be represented");
    return JS_FALSE;
  }

  

  ScopedJSFreePtr<void> rvalue;
  if (CType::GetTypeCode(returnType) != TYPE_void_t) {
    rvalue = malloc(Align(CType::GetSize(returnType),
                          sizeof(ffi_arg)));
  } 

  

  JSObject *objResult = JS_NewObject(cx, &sCDataFinalizerClass, objProto, NULL);
  if (!objResult) {
    return JS_FALSE;
  }

  
  
  
  JSObject *objBestArgType = objArgType;
  if (!JSVAL_IS_PRIMITIVE(valData)) {
    JSObject *objData = JSVAL_TO_OBJECT(valData);
    if (CData::IsCData(objData)) {
      objBestArgType = CData::GetCType(objData);
      size_t sizeBestArg;
      if (!CType::GetSafeSize(objBestArgType, &sizeBestArg)) {
        JS_NOT_REACHED("object with unknown size");
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
    return JS_FALSE;
  }

  
  ScopedJSFreePtr<CDataFinalizer::Private>
    p((CDataFinalizer::Private*)malloc(sizeof(CDataFinalizer::Private)));

  memmove(&p->CIF, &funInfoFinalizer->mCIF, sizeof(ffi_cif));

  p->cargs = cargs.forget();
  p->rvalue = rvalue.forget();
  p->cargs_size = sizeArg;
  p->code = code;


  JS_SetPrivate(objResult, p.forget());
  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(objResult));
  return JS_TRUE;
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











JSBool
CDataFinalizer::Methods::Forget(JSContext* cx, unsigned argc, jsval *vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "CDataFinalizer.prototype.forget takes no arguments");
    return JS_FALSE;
  }

  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CDataFinalizer::IsCDataFinalizer(obj)) {
    return TypeError(cx, "a CDataFinalizer", OBJECT_TO_JSVAL(obj));
  }

  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(obj);

  if (!p) {
    JS_ReportError(cx, "forget called on an empty CDataFinalizer");
    return JS_FALSE;
  }

  jsval valJSData;
  RootedObject ctype(cx, GetCType(cx, obj));
  if (!ConvertToJS(cx, ctype, NullPtr(), p->cargs, false, true, &valJSData)) {
    JS_ReportError(cx, "CDataFinalizer value cannot be represented");
    return JS_FALSE;
  }

  CDataFinalizer::Cleanup(p, obj);

  JS_SET_RVAL(cx, vp, valJSData);
  return JS_TRUE;
}











JSBool
CDataFinalizer::Methods::Dispose(JSContext* cx, unsigned argc, jsval *vp)
{
  if (argc != 0) {
    JS_ReportError(cx, "CDataFinalizer.prototype.dispose takes no arguments");
    return JS_FALSE;
  }

  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!CDataFinalizer::IsCDataFinalizer(obj)) {
    return TypeError(cx, "a CDataFinalizer", OBJECT_TO_JSVAL(obj));
  }

  CDataFinalizer::Private *p = (CDataFinalizer::Private *)
    JS_GetPrivate(obj);

  if (!p) {
    JS_ReportError(cx, "dispose called on an empty CDataFinalizer.");
    return JS_FALSE;
  }

  jsval valType = JS_GetReservedSlot(obj, SLOT_DATAFINALIZER_VALTYPE);
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(valType));

  JSObject *objCTypes = CType::GetGlobalCTypes(cx, JSVAL_TO_OBJECT(valType));
  if (!objCTypes)
    return JS_FALSE;

  jsval valCodePtrType = JS_GetReservedSlot(obj, SLOT_DATAFINALIZER_CODETYPE);
  JS_ASSERT(!JSVAL_IS_PRIMITIVE(valCodePtrType));
  JSObject *objCodePtrType = JSVAL_TO_OBJECT(valCodePtrType);

  JSObject *objCodeType = PointerType::GetBaseType(objCodePtrType);
  JS_ASSERT(objCodeType);
  JS_ASSERT(CType::GetTypeCode(objCodeType) == TYPE_function);

  RootedObject resultType(cx, FunctionType::GetFunctionInfo(objCodeType)->mReturnType);
  jsval result = JSVAL_VOID;

  int errnoStatus;
#if defined(XP_WIN)
  int32_t lastErrorStatus;
  CDataFinalizer::CallFinalizer(p, &errnoStatus, &lastErrorStatus);
#else
  CDataFinalizer::CallFinalizer(p, &errnoStatus, NULL);
#endif 

  JS_SetReservedSlot(objCTypes, SLOT_ERRNO, INT_TO_JSVAL(errnoStatus));
#if defined(XP_WIN)
  JS_SetReservedSlot(objCTypes, SLOT_LASTERROR, INT_TO_JSVAL(lastErrorStatus));
#endif 

  if (ConvertToJS(cx, resultType, NullPtr(), p->rvalue, false, true, &result)) {
    CDataFinalizer::Cleanup(p, obj);
    JS_SET_RVAL(cx, vp, result);
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

  CDataFinalizer::CallFinalizer(p, NULL, NULL);
  CDataFinalizer::Cleanup(p, NULL);
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

  JS_SetPrivate(obj, NULL);
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
  JSClass* clasp = isUnsigned ? &sUInt64Class : &sInt64Class;
  RootedObject parent(cx, JS_GetParent(proto));
  RootedObject result(cx, JS_NewObject(cx, clasp, proto, parent));
  if (!result)
    return NULL;

  
  uint64_t* buffer = cx->new_<uint64_t>(data);
  if (!buffer) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }

  JS_SetReservedSlot(result, SLOT_INT64, PRIVATE_TO_JSVAL(buffer));

  if (!JS_FreezeObject(cx, result))
    return NULL;

  return result;
}

void
Int64Base::Finalize(JSFreeOp *fop, JSObject* obj)
{
  jsval slot = JS_GetReservedSlot(obj, SLOT_INT64);
  if (JSVAL_IS_VOID(slot))
    return;

  FreeOp::get(fop)->delete_(static_cast<uint64_t*>(JSVAL_TO_PRIVATE(slot)));
}

uint64_t
Int64Base::GetInt(JSObject* obj) {
  JS_ASSERT(Int64::IsInt64(obj) || UInt64::IsUInt64(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_INT64);
  return *static_cast<uint64_t*>(JSVAL_TO_PRIVATE(slot));
}

JSBool
Int64Base::ToString(JSContext* cx,
                    JSObject* obj,
                    unsigned argc,
                    jsval* vp,
                    bool isUnsigned)
{
  if (argc > 1) {
    JS_ReportError(cx, "toString takes zero or one argument");
    return JS_FALSE;
  }

  int radix = 10;
  if (argc == 1) {
    jsval arg = JS_ARGV(cx, vp)[0];
    if (JSVAL_IS_INT(arg))
      radix = JSVAL_TO_INT(arg);
    if (!JSVAL_IS_INT(arg) || radix < 2 || radix > 36) {
      JS_ReportError(cx, "radix argument must be an integer between 2 and 36");
      return JS_FALSE;
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
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
Int64Base::ToSource(JSContext* cx,
                    JSObject* obj,
                    unsigned argc,
                    jsval* vp,
                    bool isUnsigned)
{
  if (argc != 0) {
    JS_ReportError(cx, "toSource takes zero arguments");
    return JS_FALSE;
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
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
Int64::Construct(JSContext* cx,
                 unsigned argc,
                 jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);

  
  if (argc != 1) {
    JS_ReportError(cx, "Int64 takes one argument");
    return JS_FALSE;
  }

  int64_t i = 0;
  if (!jsvalToBigInteger(cx, args[0], true, &i))
    return TypeError(cx, "int64", args[0]);

  
  jsval slot;
  RootedObject callee(cx, &args.callee());
  ASSERT_OK(JS_GetProperty(cx, callee, "prototype", &slot));
  RootedObject proto(cx, JSVAL_TO_OBJECT(slot));
  JS_ASSERT(JS_GetClass(proto) == &sInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, i, false);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

bool
Int64::IsInt64(JSObject* obj)
{
  return JS_GetClass(obj) == &sInt64Class;
}

JSBool
Int64::ToString(JSContext* cx, unsigned argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!Int64::IsInt64(obj)) {
    JS_ReportError(cx, "not an Int64");
    return JS_FALSE;
  }

  return Int64Base::ToString(cx, obj, argc, vp, false);
}

JSBool
Int64::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!Int64::IsInt64(obj)) {
    JS_ReportError(cx, "not an Int64");
    return JS_FALSE;
  }

  return Int64Base::ToSource(cx, obj, argc, vp, false);
}

JSBool
Int64::Compare(JSContext* cx, unsigned argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 2 ||
      JSVAL_IS_PRIMITIVE(argv[0]) ||
      JSVAL_IS_PRIMITIVE(argv[1]) ||
      !Int64::IsInt64(JSVAL_TO_OBJECT(argv[0])) ||
      !Int64::IsInt64(JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "compare takes two Int64 arguments");
    return JS_FALSE;
  }

  JSObject* obj1 = JSVAL_TO_OBJECT(argv[0]);
  JSObject* obj2 = JSVAL_TO_OBJECT(argv[1]);

  int64_t i1 = Int64Base::GetInt(obj1);
  int64_t i2 = Int64Base::GetInt(obj2);

  if (i1 == i2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
  else if (i1 < i2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(-1));
  else
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(1));

  return JS_TRUE;
}

#define LO_MASK ((uint64_t(1) << 32) - 1)
#define INT64_LO(i) ((i) & LO_MASK)
#define INT64_HI(i) ((i) >> 32)

JSBool
Int64::Lo(JSContext* cx, unsigned argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !Int64::IsInt64(JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "lo takes one Int64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
  int64_t u = Int64Base::GetInt(obj);
  double d = uint32_t(INT64_LO(u));

  jsval result = JS_NumberValue(d);

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
Int64::Hi(JSContext* cx, unsigned argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !Int64::IsInt64(JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "hi takes one Int64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
  int64_t u = Int64Base::GetInt(obj);
  double d = int32_t(INT64_HI(u));

  jsval result = JS_NumberValue(d);

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
Int64::Join(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 2) {
    JS_ReportError(cx, "join takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  int32_t hi;
  uint32_t lo;
  if (!jsvalToInteger(cx, argv[0], &hi))
    return TypeError(cx, "int32", argv[0]);
  if (!jsvalToInteger(cx, argv[1], &lo))
    return TypeError(cx, "uint32", argv[1]);

  int64_t i = (int64_t(hi) << 32) + int64_t(lo);

  
  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));

  jsval slot = js::GetFunctionNativeReserved(callee, SLOT_FN_INT64PROTO);
  RootedObject proto(cx, JSVAL_TO_OBJECT(slot));
  JS_ASSERT(JS_GetClass(proto) == &sInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, i, false);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

JSBool
UInt64::Construct(JSContext* cx,
                  unsigned argc,
                  jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);

  
  if (argc != 1) {
    JS_ReportError(cx, "UInt64 takes one argument");
    return JS_FALSE;
  }

  uint64_t u = 0;
  if (!jsvalToBigInteger(cx, args[0], true, &u))
    return TypeError(cx, "uint64", args[0]);

  
  jsval slot;
  RootedObject callee(cx, &args.callee());
  ASSERT_OK(JS_GetProperty(cx, callee, "prototype", &slot));
  RootedObject proto(cx, JSVAL_TO_OBJECT(slot));
  JS_ASSERT(JS_GetClass(proto) == &sUInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

bool
UInt64::IsUInt64(JSObject* obj)
{
  return JS_GetClass(obj) == &sUInt64Class;
}

JSBool
UInt64::ToString(JSContext* cx, unsigned argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!UInt64::IsUInt64(obj)) {
    JS_ReportError(cx, "not a UInt64");
    return JS_FALSE;
  }

  return Int64Base::ToString(cx, obj, argc, vp, true);
}

JSBool
UInt64::ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!UInt64::IsUInt64(obj)) {
    JS_ReportError(cx, "not a UInt64");
    return JS_FALSE;
  }

  return Int64Base::ToSource(cx, obj, argc, vp, true);
}

JSBool
UInt64::Compare(JSContext* cx, unsigned argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 2 ||
      JSVAL_IS_PRIMITIVE(argv[0]) ||
      JSVAL_IS_PRIMITIVE(argv[1]) ||
      !UInt64::IsUInt64(JSVAL_TO_OBJECT(argv[0])) ||
      !UInt64::IsUInt64(JSVAL_TO_OBJECT(argv[1]))) {
    JS_ReportError(cx, "compare takes two UInt64 arguments");
    return JS_FALSE;
  }

  JSObject* obj1 = JSVAL_TO_OBJECT(argv[0]);
  JSObject* obj2 = JSVAL_TO_OBJECT(argv[1]);

  uint64_t u1 = Int64Base::GetInt(obj1);
  uint64_t u2 = Int64Base::GetInt(obj2);

  if (u1 == u2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
  else if (u1 < u2)
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(-1));
  else
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(1));

  return JS_TRUE;
}

JSBool
UInt64::Lo(JSContext* cx, unsigned argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !UInt64::IsUInt64(JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "lo takes one UInt64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
  uint64_t u = Int64Base::GetInt(obj);
  double d = uint32_t(INT64_LO(u));

  jsval result = JS_NumberValue(d);

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
UInt64::Hi(JSContext* cx, unsigned argc, jsval* vp)
{
  jsval* argv = JS_ARGV(cx, vp);
  if (argc != 1 || JSVAL_IS_PRIMITIVE(argv[0]) ||
      !UInt64::IsUInt64(JSVAL_TO_OBJECT(argv[0]))) {
    JS_ReportError(cx, "hi takes one UInt64 argument");
    return JS_FALSE;
  }

  JSObject* obj = JSVAL_TO_OBJECT(argv[0]);
  uint64_t u = Int64Base::GetInt(obj);
  double d = uint32_t(INT64_HI(u));

  jsval result = JS_NumberValue(d);

  JS_SET_RVAL(cx, vp, result);
  return JS_TRUE;
}

JSBool
UInt64::Join(JSContext* cx, unsigned argc, jsval* vp)
{
  if (argc != 2) {
    JS_ReportError(cx, "join takes two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  uint32_t hi;
  uint32_t lo;
  if (!jsvalToInteger(cx, argv[0], &hi))
    return TypeError(cx, "uint32_t", argv[0]);
  if (!jsvalToInteger(cx, argv[1], &lo))
    return TypeError(cx, "uint32_t", argv[1]);

  uint64_t u = (uint64_t(hi) << 32) + uint64_t(lo);

  
  JSObject* callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));

  jsval slot = js::GetFunctionNativeReserved(callee, SLOT_FN_INT64PROTO);
  RootedObject proto(cx, JSVAL_TO_OBJECT(slot));
  JS_ASSERT(JS_GetClass(proto) == &sUInt64ProtoClass);

  JSObject* result = Int64Base::Construct(cx, proto, u, true);
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));
  return JS_TRUE;
}

}
}

