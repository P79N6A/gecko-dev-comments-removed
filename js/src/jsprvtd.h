






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jspubtd.h"
#include "jsutil.h"




#define JS_BITS_PER_UINT32_LOG2 5
#define JS_BITS_PER_UINT32      32


static const uintN JS_GCTHING_ALIGN = 8;
static const uintN JS_GCTHING_ZEROBITS = 3;


typedef uint8  jsbytecode;
typedef uint8  jssrcnote;
typedef uint32 jsatomid;

#ifdef __cplusplus


extern "C++" {
namespace js {
struct Parser;
struct Compiler;
}
}

#endif


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSCodeGenerator      JSCodeGenerator;
typedef union JSGCThing             JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSNativeEnumerator   JSNativeEnumerator;
typedef struct JSFunctionBox        JSFunctionBox;
typedef struct JSObjectBox          JSObjectBox;
typedef struct JSParseNode          JSParseNode;
typedef struct JSProperty           JSProperty;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSEmptyScope         JSEmptyScope;
typedef struct JSThread             JSThread;
typedef struct JSThreadData         JSThreadData;
typedef struct JSTreeContext        JSTreeContext;
typedef struct JSTryNote            JSTryNote;
typedef struct JSWeakRoots          JSWeakRoots;


typedef struct JSAtom               JSAtom;
typedef struct JSAtomList           JSAtomList;
typedef struct JSAtomListElement    JSAtomListElement;
typedef struct JSAtomMap            JSAtomMap;
typedef struct JSAtomState          JSAtomState;
typedef struct JSCodeSpec           JSCodeSpec;
typedef struct JSPrinter            JSPrinter;
typedef struct JSRegExp             JSRegExp;
typedef struct JSRegExpStatics      JSRegExpStatics;
typedef struct JSScope              JSScope;
typedef struct JSScopeOps           JSScopeOps;
typedef struct JSScopeProperty      JSScopeProperty;
typedef struct JSStackHeader        JSStackHeader;
typedef struct JSSubString          JSSubString;
typedef struct JSNativeTraceInfo    JSNativeTraceInfo;
typedef struct JSSpecializedNative  JSSpecializedNative;
typedef struct JSXML                JSXML;
typedef struct JSXMLArray           JSXMLArray;
typedef struct JSXMLArrayCursor     JSXMLArrayCursor;








#ifdef __cplusplus
extern "C++" {

namespace js {

class ExecuteArgsGuard;
class InvokeFrameGuard;
class InvokeArgsGuard;
class TraceRecorder;
struct TraceMonitor;
class StackSpace;
class CallStack;

class TokenStream;
struct Token;
struct TokenPos;
struct TokenPtr;

class ContextAllocPolicy;
class SystemAllocPolicy;

template <class T,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = ContextAllocPolicy>
class Vector;

template <class>
struct DefaultHasher;

template <class Key,
          class Value,
          class HashPolicy = DefaultHasher<Key>,
          class AllocPolicy = ContextAllocPolicy>
class HashMap;

template <class T,
          class HashPolicy = DefaultHasher<T>,
          class AllocPolicy = ContextAllocPolicy>
class HashSet;

class DeflatedStringCache;

class PropertyCache;
struct PropertyCacheEntry;


} 


typedef js::Vector<jschar, 32> JSCharBuffer;

} 
#endif  


typedef enum JSTrapStatus {
    JSTRAP_ERROR,
    JSTRAP_CONTINUE,
    JSTRAP_RETURN,
    JSTRAP_THROW,
    JSTRAP_LIMIT
} JSTrapStatus;

typedef JSTrapStatus
(* JSTrapHandler)(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                  jsval closure);

typedef JSTrapStatus
(* JSInterruptHook)(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                    void *closure);

typedef JSTrapStatus
(* JSDebuggerHandler)(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                      void *closure);

typedef JSTrapStatus
(* JSThrowHook)(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                void *closure);

typedef JSBool
(* JSWatchPointHandler)(JSContext *cx, JSObject *obj, jsid id, jsval old,
                        jsval *newp, void *closure);


