








































#ifndef jsdbg_h__
#define jsdbg_h__

#include "jsapi.h"
#include "jsclist.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jshashtable.h"
#include "jsweakmap.h"
#include "jswrapper.h"
#include "jsvalue.h"
#include "vm/GlobalObject.h"

namespace js {

class Debug {
    friend class js::Breakpoint;
    friend JSBool (::JS_DefineDebugObject)(JSContext *cx, JSObject *obj);

  private:
    JSCList link;                       
    JSObject *object;                   
    GlobalObjectSet debuggees;          
    JSObject *hooksObject;              
    JSObject *uncaughtExceptionHook;    
    bool enabled;

    
    
    bool hasDebuggerHandler;            
    bool hasThrowHandler;               

    JSCList breakpoints;                

    
    
    
    typedef HashMap<StackFrame *, JSObject *, DefaultHasher<StackFrame *>, SystemAllocPolicy>
        FrameMap;
    FrameMap frames;

    
    typedef WeakMap<JSObject *, JSObject *, DefaultHasher<JSObject *>, CrossCompartmentMarkPolicy>
        ObjectWeakMap;
    ObjectWeakMap objects;

    
    typedef WeakMap<JSObject *, JSObject *, DefaultHasher<JSObject *>, CrossCompartmentMarkPolicy>
        ScriptWeakMap;

    
    
    
    ScriptWeakMap heldScripts;

    
    
    typedef HashMap<JSScript *, JSObject *, DefaultHasher<JSScript *>, SystemAllocPolicy>
        ScriptMap;

    
    
    
    
    ScriptMap evalScripts;

    bool addDebuggeeGlobal(JSContext *cx, GlobalObject *obj);
    void removeDebuggeeGlobal(JSContext *cx, GlobalObject *global,
                              GlobalObjectSet::Enum *compartmentEnum,
                              GlobalObjectSet::Enum *debugEnum);

    JSTrapStatus handleUncaughtException(AutoCompartment &ac, Value *vp, bool callHook);
    JSTrapStatus parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
                                      bool callHook = true);
    JSObject *unwrapDebuggeeArgument(JSContext *cx, Value *vp);

    static void traceObject(JSTracer *trc, JSObject *obj);
    void trace(JSTracer *trc);
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
    static JSBool clearAllBreakpoints(JSContext *cx, uintN argc, Value *vp);
    static JSBool construct(JSContext *cx, uintN argc, Value *vp);
    static JSPropertySpec properties[];
    static JSFunctionSpec methods[];

    inline bool hasAnyLiveHooks() const;

    static void slowPathLeaveStackFrame(JSContext *cx);
    static void slowPathOnDestroyScript(JSScript *script);

    typedef bool (Debug::*DebugObservesMethod)() const;
    typedef JSTrapStatus (Debug::*DebugHandleMethod)(JSContext *, Value *) const;
    static JSTrapStatus dispatchHook(JSContext *cx, js::Value *vp,
                                     DebugObservesMethod observesEvent,
                                     DebugHandleMethod handleEvent);

    bool observesDebuggerStatement() const;
    JSTrapStatus handleDebuggerStatement(JSContext *cx, Value *vp);

    bool observesThrow() const;
    JSTrapStatus handleThrow(JSContext *cx, Value *vp);

    
    
    
    JSObject *newDebugScript(JSContext *cx, JSScript *script, JSObject *obj);

    
    JSObject *wrapHeldScript(JSContext *cx, JSScript *script, JSObject *obj);

    
    void destroyEvalScript(JSScript *script);

    inline Breakpoint *firstBreakpoint() const;

  public:
    Debug(JSObject *dbg, JSObject *hooks);
    ~Debug();

    bool init(JSContext *cx);
    inline JSObject *toJSObject() const;
    static inline Debug *fromJSObject(JSObject *obj);
    static Debug *fromChildJSObject(JSObject *obj);

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static void markCrossCompartmentDebugObjectReferents(JSTracer *tracer);
    static bool mark(GCMarker *trc, JSCompartment *compartment, JSGCInvocationKind gckind);
    static void sweepAll(JSContext *cx);
    static void detachAllDebuggersFromGlobal(JSContext *cx, GlobalObject *global,
                                             GlobalObjectSet::Enum *compartmentEnum);

