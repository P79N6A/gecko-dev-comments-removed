





#ifndef vm_Debugger_h
#define vm_Debugger_h

#include "mozilla/LinkedList.h"
#include "mozilla/Range.h"

#include "jsclist.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsweakmap.h"
#include "jswrapper.h"

#include "gc/Barrier.h"
#include "js/HashTable.h"
#include "vm/GlobalObject.h"
#include "vm/SavedStacks.h"

namespace js {

class Breakpoint;
class DebuggerMemory;





















template <class Key, class Value, bool InvisibleKeysOk=false>
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
        : Base(cx), zoneCounts(cx->runtime()) { }

  public:
    

    typedef typename Base::Entry Entry;
    typedef typename Base::Ptr Ptr;
    typedef typename Base::AddPtr AddPtr;
    typedef typename Base::Range Range;
    typedef typename Base::Enum Enum;
    typedef typename Base::Lookup Lookup;

    

    using Base::lookupForAdd;
    using Base::all;
    using Base::trace;

    bool init(uint32_t len = 16) {
        return Base::init(len) && zoneCounts.init();
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        JS_ASSERT(v->compartment() == Base::compartment);
        JS_ASSERT(!k->compartment()->options_.mergeable());
        JS_ASSERT_IF(!InvisibleKeysOk, !k->compartment()->options_.invisibleToDebugger());
        JS_ASSERT(!Base::has(k));
        if (!incZoneCount(k->zone()))
            return false;
        bool ok = Base::relookupOrAdd(p, k, v);
        if (!ok)
            decZoneCount(k->zone());
        return ok;
    }

    void remove(const Lookup &l) {
        JS_ASSERT(Base::has(l));
        Base::remove(l);
        decZoneCount(l->zone());
    }

  public:
    void markKeys(JSTracer *tracer) {
        for (Enum e(*static_cast<Base *>(this)); !e.empty(); e.popFront()) {
            Key key = e.front().key();
            gc::Mark(tracer, &key, "Debugger WeakMap key");
            if (key != e.front().key())
                e.rekeyFront(key);
            key.unsafeSet(nullptr);
        }
    }

    bool hasKeyInZone(JS::Zone *zone) {
        CountMap::Ptr p = zoneCounts.lookup(zone);
        JS_ASSERT_IF(p, p->value() > 0);
        return p;
    }

  private:
    
    void sweep() {
        for (Enum e(*static_cast<Base *>(this)); !e.empty(); e.popFront()) {
            Key k(e.front().key());
            if (gc::IsAboutToBeFinalized(&k)) {
                e.removeFront();
                decZoneCount(k->zone());
            } else {
                
                JS_ASSERT(k == e.front().key());
            }
        }
        Base::assertEntriesNotAboutToBeFinalized();
    }

    bool incZoneCount(JS::Zone *zone) {
        CountMap::Ptr p = zoneCounts.lookupWithDefault(zone, 0);
        if (!p)
            return false;
        ++p->value();
        return true;
    }

    void decZoneCount(JS::Zone *zone) {
        CountMap::Ptr p = zoneCounts.lookup(zone);
        JS_ASSERT(p);
        JS_ASSERT(p->value() > 0);
        --p->value();
        if (p->value() == 0)
            zoneCounts.remove(zone);
    }
};







typedef JSObject Env;

