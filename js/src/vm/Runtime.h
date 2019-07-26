





#ifndef vm_Runtime_h
#define vm_Runtime_h

#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Scoped.h"
#include "mozilla/ThreadLocal.h"

#include <setjmp.h>

#include "jsatom.h"
#include "jsclist.h"
#ifdef DEBUG
# include "jsproxy.h"
#endif
#include "jsscript.h"

#include "ds/FixedSizeHash.h"
#include "frontend/ParseMaps.h"
#include "gc/GCRuntime.h"
#include "gc/Tracer.h"
#ifndef JS_YARR
#include "irregexp/RegExpStack.h"
#endif
#ifdef XP_MACOSX
# include "jit/AsmJSSignalHandlers.h"
#endif
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/CommonPropertyNames.h"
#include "vm/DateTime.h"
#include "vm/MallocProvider.h"
#include "vm/SPSProfiler.h"
#include "vm/Stack.h"
#include "vm/ThreadPool.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif

namespace js {

class PerThreadData;
class ThreadSafeContext;
class AutoKeepAtoms;
#ifdef JS_TRACE_LOGGING
class TraceLogger;
#endif


extern mozilla::ThreadLocal<PerThreadData*> TlsPerThreadData;

} 

struct DtoaState;

extern void
js_ReportOutOfMemory(js::ThreadSafeContext *cx);

extern void
js_ReportAllocationOverflow(js::ThreadSafeContext *cx);

extern void
js_ReportOverRecursed(js::ThreadSafeContext *cx);

namespace JSC { class ExecutableAllocator; }

#ifdef JS_YARR
namespace WTF { class BumpPointerAllocator; }
#endif

namespace js {

typedef Rooted<JSLinearString*> RootedLinearString;

class Activation;
class ActivationIterator;
class AsmJSActivation;
class MathCache;

namespace jit {
class JitRuntime;
class JitActivation;
struct PcScriptCache;
class Simulator;
class SimulatorRuntime;
class AutoFlushICache;
}







struct GSNCache {
    typedef HashMap<jsbytecode *,
                    jssrcnote *,
                    PointerHasher<jsbytecode *, 0>,
                    SystemAllocPolicy> Map;

    jsbytecode      *code;
    Map             map;

    GSNCache() : code(nullptr) { }

    void purge();
};





struct ScopeCoordinateNameCache {
    typedef HashMap<uint32_t,
                    jsid,
                    DefaultHasher<uint32_t>,
                    SystemAllocPolicy> Map;

    Shape *shape;
    Map map;

    ScopeCoordinateNameCache() : shape(nullptr) {}
    void purge();
};

struct EvalCacheEntry
{
    JSScript *script;
    JSScript *callerScript;
    jsbytecode *pc;
};

struct EvalCacheLookup
{
    EvalCacheLookup(JSContext *cx) : str(cx), callerScript(cx) {}
    RootedLinearString str;
    RootedScript callerScript;
    JSVersion version;
    jsbytecode *pc;
};

struct EvalCacheHashPolicy
{
    typedef EvalCacheLookup Lookup;

    static HashNumber hash(const Lookup &l);
    static bool match(const EvalCacheEntry &entry, const EvalCacheLookup &l);
};

typedef HashSet<EvalCacheEntry, EvalCacheHashPolicy, SystemAllocPolicy> EvalCache;

struct LazyScriptHashPolicy
{
    struct Lookup {
        JSContext *cx;
        LazyScript *lazy;

        Lookup(JSContext *cx, LazyScript *lazy)
          : cx(cx), lazy(lazy)
        {}
    };

    static const size_t NumHashes = 3;

    static void hash(const Lookup &lookup, HashNumber hashes[NumHashes]);
    static bool match(JSScript *script, const Lookup &lookup);

    
    
    static void hash(JSScript *script, HashNumber hashes[NumHashes]);
    static bool match(JSScript *script, JSScript *lookup) { return script == lookup; }

    static void clear(JSScript **pscript) { *pscript = nullptr; }
    static bool isCleared(JSScript *script) { return !script; }
};

typedef FixedSizeHashSet<JSScript *, LazyScriptHashPolicy, 769> LazyScriptCache;

class PropertyIteratorObject;

class NativeIterCache
{
    static const size_t SIZE = size_t(1) << 8;

    
    PropertyIteratorObject *data[SIZE];

    static size_t getIndex(uint32_t key) {
        return size_t(key) % SIZE;
    }

  public:
    
    PropertyIteratorObject *last;

