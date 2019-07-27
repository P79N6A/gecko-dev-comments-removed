





#ifndef vm_Runtime_h
#define vm_Runtime_h

#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Scoped.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/UniquePtr.h"

#include <setjmp.h>

#include "jsatom.h"
#include "jsclist.h"
#ifdef DEBUG
# include "jsproxy.h"
#endif
#include "jsscript.h"

#ifdef XP_MACOSX
# include "asmjs/AsmJSSignalHandlers.h"
#endif
#include "ds/FixedSizeHash.h"
#include "frontend/ParseMaps.h"
#include "gc/GCRuntime.h"
#include "gc/Tracer.h"
#include "irregexp/RegExpStack.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/CommonPropertyNames.h"
#include "vm/DateTime.h"
#include "vm/MallocProvider.h"
#include "vm/SPSProfiler.h"
#include "vm/Stack.h"
#include "vm/Symbol.h"
#include "vm/ThreadPool.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif

namespace js {

class PerThreadData;
struct ThreadSafeContext;
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

namespace js {

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
struct AutoFlushICache;
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

typedef Vector<ScriptAndCounts, 0, SystemAllocPolicy> ScriptAndCountsVector;

struct EvalCacheEntry
{
    JSScript *script;
    JSScript *callerScript;
    jsbytecode *pc;
};

struct EvalCacheLookup
{
    explicit EvalCacheLookup(JSContext *cx) : str(cx), callerScript(cx) {}
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








class FreeOp : public JSFreeOp
{
    Vector<void *, 0, SystemAllocPolicy> freeLaterList;

  public:
    static FreeOp *get(JSFreeOp *fop) {
        return static_cast<FreeOp *>(fop);
    }

    explicit FreeOp(JSRuntime *rt)
      : JSFreeOp(rt)
    {}

    ~FreeOp() {
        for (size_t i = 0; i < freeLaterList.length(); i++)
            free_(freeLaterList[i]);
    }

    inline void free_(void *p);
    inline void freeLater(void *p);

    template <class T>
    inline void delete_(T *p) {
        if (p) {
            p->~T();
            free_(p);
        }
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










struct WellKnownSymbols
{
    js::ImmutableSymbolPtr iterator;

    ImmutableSymbolPtr &get(size_t i) {
        MOZ_ASSERT(i < JS::WellKnownSymbolLimit);
        ImmutableSymbolPtr *symbols = reinterpret_cast<ImmutableSymbolPtr *>(this);
        return symbols[i];
    }
};

#define NAME_OFFSET(name)       offsetof(JSAtomState, name)

inline HandlePropertyName
AtomStateOffsetToName(const JSAtomState &atomState, size_t offset)
{
    return *reinterpret_cast<js::ImmutablePropertyNamePtr *>((char*)&atomState + offset);
}




enum RuntimeLock {
    ExclusiveAccessLock,
    HelperThreadStateLock,
    InterruptLock,
    GCLock
};

#ifdef DEBUG
void AssertCurrentThreadCanLock(RuntimeLock which);
#else
inline void AssertCurrentThreadCanLock(RuntimeLock which) {}
#endif

inline bool
CanUseExtraThreads()
{
    extern bool gCanUseExtraThreads;
    return gCanUseExtraThreads;
}

void DisableExtraThreads();








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

    
    irregexp::RegExpStack regexpStack;

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

    



    js::Activation * volatile profilingActivation_;

    
    js::AsmJSActivation * volatile asmJSActivationStack_;

    
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

    js::Activation *profilingActivation() const {
        return profilingActivation_;
    }

    js::AsmJSActivation *asmJSActivationStack() const {
        return asmJSActivationStack_;
    }
    static js::AsmJSActivation *innermostAsmJSActivation() {
        PerThreadData *ptd = TlsPerThreadData.get();
        return ptd ? ptd->asmJSActivationStack_ : nullptr;
    }

    js::Activation *activation() const {
        return activation_;
    }

    
    DtoaState           *dtoaState;

    







    int32_t suppressGC;

#ifdef DEBUG
    
    bool ionCompiling;
#endif

    
    unsigned activeCompilations;

    explicit PerThreadData(JSRuntime *runtime);
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
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
            
            
            
            
            JS_ASSERT(!pt->simulator_);
#endif
        }

