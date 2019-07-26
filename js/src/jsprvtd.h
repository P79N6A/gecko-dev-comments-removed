





#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jsapi.h"
#include "jsutil.h"

#ifdef __cplusplus
#include "js/HashTable.h"
#include "js/Vector.h"
#endif




#define JS_BITS_PER_UINT32_LOG2 5
#define JS_BITS_PER_UINT32      32


static const unsigned JS_GCTHING_ALIGN = 8;
static const unsigned JS_GCTHING_ZEROBITS = 3;


typedef uint8_t     jsbytecode;
typedef uint8_t     jssrcnote;
typedef uintptr_t   jsatomid;


typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSNativeEnumerator   JSNativeEnumerator;
typedef struct JSTryNote            JSTryNote;


typedef struct JSAtomState          JSAtomState;
typedef struct JSCodeSpec           JSCodeSpec;
typedef struct JSPrinter            JSPrinter;
typedef struct JSStackHeader        JSStackHeader;
typedef struct JSSubString          JSSubString;
typedef struct JSSpecializedNative  JSSpecializedNative;








#ifdef __cplusplus

extern "C++" {

class JSDependentString;
class JSExtensibleString;
class JSExternalString;
class JSLinearString;
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

enum RegExpFlag
{
    IgnoreCaseFlag  = 0x01,
    GlobalFlag      = 0x02,
    MultilineFlag   = 0x04,
    StickyFlag      = 0x08,

    NoFlags         = 0x00,
    AllFlags        = 0x0f
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

class Proxy;
class JS_FRIEND_API(BaseProxyHandler);
class JS_FRIEND_API(Wrapper);
class JS_FRIEND_API(CrossCompartmentWrapper);

class TempAllocPolicy;
class RuntimeAllocPolicy;

class GlobalObject;

template <typename K,
          typename V,
          size_t InlineElems>
class InlineMap;

class LifoAlloc;

class Shape;

class Breakpoint;
class BreakpointSite;
class Debugger;
class WatchpointMap;







typedef JSObject Env;

typedef JSNative             Native;
typedef JSPropertyOp         PropertyOp;
typedef JSStrictPropertyOp   StrictPropertyOp;
typedef JSPropertyDescriptor PropertyDescriptor;

namespace frontend {

struct BytecodeEmitter;
struct Definition;
class FunctionBox;
class ObjectBox;
struct Token;
struct TokenPos;
struct TokenPtr;
class TokenStream;
class ParseMapPool;
struct ParseNode;

template <typename ParseHandler>
struct Parser;

} 

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
typedef JS::Handle<types::TypeObject*> HandleTypeObject;
typedef JS::Handle<JSAtom*>            HandleAtom;
typedef JS::Handle<PropertyName*>      HandlePropertyName;

typedef JS::MutableHandle<Shape*>      MutableHandleShape;
typedef JS::MutableHandle<JSAtom*>     MutableHandleAtom;

typedef js::Rooted<Shape*>             RootedShape;
typedef js::Rooted<types::TypeObject*> RootedTypeObject;
typedef js::Rooted<JSAtom*>            RootedAtom;
typedef js::Rooted<PropertyName*>      RootedPropertyName;

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
                    unsigned   lineno,     
                    JSScript   *script,
                    JSFunction *fun,
                    void       *callerdata);


typedef void
(* JSDestroyScriptHook)(JSFreeOp *fop,
                        JSScript *script,
                        void     *callerdata);

typedef void
(* JSSourceHandler)(const char *filename, unsigned lineno, const jschar *str,
                    size_t length, void **listenerTSData, void *closure);







typedef JSObject *
(* JSObjectOp)(JSContext *cx, JSHandleObject obj);


typedef JSObject *
(* JSClassInitializerOp)(JSContext *cx, JSHandleObject obj);





typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JSHandleObject obj, JSBool keysonly);


#endif 
