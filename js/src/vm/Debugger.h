








































#ifndef Debugger_h__
#define Debugger_h__

#include "jsapi.h"
#include "jsclist.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jshashtable.h"
#include "jsweakmap.h"
#include "jswrapper.h"
#include "vm/GlobalObject.h"

namespace js {

class Debugger {
    friend class js::Breakpoint;
    friend JSBool (::JS_DefineDebuggerObject)(JSContext *cx, JSObject *obj);

  public:
    enum Hook {
        OnDebuggerStatement,
        OnExceptionUnwind,
        OnNewScript,
        OnEnterFrame,
        HookCount
    };

    enum {
        JSSLOT_DEBUG_PROTO_START,
        JSSLOT_DEBUG_FRAME_PROTO = JSSLOT_DEBUG_PROTO_START,
        JSSLOT_DEBUG_OBJECT_PROTO,
        JSSLOT_DEBUG_SCRIPT_PROTO,
        JSSLOT_DEBUG_PROTO_STOP,
        JSSLOT_DEBUG_HOOK_START = JSSLOT_DEBUG_PROTO_STOP,
        JSSLOT_DEBUG_HOOK_STOP = JSSLOT_DEBUG_HOOK_START + HookCount,
        JSSLOT_DEBUG_COUNT = JSSLOT_DEBUG_HOOK_STOP
    };

  private:
    JSCList link;                       
    JSObject *object;                   
    GlobalObjectSet debuggees;          
    JSObject *uncaughtExceptionHook;    
    bool enabled;
    JSCList breakpoints;                

    












    typedef HashMap<StackFrame *, JSObject *, DefaultHasher<StackFrame *>, RuntimeAllocPolicy>
        FrameMap;
    FrameMap frames;

    typedef WeakMap<gc::Cell *, JSObject *, DefaultHasher<gc::Cell *>, CrossCompartmentMarkPolicy>
        CellWeakMap;

    
    CellWeakMap objects;

    
    CellWeakMap scripts;

    bool addDebuggeeGlobal(JSContext *cx, GlobalObject *obj);
    void removeDebuggeeGlobal(JSContext *cx, GlobalObject *global,
                              GlobalObjectSet::Enum *compartmentEnum,
                              GlobalObjectSet::Enum *debugEnum);

    













    JSTrapStatus handleUncaughtException(AutoCompartment &ac, Value *vp, bool callHook);

    
























    JSTrapStatus parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
                                      bool callHook = true);

    JSObject *unwrapDebuggeeArgument(JSContext *cx, const Value &v);

    static void traceObject(JSTracer *trc, JSObject *obj);
    void trace(JSTracer *trc);
    static void finalize(JSContext *cx, JSObject *obj);
    static void markKeysInCompartment(JSTracer *tracer, const CellWeakMap &map, bool scripts);

    static Class jsclass;

    static Debugger *fromThisValue(JSContext *cx, const CallArgs &ca, const char *fnname);
    static JSBool getEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool setEnabled(JSContext *cx, uintN argc, Value *vp);
    static JSBool getHookImpl(JSContext *cx, uintN argc, Value *vp, Hook which);
    static JSBool setHookImpl(JSContext *cx, uintN argc, Value *vp, Hook which);
    static JSBool getOnDebuggerStatement(JSContext *cx, uintN argc, Value *vp);
    static JSBool setOnDebuggerStatement(JSContext *cx, uintN argc, Value *vp);
    static JSBool getOnExceptionUnwind(JSContext *cx, uintN argc, Value *vp);
    static JSBool setOnExceptionUnwind(JSContext *cx, uintN argc, Value *vp);
    static JSBool getOnNewScript(JSContext *cx, uintN argc, Value *vp);
    static JSBool setOnNewScript(JSContext *cx, uintN argc, Value *vp);
    static JSBool getOnEnterFrame(JSContext *cx, uintN argc, Value *vp);
    static JSBool setOnEnterFrame(JSContext *cx, uintN argc, Value *vp);
    static JSBool getUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp);
    static JSBool setUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp);
    static JSBool addDebuggee(JSContext *cx, uintN argc, Value *vp);
    static JSBool removeDebuggee(JSContext *cx, uintN argc, Value *vp);
    static JSBool hasDebuggee(JSContext *cx, uintN argc, Value *vp);
    static JSBool getDebuggees(JSContext *cx, uintN argc, Value *vp);
    static JSBool getNewestFrame(JSContext *cx, uintN argc, Value *vp);
    static JSBool clearAllBreakpoints(JSContext *cx, uintN argc, Value *vp);
    static JSBool construct(JSContext *cx, uintN argc, Value *vp);
    static JSPropertySpec properties[];
    static JSFunctionSpec methods[];

    JSObject *getHook(Hook hook) const;
    bool hasAnyLiveHooks(JSContext *cx) const;

    static void slowPathOnEnterFrame(JSContext *cx);
    static void slowPathOnLeaveFrame(JSContext *cx);
    static void slowPathOnNewScript(JSContext *cx, JSScript *script, JSObject *obj,
                                    GlobalObject *compileAndGoGlobal);
    static JSTrapStatus dispatchHook(JSContext *cx, js::Value *vp, Hook which);

    JSTrapStatus fireDebuggerStatement(JSContext *cx, Value *vp);
    JSTrapStatus fireExceptionUnwind(JSContext *cx, Value *vp);
    void fireEnterFrame(JSContext *cx);

    




    JSObject *newDebuggerScript(JSContext *cx, JSScript *script, JSObject *obj);

    




    void fireNewScript(JSContext *cx, JSScript *script, JSObject *obj);

    static inline Debugger *fromLinks(JSCList *links);
    inline Breakpoint *firstBreakpoint() const;

  public:
    Debugger(JSContext *cx, JSObject *dbg);
    ~Debugger();

    bool init(JSContext *cx);
    inline JSObject *toJSObject() const;
    static inline Debugger *fromJSObject(JSObject *obj);
    static Debugger *fromChildJSObject(JSObject *obj);

    

    














    static void markCrossCompartmentDebuggerObjectReferents(JSTracer *tracer);
    static bool markAllIteratively(GCMarker *trc);
    static void sweepAll(JSContext *cx);
    static void detachAllDebuggersFromGlobal(JSContext *cx, GlobalObject *global,
                                             GlobalObjectSet::Enum *compartmentEnum);

    static inline void onEnterFrame(JSContext *cx);
    static inline void onLeaveFrame(JSContext *cx);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, js::Value *vp);
    static inline JSTrapStatus onExceptionUnwind(JSContext *cx, js::Value *vp);
    static inline void onNewScript(JSContext *cx, JSScript *script, JSObject *obj,
                                   GlobalObject *compileAndGoGlobal);
    static JSTrapStatus onTrap(JSContext *cx, Value *vp);
    static JSTrapStatus onSingleStep(JSContext *cx, Value *vp);

    

    inline bool observesEnterFrame() const;
    inline bool observesNewScript() const;
    inline bool observesScope(JSObject *obj) const;
    inline bool observesFrame(StackFrame *fp) const;

    








    bool wrapDebuggeeValue(JSContext *cx, Value *vp);

    


























    bool unwrapDebuggeeValue(JSContext *cx, Value *vp);

    
    bool getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp);

    











    bool newCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp);

    





    JSObject *wrapFunctionScript(JSContext *cx, JSFunction *fun);

    





    JSObject *wrapScript(JSContext *cx, JSScript *script, JSObject *obj);

  private:
    
    Debugger(const Debugger &);
    Debugger & operator=(const Debugger &);
};

