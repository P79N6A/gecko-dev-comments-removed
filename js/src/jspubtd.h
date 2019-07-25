






































#ifndef jspubtd_h___
#define jspubtd_h___



#include "jscompat.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C


typedef uint16    jschar;
typedef int32     jsint;
typedef uint32    jsuint;
typedef float64   jsdouble;
typedef int32     jsrefcount;   






typedef enum JSVersion {
    JSVERSION_1_0     = 100,
    JSVERSION_1_1     = 110,
    JSVERSION_1_2     = 120,
    JSVERSION_1_3     = 130,
    JSVERSION_1_4     = 140,
    JSVERSION_ECMA_3  = 148,
    JSVERSION_1_5     = 150,
    JSVERSION_1_6     = 160,
    JSVERSION_1_7     = 170,
    JSVERSION_1_8     = 180,
    JSVERSION_ECMA_5  = 185,
    JSVERSION_DEFAULT = 0,
    JSVERSION_UNKNOWN = -1,
    JSVERSION_LATEST  = JSVERSION_ECMA_5
} JSVersion;

#define JSVERSION_IS_ECMA(version) \
    ((version) == JSVERSION_DEFAULT || (version) >= JSVERSION_1_3)


typedef enum JSType {
    JSTYPE_VOID,                
    JSTYPE_OBJECT,              
    JSTYPE_FUNCTION,            
    JSTYPE_STRING,              
    JSTYPE_NUMBER,              
    JSTYPE_BOOLEAN,             
    JSTYPE_NULL,                
    JSTYPE_XML,                 
    JSTYPE_LIMIT
} JSType;


typedef enum JSProtoKey {
#define JS_PROTO(name,code,init) JSProto_##name = code,
#include "jsproto.tbl"
#undef JS_PROTO
    JSProto_LIMIT
} JSProtoKey;


typedef enum JSAccessMode {
    JSACC_PROTO  = 0,           
    JSACC_PARENT = 1,           

                                




    JSACC_WATCH  = 3,           
    JSACC_READ   = 4,           
    JSACC_WRITE  = 8,           
    JSACC_LIMIT
} JSAccessMode;

#define JSACC_TYPEMASK          (JSACC_WRITE - 1)





typedef enum JSIterateOp {
    JSENUMERATE_INIT,       
    JSENUMERATE_NEXT,       
    JSENUMERATE_DESTROY     
} JSIterateOp;


typedef struct JSClass           JSClass;
typedef struct JSExtendedClass   JSExtendedClass;
typedef struct JSConstDoubleSpec JSConstDoubleSpec;
typedef struct JSContext         JSContext;
typedef struct JSErrorReport     JSErrorReport;
typedef struct JSFunction        JSFunction;
typedef struct JSFunctionSpec    JSFunctionSpec;
typedef struct JSTracer          JSTracer;
typedef struct JSIdArray         JSIdArray;
typedef struct JSPropertyDescriptor JSPropertyDescriptor;
typedef struct JSPropertySpec    JSPropertySpec;
typedef struct JSObject          JSObject;
typedef struct JSObjectMap       JSObjectMap;
typedef struct JSObjectOps       JSObjectOps;
typedef struct JSRuntime         JSRuntime;
typedef struct JSScript          JSScript;
typedef struct JSStackFrame      JSStackFrame;
typedef struct JSString          JSString;
typedef struct JSXDRState        JSXDRState;
typedef struct JSExceptionState  JSExceptionState;
typedef struct JSLocaleCallbacks JSLocaleCallbacks;
typedef struct JSSecurityCallbacks JSSecurityCallbacks;
typedef struct JSONParser        JSONParser;
typedef struct JSCompartment     JSCompartment;









#if defined(__cplusplus) || !defined(_MSC_VER)
typedef enum JSValueMask16 
# if defined(_MSC_VER)
                           : uint16
