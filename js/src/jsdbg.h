








































#ifndef jsdbg_h__
#define jsdbg_h__

#include "jsapi.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jshashtable.h"
#include "jswrapper.h"
#include "jsvalue.h"
#include "vm/GlobalObject.h"

namespace js {

class Debug {
    friend JSBool ::JS_DefineDebugObject(JSContext *cx, JSObject *obj);

  private:
    JSCList link;                       
    JSObject *object;                   
    GlobalObjectSet debuggees;          
    JSObject *hooksObject;              
    JSObject *uncaughtExceptionHook;    
    bool enabled;

    
    
    bool hasDebuggerHandler;            
    bool hasThrowHandler;               

    
    
    
    typedef HashMap<StackFrame *, JSObject *, DefaultHasher<StackFrame *>, SystemAllocPolicy>
        FrameMap;
    FrameMap frames;

    
    
    
    typedef HashMap<JSObject *, JSObject *, DefaultHasher<JSObject *>, SystemAllocPolicy>
        ObjectMap;
    ObjectMap objects;

    bool addDebuggeeGlobal(JSContext *cx, GlobalObject *obj);
    void removeDebuggeeGlobal(GlobalObject *global, GlobalObjectSet::Enum *compartmentEnum,
                              GlobalObjectSet::Enum *debugEnum);

    JSTrapStatus handleUncaughtException(AutoCompartment &ac, Value *vp, bool callHook);
    JSTrapStatus parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
                                      bool callHook = true);
    JSObject *unwrapDebuggeeArgument(JSContext *cx, Value *vp);

    static void trace(JSTracer *trc, JSObject *obj);
    static void finalize(JSContext *cx, JSObject *obj);

    static Class jsclass;
    static JSBool getHooks(JSContext *cx, uintN argc, Value *vp);
    static JSBool setHooks(JSContext *cx, uintN argc, Value *vp);
    static JSBool getEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool setEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool getUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp);
    static JSBool setUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp);
    static JSBool addDebuggee(JSContext *cx, uintN argc, Value *vp);
    static JSBool removeDebuggee(JSContext *cx, uintN argc, Value *vp);
    static JSBool hasDebuggee(JSContext *cx, uintN argc, Value *vp);
    static JSBool getDebuggees(JSContext *cx, uintN argc, Value *vp);
    static JSBool getYoungestFrame(JSContext *cx, uintN argc, Value *vp);
    static JSBool construct(JSContext *cx, uintN argc, Value *vp);
    static JSPropertySpec properties[];
    static JSFunctionSpec methods[];

    inline bool hasAnyLiveHooks() const;

    static void slowPathLeaveStackFrame(JSContext *cx);

    typedef bool (Debug::*DebugObservesMethod)() const;
    typedef JSTrapStatus (Debug::*DebugHandleMethod)(JSContext *, Value *) const;
    static JSTrapStatus dispatchHook(JSContext *cx, js::Value *vp,
                                     DebugObservesMethod observesEvent,
                                     DebugHandleMethod handleEvent);

    bool observesDebuggerStatement() const;
    JSTrapStatus handleDebuggerStatement(JSContext *cx, Value *vp);

    bool observesThrow() const;
    JSTrapStatus handleThrow(JSContext *cx, Value *vp);

  public:
    Debug(JSObject *dbg, JSObject *hooks);
    ~Debug();

    bool init(JSContext *cx);
    inline JSObject *toJSObject() const;
    static inline Debug *fromJSObject(JSObject *obj);
    static Debug *fromChildJSObject(JSObject *obj);
    static void detachFromCompartment(JSCompartment *comp);

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static bool mark(GCMarker *trc, JSCompartment *compartment, JSGCInvocationKind gckind);
    static void sweepAll(JSRuntime *rt);
    static void sweepCompartment(JSCompartment *compartment);
    static void detachAllDebuggersFromGlobal(GlobalObject *global,
                                             GlobalObjectSet::Enum *compartmentEnum);

    static inline void leaveStackFrame(JSContext *cx);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, js::Value *vp);
    static inline JSTrapStatus onThrow(JSContext *cx, js::Value *vp);

    

    inline bool observesScope(JSObject *obj) const;
    inline bool observesFrame(StackFrame *fp) const;

    
    
    
    
    
    
    bool wrapDebuggeeValue(JSContext *cx, Value *vp);

    
    
    
    
    
    
    
    
    
    
    
    
    bool unwrapDebuggeeValue(JSContext *cx, Value *vp);

    
    bool getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp);

    
    
    
    
    
    
    
    
    
    
    
    bool newCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp);

  private:
    
    Debug(const Debug &);
    Debug & operator=(const Debug &);
};

bool
Debug::hasAnyLiveHooks() const
{
    return enabled && (hasDebuggerHandler || hasThrowHandler);
}

bool
Debug::observesScope(JSObject *obj) const
{
    return debuggees.has(obj->getGlobal());
}

bool
Debug::observesFrame(StackFrame *fp) const
{
    return observesScope(&fp->scopeChain());
}

JSObject *
Debug::toJSObject() const
{
    JS_ASSERT(object);
    return object;
}

Debug *
Debug::fromJSObject(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &jsclass);
    return (Debug *) obj->getPrivate();
}

void
Debug::leaveStackFrame(JSContext *cx)
{
    if (!cx->compartment->getDebuggees().empty())
        slowPathLeaveStackFrame(cx);
}

JSTrapStatus
Debug::onDebuggerStatement(JSContext *cx, js::Value *vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp,
                          DebugObservesMethod(&Debug::observesDebuggerStatement),
                          DebugHandleMethod(&Debug::handleDebuggerStatement));
}

JSTrapStatus
Debug::onThrow(JSContext *cx, js::Value *vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp,
                          DebugObservesMethod(&Debug::observesThrow),
                          DebugHandleMethod(&Debug::handleThrow));
}

extern JSBool
EvaluateInScope(JSContext *cx, JSObject *scobj, StackFrame *fp, const jschar *chars,
                uintN length, const char *filename, uintN lineno, Value *rval);

}

#endif 