class Debugger : private mozilla::LinkedListElement<Debugger>
{
    friend class Breakpoint;
    friend class DebuggerMemory;
    friend class mozilla::LinkedListElement<Debugger>;
    friend bool (::JS_DefineDebuggerObject)(JSContext *cx, JS::HandleObject obj);
    friend bool SavedStacksMetadataCallback(JSContext *cx, JSObject **pmetadata);

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
        JSSLOT_DEBUG_SOURCE_PROTO,
        JSSLOT_DEBUG_MEMORY_PROTO,
        JSSLOT_DEBUG_PROTO_STOP,
        JSSLOT_DEBUG_HOOK_START = JSSLOT_DEBUG_PROTO_STOP,
        JSSLOT_DEBUG_HOOK_STOP = JSSLOT_DEBUG_HOOK_START + HookCount,
        JSSLOT_DEBUG_MEMORY_INSTANCE = JSSLOT_DEBUG_HOOK_STOP,
        JSSLOT_DEBUG_COUNT
    };
  private:
    HeapPtrObject object;               
    GlobalObjectSet debuggees;          
    js::HeapPtrObject uncaughtExceptionHook; 
    bool enabled;
    JSCList breakpoints;                

    struct AllocationSite : public mozilla::LinkedListElement<AllocationSite>
    {
        explicit AllocationSite(HandleObject frame) : frame(frame) {
            JS_ASSERT_IF(frame, UncheckedUnwrap(frame)->is<SavedFrame>());
        };
        RelocatablePtrObject frame;
    };
    typedef mozilla::LinkedList<AllocationSite> AllocationSiteList;

    bool trackingAllocationSites;
    AllocationSiteList allocationsLog;
    size_t allocationsLogLength;
    size_t maxAllocationsLogLength;
    static const size_t DEFAULT_MAX_ALLOCATIONS_LOG_LENGTH = 5000;
    bool appendAllocationSite(JSContext *cx, HandleSavedFrame frame);
    void emptyAllocationsLog();

    





    JSCList onNewGlobalObjectWatchersLink;

    












    typedef HashMap<AbstractFramePtr,
                    RelocatablePtrObject,
                    DefaultHasher<AbstractFramePtr>,
                    RuntimeAllocPolicy> FrameMap;
    FrameMap frames;

    
    typedef DebuggerWeakMap<PreBarrieredScript, RelocatablePtrObject> ScriptWeakMap;
    ScriptWeakMap scripts;

    
    typedef DebuggerWeakMap<PreBarrieredObject, RelocatablePtrObject, true> SourceWeakMap;
    SourceWeakMap sources;

    
    typedef DebuggerWeakMap<PreBarrieredObject, RelocatablePtrObject> ObjectWeakMap;
    ObjectWeakMap objects;

    
    ObjectWeakMap environments;

    class FrameRange;
    class ScriptQuery;

    bool addDebuggeeGlobal(JSContext *cx, Handle<GlobalObject*> obj);
    bool addDebuggeeGlobal(JSContext *cx, Handle<GlobalObject*> obj,
                           AutoDebugModeInvalidation &invalidate);
    void cleanupDebuggeeGlobalBeforeRemoval(FreeOp *fop, GlobalObject *global,
                                            GlobalObjectSet::Enum *debugEnum);
    bool removeDebuggeeGlobal(JSContext *cx, Handle<GlobalObject *> global,
                              GlobalObjectSet::Enum *debugEnum);
    bool removeDebuggeeGlobal(JSContext *cx, Handle<GlobalObject *> global,
                              AutoDebugModeInvalidation &invalidate,
                              GlobalObjectSet::Enum *debugEnum);
    void removeDebuggeeGlobalUnderGC(FreeOp *fop, GlobalObject *global,
                                     GlobalObjectSet::Enum *debugEnum);

    













    JSTrapStatus handleUncaughtException(mozilla::Maybe<AutoCompartment> &ac, bool callHook);
    JSTrapStatus handleUncaughtException(mozilla::Maybe<AutoCompartment> &ac, MutableHandleValue vp, bool callHook);

    JSTrapStatus handleUncaughtExceptionHelper(mozilla::Maybe<AutoCompartment> &ac,
                                               MutableHandleValue *vp, bool callHook);

    
























    JSTrapStatus parseResumptionValue(mozilla::Maybe<AutoCompartment> &ac, bool ok, const Value &rv,
                                      MutableHandleValue vp, bool callHook = true);

    GlobalObject *unwrapDebuggeeArgument(JSContext *cx, const Value &v);

    static void traceObject(JSTracer *trc, JSObject *obj);
    void trace(JSTracer *trc);
    static void finalize(FreeOp *fop, JSObject *obj);
    void markKeysInCompartment(JSTracer *tracer);

    static const Class jsclass;

    static Debugger *fromThisValue(JSContext *cx, const CallArgs &ca, const char *fnname);
    static bool getEnabled(JSContext *cx, unsigned argc, Value *vp);
    static bool setEnabled(JSContext *cx, unsigned argc, Value *vp);
    static bool getHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which);
    static bool setHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which);
    static bool getOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp);
    static bool setOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp);
    static bool getOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp);
    static bool setOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp);
    static bool getOnNewScript(JSContext *cx, unsigned argc, Value *vp);
    static bool setOnNewScript(JSContext *cx, unsigned argc, Value *vp);
    static bool getOnEnterFrame(JSContext *cx, unsigned argc, Value *vp);
    static bool setOnEnterFrame(JSContext *cx, unsigned argc, Value *vp);
    static bool getOnNewGlobalObject(JSContext *cx, unsigned argc, Value *vp);
    static bool setOnNewGlobalObject(JSContext *cx, unsigned argc, Value *vp);
    static bool getUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp);
    static bool setUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp);
    static bool getMemory(JSContext *cx, unsigned argc, Value *vp);
    static bool addDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static bool addAllGlobalsAsDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static bool removeDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static bool removeAllDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static bool hasDebuggee(JSContext *cx, unsigned argc, Value *vp);
    static bool getDebuggees(JSContext *cx, unsigned argc, Value *vp);
    static bool getNewestFrame(JSContext *cx, unsigned argc, Value *vp);
    static bool clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp);
    static bool findScripts(JSContext *cx, unsigned argc, Value *vp);
    static bool findAllGlobals(JSContext *cx, unsigned argc, Value *vp);
    static bool makeGlobalObjectReference(JSContext *cx, unsigned argc, Value *vp);
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];

    JSObject *getHook(Hook hook) const;
    bool hasAnyLiveHooks() const;

    static JSTrapStatus slowPathOnEnterFrame(JSContext *cx, AbstractFramePtr frame,
                                             MutableHandleValue vp);
    static bool slowPathOnLeaveFrame(JSContext *cx, AbstractFramePtr frame, bool ok);
    static void slowPathOnNewScript(JSContext *cx, HandleScript script,
                                    GlobalObject *compileAndGoGlobal);
    static void slowPathOnNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global);
    static bool slowPathOnLogAllocationSite(JSContext *cx, HandleSavedFrame frame,
                                            GlobalObject::DebuggerVector &dbgs);
    static JSTrapStatus dispatchHook(JSContext *cx, MutableHandleValue vp, Hook which);

    JSTrapStatus fireDebuggerStatement(JSContext *cx, MutableHandleValue vp);
    JSTrapStatus fireExceptionUnwind(JSContext *cx, MutableHandleValue vp);
    JSTrapStatus fireEnterFrame(JSContext *cx, AbstractFramePtr frame, MutableHandleValue vp);
    JSTrapStatus fireNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global, MutableHandleValue vp);

    



    JSObject *newDebuggerScript(JSContext *cx, HandleScript script);

    



    JSObject *newDebuggerSource(JSContext *cx, js::HandleScriptSource source);

    



    void fireNewScript(JSContext *cx, HandleScript script);

    



    bool getScriptFrameWithIter(JSContext *cx, AbstractFramePtr frame,
                                const ScriptFrameIter *maybeIter, MutableHandleValue vp);

    inline Breakpoint *firstBreakpoint() const;

    static inline Debugger *fromOnNewGlobalObjectWatchersLink(JSCList *link);

    static bool replaceFrameGuts(JSContext *cx, AbstractFramePtr from, AbstractFramePtr to,
                                 ScriptFrameIter &iter);

  public:
    Debugger(JSContext *cx, JSObject *dbg);
    ~Debugger();

    bool init(JSContext *cx);
    inline const js::HeapPtrObject &toJSObject() const;
    inline js::HeapPtrObject &toJSObjectRef();
    static inline Debugger *fromJSObject(JSObject *obj);
    static Debugger *fromChildJSObject(JSObject *obj);

    bool hasMemory() const;
    DebuggerMemory &memory() const;

    

    














    static void markCrossCompartmentDebuggerObjectReferents(JSTracer *tracer);
    static bool markAllIteratively(GCMarker *trc);
    static void markAll(JSTracer *trc);
    static void sweepAll(FreeOp *fop);
    static void detachAllDebuggersFromGlobal(FreeOp *fop, GlobalObject *global);
    static void findCompartmentEdges(JS::Zone *v, gc::ComponentFinder<JS::Zone> &finder);

    static inline JSTrapStatus onEnterFrame(JSContext *cx, AbstractFramePtr frame,
                                            MutableHandleValue vp);
    static inline bool onLeaveFrame(JSContext *cx, AbstractFramePtr frame, bool ok);
    static inline JSTrapStatus onDebuggerStatement(JSContext *cx, MutableHandleValue vp);
    static inline JSTrapStatus onExceptionUnwind(JSContext *cx, MutableHandleValue vp);
    static inline void onNewScript(JSContext *cx, HandleScript script,
                                   GlobalObject *compileAndGoGlobal);
    static inline void onNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global);
    static inline bool onLogAllocationSite(JSContext *cx, HandleSavedFrame frame);
    static JSTrapStatus onTrap(JSContext *cx, MutableHandleValue vp);
    static JSTrapStatus onSingleStep(JSContext *cx, MutableHandleValue vp);
    static bool handleBaselineOsr(JSContext *cx, InterpreterFrame *from, jit::BaselineFrame *to);
    static bool handleIonBailout(JSContext *cx, jit::RematerializedFrame *from, jit::BaselineFrame *to);
    static void propagateForcedReturn(JSContext *cx, AbstractFramePtr frame, HandleValue rval);

    

    inline bool observesEnterFrame() const;
    inline bool observesNewScript() const;
    inline bool observesNewGlobalObject() const;
    inline bool observesGlobal(GlobalObject *global) const;
    bool observesFrame(AbstractFramePtr frame) const;
    bool observesFrame(const ScriptFrameIter &iter) const;
    bool observesScript(JSScript *script) const;

    




    bool wrapEnvironment(JSContext *cx, Handle<Env*> env, MutableHandleValue vp);

    



















    bool wrapDebuggeeValue(JSContext *cx, MutableHandleValue vp);

    


























    bool unwrapDebuggeeValue(JSContext *cx, MutableHandleValue vp);
    bool unwrapPropDescInto(JSContext *cx, HandleObject obj, Handle<PropDesc> wrapped,
                            MutableHandle<PropDesc> unwrapped);

    





    bool getScriptFrame(JSContext *cx, AbstractFramePtr frame, MutableHandleValue vp) {
        return getScriptFrameWithIter(cx, frame, nullptr, vp);
    }

    







    bool getScriptFrame(JSContext *cx, const ScriptFrameIter &iter, MutableHandleValue vp) {
        return getScriptFrameWithIter(cx, iter.abstractFramePtr(), &iter, vp);
    }

    






    static void resultToCompletion(JSContext *cx, bool ok, const Value &rv,
                                   JSTrapStatus *status, MutableHandleValue value);

    




    bool newCompletionValue(JSContext *cx, JSTrapStatus status, Value value,
                            MutableHandleValue result);

    











    bool receiveCompletionValue(mozilla::Maybe<AutoCompartment> &ac, bool ok,
                                HandleValue val,
                                MutableHandleValue vp);

    




    JSObject *wrapScript(JSContext *cx, HandleScript script);

    




    JSObject *wrapSource(JSContext *cx, js::HandleScriptSource source);

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

    void recompile(FreeOp *fop);

  public:
    BreakpointSite(JSScript *script, jsbytecode *pc);
    Breakpoint *firstBreakpoint() const;
    bool hasBreakpoint(Breakpoint *bp);

    void inc(FreeOp *fop);
    void dec(FreeOp *fop);
    void destroyIfEmpty(FreeOp *fop);
};



