# endif
{
    JSVAL_MASK16_NULL      = (uint16)0x0001,
    JSVAL_MASK16_UNDEFINED = (uint16)0x0002,
    JSVAL_MASK16_INT32     = (uint16)0x0004,
    JSVAL_MASK16_STRING    = (uint16)0x0008,
    JSVAL_MASK16_NONFUNOBJ = (uint16)0x0010,
    JSVAL_MASK16_FUNOBJ    = (uint16)0x0020,
    JSVAL_MASK16_BOOLEAN   = (uint16)0x0040,
    JSVAL_MASK16_MAGIC     = (uint16)0x0080,

    JSVAL_MASK16_SINGLETON = JSVAL_MASK16_NULL | JSVAL_MASK16_UNDEFINED,
    JSVAL_MASK16_OBJECT    = JSVAL_MASK16_NONFUNOBJ | JSVAL_MASK16_FUNOBJ,
    JSVAL_MASK16_OBJORNULL = JSVAL_MASK16_OBJECT | JSVAL_MASK16_NULL,
    JSVAL_MASK16_GCTHING   = JSVAL_MASK16_OBJECT | JSVAL_MASK16_STRING,

    




    JSVAL_NANBOX_PATTERN   = ((uint16)0xFFFF)
}
# if defined(__GNUC__)
__attribute__((packed))
# endif
JSValueMask16;

#else






typedef uint16 JSValueMask16;

#define JSVAL_MASK16_NULL      ((uint16)0x0001)
#define JSVAL_MASK16_UNDEFINED ((uint16)0x0002)
#define JSVAL_MASK16_INT32     ((uint16)0x0004)
#define JSVAL_MASK16_STRING    ((uint16)0x0008)
#define JSVAL_MASK16_NONFUNOBJ ((uint16)0x0010)
#define JSVAL_MASK16_FUNOBJ    ((uint16)0x0020)
#define JSVAL_MASK16_BOOLEAN   ((uint16)0x0040)
#define JSVAL_MASK16_MAGIC     ((uint16)0x0080)
#define JSVAL_MASK16_SINGLETON (JSVAL_MASK16_NULL | JSVAL_MASK16_UNDEFINED)
#define JSVAL_MASK16_OBJECT    (JSVAL_MASK16_NONFUNOBJ | JSVAL_MASK16_FUNOBJ)
#define JSVAL_MASK16_OBJORNULL (JSVAL_MASK16_OBJECT | JSVAL_MASK16_NULL)
#define JSVAL_MASK16_GCTHING   (JSVAL_MASK16_OBJECT | JSVAL_MASK16_STRING)
#define JSVAL_NANBOX_PATTERN   ((uint16)0xFFFF)

#endif

#define JSVAL_MASK32_CLEAR      ((uint32)0xFFFF0000)

#define JSVAL_MASK32_NULL       ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_NULL))
#define JSVAL_MASK32_UNDEFINED  ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_UNDEFINED))
#define JSVAL_MASK32_INT32      ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_INT32))
#define JSVAL_MASK32_STRING     ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_STRING))
#define JSVAL_MASK32_NONFUNOBJ  ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_NONFUNOBJ))
#define JSVAL_MASK32_FUNOBJ     ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_FUNOBJ))
#define JSVAL_MASK32_BOOLEAN    ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_BOOLEAN))
#define JSVAL_MASK32_MAGIC      ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_MAGIC))

#define JSVAL_MASK32_SINGLETON  ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_SINGLETON))
#define JSVAL_MASK32_OBJECT     ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_OBJECT))
#define JSVAL_MASK32_OBJORNULL  ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_OBJORNULL))
#define JSVAL_MASK32_GCTHING    ((uint32)(JSVAL_MASK32_CLEAR | JSVAL_MASK16_GCTHING))

typedef uint32 JSValueMask32;

#ifdef __GNUC__
# define VALUE_ALIGNMENT        __attribute__((aligned (8)))
# define ASSERT_DOUBLE_ALIGN()  JS_ASSERT(size_t(this) % sizeof(double) == 0)
#elif defined(_MSC_VER)
  



# define VALUE_ALIGNMENT
# define ASSERT_DOUBLE_ALIGN()
#else
# error "TODO: do something for compiler"
#endif

typedef VALUE_ALIGNMENT uint64 jsval;

#define BUILD_JSVAL(mask32, payload) ((jsval)((((uint64)(uint32)(mask32)) << 32) | (uint32)(payload)))

