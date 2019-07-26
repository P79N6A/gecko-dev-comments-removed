






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jsapi.h"
#include "jsutil.h"

#ifdef __cplusplus
#include "js/HashTable.h"
#include "js/Vector.h"
#endif

JS_BEGIN_EXTERN_C




#define JS_BITS_PER_UINT32_LOG2 5
#define JS_BITS_PER_UINT32      32


static const unsigned JS_GCTHING_ALIGN = 8;
static const unsigned JS_GCTHING_ZEROBITS = 3;


typedef uint8_t     jsbytecode;
typedef uint8_t     jssrcnote;
typedef uintptr_t   jsatomid;


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSNativeEnumerator   JSNativeEnumerator;
typedef struct JSProperty           JSProperty;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSTryNote            JSTryNote;


typedef struct JSAtomState          JSAtomState;
typedef struct JSCodeSpec           JSCodeSpec;
typedef struct JSPrinter            JSPrinter;
typedef struct JSStackHeader        JSStackHeader;
typedef struct JSSubString          JSSubString;
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
struct Class;

class RegExpGuard;
class RegExpObject;
class RegExpObjectBuilder;
class RegExpShared;
class RegExpStatics;
class MatchPairs;
class PropertyName;

namespace detail { class RegExpCode; }

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

class ExecuteArgsGuard;
class InvokeFrameGuard;
class InvokeArgsGuard;
class StringBuffer;

class FrameRegs;
class StackFrame;
class StackSegment;
class StackSpace;
class ContextStack;
class ScriptFrameIter;
class CallReceiver;
class CallArgs;

struct BytecodeEmitter;
struct Definition;
struct FunctionBox;
struct ObjectBox;
struct ParseNode;
struct Parser;
struct SharedContext;
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

template <typename K,
          typename V,
          size_t InlineElems>
class InlineMap;

class LifoAlloc;

class BaseShape;
class UnownedBaseShape;
struct Shape;
struct EmptyShape;
class ShapeKindArray;
class Bindings;

struct StackBaseShape;
struct StackShape;

class MultiDeclRange;
class ParseMapPool;
class DefnOrHeader;
typedef InlineMap<JSAtom *, Definition *, 24> AtomDefnMap;
typedef InlineMap<JSAtom *, jsatomid, 24> AtomIndexMap;
typedef InlineMap<JSAtom *, DefnOrHeader, 24> AtomDOHMap;
typedef Vector<UpvarCookie, 8> UpvarCookies;

class Breakpoint;
class BreakpointSite;
class Debugger;
class WatchpointMap;







typedef JSObject Env;

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

typedef JS::Handle<Shape*>             HandleShape;
typedef JS::Handle<BaseShape*>         HandleBaseShape;
typedef JS::Handle<types::TypeObject*> HandleTypeObject;
typedef JS::Handle<JSAtom*>            HandleAtom;
typedef JS::Handle<PropertyName*>      HandlePropertyName;

typedef JS::Root<Shape*>             RootShape;
typedef JS::Root<BaseShape*>         RootBaseShape;
typedef JS::Root<types::TypeObject*> RootTypeObject;
typedef JS::Root<JSAtom*>            RootAtom;
typedef JS::Root<PropertyName*>      RootPropertyName;

typedef JS::RootedVar<Shape*>             RootedVarShape;
typedef JS::RootedVar<BaseShape*>         RootedVarBaseShape;
typedef JS::RootedVar<types::TypeObject*> RootedVarTypeObject;
typedef JS::RootedVar<JSAtom*>            RootedVarAtom;
typedef JS::RootedVar<PropertyName*>      RootedVarPropertyName;

enum XDRMode {
    XDR_ENCODE,
    XDR_DECODE
};

template <XDRMode mode>
class XDRState;

class FreeOp;

} 

namespace JSC {

class ExecutableAllocator;

} 

namespace WTF {

class BumpPointerAllocator;

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
                    unsigned      lineno,     
                    JSScript   *script,
                    JSFunction *fun,
                    void       *callerdata);


typedef void
(* JSDestroyScriptHook)(JSFreeOp *fop,
                        JSScript  *script,
                        void      *callerdata);

typedef void
(* JSSourceHandler)(const char *filename, unsigned lineno, const jschar *str,
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
(* JSAttributesOp)(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp);





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
