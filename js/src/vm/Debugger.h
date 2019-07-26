






#ifndef Debugger_h__
#define Debugger_h__

#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"

#include "jsapi.h"
#include "jsclist.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsweakmap.h"
#include "jswrapper.h"

#include "gc/Barrier.h"
#include "gc/FindSCCs.h"
#include "js/HashTable.h"
#include "vm/GlobalObject.h"

namespace js {

















template <class Key, class Value>
class DebuggerWeakMap : private WeakMap<Key, Value, DefaultHasher<Key> >
{
  private:
    typedef HashMap<JS::Zone *,
                    uintptr_t,
                    DefaultHasher<JS::Zone *>,
                    RuntimeAllocPolicy> CountMap;

    CountMap zoneCounts;

  public:
    typedef WeakMap<Key, Value, DefaultHasher<Key> > Base;
    explicit DebuggerWeakMap(JSContext *cx)
        : Base(cx), zoneCounts(cx) { }

  public:
    

    typedef typename Base::Ptr Ptr;
    typedef typename Base::AddPtr AddPtr;
    typedef typename Base::Range Range;
    typedef typename Base::Enum Enum;
    typedef typename Base::Lookup Lookup;

    bool init(uint32_t len = 16) {
        return Base::init(len) && zoneCounts.init();
    }

    AddPtr lookupForAdd(const Lookup &l) const {
        return Base::lookupForAdd(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        JS_ASSERT(v->compartment() == Base::compartment);
        if (!incZoneCount(k->zone()))
            return false;
        bool ok = Base::relookupOrAdd(p, k, v);
        if (!ok)
            decZoneCount(k->zone());
        return ok;
    }

    Range all() const {
        return Base::all();
    }

    void remove(const Lookup &l) {
        Base::remove(l);
        decZoneCount(l->zone());
    }

  public:
    
    void trace(JSTracer *tracer) {
        Base::trace(tracer);
    }

  public:
    void markKeys(JSTracer *tracer) {
        for (Range r = all(); !r.empty(); r.popFront()) {
            Key key = r.front().key;
            gc::Mark(tracer, &key, "cross-compartment WeakMap key");
            JS_ASSERT(key == r.front().key);
        }
    }

    bool hasKeyInZone(JS::Zone *zone) {
        CountMap::Ptr p = zoneCounts.lookup(zone);
        JS_ASSERT_IF(p, p->value > 0);
        return p;
    }

  private:
    
    void sweep() {
        for (Enum e(*static_cast<Base *>(this)); !e.empty(); e.popFront()) {
            Key k(e.front().key);
            Value v(e.front().value);
            if (gc::IsAboutToBeFinalized(&k)) {
                e.removeFront();
                decZoneCount(k->zone());
            }
        }
        Base::assertEntriesNotAboutToBeFinalized();
    }

    bool incZoneCount(JS::Zone *zone) {
        CountMap::Ptr p = zoneCounts.lookupWithDefault(zone, 0);
        if (!p)
            return false;
        ++p->value;
        return true;
    }

    void decZoneCount(JS::Zone *zone) {
        CountMap::Ptr p = zoneCounts.lookup(zone);
        JS_ASSERT(p);
        JS_ASSERT(p->value > 0);
        --p->value;
        if (p->value == 0)
            zoneCounts.remove(zone);
    }
};

class Debugger : private mozilla::LinkedListElement<Debugger>
{
    friend class Breakpoint;
    friend class mozilla::LinkedListElement<Debugger>;
    friend JSBool (::JS_DefineDebuggerObject)(JSContext *cx, JSObject *obj);

  public:
    enum Hook {
        OnDebuggerStatement,
        OnExceptionUnwind,
        OnNewScript,
        OnEnterFrame,
        OnNewGlobalObject,
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
    HeapPtrObject object;               
    GlobalObjectSet debuggees;          
    js::HeapPtrObject uncaughtExceptionHook; 
    bool enabled;
    JSCList breakpoints;                

    





    JSCList onNewGlobalObjectWatchersLink;

    