typedef void
(* JSNewScriptHook)(JSContext  *cx,
                    const char *filename,  
                    uintN      lineno,     
                    JSScript   *script,
                    JSFunction *fun,
                    void       *callerdata);


typedef void
(* JSDestroyScriptHook)(JSContext *cx,
                        JSScript  *script,
                        void      *callerdata);

typedef void
(* JSSourceHandler)(const char *filename, uintN lineno, jschar *str,
                    size_t length, void **listenerTSData, void *closure);


























typedef void *
(* JSInterpreterHook)(JSContext *cx, JSStackFrame *fp, JSBool before,
                      JSBool *ok, void *closure);

typedef void
(* JSObjectHook)(JSContext *cx, JSObject *obj, JSBool isNew, void *closure);

typedef JSBool
(* JSDebugErrorHook)(JSContext *cx, const char *message, JSErrorReport *report,
                     void *closure);

typedef struct JSDebugHooks {
    JSInterruptHook     interruptHook;
    void                *interruptHookData;
    JSNewScriptHook     newScriptHook;
    void                *newScriptHookData;
    JSDestroyScriptHook destroyScriptHook;
    void                *destroyScriptHookData;
    JSDebuggerHandler   debuggerHandler;
    void                *debuggerHandlerData;
    JSSourceHandler     sourceHandler;
    void                *sourceHandlerData;
    JSInterpreterHook   executeHook;
    void                *executeHookData;
    JSInterpreterHook   callHook;
    void                *callHookData;
    JSObjectHook        objectHook;
    void                *objectHookData;
    JSThrowHook         throwHook;
    void                *throwHookData;
    JSDebugErrorHook    debugErrorHook;
    void                *debugErrorHookData;
} JSDebugHooks;




















typedef JSBool
(* JSLookupPropOp)(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                   JSProperty **propp);





typedef JSBool
(* JSDefinePropOp)(JSContext *cx, JSObject *obj, jsid id, const jsval *value,
                   JSPropertyOp getter, JSPropertyOp setter, uintN attrs);









typedef JSBool
(* JSPropertyIdOp)(JSContext *cx, JSObject *obj, jsid id, jsval *vp);





typedef JSBool
(* JSAttributesOp)(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);






typedef JSBool
(* JSCheckAccessIdOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                      jsval *vp, uintN *attrsp);





#ifdef JS_C_STRINGS_ARE_UTF8
# define js_CStringsAreUTF8 JS_TRUE
#else
extern JSBool js_CStringsAreUTF8;
#endif

#ifdef __cplusplus
namespace js {

class Value;

typedef JSBool
(* DefinePropOp)(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                 PropertyOp getter, PropertyOp setter, uintN attrs);
typedef JSBool
(* CheckAccessIdOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    Value *vp, uintN *attrsp);
typedef JSBool
(* PropertyIdOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp);









static inline DefinePropOp        Valueify(JSDefinePropOp f)      { return (DefinePropOp)f; }
static inline JSDefinePropOp      Jsvalify(DefinePropOp f)        { return (JSDefinePropOp)f; }
static inline CheckAccessIdOp     Valueify(JSCheckAccessIdOp f)   { return (CheckAccessIdOp)f; }
static inline JSCheckAccessIdOp   Jsvalify(CheckAccessIdOp f)     { return (JSCheckAccessIdOp)f; }
static inline PropertyIdOp        Valueify(JSPropertyIdOp f);     
static inline JSPropertyIdOp      Jsvalify(PropertyIdOp f);       

static const PropertyOp    PropertyStub  = (PropertyOp)JS_PropertyStub;
static const JSEnumerateOp EnumerateStub = JS_EnumerateStub;
static const JSResolveOp   ResolveStub   = JS_ResolveStub;
static const ConvertOp     ConvertStub   = (ConvertOp)JS_ConvertStub;
static const JSFinalizeOp  FinalizeStub  = JS_FinalizeStub;

}  
#endif 

#endif 
