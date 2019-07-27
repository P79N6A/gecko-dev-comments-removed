





#ifndef vm_Debugger_h
#define vm_Debugger_h

#include "mozilla/GuardObjects.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Range.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"

#include "jsclist.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsweakmap.h"
#include "jswrapper.h"

#include "gc/Barrier.h"
#include "js/Debug.h"
#include "js/HashTable.h"
#include "vm/GlobalObject.h"
#include "vm/SavedStacks.h"

enum JSTrapStatus {
    JSTRAP_ERROR,
    JSTRAP_CONTINUE,
    JSTRAP_RETURN,
    JSTRAP_THROW,
    JSTRAP_LIMIT
};

namespace js {

class Breakpoint;
class DebuggerMemory;

typedef HashSet<ReadBarrieredGlobalObject,
                DefaultHasher<ReadBarrieredGlobalObject>,
                SystemAllocPolicy> WeakGlobalObjectSet;




















template <class UnbarrieredKey, bool InvisibleKeysOk=false>
class DebuggerWeakMap : private WeakMap<PreBarriered<UnbarrieredKey>, RelocatablePtrObject>
{
  private:
    typedef PreBarriered<UnbarrieredKey> Key;
    typedef RelocatablePtrObject Value;

    typedef HashMap<JS::Zone*,
                    uintptr_t,
                    DefaultHasher<JS::Zone*>,
                    RuntimeAllocPolicy> CountMap;

    CountMap zoneCounts;

  public:
    typedef WeakMap<Key, Value, DefaultHasher<Key> > Base;
    explicit DebuggerWeakMap(JSContext* cx)
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
    bool relookupOrAdd(AddPtr& p, const KeyInput& k, const ValueInput& v) {
        MOZ_ASSERT(v->compartment() == Base::compartment);
        MOZ_ASSERT(!k->compartment()->options_.mergeable());
        MOZ_ASSERT_IF(!InvisibleKeysOk, !k->compartment()->options_.invisibleToDebugger());
        MOZ_ASSERT(!Base::has(k));
        if (!incZoneCount(k->zone()))
            return false;
        bool ok = Base::relookupOrAdd(p, k, v);
        if (!ok)
            decZoneCount(k->zone());
        return ok;
    }

    void remove(const Lookup& l) {
        MOZ_ASSERT(Base::has(l));
        Base::remove(l);
        decZoneCount(l->zone());
    }

  public:
    template <void (traceValueEdges)(JSTracer*, JSObject*)>
    void markCrossCompartmentEdges(JSTracer* tracer) {
        for (Enum e(*static_cast<Base*>(this)); !e.empty(); e.popFront()) {
            traceValueEdges(tracer, e.front().value());
            Key key = e.front().key();
            TraceEdge(tracer, &key, "Debugger WeakMap key");
            if (key != e.front().key())
                e.rekeyFront(key);
            key.unsafeSet(nullptr);
        }
    }

    bool hasKeyInZone(JS::Zone* zone) {
        CountMap::Ptr p = zoneCounts.lookup(zone);
        MOZ_ASSERT_IF(p.found(), p->value() > 0);
        return p.found();
    }

  private:
    
    void sweep() {
        for (Enum e(*static_cast<Base*>(this)); !e.empty(); e.popFront()) {
            Key k(e.front().key());
            if (gc::IsAboutToBeFinalized(&k)) {
                e.removeFront();
                decZoneCount(k->zone());
            } else {
                
                MOZ_ASSERT(k == e.front().key());
            }
        }
        Base::assertEntriesNotAboutToBeFinalized();
    }

    bool incZoneCount(JS::Zone* zone) {
        CountMap::Ptr p = zoneCounts.lookupWithDefault(zone, 0);
        if (!p)
            return false;
        ++p->value();
        return true;
    }

