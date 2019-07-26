





#ifndef jsprvtd_h
#define jsprvtd_h





#include "jsapi.h"


typedef uint8_t     jsbytecode;
typedef uint8_t     jssrcnote;
typedef uintptr_t   jsatomid;


class JSAtom;

namespace js {

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

class Shape;







typedef JSObject Env;

typedef JSNative             Native;
typedef JSParallelNative     ParallelNative;
typedef JSThreadSafeNative   ThreadSafeNative;
typedef JSPropertyOp         PropertyOp;
typedef JSStrictPropertyOp   StrictPropertyOp;
typedef JSPropertyDescriptor PropertyDescriptor;

namespace types { struct TypeObject; }

typedef JS::Handle<Shape*>             HandleShape;
typedef JS::Handle<types::TypeObject*> HandleTypeObject;
typedef JS::Handle<JSAtom*>            HandleAtom;
typedef JS::Handle<PropertyName*>      HandlePropertyName;

typedef JS::MutableHandle<Shape*>      MutableHandleShape;
typedef JS::MutableHandle<JSAtom*>     MutableHandleAtom;

typedef JS::Rooted<Shape*>             RootedShape;
typedef JS::Rooted<types::TypeObject*> RootedTypeObject;
typedef JS::Rooted<JSAtom*>            RootedAtom;
typedef JS::Rooted<PropertyName*>      RootedPropertyName;

enum XDRMode {
    XDR_ENCODE,
    XDR_DECODE
};

struct IdValuePair
{
    jsid id;
    Value value;

    IdValuePair() {}
    IdValuePair(jsid idArg)
      : id(idArg), value(UndefinedValue())
    {}
};

} 


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

typedef bool
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
(* JSObjectOp)(JSContext *cx, JS::Handle<JSObject*> obj);


typedef JSObject *
(* JSClassInitializerOp)(JSContext *cx, JS::HandleObject obj);





typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JS::HandleObject obj, bool keysonly);


#endif 
