








































#ifndef Debugger_h__
#define Debugger_h__

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jsclist.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsweakmap.h"
#include "jswrapper.h"

#include "gc/Barrier.h"
#include "js/HashTable.h"
#include "vm/GlobalObject.h"

namespace js {

class Debugger {
    friend class Breakpoint;
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
        JSSLOT_DEBUG_ENV_PROTO,
        JSSLOT_DEBUG_OBJECT_PROTO,
        JSSLOT_DEBUG_SCRIPT_PROTO,
        JSSLOT_DEBUG_PROTO_STOP,
        JSSLOT_DEBUG_HOOK_START = JSSLOT_DEBUG_PROTO_STOP,
        JSSLOT_DEBUG_HOOK_STOP = JSSLOT_DEBUG_HOOK_START + HookCount,
        JSSLOT_DEBUG_COUNT = JSSLOT_DEBUG_HOOK_STOP
    };

  private:
    JSCList link;                       
    HeapPtrObject object;               
    GlobalObjectSet debuggees;          
    js::HeapPtrObject uncaughtExceptionHook; 
    bool enabled;
    JSCList breakpoints;                

    












    typedef HashMap<StackFrame *, HeapPtrObject, DefaultHasher<StackFrame *>, RuntimeAllocPolicy>
        FrameMap;
    FrameMap frames;

    
    typedef WeakMap<HeapPtrScript, HeapPtrObject> ScriptWeakMap;
    ScriptWeakMap scripts;

    
    typedef WeakMap<HeapPtrObject, HeapPtrObject> ObjectWeakMap;
    ObjectWeakMap objects;

    
    ObjectWeakMap environments;

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
    void markKeysInCompartment(JSTracer *tracer);

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

    static JSTrapStatus slowPathOnEnterFrame(JSContext *cx, Value *vp);
    static void slowPathOnLeaveFrame(JSContext *cx);
    static void slowPathOnNewScript(JSContext *cx, JSScript *script,
                                    GlobalObject *compileAndGoGlobal);
    static JSTrapStatus dispatchHook(JSContext *cx, Value *vp, Hook which);

    JSTrapStatus fireDebuggerStatement(JSContext *cx, Value *vp);
    JSTrapStatus fireExceptionUnwind(JSContext *cx, Value *vp);
    JSTrapStatus fireEnterFrame(JSContext *cx, Value *vp);

    



    JSObject *newDebuggerScript(JSContext *cx, JSScript *script);

    



    void fireNewScript(JSContext *cx, JSScript *script);

    static inline Debugger *fromLinks(JSCList *links);
    inline Breakpoint *firstBreakpoint() const;

  public:
    Debugger(JSContext *cx, JSObject *dbg);
    ~Debugger();

    bool init(JSContext *cx);
    inline const js::HeapPtrObject &toJSObject() const;
    static inline Debugger *fromJSObject(JSObject *obj);
    static Debugger *fromChildJSObject(JSObject *obj);

    

    














    static void markCrossCompartmentDebuggerObjectReferents(JSTracer *tracer);
    static bool markAllIteratively(GCMarker *trc);
    static void sweepAll(JSContext *cx);
    static void detachAllDebuggersFromGlobal(JSContext *cx, GlobalObject *global,
                                             GlobalObjectSet::Enum *compartmentEnum);

    static inline JSTrapStatus onEnterFrame(JSContext *cx, Value *vp);
    static inline void onLeaveFrame(JSContext *cx);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, Value *vp);
    static inline JSTrapStatus onExceptionUnwind(JSContext *cx, Value *vp);
    static inline void onNewScript(JSContext *cx, JSScript *script,
                                   GlobalObject *compileAndGoGlobal);
    static JSTrapStatus onTrap(JSContext *cx, Value *vp);
    static JSTrapStatus onSingleStep(JSContext *cx, Value *vp);

    

    inline bool observesEnterFrame() const;
    inline bool observesNewScript() const;
    inline bool observesGlobal(GlobalObject *global) const;
    inline bool observesFrame(StackFrame *fp) const;

    




    bool wrapEnvironment(JSContext *cx, Env *env, Value *vp);

    








    bool wrapDebuggeeValue(JSContext *cx, Value *vp);

    


























    bool unwrapDebuggeeValue(JSContext *cx, Value *vp);

    
    bool getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp);

    











    bool newCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp);

    




    JSObject *wrapScript(JSContext *cx, JSScript *script);

  private:
    Debugger(const Debugger &) MOZ_DELETE;
    Debugger & operator=(const Debugger &) MOZ_DELETE;
};