class Breakpoint {
    friend struct ::JSCompartment;
    friend class Debugger;

  public:
    Debugger * const debugger;
    BreakpointSite * const site;
  private:
    
    js::PreBarrieredObject handler;
    JSCList debuggerLinks;
    JSCList siteLinks;

  public:
    static Breakpoint *fromDebuggerLinks(JSCList *links);
    static Breakpoint *fromSiteLinks(JSCList *links);
    Breakpoint(Debugger *debugger, BreakpointSite *site, JSObject *handler);
    void destroy(FreeOp *fop);
    Breakpoint *nextInDebugger();
    Breakpoint *nextInSite();
    const PreBarrieredObject &getHandler() const { return handler; }
    PreBarrieredObject &getHandlerRef() { return handler; }
};

Breakpoint *
Debugger::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return nullptr;
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

JSTrapStatus
Debugger::onEnterFrame(JSContext *cx, AbstractFramePtr frame, MutableHandleValue vp)
{
    if (!cx->compartment()->debugMode())
        return JSTRAP_CONTINUE;
    return slowPathOnEnterFrame(cx, frame, vp);
}

JSTrapStatus
Debugger::onDebuggerStatement(JSContext *cx, MutableHandleValue vp)
{
    return cx->compartment()->debugMode()
           ? dispatchHook(cx, vp, OnDebuggerStatement)
           : JSTRAP_CONTINUE;
}

