








































#ifndef jsdbg_h__
#define jsdbg_h__

#include "jsapi.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jshashtable.h"
#include "jswrapper.h"
#include "jsvalue.h"

namespace js {

class Debug {
    friend JSBool ::JS_DefineDebugObject(JSContext *cx, JSObject *obj);

  private:
    JSObject *object;  
    JSCompartment *debuggeeCompartment;  
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

    JSTrapStatus handleUncaughtException(AutoCompartment &ac, Value *vp, bool callHook);
    JSTrapStatus parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
                                      bool callHook = true);

    static void trace(JSTracer *trc, JSObject *obj);
    static void finalize(JSContext *cx, JSObject *obj);

    static Class jsclass;
    static JSBool getHooks(JSContext *cx, uintN argc, Value *vp);
    static JSBool setHooks(JSContext *cx, uintN argc, Value *vp);
    static JSBool getEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool setEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool getUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp);
    static JSBool setUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp);
    static JSBool construct(JSContext *cx, uintN argc, Value *vp);
    static JSPropertySpec properties[];

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
    Debug(JSObject *dbg, JSObject *hooks, JSCompartment *compartment);
    bool init();
    inline JSObject *toJSObject() const;
    static inline Debug *fromJSObject(JSObject *obj);
    static Debug *fromChildJSObject(JSObject *obj);

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static bool mark(GCMarker *trc, JSCompartment *compartment, JSGCInvocationKind gckind);
    static void sweepAll(JSRuntime *rt);
    static void sweepCompartment(JSCompartment *compartment);

    inline bool observesCompartment(JSCompartment *c) const;
    void detachFrom(JSCompartment *c);

    static inline void leaveStackFrame(JSContext *cx);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, js::Value *vp);
    static inline JSTrapStatus onThrow(JSContext *cx, js::Value *vp);

    

    
    
    
    
    
    
    bool wrapDebuggeeValue(JSContext *cx, Value *vp);

    
    
    
    
    
    
    
    
    
    
    
    
    bool unwrapDebuggeeValue(JSContext *cx, Value *vp);

    
    bool getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp);

    
    
    
    
    
    
    
    
    
    
    
    bool newCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp);
};

bool
Debug::hasAnyLiveHooks() const
{
    return observesDebuggerStatement();
}

bool
Debug::observesCompartment(JSCompartment *c) const
{
    JS_ASSERT(c);
    return debuggeeCompartment == c;
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
    if (!cx->compartment->getDebuggers().empty())
        slowPathLeaveStackFrame(cx);
}

JSTrapStatus
Debug::onDebuggerStatement(JSContext *cx, js::Value *vp)
{
    return cx->compartment->getDebuggers().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp,
                          DebugObservesMethod(&Debug::observesDebuggerStatement),
                          DebugHandleMethod(&Debug::handleDebuggerStatement));
}

JSTrapStatus
Debug::onThrow(JSContext *cx, js::Value *vp)
{
    return cx->compartment->getDebuggers().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp,
                          DebugObservesMethod(&Debug::observesThrow),
                          DebugHandleMethod(&Debug::handleThrow));
}

}

#endif 