        ~AutoEnterRuntime() {
            pt->runtime_ = nullptr;
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
            
            JS_ASSERT(!pt->simulator_);
#endif
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

    




    mozilla::Atomic<bool, mozilla::Relaxed> interruptPar;

    
    bool handlingSignal;

    JSInterruptCallback interruptCallback;

#ifdef DEBUG
    void assertCanLock(js::RuntimeLock which);
#else
    void assertCanLock(js::RuntimeLock which) {}
#endif

  private:
    



    PRLock *interruptLock;
    PRThread *interruptLockOwner;

  public:
    class AutoLockForInterrupt {
        JSRuntime *rt;
      public:
        explicit AutoLockForInterrupt(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) : rt(rt) {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
            rt->assertCanLock(js::InterruptLock);
            PR_Lock(rt->interruptLock);
            rt->interruptLockOwner = PR_GetCurrentThread();
        }
        ~AutoLockForInterrupt() {
            JS_ASSERT(rt->currentThreadOwnsInterruptLock());
            rt->interruptLockOwner = nullptr;
            PR_Unlock(rt->interruptLock);
        }

        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    };

    bool currentThreadOwnsInterruptLock() {
        return interruptLockOwner == PR_GetCurrentThread();
    }

  private:
    







    PRLock *exclusiveAccessLock;
    mozilla::DebugOnly<PRThread *> exclusiveAccessOwner;
    mozilla::DebugOnly<bool> mainThreadHasExclusiveAccess;

    
    size_t numExclusiveThreads;

    friend class js::AutoLockForExclusiveAccess;

  public:
    void setUsedByExclusiveThread(JS::Zone *zone);
    void clearUsedByExclusiveThread(JS::Zone *zone);

#ifdef DEBUG
    bool currentThreadHasExclusiveAccess() {
        return (!numExclusiveThreads && mainThreadHasExclusiveAccess) ||
               exclusiveAccessOwner == PR_GetCurrentThread();
    }
#endif 

    bool exclusiveThreadsPresent() const {
        return numExclusiveThreads > 0;
    }

    
    size_t              numCompartments;

    
    JSLocaleCallbacks *localeCallbacks;

    
    char *defaultLocale;

    
    JSVersion defaultVersion_;

  private:
    
    void *ownerThread_;
    friend bool js::CurrentThreadCanAccessRuntime(JSRuntime *rt);
  public:

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 4 * 1024;
    js::LifoAlloc tempLifoAlloc;

    



    js::LifoAlloc freeLifoAlloc;

  private:
    



    js::jit::ExecutableAllocator *execAlloc_;
    js::jit::JitRuntime *jitRuntime_;

    



    JSObject *selfHostingGlobal_;

    
    js::InterpreterStack interpreterStack_;

    js::jit::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
    js::jit::JitRuntime *createJitRuntime(JSContext *cx);

  public:
    js::jit::ExecutableAllocator *getExecAlloc(JSContext *cx) {
        return execAlloc_ ? execAlloc_ : createExecutableAllocator(cx);
    }
    js::jit::ExecutableAllocator &execAlloc() {
        JS_ASSERT(execAlloc_);
        return *execAlloc_;
    }
    js::jit::ExecutableAllocator *maybeExecAlloc() {
        return execAlloc_;
    }
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

    
    unsigned            requestDepth;

#ifdef DEBUG
    unsigned            checkRequestDepth;

    





    JSContext          *activeContext;
#endif

    
    js::gc::GCRuntime   gc;

    
    bool                gcInitialized;

    bool isHeapBusy() { return gc.isHeapBusy(); }
    bool isHeapMajorCollecting() { return gc.isHeapMajorCollecting(); }
    bool isHeapMinorCollecting() { return gc.isHeapMinorCollecting(); }
    bool isHeapCollecting() { return gc.isHeapCollecting(); }
    bool isHeapCompacting() { return gc.isHeapCompacting(); }

    bool isFJMinorCollecting() { return gc.isFJMinorCollecting(); }

    int gcZeal() { return gc.zeal(); }

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
    void setNeedsIncrementalBarrier(bool needs) {
        needsIncrementalBarrier_ = needs;
    }

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::SimulatorRuntime *simulatorRuntime() const;
    void setSimulatorRuntime(js::jit::SimulatorRuntime *srt);
#endif

    
    js::ScriptAndCountsVector *scriptAndCountsVector;

    
    const js::Value     NaNValue;
    const js::Value     negativeInfinityValue;
    const js::Value     positiveInfinityValue;

    js::PropertyName    *emptyString;

    
    mozilla::LinkedList<JSContext> contextList;

    bool hasContexts() const {
        return !contextList.isEmpty();
    }

    mozilla::UniquePtr<js::SourceHook> sourceHook;

#ifdef NIGHTLY_BUILD
    js::AssertOnScriptEntryHook assertOnScriptEntryHook_;
#endif

    
    js::SPSProfiler     spsProfiler;

    
    bool                profilingScripts;

    
  private:
    bool                suppressProfilerSampling;

  public:
    bool isProfilerSamplingEnabled() const {
        return !suppressProfilerSampling;
    }
    void disableProfilerSampling() {
        suppressProfilerSampling = true;
    }
    void enableProfilerSampling() {
        suppressProfilerSampling = false;
    }

    
    bool                hadOutOfMemory;

    
    bool                haveCreatedContext;

    
    mozilla::LinkedList<js::Debugger> debuggerList;

    



    JSCList             onNewGlobalObjectWatchers;

    
    void                *data;

#ifdef XP_MACOSX
    js::AsmJSMachExceptionHandler asmJSMachExceptionHandler;
#endif

  private:
    
    
    bool signalHandlersInstalled_;
    
    
    bool canUseSignalHandlers_;
  public:
    bool signalHandlersInstalled() const {
        return signalHandlersInstalled_;
    }
    bool canUseSignalHandlers() const {
        return canUseSignalHandlers_;
    }
    void setCanUseSignalHandlers(bool enable) {
        canUseSignalHandlers_ = signalHandlersInstalled_ && enable;
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

    
    JSErrorReporter     errorReporter;

    
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
    js::UncompressedSourceCache uncompressedSourceCache;
    js::EvalCache       evalCache;
    js::LazyScriptCache lazyScriptCache;

    js::CompressedSourceSet compressedSourceSet;
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

    
    
    
    
    js::SymbolRegistry symbolRegistry_;

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

    js::SymbolRegistry &symbolRegistry() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return symbolRegistry_;
    }

    
    
    
    

    
    js::StaticStrings *staticStrings;

    
    JSAtomState *commonNames;

    
    js::AtomSet *permanentAtoms;

    bool transformToPermanentAtoms();

    
    
    js::WellKnownSymbols *wellKnownSymbols;

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
    bool                jitSupportsSimd;

    
    
    void resetJitStackLimit();

    
    js::jit::PcScriptCache *ionPcScriptCache;

    js::ThreadPool threadPool;

    js::DefaultJSContextCallback defaultJSContextCallback;

    js::CTypesActivityCallback  ctypesActivityCallback;

    
    
    uint32_t forkJoinWarmup;

  private:
    static mozilla::Atomic<size_t> liveRuntimesCount;

  public:
    static bool hasLiveRuntimes() {
        return liveRuntimesCount > 0;
    }

    explicit JSRuntime(JSRuntime *parentRuntime);
    ~JSRuntime();

    bool init(uint32_t maxbytes, uint32_t maxNurseryBytes);

    JSRuntime *thisFromCtor() { return this; }

    







    void updateMallocCounter(size_t nbytes);
    void updateMallocCounter(JS::Zone *zone, size_t nbytes);

    void reportAllocationOverflow() { js_ReportAllocationOverflow(nullptr); }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes);
    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);

    
    JS_FRIEND_API(void *) onOutOfMemoryCanGC(void *p, size_t bytes);

    
    
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

    
    bool offthreadIonCompilationEnabled_;
    bool parallelParsingEnabled_;