typedef enum JSWhyMagic
{
    JS_ARRAY_HOLE,               
    JS_ARGS_HOLE,                
    JS_NATIVE_ENUMERATE,         


    JS_NO_ITER_VALUE,            
    JS_GENERATOR_CLOSING,        
    JS_NO_CONSTANT               
} JSWhyMagic;

#if !defined(IS_LITTLE_ENDIAN)
# error "Unsupported configuration"
#endif

typedef union jsval_layout
{
    struct {
        union {
            int32          i32;
            uint32         u32;
            JSBool         boo;
#if JS_BITS_PER_WORD == 32
            JSString       *str;
            JSObject       *obj;
            void           *ptr;
#elif JS_BITS_PER_WORD == 64
            uint32         ptr;
#else
# error "Unsupported configuration"
#endif
            JSWhyMagic     why;
        } payload;
        union {
            struct {
                JSValueMask16 mask16;
                uint16 nanBits;
            } tag;
            JSValueMask32 mask32;
        } u;
    } s;
    double asDouble;
    uint64 asBits;
} jsval_layout;

#if JS_BITS_PER_WORD == 32

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_NULL_IMPL(jsval_layout l)
{
    return l.s.u.mask32 == JSVAL_MASK32_NULL;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_UNDEFINED_IMPL(jsval_layout l)
{
    return l.s.u.mask32 == JSVAL_MASK32_UNDEFINED;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_SPECIFIC_INT32_IMPL(jsval_layout l, int32 i32)
{
    return l.s.u.mask32 == JSVAL_MASK32_INT32 && l.s.payload.i32 == i32;
}

static JS_ALWAYS_INLINE jsval_layout
INT32_TO_JSVAL_IMPL(int32 i)
{
    jsval_layout l;
    l.s.u.mask32 = JSVAL_MASK32_INT32;
    l.s.payload.i32 = i;
    return l;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_DOUBLE_IMPL(jsval_layout l)
{
    return l.s.u.mask32 < JSVAL_MASK32_CLEAR;
}

static JS_ALWAYS_INLINE jsval_layout
DOUBLE_TO_JSVAL_IMPL(jsdouble d)
{
    jsval_layout l;
    l.asDouble = d;
    JS_ASSERT(l.s.u.tag.nanBits != JSVAL_NANBOX_PATTERN);
    return l;
}

static JS_ALWAYS_INLINE jsval_layout
STRING_TO_JSVAL_IMPL(JSString *str)
{
    jsval_layout l;
    l.s.u.mask32 = JSVAL_MASK32_STRING;
    l.s.payload.str = str;
    return l;
}

static JS_ALWAYS_INLINE JSString *
JSVAL_TO_STRING_IMPL(jsval_layout l)
{
    return l.s.payload.str;
}

static JS_ALWAYS_INLINE JSObject *
JSVAL_TO_OBJECT_IMPL(jsval_layout l)
{
    return l.s.payload.obj;
}

static JS_ALWAYS_INLINE jsval_layout
OBJECT_TO_JSVAL_IMPL(JSValueMask32 mask, JSObject *obj)
{
    jsval_layout l;
    l.s.u.mask32 = mask;
    l.s.payload.obj = obj;
    return l;
}

static JS_ALWAYS_INLINE jsval_layout
BOOLEAN_TO_JSVAL_IMPL(JSBool b)
{
    jsval_layout l;
    l.s.u.mask32 = JSVAL_MASK32_BOOLEAN;
    l.s.payload.boo = b;
    return l;
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_GCTHING_IMPL(jsval_layout l)
{
    return l.s.payload.ptr;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_SPECIFIC_BOOLEAN(jsval_layout l, JSBool b)
{
    return (l.s.u.mask32 == JSVAL_MASK32_BOOLEAN) && (l.s.payload.boo == b);
}

static JS_ALWAYS_INLINE jsval_layout
PRIVATE_PTR_TO_JSVAL_IMPL(void *ptr)
{
    jsval_layout l;
    JS_ASSERT(((uint32)ptr & 1) == 0);
    l.s.u.mask32 = 0;
    l.s.payload.ptr = ptr;
    return l;
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_PRIVATE_PTR_IMPL(jsval_layout l)
{
    return l.s.payload.ptr;
}

static JS_ALWAYS_INLINE jsval_layout
MAGIC_TO_JSVAL_IMPL(JSWhyMagic why)
{
    jsval_layout l;
    l.s.u.mask32 = JSVAL_MASK32_MAGIC;
    l.s.payload.why = why;
    return l;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_SAME_PRIMITIVE_TYPE_OR_BOTH_OBJECTS_IMPL(jsval_layout lhs, jsval_layout rhs)
{
    return ((lhs.s.u.mask32 ^ rhs.s.u.mask32) & ~(uint32)JSVAL_MASK16_OBJECT) == 0 ||
           (lhs.s.u.mask32 < JSVAL_MASK32_CLEAR && rhs.s.u.mask32 < JSVAL_MASK32_CLEAR);
}

static JS_ALWAYS_INLINE JSBool
JSVAL_BOTH_STRING_IMPL(jsval_layout lhs, jsval_layout rhs)
{
    return (lhs.s.u.mask32 & rhs.s.u.mask32) == JSVAL_MASK32_STRING;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_BOTH_INT32_IMPL(jsval_layout lhs, jsval_layout rhs)
{
    return (lhs.s.u.mask32 & rhs.s.u.mask32) == JSVAL_MASK32_INT32;
}

#elif JS_BITS_PER_WORD == 64

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_NULL_IMPL(jsval_layout l)
{
    return l.asBits == BUILD_JSVAL(JSVAL_MASK32_NULL, 0);
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_UNDEFINED_IMPL(jsval_layout l)
{
    return l.asBits == BUILD_JSVAL(JSVAL_MASK32_UNDEFINED, 0);
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_SPECIFIC_INT32_IMPL(jsval_layout l, int32 i32)
{
    return l.asBits == BUILD_JSVAL(JSVAL_MASK32_INT32, i32);
}

static JS_ALWAYS_INLINE jsval_layout
INT32_TO_JSVAL_IMPL(int32 i)
{
    jsval_layout l;
    l.asBits = BUILD_JSVAL(JSVAL_MASK32_INT32, i);
    return l;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_DOUBLE_IMPL(jsval_layout l)
{
    return l.asBits < BUILD_JSVAL(JSVAL_MASK32_CLEAR, 0);
}

static JS_ALWAYS_INLINE jsval_layout
DOUBLE_TO_JSVAL_IMPL(jsdouble d)
{
    jsval_layout l;
    l.asDouble = d;
    JS_ASSERT(l.s.u.tag.nanBits != JSVAL_NANBOX_PATTERN);
    return l;
}

static JS_ALWAYS_INLINE jsval_layout
STRING_TO_JSVAL_IMPL(JSString *str)
{
    JS_ASSERT((size_t)str < (size_t)0xFFFFFFFF);
    jsval_layout l;
    l.asBits = BUILD_JSVAL(JSVAL_MASK32_STRING, (uint32)(size_t)str);
    return l;
}

static JS_ALWAYS_INLINE JSString *
JSVAL_TO_STRING_IMPL(jsval_layout l)
{
    return (JSString *)(uint64)l.s.payload.ptr;
}

static JS_ALWAYS_INLINE JSObject *
JSVAL_TO_OBJECT_IMPL(jsval_layout l)
{
    return (JSObject *)(uint64)l.s.payload.ptr;
}

static JS_ALWAYS_INLINE jsval_layout
OBJECT_TO_JSVAL_IMPL(JSValueMask32 mask32, JSObject *obj)
{
    JS_ASSERT((size_t)obj < (size_t)0xFFFFFFFF);
    jsval_layout l;
    l.asBits = BUILD_JSVAL(mask32, (uint32)(size_t)obj);
    return l;
}

static JS_ALWAYS_INLINE jsval_layout
BOOLEAN_TO_JSVAL_IMPL(JSBool b)
{
    jsval_layout l;
    l.asBits = BUILD_JSVAL(JSVAL_MASK32_BOOLEAN, b);
    return l;
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_GCTHING_IMPL(jsval_layout l)
{
    return (void *)(uint64)l.s.payload.ptr;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_SPECIFIC_BOOLEAN(jsval_layout l, JSBool b)
{
    return l.asBits == BUILD_JSVAL(JSVAL_MASK32_BOOLEAN, b);
}

static JS_ALWAYS_INLINE jsval_layout
PRIVATE_PTR_TO_JSVAL_IMPL(void *ptr)
{
    JS_ASSERT(((uint32)(size_t)ptr & 1) == 0);
    jsval_layout l;
    l.asBits = (size_t)ptr >> 1;
    return l;
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_PRIVATE_PTR_IMPL(jsval_layout l)
{
    return (void *)(l.asBits << 1);
}

static JS_ALWAYS_INLINE jsval_layout
MAGIC_TO_JSVAL_IMPL(JSWhyMagic why)
{
    jsval_layout l;
    l.asBits = BUILD_JSVAL(JSVAL_MASK32_MAGIC, why);
    return l;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_SAME_PRIMITIVE_TYPE_OR_BOTH_OBJECTS_IMPL(jsval_layout lhs, jsval_layout rhs)
{
    uint32 xor32 = (uint32)((lhs.asBits ^ rhs.asBits) >> 32);
    return (xor32 & ~(uint32)JSVAL_MASK16_OBJECT) == 0 ||
           (JSVAL_IS_DOUBLE_IMPL(lhs) && JSVAL_IS_DOUBLE_IMPL(rhs));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_BOTH_STRING_IMPL(jsval_layout lhs, jsval_layout rhs)
{
    return (uint32)((lhs.asBits & rhs.asBits) >> 32) == JSVAL_MASK32_STRING;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_BOTH_INT32_IMPL(jsval_layout lhs, jsval_layout rhs)
{
    return (uint32)((lhs.asBits & rhs.asBits) >> 32) == JSVAL_MASK32_INT32;
}

#else
# error "Unsupported configuration"
#endif

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_SINGLETON_IMPL(jsval_layout l)
{
    return (l.s.u.mask32 & JSVAL_MASK32_SINGLETON) > JSVAL_MASK32_CLEAR;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_NUMBER_IMPL(jsval_layout l)
{
    JSValueMask32 mask = l.s.u.mask32;
    return mask < JSVAL_MASK32_CLEAR || mask == JSVAL_MASK32_INT32;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_OBJECT_IMPL(jsval_layout l)
{
    return (l.s.u.mask32 & JSVAL_MASK32_OBJECT) > JSVAL_MASK32_CLEAR;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_OBJECT_OR_NULL_IMPL(jsval_layout l)
{
    return (l.s.u.mask32 & JSVAL_MASK32_OBJORNULL) > JSVAL_MASK32_CLEAR;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_PRIMITIVE_IMPL(jsval_layout l)
{
    return (l.s.u.mask32 & JSVAL_MASK32_OBJECT) <= JSVAL_MASK32_CLEAR;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_GCTHING_IMPL(jsval_layout l)
{
    return (l.s.u.mask32 & JSVAL_MASK32_GCTHING) > JSVAL_MASK32_CLEAR;
}

static JS_ALWAYS_INLINE uint32
JSVAL_TRACE_KIND_IMPL(jsval_layout l)
{
    return (uint32)(l.s.u.mask32 == JSVAL_MASK32_STRING);
}

static JS_ALWAYS_INLINE jsval_layout
PRIVATE_UINT32_TO_JSVAL_IMPL(uint32 ui)
{
    jsval_layout l;
    l.s.u.mask32 = 0;
    l.s.payload.u32 = ui;
    return l;
}

static JS_ALWAYS_INLINE uint32
JSVAL_TO_PRIVATE_UINT32_IMPL(jsval_layout l)
{
    return l.s.payload.u32;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_UNDERLYING_TYPE_OF_PRIVATE_IMPL(jsval_layout l)
{
    return JSVAL_IS_DOUBLE_IMPL(l);
}





#ifdef DEBUG
typedef struct jsid
{
    size_t bits;
} jsid;
# define JSID_BITS(id) (id.bits)
#else
typedef size_t jsid;
# define JSID_BITS(id) (id)
#endif










typedef JSBool
(* JSPropertyOp)(JSContext *cx, JSObject *obj, jsid id, jsval *vp);




























typedef JSBool
(* JSNewEnumerateOp)(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                     jsval *statep, jsid *idp);





typedef JSBool
(* JSEnumerateOp)(JSContext *cx, JSObject *obj);













typedef JSBool
(* JSResolveOp)(JSContext *cx, JSObject *obj, jsid id);






























typedef JSBool
(* JSNewResolveOp)(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                   JSObject **objp);





typedef JSBool
(* JSConvertOp)(JSContext *cx, JSObject *obj, JSType type, jsval *vp);




typedef JSType
(* JSTypeOfOp)(JSContext *cx, JSObject *obj);






typedef void
(* JSFinalizeOp)(JSContext *cx, JSObject *obj);





typedef void
(* JSStringFinalizeOp)(JSContext *cx, JSString *str);





















typedef JSObjectOps *
(* JSGetObjectOps)(JSContext *cx, JSClass *clasp);













typedef JSBool
(* JSCheckAccessOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp);





typedef JSBool
(* JSXDRObjectOp)(JSXDRState *xdr, JSObject **objp);






typedef JSBool
(* JSHasInstanceOp)(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);






typedef uint32
(* JSMarkOp)(JSContext *cx, JSObject *obj, void *arg);























typedef void
(* JSTraceOp)(JSTracer *trc, JSObject *obj);

#if defined __GNUC__ && __GNUC__ >= 4 && !defined __cplusplus
# define JS_CLASS_TRACE(method)                                               \
    (__builtin_types_compatible_p(JSTraceOp, __typeof(&(method)))             \
     ? (JSMarkOp)(method)                                                     \
     : js_WrongTypeForClassTracer)

extern JSMarkOp js_WrongTypeForClassTracer;

#else
# define JS_CLASS_TRACE(method) ((JSMarkOp)(method))
#endif













typedef void
(* JSTraceCallback)(JSTracer *trc, void *thing, uint32 kind);





typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);













typedef uint32
(* JSReserveSlotsOp)(JSContext *cx, JSObject *obj);






typedef JSBool
(* JSEqualityOp)(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);






typedef JSObject *
(* JSObjectOp)(JSContext *cx, JSObject *obj);





typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JSObject *obj, JSBool keysonly);



typedef JSBool
(* JSNative)(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
             jsval *rval);

typedef JSBool
(* JSFastNative)(JSContext *cx, uintN argc, jsval *vp);



typedef enum JSContextOp {
    JSCONTEXT_NEW,
    JSCONTEXT_DESTROY
} JSContextOp;














typedef JSBool
(* JSContextCallback)(JSContext *cx, uintN contextOp);

#ifndef JS_THREADSAFE
typedef void
(* JSHeartbeatCallback)(JSRuntime *rt);
#endif

typedef enum JSGCStatus {
    JSGC_BEGIN,
    JSGC_END,
    JSGC_MARK_END,
    JSGC_FINALIZE_END
} JSGCStatus;

typedef JSBool
(* JSGCCallback)(JSContext *cx, JSGCStatus status);





typedef void
(* JSTraceDataOp)(JSTracer *trc, void *data);

typedef JSBool
(* JSOperationCallback)(JSContext *cx);




typedef JSBool
(* JSBranchCallback)(JSContext *cx, JSScript *script);

typedef void
(* JSErrorReporter)(JSContext *cx, const char *message, JSErrorReport *report);






typedef enum JSExnType {
    JSEXN_NONE = -1,
      JSEXN_ERR,
        JSEXN_INTERNALERR,
        JSEXN_EVALERR,
        JSEXN_RANGEERR,
        JSEXN_REFERENCEERR,
        JSEXN_SYNTAXERR,
        JSEXN_TYPEERR,
        JSEXN_URIERR,
        JSEXN_LIMIT
} JSExnType;

typedef struct JSErrorFormatString {
    
    const char *format;

    
    uint16 argCount;

    
    int16 exnType;
} JSErrorFormatString;

typedef const JSErrorFormatString *
(* JSErrorCallback)(void *userRef, const char *locale,
                    const uintN errorNumber);

#ifdef va_start
#define JS_ARGUMENT_FORMATTER_DEFINED 1

typedef JSBool
(* JSArgumentFormatter)(JSContext *cx, const char *format, JSBool fromJS,
                        jsval **vpp, va_list *app);
#endif

typedef JSBool
(* JSLocaleToUpperCase)(JSContext *cx, JSString *src, jsval *rval);

typedef JSBool
(* JSLocaleToLowerCase)(JSContext *cx, JSString *src, jsval *rval);

typedef JSBool
(* JSLocaleCompare)(JSContext *cx, JSString *src1, JSString *src2,
                    jsval *rval);

typedef JSBool
(* JSLocaleToUnicode)(JSContext *cx, char *src, jsval *rval);




typedef struct JSPrincipals JSPrincipals;








typedef JSBool
(* JSPrincipalsTranscoder)(JSXDRState *xdr, JSPrincipals **principalsp);









typedef JSPrincipals *
(* JSObjectPrincipalsFinder)(JSContext *cx, JSObject *obj);





typedef JSBool
(* JSCSPEvalChecker)(JSContext *cx);

JS_END_EXTERN_C

#ifdef __cplusplus
namespace js {

class Value;
struct Class;

typedef JSBool
(* Native)(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval);
typedef JSBool
(* FastNative)(JSContext *cx, uintN argc, Value *vp);
typedef JSBool
(* PropertyOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp);
typedef JSBool
(* ConvertOp)(JSContext *cx, JSObject *obj, JSType type, Value *vp);
typedef JSBool
(* NewEnumerateOp)(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                   Value *statep, jsid *idp);
typedef JSBool
(* HasInstanceOp)(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp);
typedef JSBool
(* CheckAccessOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                  Value *vp);
typedef JSObjectOps *
(* GetObjectOps)(JSContext *cx, Class *clasp);
typedef JSBool
(* EqualityOp)(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp);









static inline Native           Valueify(JSNative f)         { return (Native)f; }
static inline JSNative         Jsvalify(Native f)           { return (JSNative)f; }
static inline FastNative       Valueify(JSFastNative f)     { return (FastNative)f; }
static inline JSFastNative     Jsvalify(FastNative f)       { return (JSFastNative)f; }
static inline PropertyOp       Valueify(JSPropertyOp f)     { return (PropertyOp)f; }
static inline JSPropertyOp     Jsvalify(PropertyOp f)       { return (JSPropertyOp)f; }
static inline ConvertOp        Valueify(JSConvertOp f)      { return (ConvertOp)f; }
static inline JSConvertOp      Jsvalify(ConvertOp f)        { return (JSConvertOp)f; }
static inline NewEnumerateOp   Valueify(JSNewEnumerateOp f) { return (NewEnumerateOp)f; }
static inline JSNewEnumerateOp Jsvalify(NewEnumerateOp f)   { return (JSNewEnumerateOp)f; }
static inline HasInstanceOp    Valueify(JSHasInstanceOp f)  { return (HasInstanceOp)f; }
static inline JSHasInstanceOp  Jsvalify(HasInstanceOp f)    { return (JSHasInstanceOp)f; }
static inline CheckAccessOp    Valueify(JSCheckAccessOp f)  { return (CheckAccessOp)f; }
static inline JSCheckAccessOp  Jsvalify(CheckAccessOp f)    { return (JSCheckAccessOp)f; }
static inline GetObjectOps     Valueify(JSGetObjectOps f)   { return (GetObjectOps)f; }
static inline JSGetObjectOps   Jsvalify(GetObjectOps f)     { return (JSGetObjectOps)f; }
static inline EqualityOp       Valueify(JSEqualityOp f);    
static inline JSEqualityOp     Jsvalify(EqualityOp f);      

}  

typedef js::Class JSFunctionClassType;
extern "C" JS_FRIEND_DATA(JSFunctionClassType) js_FunctionClass;

#else  

typedef JSClass JSFunctionClassType;
extern JS_FRIEND_DATA(JSFunctionClassType) js_FunctionClass;

#endif 

typedef struct JSPretendObject
{
    void                *_;
    JSFunctionClassType *clasp;
} JSPretendObject;

static JS_ALWAYS_INLINE JSBool
JS_OBJ_IS_FUN_IMPL(JSObject *obj)
{
    return ((JSPretendObject *)obj)->clasp == &js_FunctionClass;
}

#endif
