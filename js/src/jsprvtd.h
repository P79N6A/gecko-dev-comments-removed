






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jspubtd.h"
#include "jsutil.h"



#define JSID_IS_ATOM(id)            JSVAL_IS_STRING((jsval)(id))
#define JSID_TO_ATOM(id)            ((JSAtom *)(id))
#define ATOM_TO_JSID(atom)          (JS_ASSERT(ATOM_IS_STRING(atom)),         \
                                     (jsid)(atom))

#define JSID_IS_INT(id)             JSVAL_IS_INT((jsval)(id))
#define JSID_TO_INT(id)             JSVAL_TO_INT((jsval)(id))
#define INT_TO_JSID(i)              ((jsid)INT_TO_JSVAL(i))
#define INT_JSVAL_TO_JSID(v)        ((jsid)(v))
#define INT_JSID_TO_JSVAL(id)       ((jsval)(id))

#define JSID_IS_OBJECT(id)          JSVAL_IS_OBJECT((jsval)(id))
#define JSID_TO_OBJECT(id)          JSVAL_TO_OBJECT((jsval)(id))
#define OBJECT_TO_JSID(obj)         ((jsid)OBJECT_TO_JSVAL(obj))
#define OBJECT_JSVAL_TO_JSID(v)     ((jsid)v)

#define ID_TO_VALUE(id)             ((jsval)(id))




#define JS_BITS_PER_UINT32_LOG2 5
#define JS_BITS_PER_UINT32      32


typedef uint8  jsbytecode;
typedef uint8  jssrcnote;
typedef uint32 jsatomid;


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSCodeGenerator      JSCodeGenerator;
typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSNativeEnumerator   JSNativeEnumerator;
typedef struct JSCompiler           JSCompiler;
typedef struct JSFunctionBox        JSFunctionBox;
typedef struct JSObjectBox          JSObjectBox;
typedef struct JSParseNode          JSParseNode;
typedef struct JSPropCacheEntry     JSPropCacheEntry;
typedef struct JSProperty           JSProperty;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSTempValueRooter    JSTempValueRooter;
typedef struct JSThread             JSThread;
typedef struct JSThreadData         JSThreadData;
typedef struct JSToken              JSToken;
typedef struct JSTokenPos           JSTokenPos;
typedef struct JSTokenPtr           JSTokenPtr;
typedef struct JSTokenStream        JSTokenStream;
typedef struct JSTraceMonitor       JSTraceMonitor;
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

class ContextAllocPolicy;
class SystemAllocPolicy;

template <class T,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = ContextAllocPolicy>
class Vector;

template <class T,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = ContextAllocPolicy>
class Pool;

template <class>
struct DefaultHasher;

template <class Key,
          class Value = void,
          class Hasher = DefaultHasher<Key>,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = ContextAllocPolicy>
class HashMap;

} 


typedef js::Vector<jschar, 32> JSCharBuffer;

typedef js::HashMap<JSObject *, bool, js::DefaultHasher<JSObject *>, 4,
                    js::ContextAllocPolicy> JSBusyArrayTable;

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
                  void *closure);

typedef JSBool
(* JSWatchPointHandler)(JSContext *cx, JSObject *obj, jsval id, jsval old,
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
    JSTrapHandler       interruptHandler;
    void                *interruptHandlerData;
    JSNewScriptHook     newScriptHook;
    void                *newScriptHookData;
    JSDestroyScriptHook destroyScriptHook;
    void                *destroyScriptHookData;
    JSTrapHandler       debuggerHandler;
    void                *debuggerHandlerData;
    JSSourceHandler     sourceHandler;
    void                *sourceHandlerData;
    JSInterpreterHook   executeHook;
    void                *executeHookData;
    JSInterpreterHook   callHook;
    void                *callHookData;
    JSObjectHook        objectHook;
    void                *objectHookData;
    JSTrapHandler       throwHook;
    void                *throwHookData;
    JSDebugErrorHook    debugErrorHook;
    void                *debugErrorHookData;
} JSDebugHooks;





typedef void
(* JSTempValueTrace)(JSTracer *trc, JSTempValueRooter *tvr);

typedef union JSTempValueUnion {
    jsval               value;
    JSObject            *object;
    JSXML               *xml;
    JSTempValueTrace    trace;
    JSScopeProperty     *sprop;
    JSWeakRoots         *weakRoots;
    JSCompiler          *compiler;
    JSScript            *script;
    jsval               *array;
} JSTempValueUnion;

struct JSTempValueRooter {
    JSTempValueRooter   *down;
    ptrdiff_t           count;
    JSTempValueUnion    u;
};




















typedef JSBool
(* JSLookupPropOp)(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                   JSProperty **propp);





typedef JSBool
(* JSDefinePropOp)(JSContext *cx, JSObject *obj, jsid id, jsval value,
                   JSPropertyOp getter, JSPropertyOp setter, uintN attrs);









typedef JSBool
(* JSPropertyIdOp)(JSContext *cx, JSObject *obj, jsid id, jsval *vp);







typedef JSBool
(* JSAttributesOp)(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                   uintN *attrsp);






typedef JSBool
(* JSCheckAccessIdOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                      jsval *vp, uintN *attrsp);







typedef void
(* JSPropertyRefOp)(JSContext *cx, JSObject *obj, JSProperty *prop);





#ifdef JS_C_STRINGS_ARE_UTF8
# define js_CStringsAreUTF8 JS_TRUE
#else
extern JSBool js_CStringsAreUTF8;
#endif







#define JS_ARGS_LENGTH_MAX      (JS_BIT(24) - 1)

#endif 