  public:

    
    
    void setOffthreadIonCompilationEnabled(bool value) {
        offthreadIonCompilationEnabled_ = value;
    }
    bool canUseOffthreadIonCompilation() const {
        return offthreadIonCompilationEnabled_;
    }
    void setParallelParsingEnabled(bool value) {
        parallelParsingEnabled_ = value;
    }
    bool canUseParallelParsing() const {
        return parallelParsingEnabled_;
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
    void *largeAllocationFailureCallbackData;

    
    JS::OutOfMemoryCallback oomCallback;
    void *oomCallbackData;

    




    static const unsigned LARGE_ALLOCATION = 25 * 1024 * 1024;

    template <typename T>
    T *pod_callocCanGC(size_t numElems) {
        T *p = pod_calloc<T>(numElems);
        if (MOZ_LIKELY(!!p))
            return p;
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            reportAllocationOverflow();
            return nullptr;
        }
        return (T *)onOutOfMemoryCanGC(reinterpret_cast<void *>(1), numElems * sizeof(T));
    }

    template <typename T>
    T *pod_reallocCanGC(T *p, size_t oldSize, size_t newSize) {
        T *p2 = pod_realloc<T>(p, oldSize, newSize);
        if (MOZ_LIKELY(!!p2))
            return p2;
        if (newSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            reportAllocationOverflow();
            return nullptr;
        }
        return (T *)onOutOfMemoryCanGC(p, newSize * sizeof(T));
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
    js_free(p);
}

inline void
FreeOp::freeLater(void *p)
{
    
    
    JS_ASSERT(this != runtime()->defaultFreeOp());

    if (!freeLaterList.append(p))
        CrashAtUnhandlableOOM("FreeOp::freeLater");
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
    return (runtime_ && CurrentThreadCanAccessRuntime(runtime_)) ? runtime_ : nullptr;
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
    MOZ_IMPLICIT RuntimeAllocPolicy(JSRuntime *rt) : runtime(rt) {}

    template <typename T>
    T *pod_malloc(size_t numElems) {
        return runtime->pod_malloc<T>(numElems);
    }

    template <typename T>
    T *pod_calloc(size_t numElems) {
        return runtime->pod_calloc<T>(numElems);
    }

    template <typename T>
    T *pod_realloc(T *p, size_t oldSize, size_t newSize) {
        return runtime->pod_realloc<T>(p, oldSize, newSize);
    }

    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const {}
};

extern const JSSecurityCallbacks NullSecurityCallbacks;



class AutoEnterIonCompilation
{
  public:
    explicit AutoEnterIonCompilation(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;

#ifdef DEBUG
        PerThreadData *pt = js::TlsPerThreadData.get();
        JS_ASSERT(!pt->ionCompiling);
        pt->ionCompiling = true;
#endif
    }

    ~AutoEnterIonCompilation() {
#ifdef DEBUG
        PerThreadData *pt = js::TlsPerThreadData.get();
        JS_ASSERT(pt->ionCompiling);
        pt->ionCompiling = false;
#endif
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