JSTrapStatus
Debugger::onExceptionUnwind(JSContext *cx, MutableHandleValue vp)
{
    return cx->compartment()->debugMode()
           ? dispatchHook(cx, vp, OnExceptionUnwind)
           : JSTRAP_CONTINUE;
}

void
Debugger::onNewScript(JSContext *cx, HandleScript script, GlobalObject *compileAndGoGlobal)
{
    JS_ASSERT_IF(script->compileAndGo(), compileAndGoGlobal);
    JS_ASSERT_IF(script->compileAndGo(), compileAndGoGlobal == &script->uninlinedGlobal());
    
    
    JS_ASSERT_IF(!script->compartment()->options().invisibleToDebugger() &&
                 !script->selfHosted(),
                 script->compartment()->firedOnNewGlobalObject);
    JS_ASSERT_IF(!script->compileAndGo(), !compileAndGoGlobal);
    if (script->compartment()->debugMode())
        slowPathOnNewScript(cx, script, compileAndGoGlobal);
}

void
Debugger::onNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global)
{
    JS_ASSERT(!global->compartment()->firedOnNewGlobalObject);
#ifdef DEBUG
    global->compartment()->firedOnNewGlobalObject = true;
#endif
    if (!JS_CLIST_IS_EMPTY(&cx->runtime()->onNewGlobalObjectWatchers))
        Debugger::slowPathOnNewGlobalObject(cx, global);
}

bool
Debugger::onLogAllocationSite(JSContext *cx, HandleSavedFrame frame)
{
    GlobalObject::DebuggerVector *dbgs = cx->global()->getDebuggers();
    if (!dbgs || dbgs->empty())
        return true;
    return Debugger::slowPathOnLogAllocationSite(cx, frame, *dbgs);
}

extern bool
EvaluateInEnv(JSContext *cx, Handle<Env*> env, HandleValue thisv, AbstractFramePtr frame,
              mozilla::Range<const char16_t> chars, const char *filename, unsigned lineno,
              MutableHandleValue rval);

bool ReportObjectRequired(JSContext *cx);

} 

#endif 
