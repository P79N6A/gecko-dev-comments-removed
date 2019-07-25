






































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



























#if JS_BITS_PER_WORD == 32
typedef uint32 JSValueMaskType;
# define JSVAL_TYPE_BITS 32
# define JS_INSERT_VALUE_PADDING() uint32 padding;
#elif JS_BITS_PER_WORD == 64
typedef JSUint64 JSValueMaskType;
# define JSVAL_TYPE_BITS 32
# define JS_INSERT_VALUE_PADDING()
#else
# error "Unsupported word size"
#endif

#define JSVAL_NULL_MASK        ((JSValueMaskType)0x00)
#define JSVAL_UNDEFINED_MASK   ((JSValueMaskType)0x01)
#define JSVAL_INT32_MASK       ((JSValueMaskType)0x02)
#define JSVAL_DOUBLE_MASK      ((JSValueMaskType)0x04)
#define JSVAL_STRING_MASK      ((JSValueMaskType)0x08)
#define JSVAL_NONFUNOBJ_MASK   ((JSValueMaskType)0x10)
#define JSVAL_FUNOBJ_MASK      ((JSValueMaskType)0x20)
#define JSVAL_BOOLEAN_MASK     ((JSValueMaskType)0x40)
#define JSVAL_MAGIC_MASK       ((JSValueMaskType)0x80)








typedef enum JSWhyMagic
{
    JS_ARRAY_HOLE,               
    JS_ARGS_HOLE,                
    JS_NATIVE_ENUMERATE,         


    JS_NO_ITER_VALUE,            
    JS_GENERATOR_CLOSING         
} JSWhyMagic;

typedef union jsval_data
{
    int32          i32;
    uint32         u32;
    double         dbl;
    JSString *     str;
    JSObject *     obj;
    void *         ptr;
    JSBool         boo;
#ifdef DEBUG
    JSWhyMagic     why;
#endif
    struct { int32 first; int32 second; } bits;
} jsval_data;


typedef struct jsval
{
    JSValueMaskType mask;
    JS_INSERT_VALUE_PADDING()
    jsval_data data;
} jsval;















typedef jsword jsboxedword;

#define JSBOXEDWORD_TYPE_OBJECT     0x0
#define JSBOXEDWORD_TYPE_INT        0x1
#define JSBOXEDWORD_TYPE_DOUBLE     0x2
#define JSBOXEDWORD_TYPE_STRING     0x4
#define JSBOXEDWORD_TYPE_SPECIAL    0x6


#define JSBOXEDWORD_TAGBITS         3
#define JSBOXEDWORD_TAGMASK         ((jsboxedword) JS_BITMASK(JSBOXEDWORD_TAGBITS))
#define JSBOXEDWORD_ALIGN           JS_BIT(JSBOXEDWORD_TAGBITS)