    NativeIterCache()
      : last(nullptr)
    {
        mozilla::PodArrayZero(data);
    }

    void purge() {
        last = nullptr;
        mozilla::PodArrayZero(data);
    }

    PropertyIteratorObject *get(uint32_t key) const {
        return data[getIndex(key)];
    }

    void set(uint32_t key, PropertyIteratorObject *iterobj) {
        data[getIndex(key)] = iterobj;
    }
};






class NewObjectCache
{
    
    static const unsigned MAX_OBJ_SIZE = 4 * sizeof(void*) + 16 * sizeof(Value);

    static void staticAsserts() {
        JS_STATIC_ASSERT(NewObjectCache::MAX_OBJ_SIZE == sizeof(JSObject_Slots16));
        JS_STATIC_ASSERT(gc::FINALIZE_OBJECT_LAST == gc::FINALIZE_OBJECT16_BACKGROUND);
    }

    struct Entry
    {
        
        const Class *clasp;

        












        gc::Cell *key;

        
        gc::AllocKind kind;

        
        uint32_t nbytes;

        



        char templateObject[MAX_OBJ_SIZE];
    };

    Entry entries[41];  

  public:

    typedef int EntryIndex;

    NewObjectCache() { mozilla::PodZero(this); }
    void purge() { mozilla::PodZero(this); }

    
    void clearNurseryObjects(JSRuntime *rt);

    



    inline bool lookupProto(const Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupGlobal(const Class *clasp, js::GlobalObject *global, gc::AllocKind kind,
                             EntryIndex *pentry);

    bool lookupType(js::types::TypeObject *type, gc::AllocKind kind, EntryIndex *pentry) {
        return lookup(type->clasp(), type, kind, pentry);
    }

    




    template <AllowGC allowGC>
    inline JSObject *newObjectFromHit(JSContext *cx, EntryIndex entry, js::gc::InitialHeap heap);

    
    void fillProto(EntryIndex entry, const Class *clasp, js::TaggedProto proto, gc::AllocKind kind, JSObject *obj);

    inline void fillGlobal(EntryIndex entry, const Class *clasp, js::GlobalObject *global,
                           gc::AllocKind kind, JSObject *obj);

    void fillType(EntryIndex entry, js::types::TypeObject *type, gc::AllocKind kind,
                  JSObject *obj)
    {
        JS_ASSERT(obj->type() == type);
        return fill(entry, type->clasp(), type, kind, obj);
    }

    
    void invalidateEntriesForShape(JSContext *cx, HandleShape shape, HandleObject proto);

  private:
    bool lookup(const Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry) {
        uintptr_t hash = (uintptr_t(clasp) ^ uintptr_t(key)) + kind;
        *pentry = hash % mozilla::ArrayLength(entries);

        Entry *entry = &entries[*pentry];

        
        return entry->clasp == clasp && entry->key == key;
    }

    void fill(EntryIndex entry_, const Class *clasp, gc::Cell *key, gc::AllocKind kind, JSObject *obj) {
        JS_ASSERT(unsigned(entry_) < mozilla::ArrayLength(entries));
        Entry *entry = &entries[entry_];

        JS_ASSERT(!obj->hasDynamicSlots() && !obj->hasDynamicElements());

        entry->clasp = clasp;
        entry->key = key;
        entry->kind = kind;

        entry->nbytes = gc::Arena::thingSize(kind);
        js_memcpy(&entry->templateObject, obj, entry->nbytes);
    }

    static void copyCachedToObject(JSObject *dst, JSObject *src, gc::AllocKind kind) {
        js_memcpy(dst, src, gc::Arena::thingSize(kind));
#ifdef JSGC_GENERATIONAL
        Shape::writeBarrierPost(dst->shape_, &dst->shape_);
        types::TypeObject::writeBarrierPost(dst->type_, &dst->type_);
#endif
    }
};








class FreeOp : public JSFreeOp {
    bool        shouldFreeLater_;

  public:
    static FreeOp *get(JSFreeOp *fop) {
        return static_cast<FreeOp *>(fop);
    }

    FreeOp(JSRuntime *rt, bool shouldFreeLater)
      : JSFreeOp(rt),
        shouldFreeLater_(shouldFreeLater)
    {
    }

    bool shouldFreeLater() const {
        return shouldFreeLater_;
    }

    inline void free_(void *p);

    template <class T>
    inline void delete_(T *p) {
        if (p) {
            p->~T();
            free_(p);
        }
    }