class BreakpointSite {
    friend class js::Breakpoint;
    friend struct ::JSCompartment;
    friend class js::Debugger;

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
    bool hasTrap() const { return !!trapHandler; }
    JSObject *getScriptObject() const { return scriptObject; }

    bool inc(JSContext *cx);
    void dec(JSContext *cx);
    bool setTrap(JSContext *cx, JSTrapHandler handler, const Value &closure);
    void clearTrap(JSContext *cx, BreakpointSiteMap::Enum *e = NULL,
                   JSTrapHandler *handlerp = NULL, Value *closurep = NULL);
    void destroyIfEmpty(JSRuntime *rt, BreakpointSiteMap::Enum *e);
};



















class Breakpoint {
    friend struct ::JSCompartment;
    friend class js::Debugger;

  public:
    Debugger * const debugger;
    BreakpointSite * const site;
  private:
    JSObject *handler;
    JSCList debuggerLinks;
    JSCList siteLinks;

  public:
    static Breakpoint *fromDebuggerLinks(JSCList *links);
    static Breakpoint *fromSiteLinks(JSCList *links);
    Breakpoint(Debugger *debugger, BreakpointSite *site, JSObject *handler);
    void destroy(JSContext *cx, BreakpointSiteMap::Enum *e = NULL);
    Breakpoint *nextInDebugger();
    Breakpoint *nextInSite();
    JSObject *getHandler() const { return handler; }
};

Debugger *
Debugger::fromLinks(JSCList *links)
{
    unsigned char *p = reinterpret_cast<unsigned char *>(links);
    return reinterpret_cast<Debugger *>(p - offsetof(Debugger, link));
}

Breakpoint *
Debugger::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return NULL;
    return Breakpoint::fromDebuggerLinks(JS_NEXT_LINK(&breakpoints));
}

JSObject *
Debugger::toJSObject() const
{
    JS_ASSERT(object);
    return object;
}

Debugger *
Debugger::fromJSObject(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &jsclass);
    return (Debugger *) obj->getPrivate();
}

bool
Debugger::observesEnterFrame() const
{
    return enabled && getHook(OnEnterFrame);
}

bool
Debugger::observesNewScript() const
{
    return enabled && getHook(OnNewScript);
}

bool
Debugger::observesScope(JSObject *obj) const
{
    return debuggees.has(obj->getGlobal());
}

bool
Debugger::observesFrame(StackFrame *fp) const
{
    return observesScope(&fp->scopeChain());
}

void
Debugger::onEnterFrame(JSContext *cx)
{
    if (!cx->compartment->getDebuggees().empty())
        slowPathOnEnterFrame(cx);
}

void
Debugger::onLeaveFrame(JSContext *cx)
{
    if (!cx->compartment->getDebuggees().empty() || !cx->compartment->breakpointSites.empty())
        slowPathOnLeaveFrame(cx);
}

JSTrapStatus
Debugger::onDebuggerStatement(JSContext *cx, js::Value *vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp, OnDebuggerStatement);
}

JSTrapStatus
Debugger::onExceptionUnwind(JSContext *cx, js::Value *vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp, OnExceptionUnwind);
}

void
Debugger::onNewScript(JSContext *cx, JSScript *script, JSObject *obj,
                      GlobalObject *compileAndGoGlobal)
{
    JS_ASSERT_IF(script->compileAndGo, compileAndGoGlobal);
    JS_ASSERT_IF(!script->compileAndGo, !compileAndGoGlobal);
    if (!script->compartment()->getDebuggees().empty())
        slowPathOnNewScript(cx, script, obj, compileAndGoGlobal);
}

extern JSBool
EvaluateInScope(JSContext *cx, JSObject *scobj, StackFrame *fp, const jschar *chars,
                uintN length, const char *filename, uintN lineno, Value *rval);

}

#endif