    static inline void leaveStackFrame(JSContext *cx);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, js::Value *vp);
    static inline JSTrapStatus onThrow(JSContext *cx, js::Value *vp);
    static inline void onDestroyScript(JSScript *script);
    static JSTrapStatus onTrap(JSContext *cx, Value *vp);

    

    inline bool observesScope(JSObject *obj) const;
    inline bool observesFrame(StackFrame *fp) const;

    
    
    
    
    
    
    bool wrapDebuggeeValue(JSContext *cx, Value *vp);

    
    
    
    
    
    
    
    
    
    
    
    
    bool unwrapDebuggeeValue(JSContext *cx, Value *vp);

    
    bool getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp);

    
    
    
    
    
    
    
    
    
    
    
    bool newCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp);

    
    
    
    JSObject *wrapFunctionScript(JSContext *cx, JSFunction *fun);

    
    
    
    
    JSObject *wrapJSAPIScript(JSContext *cx, JSObject *scriptObj);

    
    
    
    JSObject *wrapEvalScript(JSContext *cx, JSScript *script);

  private:
    
    Debug(const Debug &);
    Debug & operator=(const Debug &);
};

bool
Debug::hasAnyLiveHooks() const
{
    return enabled && (hasDebuggerHandler || hasThrowHandler || !JS_CLIST_IS_EMPTY(&breakpoints));
}

class BreakpointSite {
    friend class js::Breakpoint;
    friend class ::JSCompartment;
    friend class js::Debug;

  public:
    JSScript * const script;
    jsbytecode * const pc;
    const JSOp realOpcode;

  private:
    
    
    
    JSObject *scriptObject;

    JSCList breakpoints;  
    size_t enabledCount;  
    JSTrapHandler trapHandler;  
    Value trapClosure;

    bool recompile(JSContext *cx, bool forTrap);

  public:
    BreakpointSite(JSScript *script, jsbytecode *pc);
    Breakpoint *firstBreakpoint() const;
    bool hasBreakpoint(Breakpoint *bp);

    bool inc(JSContext *cx);
    void dec(JSContext *cx);
    bool setTrap(JSContext *cx, JSTrapHandler handler, const Value &closure);
    void clearTrap(JSContext *cx, BreakpointSiteMap::Enum *e = NULL,
                   JSTrapHandler *handlerp = NULL, Value *closurep = NULL);
    void destroyIfEmpty(JSRuntime *rt, BreakpointSiteMap::Enum *e);
};

class Breakpoint {
    friend class ::JSCompartment;
    friend class js::Debug;

  public:
    Debug * const debugger;
    BreakpointSite * const site;
  private:
    JSObject *handler;
    JSCList debuggerLinks;
    JSCList siteLinks;

  public:
    static Breakpoint *fromDebuggerLinks(JSCList *links);
    static Breakpoint *fromSiteLinks(JSCList *links);
    Breakpoint(Debug *debugger, BreakpointSite *site, JSObject *handler);
    void destroy(JSContext *cx, BreakpointSiteMap::Enum *e = NULL);
    Breakpoint *nextInDebugger();
    Breakpoint *nextInSite();
    JSObject *getHandler() const { return handler; }
};

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

Breakpoint *
Debug::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return NULL;
    return Breakpoint::fromDebuggerLinks(JS_NEXT_LINK(&breakpoints));
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
    if (!cx->compartment->getDebuggees().empty() || !cx->compartment->breakpointSites.empty())
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

void
Debug::onDestroyScript(JSScript *script)
{
    if (!script->compartment->getDebuggees().empty())
        slowPathOnDestroyScript(script);
}

extern JSBool
EvaluateInScope(JSContext *cx, JSObject *scobj, StackFrame *fp, const jschar *chars,
                uintN length, const char *filename, uintN lineno, Value *rval);

}

#endif 