    static void staticAsserts() {
        





        JS_STATIC_ASSERT(offsetof(FreeOp, shouldFreeLater_) == sizeof(JSFreeOp));
    }
};

} 

namespace JS {
struct RuntimeSizes;
}


struct JSAtomState
{
#define PROPERTYNAME_FIELD(idpart, id, text) js::ImmutablePropertyNamePtr id;
    FOR_EACH_COMMON_PROPERTYNAME(PROPERTYNAME_FIELD)
#undef PROPERTYNAME_FIELD
#define PROPERTYNAME_FIELD(name, code, init, clasp) js::ImmutablePropertyNamePtr name;
    JS_FOR_EACH_PROTOTYPE(PROPERTYNAME_FIELD)
#undef PROPERTYNAME_FIELD
};

namespace js {

#define NAME_OFFSET(name)       offsetof(JSAtomState, name)

inline HandlePropertyName
AtomStateOffsetToName(const JSAtomState &atomState, size_t offset)
{
    return *reinterpret_cast<js::ImmutablePropertyNamePtr *>((char*)&atomState + offset);
}




enum RuntimeLock {
    ExclusiveAccessLock,
    WorkerThreadStateLock,
    InterruptLock,
    GCLock
};

#ifdef DEBUG
void AssertCurrentThreadCanLock(RuntimeLock which);
#else
inline void AssertCurrentThreadCanLock(RuntimeLock which) {}
#endif








class PerThreadData : public PerThreadDataFriendFields
{
    






    JSRuntime *runtime_;

  public:
    





#ifdef DEBUG
    struct SavedGCRoot {
        void *thing;
        JSGCTraceKind kind;

        SavedGCRoot(void *thing, JSGCTraceKind kind) : thing(thing), kind(kind) {}
    };
    js::Vector<SavedGCRoot, 0, js::SystemAllocPolicy> gcSavedRoots;
#endif





    uint8_t             *jitTop;

    




    JSContext           *jitJSContext;

    



    uintptr_t            jitStackLimit;

    inline void setJitStackLimit(uintptr_t limit);

#ifndef JS_YARR
    
    irregexp::RegExpStack regexpStack;
#endif

#ifdef JS_TRACE_LOGGING
    TraceLogger         *traceLogger;
#endif

    







  private:
    friend class js::Activation;
    friend class js::ActivationIterator;
    friend class js::jit::JitActivation;
    friend class js::AsmJSActivation;
#ifdef DEBUG
    friend void js::AssertCurrentThreadCanLock(RuntimeLock which);
#endif

    



    js::Activation *activation_;

    
    js::AsmJSActivation *asmJSActivationStack_;

    
    js::jit::AutoFlushICache *autoFlushICache_;

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::Simulator *simulator_;
    uintptr_t simulatorStackLimit_;
#endif

  public:
    js::Activation *const *addressOfActivation() const {
        return &activation_;
    }
    static unsigned offsetOfAsmJSActivationStackReadOnly() {
        return offsetof(PerThreadData, asmJSActivationStack_);
    }
    static unsigned offsetOfActivation() {
        return offsetof(PerThreadData, activation_);
    }

    js::AsmJSActivation *asmJSActivationStackFromAnyThread() const {
        return asmJSActivationStack_;
    }
    js::AsmJSActivation *asmJSActivationStackFromOwnerThread() const {
        return asmJSActivationStack_;
    }

    js::Activation *activation() const {
        return activation_;
    }

    
    DtoaState           *dtoaState;

    







    int32_t suppressGC;

    
    unsigned activeCompilations;

    PerThreadData(JSRuntime *runtime);
    ~PerThreadData();

    bool init();

    bool associatedWith(const JSRuntime *rt) { return runtime_ == rt; }
    inline JSRuntime *runtimeFromMainThread();
    inline JSRuntime *runtimeIfOnOwnerThread();

    inline bool exclusiveThreadsPresent();
    inline void addActiveCompilation();
    inline void removeActiveCompilation();

    
    
    class MOZ_STACK_CLASS AutoEnterRuntime
    {
        PerThreadData *pt;

      public:
        AutoEnterRuntime(PerThreadData *pt, JSRuntime *rt)
          : pt(pt)
        {
            JS_ASSERT(!pt->runtime_);
            pt->runtime_ = rt;
        }

        ~AutoEnterRuntime() {
            pt->runtime_ = nullptr;
        }
    };

