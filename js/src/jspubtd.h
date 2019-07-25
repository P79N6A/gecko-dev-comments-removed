






































#ifndef jspubtd_h___
#define jspubtd_h___



#include "jstypes.h"
#include "jscompat.h"
#include "jsval.h"

JS_BEGIN_EXTERN_C


typedef JSInt32   jsint;
typedef JSUint32  jsuint;
typedef float64   jsdouble;
typedef JSInt32   jsrefcount;   

#ifdef WIN32
typedef wchar_t   jschar;
#else
typedef JSUint16  jschar;
#endif







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

    
    JSENUMERATE_INIT_ALL,

    
    JSENUMERATE_NEXT,

    
    JSENUMERATE_DESTROY
} JSIterateOp;


typedef struct JSClass           JSClass;
typedef struct JSConstDoubleSpec JSConstDoubleSpec;
typedef struct JSContext         JSContext;
typedef struct JSErrorReport     JSErrorReport;
typedef struct JSFunction        JSFunction;
typedef struct JSFunctionSpec    JSFunctionSpec;
typedef struct JSTracer          JSTracer;
typedef struct JSIdArray         JSIdArray;
typedef struct JSPropertyDescriptor JSPropertyDescriptor;
typedef struct JSPropertySpec    JSPropertySpec;
typedef struct JSObjectMap       JSObjectMap;
typedef struct JSRuntime         JSRuntime;
typedef struct JSStackFrame      JSStackFrame;
typedef struct JSXDRState        JSXDRState;
typedef struct JSExceptionState  JSExceptionState;
typedef struct JSLocaleCallbacks JSLocaleCallbacks;
typedef struct JSSecurityCallbacks JSSecurityCallbacks;
typedef struct JSCompartment     JSCompartment;
typedef struct JSCrossCompartmentCall JSCrossCompartmentCall;
typedef struct JSStructuredCloneWriter JSStructuredCloneWriter;
typedef struct JSStructuredCloneReader JSStructuredCloneReader;
typedef struct JSStructuredCloneCallbacks JSStructuredCloneCallbacks;

#ifdef __cplusplus
typedef class JSWrapper          JSWrapper;
typedef class JSCrossCompartmentWrapper JSCrossCompartmentWrapper;
#endif










typedef JSBool
(* JSPropertyOp)(JSContext *cx, JSObject *obj, jsid id, jsval *vp);








typedef JSBool
(* JSStrictPropertyOp)(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

































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







typedef JSBool
(* JSCheckAccessOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp);





typedef JSBool
(* JSXDRObjectOp)(JSXDRState *xdr, JSObject **objp);






typedef JSBool
(* JSHasInstanceOp)(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);


















typedef void
(* JSTraceOp)(JSTracer *trc, JSObject *obj);













typedef void
(* JSTraceCallback)(JSTracer *trc, void *thing, uint32 kind);





typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);

typedef JSBool
(* JSEqualityOp)(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);







typedef JSBool
(* JSNative)(JSContext *cx, uintN argc, jsval *vp);



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
(* JSLocaleToUnicode)(JSContext *cx, const char *src, jsval *rval);




typedef struct JSPrincipals JSPrincipals;








typedef JSBool
(* JSPrincipalsTranscoder)(JSXDRState *xdr, JSPrincipals **principalsp);









typedef JSPrincipals *
(* JSObjectPrincipalsFinder)(JSContext *cx, JSObject *obj);





typedef JSBool
(* JSCSPEvalChecker)(JSContext *cx);






typedef JSObject *
(* JSWrapObjectCallback)(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent,
                         uintN flags);






typedef JSObject *
(* JSPreWrapCallback)(JSContext *cx, JSObject *scope, JSObject *obj, uintN flags);

typedef enum {
    JSCOMPARTMENT_NEW, 
    JSCOMPARTMENT_DESTROY
} JSCompartmentOp;

typedef JSBool
(* JSCompartmentCallback)(JSContext *cx, JSCompartment *compartment, uintN compartmentOp);










typedef JSObject *(*ReadStructuredCloneOp)(JSContext *cx, JSStructuredCloneReader *r,
                                           uint32 tag, uint32 data, void *closure);












typedef JSBool (*WriteStructuredCloneOp)(JSContext *cx, JSStructuredCloneWriter *w,
                                         JSObject *obj, void *closure);






typedef void (*StructuredCloneErrorOp)(JSContext *cx, uint32 errorid);

JS_END_EXTERN_C

#endif 
