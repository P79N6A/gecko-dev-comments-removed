






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jsapi.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C




#define JS_BITS_PER_UINT32_LOG2 5
#define JS_BITS_PER_UINT32      32


static const uintN JS_GCTHING_ALIGN = 8;
static const uintN JS_GCTHING_ZEROBITS = 3;


typedef uint8       jsbytecode;
typedef uint8       jssrcnote;
typedef uintptr_t   jsatomid;


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSNativeEnumerator   JSNativeEnumerator;
typedef struct JSProperty           JSProperty;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSThread             JSThread;
typedef struct JSTryNote            JSTryNote;


typedef struct JSAtomState          JSAtomState;
typedef struct JSCodeSpec           JSCodeSpec;
typedef struct JSPrinter            JSPrinter;
typedef struct JSStackHeader        JSStackHeader;
typedef struct JSSubString          JSSubString;
typedef struct JSNativeTraceInfo    JSNativeTraceInfo;
typedef struct JSSpecializedNative  JSSpecializedNative;
typedef struct JSXML                JSXML;








#ifdef __cplusplus

extern "C++" {

class JSDependentString;
class JSExtensibleString;
class JSExternalString;
class JSLinearString;
class JSFixedString;
class JSStaticAtom;
class JSRope;
class JSAtom;
class JSWrapper;

namespace js {

struct ArgumentsData;
struct FlatClosureData;
struct Class;

class RegExpObject;
class RegExpMatcher;
class RegExpObjectBuilder;
class RegExpStatics;
class MatchPairs;

namespace detail {

class RegExpPrivate;
class RegExpPrivateCode;

} 

enum RegExpFlag
{
    IgnoreCaseFlag  = 0x01,
    GlobalFlag      = 0x02,
    MultilineFlag   = 0x04,
    StickyFlag      = 0x08,

    NoFlags         = 0x00,
    AllFlags        = 0x0f
};

enum RegExpExecType
{
    RegExpExec,
    RegExpTest
};

class AutoStringRooter;
class ExecuteArgsGuard;
class InvokeFrameGuard;
class InvokeArgsGuard;
class StringBuffer;
class TraceRecorder;
struct TraceMonitor;

class FrameRegs;
class StackFrame;
class StackSegment;
class StackSpace;
class ContextStack;
class FrameRegsIter;
class CallReceiver;
class CallArgs;

struct BytecodeEmitter;
struct Definition;
struct FunctionBox;
struct ObjectBox;
struct ParseNode;
struct Parser;
class TokenStream;
struct Token;
struct TokenPos;
struct TokenPtr;
struct TreeContext;
class UpvarCookie;

class Proxy;
class ProxyHandler;
class Wrapper;
class CrossCompartmentWrapper;

class TempAllocPolicy;
class RuntimeAllocPolicy;

class GlobalObject;

template <class T,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = TempAllocPolicy>
class Vector;

template <class>
struct DefaultHasher;

template <class Key,
          class Value,
          class HashPolicy = DefaultHasher<Key>,
          class AllocPolicy = TempAllocPolicy>
class HashMap;

template <class T,
          class HashPolicy = DefaultHasher<T>,
          class AllocPolicy = TempAllocPolicy>
class HashSet;

template <typename K,
          typename V,
          size_t InlineElems>
class InlineMap;

class LifoAlloc;

class PropertyCache;
struct PropertyCacheEntry;

struct Shape;
struct EmptyShape;
class Bindings;

class MultiDeclRange;
class ParseMapPool;
class DefnOrHeader;
typedef InlineMap<JSAtom *, Definition *, 24> AtomDefnMap;
typedef InlineMap<JSAtom *, jsatomid, 24> AtomIndexMap;
typedef InlineMap<JSAtom *, DefnOrHeader, 24> AtomDOHMap;
typedef Vector<UpvarCookie, 8> UpvarCookies;

class Breakpoint;
class BreakpointSite;
typedef HashMap<jsbytecode *, BreakpointSite *, DefaultHasher<jsbytecode *>, RuntimeAllocPolicy>
    BreakpointSiteMap;
class Debugger;
class WatchpointMap;

typedef HashMap<JSAtom *, detail::RegExpPrivate *, DefaultHasher<JSAtom *>, RuntimeAllocPolicy>
    RegExpPrivateCache;

typedef JSNative             Native;
typedef JSPropertyOp         PropertyOp;
typedef JSStrictPropertyOp   StrictPropertyOp;
typedef JSPropertyDescriptor PropertyDescriptor;

namespace analyze {

struct LifetimeVariable;
class LoopAnalysis;
class ScriptAnalysis;
class SlotValue;
class SSAValue;
class SSAUseChain;

} 

namespace types {

class TypeSet;
struct TypeCallsite;
struct TypeObject;
struct TypeCompartment;

} 

} 

} 

#else

typedef struct JSAtom JSAtom;

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
(* JSSourceHandler)(const char *filename, uintN lineno, const jschar *str,
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

JS_END_EXTERN_C

#endif 