    typedef HashMap<AbstractFramePtr,
                    RelocatablePtrObject,
                    DefaultHasher<AbstractFramePtr>,
                    RuntimeAllocPolicy> FrameMap;
    FrameMap frames;

    
    typedef DebuggerWeakMap<EncapsulatedPtrScript, RelocatablePtrObject> ScriptWeakMap;
    ScriptWeakMap scripts;

    
    typedef DebuggerWeakMap<EncapsulatedPtrObject, RelocatablePtrObject> ObjectWeakMap;
    ObjectWeakMap objects;

    
    ObjectWeakMap environments;

    class FrameRange;
    class ScriptQuery;

    bool addDebuggeeGlobal(JSContext *cx, Handle<GlobalObject*> obj);
    bool addDebuggeeGlobal(JSContext *cx, Handle<GlobalObject*> obj,
                           AutoDebugModeGC &dmgc);
    void removeDebuggeeGlobal(FreeOp *fop, GlobalObject *global,
                              GlobalObjectSet::Enum *compartmentEnum,
                              GlobalObjectSet::Enum *debugEnum);
    void removeDebuggeeGlobal(FreeOp *fop, GlobalObject *global,
                              AutoDebugModeGC &dmgc,
                              GlobalObjectSet::Enum *compartmentEnum,
                              GlobalObjectSet::Enum *debugEnum);

    













    JSTrapStatus handleUncaughtException(mozilla::Maybe<AutoCompartment> &ac, bool callHook);
    JSTrapStatus handleUncaughtException(mozilla::Maybe<AutoCompartment> &ac, MutableHandleValue vp, bool callHook);

    JSTrapStatus handleUncaughtExceptionHelper(mozilla::Maybe<AutoCompartment> &ac,
                                               MutableHandleValue *vp, bool callHook);

    
























    JSTrapStatus parseResumptionValue(mozilla::Maybe<AutoCompartment> &ac, bool ok, const Value &rv,
                                      MutableHandleValue vp, bool callHook = true);

    GlobalObject *unwrapDebuggeeArgument(JSContext *cx, const Value &v);

    static void traceObject(JSTracer *trc, RawObject obj);
    void trace(JSTracer *trc);
    static void finalize(FreeOp *fop, RawObject obj);
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
    static JSBool getOnNewGlobalObject(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setOnNewGlobalObject(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp);
    static JSBool setUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp);
    static JSBool addDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static JSBool addAllGlobalsAsDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static JSBool removeDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static JSBool removeAllDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static JSBool hasDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static JSBool getNewestFrame(JSContext *cx, unsigned argc, Value *vp);
    static JSBool clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp);
    static JSBool findScripts(JSContext *cx, unsigned argc, Value *vp);
    static JSBool findAllGlobals(JSContext *cx, unsigned argc, Value *vp);
    static JSBool wrap(JSContext *cx, unsigned argc, Value *vp);
    static JSBool construct(JSContext *cx, unsigned argc, Value *vp);
    static JSPropertySpec properties[];
    static JSFunctionSpec methods[];

    JSObject *getHook(Hook hook) const;
    bool hasAnyLiveHooks() const;

    static JSTrapStatus slowPathOnEnterFrame(JSContext *cx, MutableHandleValue vp);
    static bool slowPathOnLeaveFrame(JSContext *cx, bool ok);
    static void slowPathOnNewScript(JSContext *cx, HandleScript script,
                                    GlobalObject *compileAndGoGlobal);
    static bool slowPathOnNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global);
    static JSTrapStatus dispatchHook(JSContext *cx, MutableHandleValue vp, Hook which);

    JSTrapStatus fireDebuggerStatement(JSContext *cx, MutableHandleValue vp);
    JSTrapStatus fireExceptionUnwind(JSContext *cx, MutableHandleValue vp);
    JSTrapStatus fireEnterFrame(JSContext *cx, MutableHandleValue vp);
    JSTrapStatus fireNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global, MutableHandleValue vp);

    



    JSObject *newDebuggerScript(JSContext *cx, HandleScript script);

    



    void fireNewScript(JSContext *cx, HandleScript script);

    inline Breakpoint *firstBreakpoint() const;

    static inline Debugger *fromOnNewGlobalObjectWatchersLink(JSCList *link);

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
    static bool isDebugWrapper(RawObject o);
    static void findCompartmentEdges(JS::Zone *v, gc::ComponentFinder<JS::Zone> &finder);