    js::jit::AutoFlushICache *autoFlushICache() const;
    void setAutoFlushICache(js::jit::AutoFlushICache *afc);

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::Simulator *simulator() const;
    void setSimulator(js::jit::Simulator *sim);
    js::jit::SimulatorRuntime *simulatorRuntime() const;
    uintptr_t *addressOfSimulatorStackLimit();
#endif
};

class AutoLockForExclusiveAccess;

void RecomputeStackLimit(JSRuntime *rt, StackKind kind);

} 

struct JSRuntime : public JS::shadow::Runtime,
                   public js::MallocProvider<JSRuntime>
{
    









    js::PerThreadData mainThread;

    



    JSRuntime *parentRuntime;

    



    mozilla::Atomic<bool, mozilla::Relaxed> interrupt;

#if defined(JS_THREADSAFE) && defined(JS_ION)
    




    mozilla::Atomic<bool, mozilla::Relaxed> interruptPar;
#endif

    
    bool handlingSignal;

    JSInterruptCallback interruptCallback;

#ifdef DEBUG
    void assertCanLock(js::RuntimeLock which);
#else
    void assertCanLock(js::RuntimeLock which) {}
#endif

  private:
    



#ifdef JS_THREADSAFE
    PRLock *interruptLock;
    PRThread *interruptLockOwner;
#else
    bool interruptLockTaken;
#endif 
  public:

    class AutoLockForInterrupt {
        JSRuntime *rt;
      public:
        AutoLockForInterrupt(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) : rt(rt) {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
            rt->assertCanLock(js::InterruptLock);
#ifdef JS_THREADSAFE
            PR_Lock(rt->interruptLock);
            rt->interruptLockOwner = PR_GetCurrentThread();
#else
            rt->interruptLockTaken = true;
#endif 
        }
        ~AutoLockForInterrupt() {
            JS_ASSERT(rt->currentThreadOwnsInterruptLock());
#ifdef JS_THREADSAFE
            rt->interruptLockOwner = nullptr;
            PR_Unlock(rt->interruptLock);
#else
            rt->interruptLockTaken = false;
#endif 
        }

        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    };

    bool currentThreadOwnsInterruptLock() {
#if defined(JS_THREADSAFE)
        return interruptLockOwner == PR_GetCurrentThread();
#else
        return interruptLockTaken;
#endif
    }

#ifdef JS_THREADSAFE

  private:
    







    PRLock *exclusiveAccessLock;
    mozilla::DebugOnly<PRThread *> exclusiveAccessOwner;
    mozilla::DebugOnly<bool> mainThreadHasExclusiveAccess;

    
    size_t numExclusiveThreads;

    friend class js::AutoLockForExclusiveAccess;

  public:
    void setUsedByExclusiveThread(JS::Zone *zone);
    void clearUsedByExclusiveThread(JS::Zone *zone);

#endif 

#ifdef DEBUG
    bool currentThreadHasExclusiveAccess() {
#ifdef JS_THREADSAFE
        return (!numExclusiveThreads && mainThreadHasExclusiveAccess) ||
               exclusiveAccessOwner == PR_GetCurrentThread();
#else
        return true;
#endif
    }
#endif 

    bool exclusiveThreadsPresent() const {
#ifdef JS_THREADSAFE
        return numExclusiveThreads > 0;
#else
        return false;
#endif
    }

    
    size_t              numCompartments;

    
    JSLocaleCallbacks *localeCallbacks;

    
    char *defaultLocale;

    
    JSVersion defaultVersion_;

#ifdef JS_THREADSAFE
  private:
    
    void *ownerThread_;
    friend bool js::CurrentThreadCanAccessRuntime(JSRuntime *rt);
  public:
#endif

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 4 * 1024;
    js::LifoAlloc tempLifoAlloc;

    



    js::LifoAlloc freeLifoAlloc;

  private:
    



    JSC::ExecutableAllocator *execAlloc_;
#ifdef JS_YARR
    WTF::BumpPointerAllocator *bumpAlloc_;
#endif
    js::jit::JitRuntime *jitRuntime_;

    



    JSObject *selfHostingGlobal_;

    
    js::InterpreterStack interpreterStack_;

    JSC::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
#ifdef JS_YARR
    WTF::BumpPointerAllocator *createBumpPointerAllocator(JSContext *cx);
#endif
    js::jit::JitRuntime *createJitRuntime(JSContext *cx);

  public:
    JSC::ExecutableAllocator *getExecAlloc(JSContext *cx) {
        return execAlloc_ ? execAlloc_ : createExecutableAllocator(cx);
    }
    JSC::ExecutableAllocator &execAlloc() {
        JS_ASSERT(execAlloc_);
        return *execAlloc_;
    }
    JSC::ExecutableAllocator *maybeExecAlloc() {
        return execAlloc_;
    }
#ifdef JS_YARR
    WTF::BumpPointerAllocator *getBumpPointerAllocator(JSContext *cx) {
        return bumpAlloc_ ? bumpAlloc_ : createBumpPointerAllocator(cx);
    }
#endif
    js::jit::JitRuntime *getJitRuntime(JSContext *cx) {
        return jitRuntime_ ? jitRuntime_ : createJitRuntime(cx);
    }
    js::jit::JitRuntime *jitRuntime() const {
        return jitRuntime_;
    }
    bool hasJitRuntime() const {
        return !!jitRuntime_;
    }
    js::InterpreterStack &interpreterStack() {
        return interpreterStack_;
    }

    
    
    

    bool initSelfHosting(JSContext *cx);
    void finishSelfHosting();
    void markSelfHostingGlobal(JSTracer *trc);
    bool isSelfHostingGlobal(JSObject *global) {
        return global == selfHostingGlobal_;
    }
    bool isSelfHostingCompartment(JSCompartment *comp);
    bool cloneSelfHostedFunctionScript(JSContext *cx, js::Handle<js::PropertyName*> name,
                                       js::Handle<JSFunction*> targetFun);
    bool cloneSelfHostedValue(JSContext *cx, js::Handle<js::PropertyName*> name,
                              js::MutableHandleValue vp);

    
    
    

    






    bool setDefaultLocale(const char *locale);

    
    void resetDefaultLocale();

    
    const char *getDefaultLocale();

    JSVersion defaultVersion() { return defaultVersion_; }
    void setDefaultVersion(JSVersion v) { defaultVersion_ = v; }

    
    uintptr_t           nativeStackBase;

    
    size_t              nativeStackQuota[js::StackKindCount];

    
    JSContextCallback   cxCallback;
    void               *cxCallbackData;

    
    JSDestroyCompartmentCallback destroyCompartmentCallback;

    
    JSZoneCallback destroyZoneCallback;

    
    JSZoneCallback sweepZoneCallback;

    
    JSCompartmentNameCallback compartmentNameCallback;

    js::ActivityCallback  activityCallback;
    void                 *activityCallbackArg;
    void triggerActivityCallback(bool active);

#ifdef JS_THREADSAFE
    
    unsigned            requestDepth;

# ifdef DEBUG
    unsigned            checkRequestDepth;
# endif
#endif

#ifdef DEBUG
    





    JSContext          *activeContext;
#endif

    
    js::gc::GCRuntime   gc;

    
    bool                gcInitialized;

    JSGCMode gcMode() const { return gc.mode; }
    void setGCMode(JSGCMode mode) {
        gc.mode = mode;
        gc.marker.setGCMode(mode);
    }

    bool isHeapBusy() { return gc.isHeapBusy(); }
    bool isHeapMajorCollecting() { return gc.isHeapMajorCollecting(); }
    bool isHeapMinorCollecting() { return gc.isHeapMinorCollecting(); }
    bool isHeapCollecting() { return gc.isHeapCollecting(); }

#ifdef JS_GC_ZEAL
    int gcZeal() { return gc.zealMode; }

    bool upcomingZealousGC() {
        return gc.nextScheduled == 1;
    }

    bool needZealousGC() {
        if (gc.nextScheduled > 0 && --gc.nextScheduled == 0) {
            if (gcZeal() == js::gc::ZealAllocValue ||
                gcZeal() == js::gc::ZealGenerationalGCValue ||
                (gcZeal() >= js::gc::ZealIncrementalRootsThenFinish &&
                 gcZeal() <= js::gc::ZealIncrementalMultipleSlices))
            {
                gc.nextScheduled = gc.zealFrequency;
            }
            return true;
        }
        return false;
    }
#else
    int gcZeal() { return 0; }
    bool upcomingZealousGC() { return false; }
    bool needZealousGC() { return false; }
#endif

    void lockGC() {
        assertCanLock(js::GCLock);
        gc.lockGC();
    }

    void unlockGC() {
        gc.unlockGC();
    }

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::SimulatorRuntime *simulatorRuntime_;
#endif

  public:
    void setNeedsBarrier(bool needs) {
        needsBarrier_ = needs;
    }

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::SimulatorRuntime *simulatorRuntime() const;
    void setSimulatorRuntime(js::jit::SimulatorRuntime *srt);
#endif

    
    const js::Value     NaNValue;
    const js::Value     negativeInfinityValue;
    const js::Value     positiveInfinityValue;

    js::PropertyName    *emptyString;

    
    mozilla::LinkedList<JSContext> contextList;

    bool hasContexts() const {
        return !contextList.isEmpty();
    }

    mozilla::ScopedDeletePtr<js::SourceHook> sourceHook;

    
    JSDebugHooks        debugHooks;

    
    bool                debugMode;

    
    js::SPSProfiler     spsProfiler;

    
    bool                profilingScripts;

    
    bool                hadOutOfMemory;

    
    bool                haveCreatedContext;

    
    mozilla::LinkedList<js::Debugger> debuggerList;

    



    JSCList             onNewGlobalObjectWatchers;

    
    void                *data;

#if defined(XP_MACOSX) && defined(JS_ION)
    js::AsmJSMachExceptionHandler asmJSMachExceptionHandler;
#endif

    
    
  private:
    bool signalHandlersInstalled_;
  public:
    bool signalHandlersInstalled() const {
        return signalHandlersInstalled_;
    }

  private:
    js::FreeOp          defaultFreeOp_;

  public:
    js::FreeOp *defaultFreeOp() {
        return &defaultFreeOp_;
    }

    uint32_t            debuggerMutations;

    const JSSecurityCallbacks *securityCallbacks;
    const js::DOMCallbacks *DOMcallbacks;
    JSDestroyPrincipalsOp destroyPrincipals;

    
    const JSStructuredCloneCallbacks *structuredCloneCallbacks;

    
    JSAccumulateTelemetryDataCallback telemetryCallback;

    
    JS::AsmJSCacheOps asmJSCacheOps;

    




    uint32_t            propertyRemovals;

#if !EXPOSE_INTL_API
    
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;
#endif

  private:
    js::MathCache *mathCache_;
    js::MathCache *createMathCache(JSContext *cx);
  public:
    js::MathCache *getMathCache(JSContext *cx) {
        return mathCache_ ? mathCache_ : createMathCache(cx);
    }
    js::MathCache *maybeGetMathCache() {
        return mathCache_;
    }

    js::GSNCache        gsnCache;
    js::ScopeCoordinateNameCache scopeCoordinateNameCache;
    js::NewObjectCache  newObjectCache;
    js::NativeIterCache nativeIterCache;
    js::SourceDataCache sourceDataCache;
    js::EvalCache       evalCache;
    js::LazyScriptCache lazyScriptCache;

    js::DateTimeInfo    dateTimeInfo;

    
    
    
  private:
    js::frontend::ParseMapPool parseMapPool_;
    unsigned activeCompilations_;
  public:
    js::frontend::ParseMapPool &parseMapPool() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return parseMapPool_;
    }
    bool hasActiveCompilations() {
        return activeCompilations_ != 0;
    }
    void addActiveCompilation() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        activeCompilations_++;
    }
    void removeActiveCompilation() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        activeCompilations_--;
    }

    
    
    
    
    
    
    
    
    
  private:
    unsigned keepAtoms_;
    friend class js::AutoKeepAtoms;
  public:
    bool keepAtoms() {
        JS_ASSERT(CurrentThreadCanAccessRuntime(this));
        return keepAtoms_ != 0 || exclusiveThreadsPresent();
    }

  private:
    const JSPrincipals  *trustedPrincipals_;
  public:
    void setTrustedPrincipals(const JSPrincipals *p) { trustedPrincipals_ = p; }
    const JSPrincipals *trustedPrincipals() const { return trustedPrincipals_; }

  private:
    bool beingDestroyed_;
  public:
    bool isBeingDestroyed() const {
        return beingDestroyed_;
    }

  private:
    
    
    
    js::AtomSet *atoms_;

    
    
    
    JSCompartment *atomsCompartment_;

  public:
    bool initializeAtoms(JSContext *cx);
    void finishAtoms();

    void sweepAtoms();

    js::AtomSet &atoms() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return *atoms_;
    }
    JSCompartment *atomsCompartment() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return atomsCompartment_;
    }

    bool isAtomsCompartment(JSCompartment *comp) {
        return comp == atomsCompartment_;
    }

    
    inline bool isAtomsZone(JS::Zone *zone);

    bool activeGCInAtomsZone();

    
    
    
    

    
    js::StaticStrings *staticStrings;

    
    JSAtomState *commonNames;

    
    js::AtomSet *permanentAtoms;

    bool transformToPermanentAtoms();

    const JSWrapObjectCallbacks            *wrapObjectCallbacks;
    js::PreserveWrapperCallback            preserveWrapperCallback;

    
    
    
  private:
    js::ScriptDataTable scriptDataTable_;
  public:
    js::ScriptDataTable &scriptDataTable() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return scriptDataTable_;
    }

    bool                jitSupportsFloatingPoint;

    
    
    void resetJitStackLimit();

    
    js::jit::PcScriptCache *ionPcScriptCache;

    js::ThreadPool threadPool;

    js::DefaultJSContextCallback defaultJSContextCallback;

    js::CTypesActivityCallback  ctypesActivityCallback;

    
    
    uint32_t forkJoinWarmup;

  private:
#ifdef JS_THREADSAFE
    static mozilla::Atomic<size_t> liveRuntimesCount;
#else
    static size_t liveRuntimesCount;
#endif

  public:
    static bool hasLiveRuntimes() {
        return liveRuntimesCount > 0;
    }

    JSRuntime(JSRuntime *parentRuntime, JSUseHelperThreads useHelperThreads);
    ~JSRuntime();

    bool init(uint32_t maxbytes);

    JSRuntime *thisFromCtor() { return this; }

    void setGCMaxMallocBytes(size_t value);

    void resetGCMallocBytes() {
        gc.mallocBytes = ptrdiff_t(gc.maxMallocBytes);
        gc.mallocGCTriggered = false;
    }

    







    void updateMallocCounter(size_t nbytes);
    void updateMallocCounter(JS::Zone *zone, size_t nbytes);

    void reportAllocationOverflow() { js_ReportAllocationOverflow(nullptr); }

    bool isTooMuchMalloc() const {
        return gc.mallocBytes <= 0;
    }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes);
    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);

    
    
    enum InterruptMode {
        RequestInterruptMainThread,
        RequestInterruptAnyThread,
        RequestInterruptAnyThreadDontStopIon,
        RequestInterruptAnyThreadForkJoin
    };

    void requestInterrupt(InterruptMode mode);

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::RuntimeSizes *runtime);

  private:
    JS::RuntimeOptions options_;

    JSUseHelperThreads useHelperThreads_;

    
    bool parallelIonCompilationEnabled_;
    bool parallelParsingEnabled_;

    
    bool isWorkerRuntime_;

  public:

    
    
    
    bool useHelperThreads() const {
#ifdef JS_THREADSAFE
        return useHelperThreads_ == JS_USE_HELPER_THREADS;
#else
        return false;
#endif
    }

    
    
    void setParallelIonCompilationEnabled(bool value) {
        parallelIonCompilationEnabled_ = value;
    }
    bool canUseParallelIonCompilation() const {
        return useHelperThreads() &&
               parallelIonCompilationEnabled_;
    }
    void setParallelParsingEnabled(bool value) {
        parallelParsingEnabled_ = value;
    }
    bool canUseParallelParsing() const {
        return useHelperThreads() &&
               parallelParsingEnabled_;
    }

    void setIsWorkerRuntime() {
        isWorkerRuntime_ = true;
    }
    bool isWorkerRuntime() const {
        return isWorkerRuntime_;
    }

    const JS::RuntimeOptions &options() const {
        return options_;
    }
    JS::RuntimeOptions &options() {
        return options_;
    }

