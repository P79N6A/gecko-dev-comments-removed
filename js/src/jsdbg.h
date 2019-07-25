








































#ifndef jsdbg_h__
#define jsdbg_h__

#include "jsapi.h"
#include "jsgc.h"
#include "jswrapper.h"
#include "jsvalue.h"

namespace js {

class Debug {
    friend JSBool ::JS_DefineDebugObject(JSContext *cx, JSObject *obj);

  private:
    JSObject *object;  
    JSCompartment *debuggeeCompartment;  
    JSObject *hooksObject;  

    bool enabled;

    
    
    bool hasDebuggerHandler;

    JSTrapStatus fireUncaughtExceptionHook(JSContext *cx);
    JSTrapStatus parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp);

    static void trace(JSTracer *trc, JSObject *obj);
    static void finalize(JSContext *cx, JSObject *obj);

    static Class jsclass;
    static JSBool getHooks(JSContext *cx, uintN argc, Value *vp);
    static JSBool setHooks(JSContext *cx, uintN argc, Value *vp);
    static JSBool getEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool setEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool construct(JSContext *cx, uintN argc, Value *vp);
    static JSPropertySpec properties[];

    bool hasAnyLiveHooks() const { return observesDebuggerStatement(); }

  public:
    Debug(JSObject *dbg, JSObject *hooks, JSCompartment *compartment);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static bool mark(GCMarker *trc, JSCompartment *compartment, JSGCInvocationKind gckind);

    JSObject *toJSObject() const {
        JS_ASSERT(object);
        return object;
    }

    static Debug *fromJSObject(JSObject *obj) {
        JS_ASSERT(obj->getClass() == &jsclass);
        return (Debug *) obj->getPrivate();
    }

    bool observesCompartment(JSCompartment *c) const {
        JS_ASSERT(c);
        return debuggeeCompartment == c;
    }

    bool observesDebuggerStatement() const { return enabled && hasDebuggerHandler; }
    JSTrapStatus onDebuggerStatement(JSContext *cx, Value *vp);
};

}

#endif 
