






































#ifndef jspubtd_h___
#define jspubtd_h___



#include "jstypes.h"
#include "jscompat.h"

JS_BEGIN_EXTERN_C


typedef uint16    jschar;
typedef int32     jsint;
typedef uint32    jsuint;
typedef float64   jsdouble;
typedef jsword    jsval;
typedef jsword    jsid;
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
    JSVERSION_DEFAULT = 0,
    JSVERSION_UNKNOWN = -1,
    JSVERSION_LATEST  = JSVERSION_1_8
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
    JSACC_IMPORT = 2,           
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
typedef struct JSFastNativeSpec  JSFastNativeSpec;
typedef struct JSFastNativeBlock JSFastNativeBlock;
typedef struct JSTracer          JSTracer;
typedef struct JSIdArray         JSIdArray;
typedef struct JSProperty        JSProperty;
typedef struct JSPropertySpec    JSPropertySpec;
typedef struct JSObject          JSObject;
typedef struct JSObjectMap       JSObjectMap;
typedef struct JSObjectOps       JSObjectOps;
typedef struct JSXMLObjectOps    JSXMLObjectOps;
typedef struct JSRuntime         JSRuntime;
typedef struct JSRuntime         JSTaskState;   
typedef struct JSScript          JSScript;
typedef struct JSStackFrame      JSStackFrame;
typedef struct JSString          JSString;
typedef struct JSXDRState        JSXDRState;
typedef struct JSExceptionState  JSExceptionState;
typedef struct JSLocaleCallbacks JSLocaleCallbacks;










typedef JSBool
(* JS_DLL_CALLBACK JSPropertyOp)(JSContext *cx, JSObject *obj, jsval id,
                                 jsval *vp);




























typedef JSBool
(* JS_DLL_CALLBACK JSNewEnumerateOp)(JSContext *cx, JSObject *obj,
                                     JSIterateOp enum_op,
                                     jsval *statep, jsid *idp);





typedef JSBool
(* JS_DLL_CALLBACK JSEnumerateOp)(JSContext *cx, JSObject *obj);













typedef JSBool
(* JS_DLL_CALLBACK JSResolveOp)(JSContext *cx, JSObject *obj, jsval id);






























typedef JSBool
(* JS_DLL_CALLBACK JSNewResolveOp)(JSContext *cx, JSObject *obj, jsval id,
                                   uintN flags, JSObject **objp);





typedef JSBool
(* JS_DLL_CALLBACK JSConvertOp)(JSContext *cx, JSObject *obj, JSType type,
                                jsval *vp);






typedef void
(* JS_DLL_CALLBACK JSFinalizeOp)(JSContext *cx, JSObject *obj);





typedef void
(* JS_DLL_CALLBACK JSStringFinalizeOp)(JSContext *cx, JSString *str);

























typedef JSObjectOps *
(* JS_DLL_CALLBACK JSGetObjectOps)(JSContext *cx, JSClass *clasp);













typedef JSBool
(* JS_DLL_CALLBACK JSCheckAccessOp)(JSContext *cx, JSObject *obj, jsval id,
                                    JSAccessMode mode, jsval *vp);





typedef JSBool
(* JS_DLL_CALLBACK JSXDRObjectOp)(JSXDRState *xdr, JSObject **objp);






typedef JSBool
(* JS_DLL_CALLBACK JSHasInstanceOp)(JSContext *cx, JSObject *obj, jsval v,
                                    JSBool *bp);






typedef uint32
(* JS_DLL_CALLBACK JSMarkOp)(JSContext *cx, JSObject *obj, void *arg);























typedef void
(* JS_DLL_CALLBACK JSTraceOp)(JSTracer *trc, JSObject *obj);

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
(* JS_DLL_CALLBACK JSTraceCallback)(JSTracer *trc, void *thing, uint32 kind);





#ifdef DEBUG
typedef void
(* JS_DLL_CALLBACK JSTraceNamePrinter)(JSTracer *trc, char *buf,
                                       size_t bufsize);
#endif













typedef uint32
(* JS_DLL_CALLBACK JSReserveSlotsOp)(JSContext *cx, JSObject *obj);
















typedef JSObjectMap *
(* JS_DLL_CALLBACK JSNewObjectMapOp)(JSContext *cx, jsrefcount nrefs,
                                     JSObjectOps *ops, JSClass *clasp,
                                     JSObject *obj);





typedef void
(* JS_DLL_CALLBACK JSObjectMapOp)(JSContext *cx, JSObjectMap *map);


















typedef JSBool
(* JS_DLL_CALLBACK JSLookupPropOp)(JSContext *cx, JSObject *obj, jsid id,
                                   JSObject **objp, JSProperty **propp);













typedef JSBool
(* JS_DLL_CALLBACK JSDefinePropOp)(JSContext *cx, JSObject *obj,
                                   jsid id, jsval value,
                                   JSPropertyOp getter, JSPropertyOp setter,
                                   uintN attrs, JSProperty **propp);