    static inline JSTrapStatus onEnterFrame(JSContext *cx, MutableHandleValue vp);
    static inline bool onLeaveFrame(JSContext *cx, bool ok);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, MutableHandleValue vp);
    static inline JSTrapStatus onExceptionUnwind(JSContext *cx, MutableHandleValue vp);
    static inline void onNewScript(JSContext *cx, HandleScript script,
                                   GlobalObject *compileAndGoGlobal);
    static inline bool onNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global);
    static JSTrapStatus onTrap(JSContext *cx, MutableHandleValue vp);
    static JSTrapStatus onSingleStep(JSContext *cx, MutableHandleValue vp);

    

    inline bool observesEnterFrame() const;
    inline bool observesNewScript() const;
    inline bool observesNewGlobalObject() const;
    inline bool observesGlobal(GlobalObject *global) const;
    inline bool observesFrame(AbstractFramePtr frame) const;
    bool observesScript(JSScript *script) const;

    




    bool wrapEnvironment(JSContext *cx, Handle<Env*> env, MutableHandleValue vp);

    








    bool wrapDebuggeeValue(JSContext *cx, MutableHandleValue vp);

    


























    bool unwrapDebuggeeValue(JSContext *cx, MutableHandleValue vp);

    
    bool getScriptFrame(JSContext *cx, const ScriptFrameIter &iter, MutableHandleValue vp);

    






    static void resultToCompletion(JSContext *cx, bool ok, const Value &rv,
                                   JSTrapStatus *status, MutableHandleValue value);

    




    bool newCompletionValue(JSContext *cx, JSTrapStatus status, Value value,
                            MutableHandleValue result);

    











    bool receiveCompletionValue(mozilla::Maybe<AutoCompartment> &ac, bool ok, Value val,
                                MutableHandleValue vp);

    




    JSObject *wrapScript(JSContext *cx, HandleScript script);

  private:
    Debugger(const Debugger &) MOZ_DELETE;
    Debugger & operator=(const Debugger &) MOZ_DELETE;
};

class BreakpointSite {
    friend class Breakpoint;
    friend struct ::JSCompartment;
    friend class ::JSScript;
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

Breakpoint *
Debugger::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return NULL;
    return Breakpoint::fromDebuggerLinks(JS_NEXT_LINK(&breakpoints));
}

Debugger *
Debugger::fromOnNewGlobalObjectWatchersLink(JSCList *link) {
    char *p = reinterpret_cast<char *>(link);
    return reinterpret_cast<Debugger *>(p - offsetof(Debugger, onNewGlobalObjectWatchersLink));
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
Debugger::observesNewGlobalObject() const
{
    return enabled && getHook(OnNewGlobalObject);
}

bool
Debugger::observesGlobal(GlobalObject *global) const
{
    return debuggees.has(global);
}

bool
Debugger::observesFrame(AbstractFramePtr frame) const
{
    return observesGlobal(&frame.script()->global());
}

JSTrapStatus
Debugger::onEnterFrame(JSContext *cx, MutableHandleValue vp)
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
Debugger::onDebuggerStatement(JSContext *cx, MutableHandleValue vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp, OnDebuggerStatement);
}

JSTrapStatus
Debugger::onExceptionUnwind(JSContext *cx, MutableHandleValue vp)
{
    return cx->compartment->getDebuggees().empty()
           ? JSTRAP_CONTINUE
           : dispatchHook(cx, vp, OnExceptionUnwind);
}

void
Debugger::onNewScript(JSContext *cx, HandleScript script, GlobalObject *compileAndGoGlobal)
{
    JS_ASSERT_IF(script->compileAndGo, compileAndGoGlobal);
    JS_ASSERT_IF(!script->compileAndGo, !compileAndGoGlobal);
    if (!script->compartment()->getDebuggees().empty())
        slowPathOnNewScript(cx, script, compileAndGoGlobal);
}

bool
Debugger::onNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global)
{
    if (JS_CLIST_IS_EMPTY(&cx->runtime->onNewGlobalObjectWatchers))
        return true;
    return Debugger::slowPathOnNewGlobalObject(cx, global);
}

extern JSBool
EvaluateInEnv(JSContext *cx, Handle<Env*> env, HandleValue thisv, AbstractFramePtr frame,
              StableCharPtr chars, unsigned length, const char *filename, unsigned lineno,
              Value *rval);

}

#endif 