static const jsboxedword JSBOXEDWORD_NULL  = (jsboxedword)0x0;
static const jsboxedword JSBOXEDWORD_FALSE = (jsboxedword)0x6;
static const jsboxedword JSBOXEDWORD_TRUE  = (jsboxedword)0xe;
static const jsboxedword JSBOXEDWORD_VOID  = (jsboxedword)0x16;

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_NULL(jsboxedword w)
{
    return w == JSBOXEDWORD_NULL;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_VOID(jsboxedword w)
{
    return w == JSBOXEDWORD_VOID;
}

static JS_ALWAYS_INLINE unsigned
JSBOXEDWORD_TAG(jsboxedword w)
{
    return (unsigned)(w & JSBOXEDWORD_TAGMASK);
}

static JS_ALWAYS_INLINE jsboxedword
JSBOXEDWORD_SETTAG(jsboxedword w, unsigned t)
{
    return w | t;
}

static JS_ALWAYS_INLINE jsboxedword
JSBOXEDWORD_CLRTAG(jsboxedword w)
{
    return w & ~(jsboxedword)JSBOXEDWORD_TAGMASK;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_DOUBLE(jsboxedword w)
{
    return JSBOXEDWORD_TAG(w) == JSBOXEDWORD_TYPE_DOUBLE;
}

static JS_ALWAYS_INLINE double *
JSBOXEDWORD_TO_DOUBLE(jsboxedword w)
{
    JS_ASSERT(JSBOXEDWORD_IS_DOUBLE(w));
    return (double *)JSBOXEDWORD_CLRTAG(w);
}

static JS_ALWAYS_INLINE jsboxedword
DOUBLE_TO_JSBOXEDWORD(double *d)
{
    JS_ASSERT(((JSUword)d & JSBOXEDWORD_TAGMASK) == 0);
    return (jsboxedword)((JSUword)d | JSBOXEDWORD_TYPE_DOUBLE);
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_STRING(jsboxedword w)
{
    return JSBOXEDWORD_TAG(w) == JSBOXEDWORD_TYPE_STRING;
}

static JS_ALWAYS_INLINE JSString *
JSBOXEDWORD_TO_STRING(jsboxedword w)
{
    JS_ASSERT(JSBOXEDWORD_IS_STRING(w));
    return (JSString *)JSBOXEDWORD_CLRTAG(w);
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_SPECIAL(jsboxedword w)
{
    return JSBOXEDWORD_TAG(w) == JSBOXEDWORD_TYPE_SPECIAL;
}

static JS_ALWAYS_INLINE jsint
JSBOXEDWORD_TO_SPECIAL(jsboxedword w)
{
    JS_ASSERT(JSBOXEDWORD_IS_SPECIAL(w));
    return w >> JSBOXEDWORD_TAGBITS;
}

static JS_ALWAYS_INLINE jsboxedword
SPECIAL_TO_JSBOXEDWORD(jsint i)
{
    return (i << JSBOXEDWORD_TAGBITS) | JSBOXEDWORD_TYPE_SPECIAL;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_BOOLEAN(jsboxedword w)
{
    return (w & ~((jsboxedword)1 << JSBOXEDWORD_TAGBITS)) == JSBOXEDWORD_TYPE_SPECIAL;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_TO_BOOLEAN(jsboxedword w)
{
    JS_ASSERT(w == JSBOXEDWORD_TRUE || w == JSBOXEDWORD_FALSE);
    return JSBOXEDWORD_TO_SPECIAL(w);
}

static JS_ALWAYS_INLINE jsboxedword
BOOLEAN_TO_JSBOXEDWORD(JSBool b)
{
    JS_ASSERT(b == JS_TRUE || b == JS_FALSE);
    return SPECIAL_TO_JSBOXEDWORD(b);
}

static JS_ALWAYS_INLINE jsboxedword
STRING_TO_JSBOXEDWORD(JSString *str)
{
    JS_ASSERT(((JSUword)str & JSBOXEDWORD_TAGMASK) == 0);
    return (jsboxedword)str | JSBOXEDWORD_TYPE_STRING;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_GCTHING(jsboxedword w)
{
    return !(w & JSBOXEDWORD_TYPE_INT) &&
           JSBOXEDWORD_TAG(w) != JSBOXEDWORD_TYPE_SPECIAL;
}

static JS_ALWAYS_INLINE void *
JSBOXEDWORD_TO_GCTHING(jsboxedword w)
{
    JS_ASSERT(JSBOXEDWORD_IS_GCTHING(w));
    return (void *)JSBOXEDWORD_CLRTAG(w);
}

static JS_ALWAYS_INLINE uint32
JSBOXEDWORD_TRACE_KIND(jsboxedword w)
{
    JS_ASSERT(w == 0x0 || w == 0x2 || w == 0x4);
    





    return (w | ((w & 0x4) >> 2)) & 0x3;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_OBJECT(jsboxedword w)
{
    return JSBOXEDWORD_TAG(w) == JSBOXEDWORD_TYPE_OBJECT;
}

static JS_ALWAYS_INLINE JSObject *
JSBOXEDWORD_TO_OBJECT(jsboxedword w)
{
    JS_ASSERT(JSBOXEDWORD_IS_OBJECT(w));
    return (JSObject *)JSBOXEDWORD_TO_GCTHING(w);
}

static JS_ALWAYS_INLINE jsboxedword
OBJECT_TO_JSBOXEDWORD(JSObject *obj)
{
    JS_ASSERT(((JSUword)obj & JSBOXEDWORD_TAGMASK) == 0);
    return (jsboxedword)obj | JSBOXEDWORD_TYPE_OBJECT;
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_PRIMITIVE(jsboxedword w)
{
    return !JSBOXEDWORD_IS_OBJECT(w) || JSBOXEDWORD_IS_NULL(w);
}


#define JSBOXEDWORD_INT_BITS          31
#define JSBOXEDWORD_INT_POW2(n)       ((jsboxedword)1 << (n))
#define JSBOXEDWORD_INT_MIN           (-JSBOXEDWORD_INT_POW2(30))
#define JSBOXEDWORD_INT_MAX           (JSBOXEDWORD_INT_POW2(30) - 1)

static JS_ALWAYS_INLINE JSBool
INT32_FITS_IN_JSBOXEDWORD(jsint i)
{
    return ((jsuint)(i) - (jsuint)JSBOXEDWORD_INT_MIN <=
            (jsuint)(JSBOXEDWORD_INT_MAX - JSBOXEDWORD_INT_MIN));
}

static JS_ALWAYS_INLINE JSBool
JSBOXEDWORD_IS_INT(jsboxedword w)
{
    return w & JSBOXEDWORD_TYPE_INT;
}

static JS_ALWAYS_INLINE jsint
JSBOXEDWORD_TO_INT(jsboxedword v)
{
    JS_ASSERT(JSBOXEDWORD_IS_INT(v));
    return (jsint)(v >> 1);
}

static JS_ALWAYS_INLINE jsboxedword
INT_TO_JSBOXEDWORD(jsint i)
{
    JS_ASSERT(INT32_FITS_IN_JSBOXEDWORD(i));
    return (i << 1) | JSBOXEDWORD_TYPE_INT;
}





typedef jsboxedword jsid;

#define JSID_NULL                     ((jsid)JSBOXEDWORD_NULL)
#define JSID_VOID                     ((jsid)JSBOXEDWORD_VOID)
#define JSID_IS_NULL(id)              JSBOXEDWORD_IS_NULL((jsboxedword)(id))
#define JSID_IS_VOID(id)              JSBOXEDWORD_IS_VOID((jsboxedword)(id))
#define JSID_IS_ATOM(id)              JSBOXEDWORD_IS_STRING((jsboxedword)(id))
#define JSID_TO_ATOM(id)              ((JSAtom *)(id))
#define ATOM_TO_JSID(atom)            (JS_ASSERT(ATOM_IS_STRING(atom)),        \
                                       (jsid)(atom))

#define INT32_FITS_IN_JSID(id)        INT32_FITS_IN_JSBOXEDWORD(id)
#define JSID_IS_INT(id)               JSBOXEDWORD_IS_INT((jsboxedword)(id))
#define JSID_TO_INT(id)               JSBOXEDWORD_TO_INT((jsboxedword)(id))
#define INT_TO_JSID(i)                ((jsid)INT_TO_JSBOXEDWORD((i)))

#define JSID_IS_OBJECT(id)            JSBOXEDWORD_IS_OBJECT((jsboxedword)(id))
#define JSID_TO_OBJECT(id)            JSBOXEDWORD_TO_OBJECT((jsboxedword)(id))
#define OBJECT_TO_JSID(obj)           ((jsid)OBJECT_TO_JSBOXEDWORD((obj)))


#define JSID_IS_GCTHING(id)           JSBOXEDWORD_IS_GCTHING(id)
#define JSID_TO_GCTHING(id)           (JS_ASSERT(JSID_IS_GCTHING((id))),       \
                                       JSBOXEDWORD_TO_GCTHING((jsboxedword)(id)))
#define JSID_TRACE_KIND(id)           (JS_ASSERT(JSID_IS_GCTHING((id))),       \
                                       JSBOXEDWORD_TRACE_KIND((jsboxedword)(id)))

JS_PUBLIC_API(jsval)
JSID_TO_JSVAL(jsid id);










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
(* JSEqualityOp)(JSContext *cx, JSObject *obj, const jsval *vp, JSBool *bp);






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
class Class;

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
#endif 
#endif 