class BreakpointSite {
    friend class Breakpoint;
    friend struct ::JSCompartment;
    friend struct ::JSScript;
    friend class Debugger;

  public:
    JSScript * const script;
    jsbytecode * const pc;

  private:
    




    GlobalObject *scriptGlobal;

    JSCList breakpoints;  
    size_t enabledCount;  
    JSTrapHandler trapHandler;  
    HeapValue trapClosure;

    bool recompile(JSContext *cx, bool forTrap);

  public:
    BreakpointSite(JSScript *script, jsbytecode *pc);
    Breakpoint *firstBreakpoint() const;
    bool hasBreakpoint(Breakpoint *bp);
    bool hasTrap() const { return !!trapHandler; }
    GlobalObject *getScriptGlobal() const { return scriptGlobal; }

    bool inc(JSContext *cx);
    void dec(JSContext *cx);
    bool setTrap(JSContext *cx, JSTrapHandler handler, const Value &closure);
    void clearTrap(JSContext *cx, JSTrapHandler *handlerp = NULL, Value *closurep = NULL);
    void destroyIfEmpty(JSRuntime *rt);
};



















class Breakpoint {
    friend struct ::JSCompartment;
    friend class Debugger;

  public:
    Debugger * const debugger;
    BreakpointSite * const site;
  private:
    js::HeapPtrObject handler;
    JSCList debuggerLinks;
    JSCList siteLinks;

  public:
    static Breakpoint *fromDebuggerLinks(JSCList *links);
    static Breakpoint *fromSiteLinks(JSCList *links);
    Breakpoint(Debugger *debugger, BreakpointSite *site, JSObject *handler);
    void destroy(JSContext *cx);
    Breakpoint *nextInDebugger();
    Breakpoint *nextInSite();
    const HeapPtrObject &getHandler() const { return handler; }
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

const js::HeapPtrObject &
Debugger::toJSObject() const
{
    JS_ASSERT(object);
    return object;
}

Debugger *
Debugger::fromJSObject(JSObject *obj)
{
    JS_ASSERT(js::GetObjectClass(obj) == &jsclass);
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
Debugger::observesGlobal(GlobalObject *global) const
{
    return debuggees.has(global);
}

bool
Debugger::observesFrame(StackFrame *fp) const
{
    return observesGlobal(fp->scopeChain().getGlobal());
}

JSTrapStatus
Debugger::onEnterFrame(JSContext *cx, Value *vp)
{
    if (cx->compartment->getDebuggees().empty())
        return JSTRAP_CONTINUE;
    return slowPathOnEnterFrame(cx, vp);
}

void
Debugger::onLeaveFrame(JSContext *cx)
{
    
    bool evalTraps = cx->fp()->isEvalFrame() &&
                     cx->fp()->script()->hasAnyBreakpointsOrStepMode();
    if (!cx->compartment->getDebuggees().empty() || evalTraps)
        slowPathOnLeaveFrame(cx);
}

JSTrapStatus
Debugger::onDebuggerStatement(JSContext *cx, Value *vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp, OnDebuggerStatement);
}

JSTrapStatus
Debugger::onExceptionUnwind(JSContext *cx, Value *vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp, OnExceptionUnwind);
}

void
Debugger::onNewScript(JSContext *cx, JSScript *script, GlobalObject *compileAndGoGlobal)
{
    JS_ASSERT_IF(script->compileAndGo, compileAndGoGlobal);
    JS_ASSERT_IF(!script->compileAndGo, !compileAndGoGlobal);
    if (!script->compartment()->getDebuggees().empty())
        slowPathOnNewScript(cx, script, compileAndGoGlobal);
}

extern JSBool
EvaluateInEnv(JSContext *cx, Env *env, StackFrame *fp, const jschar *chars,
              uintN length, const char *filename, uintN lineno, Value *rval);

}

#endif
