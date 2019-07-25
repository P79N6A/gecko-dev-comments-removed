






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

    class FrameRange;
    class ScriptQuery;

    bool addDebuggeeGlobal(JSContext *cx, Handle<GlobalObject*> obj);
    void removeDebuggeeGlobal(FreeOp *fop, GlobalObject *global,
                              GlobalObjectSet::Enum *compartmentEnum,
                              GlobalObjectSet::Enum *debugEnum);

    













    JSTrapStatus handleUncaughtException(Maybe<AutoCompartment> &ac, Value *vp, bool callHook);

    
























    JSTrapStatus parseResumptionValue(Maybe<AutoCompartment> &ac, bool ok, const Value &rv,
                                      Value *vp, bool callHook = true);

    JSObject *unwrapDebuggeeArgument(JSContext *cx, const Value &v);

    static void traceObject(JSTracer *trc, JSObject *obj);
    void trace(JSTracer *trc);
    static void finalize(FreeOp *fop, JSObject *obj);
    void markKeysInCompartment(JSTracer *tracer);

    static Class jsclass;

    static Debugger *fromThisValue(JSContext *cx, const CallArgs &ca, const char *fnname);
    static JSBool getEnabled(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setEnabled(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which);
    static JSBool setHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which);
    static JSBool getOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getOnNewScript(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setOnNewScript(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getOnEnterFrame(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setOnEnterFrame(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp);
    static JSBool addDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static JSBool removeDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static JSBool hasDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getNewestFrame(JSContext *cx, unsigned argc, Value *vp);
    static JSBool clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp);
    static JSBool findScripts(JSContext *cx, unsigned argc, Value *vp);
    static JSBool wrap(JSContext *cx, unsigned argc, Value *vp);
    static JSBool construct(JSContext *cx, unsigned argc, Value *vp);
    static JSPropertySpec properties[];
    static JSFunctionSpec methods[];

    JSObject *getHook(Hook hook) const;
    bool hasAnyLiveHooks() const;

    static JSTrapStatus slowPathOnEnterFrame(JSContext *cx, Value *vp);
    static bool slowPathOnLeaveFrame(JSContext *cx, bool ok);
    static void slowPathOnNewScript(JSContext *cx, JSScript *script,
                                    GlobalObject *compileAndGoGlobal);
    static JSTrapStatus dispatchHook(JSContext *cx, Value *vp, Hook which);

    JSTrapStatus fireDebuggerStatement(JSContext *cx, Value *vp);
    JSTrapStatus fireExceptionUnwind(JSContext *cx, Value *vp);
    JSTrapStatus fireEnterFrame(JSContext *cx, Value *vp);

    



    JSObject *newDebuggerScript(JSContext *cx, HandleScript script);

    



    void fireNewScript(JSContext *cx, HandleScript script);

    static inline Debugger *fromLinks(JSCList *links);
    inline Breakpoint *firstBreakpoint() const;

  public:
    Debugger(JSContext *cx, JSObject *dbg);
    ~Debugger();

    bool init(JSContext *cx);
    inline const js::HeapPtrObject &toJSObject() const;
    inline js::HeapPtrObject &toJSObjectRef();
    static inline Debugger *fromJSObject(JSObject *obj);
    static Debugger *fromChildJSObject(JSObject *obj);

    

    














    static void markCrossCompartmentDebuggerObjectReferents(JSTracer *tracer);
    static bool markAllIteratively(GCMarker *trc);
    static void sweepAll(FreeOp *fop);
    static void detachAllDebuggersFromGlobal(FreeOp *fop, GlobalObject *global,
                                             GlobalObjectSet::Enum *compartmentEnum);

    static inline JSTrapStatus onEnterFrame(JSContext *cx, Value *vp);
    static inline bool onLeaveFrame(JSContext *cx, bool ok);
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
    bool observesScript(JSScript *script) const;

    




    bool wrapEnvironment(JSContext *cx, Handle<Env*> env, Value *vp);

    








    bool wrapDebuggeeValue(JSContext *cx, Value *vp);

    


























    bool unwrapDebuggeeValue(JSContext *cx, Value *vp);

    
    bool getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp);

    






    static void resultToCompletion(JSContext *cx, bool ok, const Value &rv,
                                   JSTrapStatus *status, Value *value);

    




    bool newCompletionValue(JSContext *cx, JSTrapStatus status, Value value, Value *result);

    











    bool receiveCompletionValue(Maybe<AutoCompartment> &ac, bool ok, Value val, Value *vp);

    




    JSObject *wrapScript(JSContext *cx, HandleScript script);

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
    JSScript *script;
    jsbytecode * const pc;

  private:
    JSCList breakpoints;  
    size_t enabledCount;  
    JSTrapHandler trapHandler;  
    HeapValue trapClosure;

    void recompile(FreeOp *fop);

  public:
    BreakpointSite(JSScript *script, jsbytecode *pc);
    Breakpoint *firstBreakpoint() const;
    bool hasBreakpoint(Breakpoint *bp);
    bool hasTrap() const { return !!trapHandler; }

    void inc(FreeOp *fop);
    void dec(FreeOp *fop);
    void setTrap(FreeOp *fop, JSTrapHandler handler, const Value &closure);
    void clearTrap(FreeOp *fop, JSTrapHandler *handlerp = NULL, Value *closurep = NULL);
    void destroyIfEmpty(FreeOp *fop);
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
    void destroy(FreeOp *fop);
    Breakpoint *nextInDebugger();
    Breakpoint *nextInSite();
    const HeapPtrObject &getHandler() const { return handler; }
    HeapPtrObject &getHandlerRef() { return handler; }
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

js::HeapPtrObject &
Debugger::toJSObjectRef()
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
    return observesGlobal(&fp->global());
}

JSTrapStatus
Debugger::onEnterFrame(JSContext *cx, Value *vp)
{
    if (cx->compartment->getDebuggees().empty())
        return JSTRAP_CONTINUE;
    return slowPathOnEnterFrame(cx, vp);
}

bool
Debugger::onLeaveFrame(JSContext *cx, bool ok)
{
    
    bool evalTraps = cx->fp()->isEvalFrame() &&
                     cx->fp()->script()->hasAnyBreakpointsOrStepMode();
    if (!cx->compartment->getDebuggees().empty() || evalTraps)
        ok = slowPathOnLeaveFrame(cx, ok);
    return ok;
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
EvaluateInEnv(JSContext *cx, Handle<Env*> env, StackFrame *fp, const jschar *chars,
              unsigned length, const char *filename, unsigned lineno, Value *rval);

}

#endif