    void decZoneCount(JS::Zone* zone) {
        CountMap::Ptr p = zoneCounts.lookup(zone);
        MOZ_ASSERT(p);
        MOZ_ASSERT(p->value() > 0);
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
    friend class SavedStacks;
    friend class mozilla::LinkedListElement<Debugger>;
    friend bool (::JS_DefineDebuggerObject)(JSContext* cx, JS::HandleObject obj);
    friend bool (::JS::dbg::IsDebugger)(JS::Value val);
    friend JSObject* SavedStacksMetadataCallback(JSContext* cx);
    friend void JS::dbg::onNewPromise(JSContext* cx, HandleObject promise);
    friend void JS::dbg::onPromiseSettled(JSContext* cx, HandleObject promise);
    friend bool JS::dbg::FireOnGarbageCollectionHook(JSContext* cx,
                                                     JS::dbg::GarbageCollectionEvent::Ptr&& data);

  public:
    enum Hook {
        OnDebuggerStatement,
        OnExceptionUnwind,
        OnNewScript,
        OnEnterFrame,
        OnNewGlobalObject,
        OnNewPromise,
        OnPromiseSettled,
        OnGarbageCollection,
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

    class ExecutionObservableSet
    {
      public:
        typedef HashSet<Zone*>::Range ZoneRange;

        virtual Zone* singleZone() const { return nullptr; }
        virtual JSScript* singleScriptForZoneInvalidation() const { return nullptr; }
        virtual const HashSet<Zone*>* zones() const { return nullptr; }

        virtual bool shouldRecompileOrInvalidate(JSScript* script) const = 0;
        virtual bool shouldMarkAsDebuggee(ScriptFrameIter& iter) const = 0;
    };

    
    
    enum IsObserving {
        NotObserving = 0,
        Observing = 1
    };

    
    
    bool isDebuggee(const JSCompartment* compartment) const;

    
    
    bool observedGC(uint64_t majorGCNumber) const {
        return observedGCs.has(majorGCNumber);
    }

    
    
    bool debuggeeIsBeingCollected(uint64_t majorGCNumber) {
        return observedGCs.put(majorGCNumber);
    }

  private:
    HeapPtrNativeObject object;         
    WeakGlobalObjectSet debuggees;      
    js::HeapPtrObject uncaughtExceptionHook; 
    bool enabled;
    JSCList breakpoints;                

    
    
    js::HashSet<uint64_t> observedGCs;

    struct AllocationSite : public mozilla::LinkedListElement<AllocationSite>
    {
        AllocationSite(HandleObject frame, int64_t when, const char* className)
            : frame(frame),
              when(when),
              className(className)
        {
            MOZ_ASSERT_IF(frame, UncheckedUnwrap(frame)->is<SavedFrame>());
        };

        RelocatablePtrObject frame;
        int64_t when;
        const char* className;
    };
    typedef mozilla::LinkedList<AllocationSite> AllocationSiteList;

    bool allowUnobservedAsmJS;
    bool trackingAllocationSites;
    double allocationSamplingProbability;
    AllocationSiteList allocationsLog;
    size_t allocationsLogLength;
    size_t maxAllocationsLogLength;
    bool allocationsLogOverflowed;

    static const size_t DEFAULT_MAX_ALLOCATIONS_LOG_LENGTH = 5000;

    bool appendAllocationSite(JSContext* cx, HandleObject obj, HandleSavedFrame frame,
                              int64_t when);
    void emptyAllocationsLog();

    




    static bool cannotTrackAllocations(const GlobalObject& global);

    



    static bool isObservedByDebuggerTrackingAllocations(const GlobalObject& global);

    




    static bool addAllocationsTracking(JSContext* cx, GlobalObject& debuggee);

    




    static void removeAllocationsTracking(GlobalObject& global);

    


    bool addAllocationsTrackingForAllDebuggees(JSContext* cx);
    void removeAllocationsTrackingForAllDebuggees();

    





    JSCList onNewGlobalObjectWatchersLink;

    












    typedef HashMap<AbstractFramePtr,
                    RelocatablePtrNativeObject,
                    DefaultHasher<AbstractFramePtr>,
                    RuntimeAllocPolicy> FrameMap;
    FrameMap frames;

    
    typedef DebuggerWeakMap<JSScript*> ScriptWeakMap;
    ScriptWeakMap scripts;

    
    typedef DebuggerWeakMap<JSObject*, true> SourceWeakMap;
    SourceWeakMap sources;

    
    typedef DebuggerWeakMap<JSObject*> ObjectWeakMap;
    ObjectWeakMap objects;

    
    ObjectWeakMap environments;

    



#ifdef NIGHTLY_BUILD
    uint32_t traceLoggerLastDrainedId;
    uint32_t traceLoggerLastDrainedIteration;
#endif
    uint32_t traceLoggerScriptedCallsLastDrainedId;
    uint32_t traceLoggerScriptedCallsLastDrainedIteration;

    class FrameRange;
    class ScriptQuery;
    class ObjectQuery;

    bool addDebuggeeGlobal(JSContext* cx, Handle<GlobalObject*> obj);
    void removeDebuggeeGlobal(FreeOp* fop, GlobalObject* global,
                              WeakGlobalObjectSet::Enum* debugEnum);

    













    JSTrapStatus handleUncaughtException(mozilla::Maybe<AutoCompartment>& ac, bool callHook);
    JSTrapStatus handleUncaughtException(mozilla::Maybe<AutoCompartment>& ac, MutableHandleValue vp, bool callHook);

    JSTrapStatus handleUncaughtExceptionHelper(mozilla::Maybe<AutoCompartment>& ac,
                                               MutableHandleValue* vp, bool callHook);

    
























    JSTrapStatus parseResumptionValue(mozilla::Maybe<AutoCompartment>& ac, bool ok, const Value& rv,
                                      MutableHandleValue vp, bool callHook = true);

    GlobalObject* unwrapDebuggeeArgument(JSContext* cx, const Value& v);

    static void traceObject(JSTracer* trc, JSObject* obj);
    void trace(JSTracer* trc);
    static void finalize(FreeOp* fop, JSObject* obj);
    void markCrossCompartmentEdges(JSTracer* tracer);

    static const Class jsclass;

    static bool getHookImpl(JSContext* cx, CallArgs& args, Debugger& dbg, Hook which);
    static bool setHookImpl(JSContext* cx, CallArgs& args, Debugger& dbg, Hook which);

    static Debugger* fromThisValue(JSContext* cx, const CallArgs& ca, const char* fnname);
    static bool getEnabled(JSContext* cx, unsigned argc, Value* vp);
    static bool setEnabled(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnDebuggerStatement(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnDebuggerStatement(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnExceptionUnwind(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnExceptionUnwind(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnNewScript(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnNewScript(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnEnterFrame(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnEnterFrame(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnNewGlobalObject(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnNewGlobalObject(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnNewPromise(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnNewPromise(JSContext* cx, unsigned argc, Value* vp);
    static bool getOnPromiseSettled(JSContext* cx, unsigned argc, Value* vp);
    static bool setOnPromiseSettled(JSContext* cx, unsigned argc, Value* vp);
    static bool getUncaughtExceptionHook(JSContext* cx, unsigned argc, Value* vp);
    static bool setUncaughtExceptionHook(JSContext* cx, unsigned argc, Value* vp);
    static bool getAllowUnobservedAsmJS(JSContext* cx, unsigned argc, Value* vp);
    static bool setAllowUnobservedAsmJS(JSContext* cx, unsigned argc, Value* vp);
    static bool getMemory(JSContext* cx, unsigned argc, Value* vp);
    static bool addDebuggee(JSContext* cx, unsigned argc, Value* vp);
    static bool addAllGlobalsAsDebuggees(JSContext* cx, unsigned argc, Value* vp);
    static bool removeDebuggee(JSContext* cx, unsigned argc, Value* vp);
    static bool removeAllDebuggees(JSContext* cx, unsigned argc, Value* vp);
    static bool hasDebuggee(JSContext* cx, unsigned argc, Value* vp);
    static bool getDebuggees(JSContext* cx, unsigned argc, Value* vp);
    static bool getNewestFrame(JSContext* cx, unsigned argc, Value* vp);
    static bool clearAllBreakpoints(JSContext* cx, unsigned argc, Value* vp);
    static bool findScripts(JSContext* cx, unsigned argc, Value* vp);
    static bool findObjects(JSContext* cx, unsigned argc, Value* vp);
    static bool findAllGlobals(JSContext* cx, unsigned argc, Value* vp);
    static bool makeGlobalObjectReference(JSContext* cx, unsigned argc, Value* vp);
    static bool setupTraceLoggerScriptCalls(JSContext* cx, unsigned argc, Value* vp);
    static bool drainTraceLoggerScriptCalls(JSContext* cx, unsigned argc, Value* vp);
    static bool startTraceLogger(JSContext* cx, unsigned argc, Value* vp);
    static bool endTraceLogger(JSContext* cx, unsigned argc, Value* vp);
#ifdef NIGHTLY_BUILD
    static bool setupTraceLogger(JSContext* cx, unsigned argc, Value* vp);
    static bool drainTraceLogger(JSContext* cx, unsigned argc, Value* vp);
#endif
    static bool construct(JSContext* cx, unsigned argc, Value* vp);
    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];

    static void removeFromFrameMapsAndClearBreakpointsIn(JSContext* cx, AbstractFramePtr frame);
    static bool updateExecutionObservabilityOfFrames(JSContext* cx, const ExecutionObservableSet& obs,
                                                     IsObserving observing);
    static bool updateExecutionObservabilityOfScripts(JSContext* cx, const ExecutionObservableSet& obs,
                                                      IsObserving observing);
    static bool updateExecutionObservability(JSContext* cx, ExecutionObservableSet& obs,
                                             IsObserving observing);

  public:
    static bool ensureExecutionObservabilityOfOsrFrame(JSContext* cx, InterpreterFrame* frame);

    
    static bool ensureExecutionObservabilityOfScript(JSContext* cx, JSScript* script);

    
    
    IsObserving observesAllExecution() const;

    
    
    IsObserving observesAsmJS() const;

  private:
    static bool ensureExecutionObservabilityOfFrame(JSContext* cx, AbstractFramePtr frame);
    static bool ensureExecutionObservabilityOfCompartment(JSContext* cx, JSCompartment* comp);

    static bool hookObservesAllExecution(Hook which);

    bool updateObservesAllExecutionOnDebuggees(JSContext* cx, IsObserving observing);
    void updateObservesAsmJSOnDebuggees(IsObserving observing);

    JSObject* getHook(Hook hook) const;
    bool hasAnyLiveHooks() const;

    static JSTrapStatus slowPathOnEnterFrame(JSContext* cx, AbstractFramePtr frame);
    static bool slowPathOnLeaveFrame(JSContext* cx, AbstractFramePtr frame, bool ok);
    static JSTrapStatus slowPathOnDebuggerStatement(JSContext* cx, AbstractFramePtr frame);
    static JSTrapStatus slowPathOnExceptionUnwind(JSContext* cx, AbstractFramePtr frame);
    static void slowPathOnNewScript(JSContext* cx, HandleScript script);
    static void slowPathOnNewGlobalObject(JSContext* cx, Handle<GlobalObject*> global);
    static bool slowPathOnLogAllocationSite(JSContext* cx, HandleObject obj, HandleSavedFrame frame,
                                            int64_t when, GlobalObject::DebuggerVector& dbgs);
    static void slowPathPromiseHook(JSContext* cx, Hook hook, HandleObject promise);
    static JSTrapStatus dispatchHook(JSContext* cx, MutableHandleValue vp, Hook which,
                                     HandleObject payload);

    JSTrapStatus fireDebuggerStatement(JSContext* cx, MutableHandleValue vp);
    JSTrapStatus fireExceptionUnwind(JSContext* cx, MutableHandleValue vp);
    JSTrapStatus fireEnterFrame(JSContext* cx, AbstractFramePtr frame, MutableHandleValue vp);
    JSTrapStatus fireNewGlobalObject(JSContext* cx, Handle<GlobalObject*> global, MutableHandleValue vp);
    JSTrapStatus firePromiseHook(JSContext* cx, Hook hook, HandleObject promise, MutableHandleValue vp);

    



    JSObject* newDebuggerScript(JSContext* cx, HandleScript script);

    



    JSObject* newDebuggerSource(JSContext* cx, js::HandleScriptSource source);

    



    void fireNewScript(JSContext* cx, HandleScript script);

    



    void fireOnGarbageCollectionHook(JSContext* cx,
                                     const JS::dbg::GarbageCollectionEvent::Ptr& gcData);

    



    bool getScriptFrameWithIter(JSContext* cx, AbstractFramePtr frame,
                                const ScriptFrameIter* maybeIter, MutableHandleValue vp);

    inline Breakpoint* firstBreakpoint() const;

    static inline Debugger* fromOnNewGlobalObjectWatchersLink(JSCList* link);

    static bool replaceFrameGuts(JSContext* cx, AbstractFramePtr from, AbstractFramePtr to,
                                 ScriptFrameIter& iter);

  public:
    Debugger(JSContext* cx, NativeObject* dbg);
    ~Debugger();

    bool init(JSContext* cx);
    inline const js::HeapPtrNativeObject& toJSObject() const;
    inline js::HeapPtrNativeObject& toJSObjectRef();
    static inline Debugger* fromJSObject(JSObject* obj);
    static Debugger* fromChildJSObject(JSObject* obj);

    bool hasMemory() const;
    DebuggerMemory& memory() const;

    WeakGlobalObjectSet::Range allDebuggees() const { return debuggees.all(); }

    

    














    static void markIncomingCrossCompartmentEdges(JSTracer* tracer);
    static bool markAllIteratively(GCMarker* trc);
    static void markAll(JSTracer* trc);
    static void sweepAll(FreeOp* fop);
    static void detachAllDebuggersFromGlobal(FreeOp* fop, GlobalObject* global);
    static void findCompartmentEdges(JS::Zone* v, gc::ComponentFinder<JS::Zone>& finder);

    




















    




    static inline JSTrapStatus onEnterFrame(JSContext* cx, AbstractFramePtr frame);

    







    static inline JSTrapStatus onDebuggerStatement(JSContext* cx, AbstractFramePtr frame);

    



    static inline JSTrapStatus onExceptionUnwind(JSContext* cx, AbstractFramePtr frame);

    













    static inline bool onLeaveFrame(JSContext* cx, AbstractFramePtr frame, bool ok);

    static inline void onNewScript(JSContext* cx, HandleScript script);
    static inline void onNewGlobalObject(JSContext* cx, Handle<GlobalObject*> global);
    static inline bool onLogAllocationSite(JSContext* cx, JSObject* obj, HandleSavedFrame frame,
                                           int64_t when);
    static JSTrapStatus onTrap(JSContext* cx, MutableHandleValue vp);
    static JSTrapStatus onSingleStep(JSContext* cx, MutableHandleValue vp);
    static bool handleBaselineOsr(JSContext* cx, InterpreterFrame* from, jit::BaselineFrame* to);
    static bool handleIonBailout(JSContext* cx, jit::RematerializedFrame* from, jit::BaselineFrame* to);
    static void handleUnrecoverableIonBailoutError(JSContext* cx, jit::RematerializedFrame* frame);
    static void propagateForcedReturn(JSContext* cx, AbstractFramePtr frame, HandleValue rval);
    static bool hasLiveHook(GlobalObject* global, Hook which);
    static bool inFrameMaps(AbstractFramePtr frame);

    

    inline bool observesEnterFrame() const;
    inline bool observesNewScript() const;
    inline bool observesNewGlobalObject() const;
    inline bool observesGlobal(GlobalObject* global) const;
    bool observesFrame(AbstractFramePtr frame) const;
    bool observesFrame(const ScriptFrameIter& iter) const;
    bool observesScript(JSScript* script) const;

    




    bool wrapEnvironment(JSContext* cx, Handle<Env*> env, MutableHandleValue vp);

    



















    bool wrapDebuggeeValue(JSContext* cx, MutableHandleValue vp);

    


























    bool unwrapDebuggeeValue(JSContext* cx, MutableHandleValue vp);
    bool unwrapDebuggeeObject(JSContext* cx, MutableHandleObject obj);
    bool unwrapPropertyDescriptor(JSContext* cx, HandleObject obj,
                                  MutableHandle<PropertyDescriptor> desc);

    





    bool getScriptFrame(JSContext* cx, AbstractFramePtr frame, MutableHandleValue vp) {
        return getScriptFrameWithIter(cx, frame, nullptr, vp);
    }

    







    bool getScriptFrame(JSContext* cx, const ScriptFrameIter& iter, MutableHandleValue vp) {
        return getScriptFrameWithIter(cx, iter.abstractFramePtr(), &iter, vp);
    }

    






    static void resultToCompletion(JSContext* cx, bool ok, const Value& rv,
                                   JSTrapStatus* status, MutableHandleValue value);

    




    bool newCompletionValue(JSContext* cx, JSTrapStatus status, Value value,
                            MutableHandleValue result);

    











    bool receiveCompletionValue(mozilla::Maybe<AutoCompartment>& ac, bool ok,
                                HandleValue val,
                                MutableHandleValue vp);

    




    JSObject* wrapScript(JSContext* cx, HandleScript script);

    




    JSObject* wrapSource(JSContext* cx, js::HandleScriptSource source);

  private:
    Debugger(const Debugger&) = delete;
    Debugger & operator=(const Debugger&) = delete;
};

class BreakpointSite {
    friend class Breakpoint;
    friend struct ::JSCompartment;
    friend class ::JSScript;
    friend class Debugger;

  public:
    JSScript* script;
    jsbytecode * const pc;

  private:
    JSCList breakpoints;  
    size_t enabledCount;  

    void recompile(FreeOp* fop);

  public:
    BreakpointSite(JSScript* script, jsbytecode* pc);
    Breakpoint* firstBreakpoint() const;
    bool hasBreakpoint(Breakpoint* bp);

    void inc(FreeOp* fop);
    void dec(FreeOp* fop);
    void destroyIfEmpty(FreeOp* fop);
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
    static Breakpoint* fromDebuggerLinks(JSCList* links);
    static Breakpoint* fromSiteLinks(JSCList* links);
    Breakpoint(Debugger* debugger, BreakpointSite* site, JSObject* handler);
    void destroy(FreeOp* fop);
    Breakpoint* nextInDebugger();
    Breakpoint* nextInSite();
    const PreBarrieredObject& getHandler() const { return handler; }
    PreBarrieredObject& getHandlerRef() { return handler; }
};

Breakpoint*
Debugger::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return nullptr;
    return Breakpoint::fromDebuggerLinks(JS_NEXT_LINK(&breakpoints));
}

 Debugger*
Debugger::fromOnNewGlobalObjectWatchersLink(JSCList* link) {
    char* p = reinterpret_cast<char*>(link);
    return reinterpret_cast<Debugger*>(p - offsetof(Debugger, onNewGlobalObjectWatchersLink));
}

const js::HeapPtrNativeObject&
Debugger::toJSObject() const
{
    MOZ_ASSERT(object);
    return object;
}

js::HeapPtrNativeObject&
Debugger::toJSObjectRef()
{
    MOZ_ASSERT(object);
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
Debugger::observesGlobal(GlobalObject* global) const
{
    ReadBarriered<GlobalObject*> debuggee(global);
    return debuggees.has(debuggee);
}

 void
Debugger::onNewScript(JSContext* cx, HandleScript script)
{
    
    
    MOZ_ASSERT_IF(!script->compartment()->options().invisibleToDebugger() &&
                  !script->selfHosted(),
                  script->compartment()->firedOnNewGlobalObject);
    if (script->compartment()->isDebuggee())
        slowPathOnNewScript(cx, script);
}

 void
Debugger::onNewGlobalObject(JSContext* cx, Handle<GlobalObject*> global)
{
    MOZ_ASSERT(!global->compartment()->firedOnNewGlobalObject);
#ifdef DEBUG
    global->compartment()->firedOnNewGlobalObject = true;
#endif
    if (!JS_CLIST_IS_EMPTY(&cx->runtime()->onNewGlobalObjectWatchers))
        Debugger::slowPathOnNewGlobalObject(cx, global);
}

 bool
Debugger::onLogAllocationSite(JSContext* cx, JSObject* obj, HandleSavedFrame frame, int64_t when)
{
    GlobalObject::DebuggerVector* dbgs = cx->global()->getDebuggers();
    if (!dbgs || dbgs->empty())
        return true;
    RootedObject hobj(cx, obj);
    return Debugger::slowPathOnLogAllocationSite(cx, hobj, frame, when, *dbgs);
}

bool ReportObjectRequired(JSContext* cx);

} 


#endif 
