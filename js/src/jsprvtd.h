






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jspubtd.h"
#include "jsstaticcheck.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C




#define JS_BITS_PER_UINT32_LOG2 5
#define JS_BITS_PER_UINT32      32


static const uintN JS_GCTHING_ALIGN = 8;
static const uintN JS_GCTHING_ZEROBITS = 3;


typedef uint8  jsbytecode;
typedef uint8  jssrcnote;
typedef uint32 jsatomid;


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSCodeGenerator      JSCodeGenerator;
typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSNativeEnumerator   JSNativeEnumerator;
typedef struct JSFunctionBox        JSFunctionBox;
typedef struct JSObjectBox          JSObjectBox;
typedef struct JSParseNode          JSParseNode;
typedef struct JSProperty           JSProperty;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSThread             JSThread;
typedef struct JSThreadData         JSThreadData;
typedef struct JSTreeContext        JSTreeContext;
typedef struct JSTryNote            JSTryNote;


typedef struct JSAtom               JSAtom;
typedef struct JSAtomList           JSAtomList;
typedef struct JSAtomListElement    JSAtomListElement;
typedef struct JSAtomMap            JSAtomMap;
typedef struct JSAtomState          JSAtomState;
typedef struct JSCodeSpec           JSCodeSpec;
typedef struct JSPrinter            JSPrinter;
typedef struct JSRegExpStatics      JSRegExpStatics;
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

struct ArgumentsData;

class RegExp;
class RegExpStatics;
class AutoStringRooter;
class ExecuteArgsGuard;
class InvokeFrameGuard;
class InvokeArgsGuard;
class InvokeSessionGuard;
class TraceRecorder;
struct TraceMonitor;
class StackSpace;
class StackSegment;
class FrameRegsIter;

struct Compiler;
struct Parser;
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

struct Shape;
struct EmptyShape;

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
    JSThrowHook         throwHook;
    void                *throwHookData;
    JSDebugErrorHook    debugErrorHook;
    void                *debugErrorHookData;
} JSDebugHooks;



















typedef JSBool
(* JSLookupPropOp)(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                   JSProperty **propp);





typedef JSBool
(* JSAttributesOp)(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);





typedef JSObject *
(* JSObjectOp)(JSContext *cx, JSObject *obj);





typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JSObject *obj, JSBool keysonly);





#ifdef JS_C_STRINGS_ARE_UTF8
# define js_CStringsAreUTF8 JS_TRUE
#else
extern JSBool js_CStringsAreUTF8;
#endif





extern JS_FRIEND_API(JSObject *)
js_ObjectToOuterObject(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif 