#ifdef DEBUG
  public:
    js::AutoEnterPolicy *enteredPolicy;
#endif

    
    JS::LargeAllocationFailureCallback largeAllocationFailureCallback;
    
    JS::OutOfMemoryCallback oomCallback;

    




    static const unsigned LARGE_ALLOCATION = 25 * 1024 * 1024;

    void *callocCanGC(size_t bytes) {
        void *p = calloc_(bytes);
        if (MOZ_LIKELY(!!p))
            return p;
        if (!largeAllocationFailureCallback || bytes < LARGE_ALLOCATION)
            return nullptr;
        largeAllocationFailureCallback();
        return onOutOfMemory(reinterpret_cast<void *>(1), bytes);
    }

    void *reallocCanGC(void *p, size_t bytes) {
        void *p2 = realloc_(p, bytes);
        if (MOZ_LIKELY(!!p2))
            return p2;
        if (!largeAllocationFailureCallback || bytes < LARGE_ALLOCATION)
            return nullptr;
        largeAllocationFailureCallback();
        return onOutOfMemory(p, bytes);
    }
};

namespace js {






static inline JSContext *
GetJSContextFromJitCode()
{
    JSContext *cx = TlsPerThreadData.get()->jitJSContext;
    JS_ASSERT(cx);
    return cx;
}









namespace VersionFlags {
static const unsigned MASK      = 0x0FFF; 
} 

static inline JSVersion
VersionNumber(JSVersion version)
{
    return JSVersion(uint32_t(version) & VersionFlags::MASK);
}

static inline JSVersion
VersionExtractFlags(JSVersion version)
{
    return JSVersion(uint32_t(version) & ~VersionFlags::MASK);
}

static inline void
VersionCopyFlags(JSVersion *version, JSVersion from)
{
    *version = JSVersion(VersionNumber(*version) | VersionExtractFlags(from));
}

static inline bool
VersionHasFlags(JSVersion version)
{
    return !!VersionExtractFlags(version);
}

static inline bool
VersionIsKnown(JSVersion version)
{
    return VersionNumber(version) != JSVERSION_UNKNOWN;
}

inline void
FreeOp::free_(void *p)
{
    if (shouldFreeLater()) {
        runtime()->gc.freeLater(p);
        return;
    }
    js_free(p);
}

class AutoLockGC
{
  public:
    explicit AutoLockGC(JSRuntime *rt = nullptr
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : runtime(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        
        if (rt)
            rt->lockGC();
    }

    ~AutoLockGC()
    {
        if (runtime)
            runtime->unlockGC();
    }

    bool locked() const {
        return !!runtime;
    }

    void lock(JSRuntime *rt) {
        JS_ASSERT(rt);
        JS_ASSERT(!runtime);
        runtime = rt;
        rt->lockGC();
    }

  private:
    JSRuntime *runtime;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoUnlockGC
{
  private:
    JSRuntime *rt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoUnlockGC(JSRuntime *rt
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        rt->unlockGC();
    }
    ~AutoUnlockGC() { rt->lockGC(); }
};

class MOZ_STACK_CLASS AutoKeepAtoms
{
    PerThreadData *pt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoKeepAtoms(PerThreadData *pt
                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : pt(pt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (JSRuntime *rt = pt->runtimeIfOnOwnerThread()) {
            rt->keepAtoms_++;
        } else {
            
            
            JS_ASSERT(pt->exclusiveThreadsPresent());
        }
    }
    ~AutoKeepAtoms() {
        if (JSRuntime *rt = pt->runtimeIfOnOwnerThread()) {
            JS_ASSERT(rt->keepAtoms_);
            rt->keepAtoms_--;
        }
    }
};

inline void
PerThreadData::setJitStackLimit(uintptr_t limit)
{
    JS_ASSERT(runtime_->currentThreadOwnsInterruptLock());
    jitStackLimit = limit;
}

inline JSRuntime *
PerThreadData::runtimeFromMainThread()
{
    JS_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
    return runtime_;
}

inline JSRuntime *
PerThreadData::runtimeIfOnOwnerThread()
{
    return CurrentThreadCanAccessRuntime(runtime_) ? runtime_ : nullptr;
}

inline bool
PerThreadData::exclusiveThreadsPresent()
{
    return runtime_->exclusiveThreadsPresent();
}

inline void
PerThreadData::addActiveCompilation()
{
    activeCompilations++;
    runtime_->addActiveCompilation();
}

inline void
PerThreadData::removeActiveCompilation()
{
    JS_ASSERT(activeCompilations);
    activeCompilations--;
    runtime_->removeActiveCompilation();
}



static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Value *vec, size_t len)
{
    mozilla::PodZero(vec, len);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Value *beg, Value *end)
{
    mozilla::PodZero(beg, end - beg);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *beg, jsid *end)
{
    for (jsid *id = beg; id != end; ++id)
        *id = INT_TO_JSID(0);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *vec, size_t len)
{
    MakeRangeGCSafe(vec, vec + len);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Shape **beg, Shape **end)
{
    mozilla::PodZero(beg, end - beg);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Shape **vec, size_t len)
{
    mozilla::PodZero(vec, len);
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setUndefined();
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *vec, size_t len)
{
    SetValueRangeToUndefined(vec, vec + len);
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToNull(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setNull();
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToNull(Value *vec, size_t len)
{
    SetValueRangeToNull(vec, vec + len);
}












class RuntimeAllocPolicy
{
    JSRuntime *const runtime;

  public:
    RuntimeAllocPolicy(JSRuntime *rt) : runtime(rt) {}
    void *malloc_(size_t bytes) { return runtime->malloc_(bytes); }
    void *calloc_(size_t bytes) { return runtime->calloc_(bytes); }
    void *realloc_(void *p, size_t bytes) { return runtime->realloc_(p, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const {}
};

extern const JSSecurityCallbacks NullSecurityCallbacks;

} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