typedef JSBool
(* JS_DLL_CALLBACK JSPropertyIdOp)(JSContext *cx, JSObject *obj, jsid id,
                                   jsval *vp);







typedef JSBool
(* JS_DLL_CALLBACK JSAttributesOp)(JSContext *cx, JSObject *obj, jsid id,
                                   JSProperty *prop, uintN *attrsp);






typedef JSBool
(* JS_DLL_CALLBACK JSCheckAccessIdOp)(JSContext *cx, JSObject *obj, jsid id,
                                      JSAccessMode mode, jsval *vp,
                                      uintN *attrsp);






typedef JSObject *
(* JS_DLL_CALLBACK JSObjectOp)(JSContext *cx, JSObject *obj);







typedef void
(* JS_DLL_CALLBACK JSPropertyRefOp)(JSContext *cx, JSObject *obj,
                                    JSProperty *prop);






typedef JSBool
(* JS_DLL_CALLBACK JSSetObjectSlotOp)(JSContext *cx, JSObject *obj,
                                      uint32 slot, JSObject *pobj);














typedef jsval
(* JS_DLL_CALLBACK JSGetRequiredSlotOp)(JSContext *cx, JSObject *obj,
                                        uint32 slot);

typedef JSBool
(* JS_DLL_CALLBACK JSSetRequiredSlotOp)(JSContext *cx, JSObject *obj,
                                        uint32 slot, jsval v);

typedef JSObject *
(* JS_DLL_CALLBACK JSGetMethodOp)(JSContext *cx, JSObject *obj, jsid id,
                                  jsval *vp);

typedef JSBool
(* JS_DLL_CALLBACK JSSetMethodOp)(JSContext *cx, JSObject *obj, jsid id,
                                  jsval *vp);

typedef JSBool
(* JS_DLL_CALLBACK JSEnumerateValuesOp)(JSContext *cx, JSObject *obj,
                                        JSIterateOp enum_op,
                                        jsval *statep, jsid *idp, jsval *vp);

typedef JSBool
(* JS_DLL_CALLBACK JSEqualityOp)(JSContext *cx, JSObject *obj, jsval v,
                                 JSBool *bp);

typedef JSBool
(* JS_DLL_CALLBACK JSConcatenateOp)(JSContext *cx, JSObject *obj, jsval v,
                                    jsval *vp);



typedef JSBool
(* JS_DLL_CALLBACK JSNative)(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval);


typedef JSBool
(* JS_DLL_CALLBACK JSFastNative)(JSContext *cx, uintN argc, jsval *vp);



typedef enum JSContextOp {
    JSCONTEXT_NEW,
    JSCONTEXT_DESTROY
} JSContextOp;














typedef JSBool
(* JS_DLL_CALLBACK JSContextCallback)(JSContext *cx, uintN contextOp);

typedef enum JSGCStatus {
    JSGC_BEGIN,
    JSGC_END,
    JSGC_MARK_END,
    JSGC_FINALIZE_END
} JSGCStatus;

typedef JSBool
(* JS_DLL_CALLBACK JSGCCallback)(JSContext *cx, JSGCStatus status);

typedef void
(* JS_DLL_CALLBACK JSGCThingCallback)(void *thing, uint8 flags, void *closure);





typedef void
(* JS_DLL_CALLBACK JSTraceDataOp)(JSTracer *trc, void *data);

typedef JSBool
(* JS_DLL_CALLBACK JSBranchCallback)(JSContext *cx, JSScript *script);

typedef void
(* JS_DLL_CALLBACK JSErrorReporter)(JSContext *cx, const char *message,
                                    JSErrorReport *report);






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
(* JS_DLL_CALLBACK JSErrorCallback)(void *userRef, const char *locale,
                                    const uintN errorNumber);

#ifdef va_start
#define JS_ARGUMENT_FORMATTER_DEFINED 1

typedef JSBool
(* JS_DLL_CALLBACK JSArgumentFormatter)(JSContext *cx, const char *format,
                                        JSBool fromJS, jsval **vpp,
                                        va_list *app);
#endif

typedef JSBool
(* JS_DLL_CALLBACK JSLocaleToUpperCase)(JSContext *cx, JSString *src,
                                        jsval *rval);

typedef JSBool
(* JS_DLL_CALLBACK JSLocaleToLowerCase)(JSContext *cx, JSString *src,
                                        jsval *rval);

typedef JSBool
(* JS_DLL_CALLBACK JSLocaleCompare)(JSContext *cx,
                                    JSString *src1, JSString *src2,
                                    jsval *rval);

typedef JSBool
(* JS_DLL_CALLBACK JSLocaleToUnicode)(JSContext *cx, char *src, jsval *rval);




typedef struct JSPrincipals JSPrincipals;








typedef JSBool
(* JS_DLL_CALLBACK JSPrincipalsTranscoder)(JSXDRState *xdr,
                                           JSPrincipals **principalsp);









typedef JSPrincipals *
(* JS_DLL_CALLBACK JSObjectPrincipalsFinder)(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif 
