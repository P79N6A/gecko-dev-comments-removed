





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
#include "jsscript.h"

#ifdef XP_MACOSX
# include "asmjs/AsmJSSignalHandlers.h"
#endif
#include "builtin/AtomicsObject.h"
#include "ds/FixedSizeHash.h"
#include "frontend/ParseMaps.h"
#include "gc/GCRuntime.h"
#include "gc/Tracer.h"
#include "irregexp/RegExpStack.h"
#include "js/HashTable.h"
#ifdef DEBUG
# include "js/Proxy.h" 
#endif
#include "js/Vector.h"
#include "vm/CommonPropertyNames.h"
#include "vm/DateTime.h"
#include "vm/MallocProvider.h"
#include "vm/SPSProfiler.h"
#include "vm/Stack.h"
#include "vm/Symbol.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif

namespace js {

class PerThreadData;
class ExclusiveContext;
class AutoKeepAtoms;
#ifdef JS_TRACE_LOGGING
class TraceLoggerThread;
#endif


extern mozilla::ThreadLocal<PerThreadData*> TlsPerThreadData;

} 

struct DtoaState;

namespace js {

extern MOZ_COLD void
ReportOutOfMemory(ExclusiveContext* cx);

extern MOZ_COLD void
ReportAllocationOverflow(ExclusiveContext* maybecx);

extern MOZ_COLD void
ReportOverRecursed(ExclusiveContext* cx);

class Activation;
class ActivationIterator;
class AsmJSActivation;
class AsmJSModule;
class MathCache;

namespace jit {
class JitRuntime;
class JitActivation;
struct PcScriptCache;
class Simulator;
struct AutoFlushICache;
class CompileRuntime;
}







struct GSNCache {
    typedef HashMap<jsbytecode*,
                    jssrcnote*,
                    PointerHasher<jsbytecode*, 0>,
                    SystemAllocPolicy> Map;

    jsbytecode*     code;
    Map             map;

    GSNCache() : code(nullptr) { }

    void purge();
};





struct ScopeCoordinateNameCache {
    typedef HashMap<uint32_t,
                    jsid,
                    DefaultHasher<uint32_t>,
                    SystemAllocPolicy> Map;

    Shape* shape;
    Map map;

    ScopeCoordinateNameCache() : shape(nullptr) {}
    void purge();
};

typedef Vector<ScriptAndCounts, 0, SystemAllocPolicy> ScriptAndCountsVector;

struct EvalCacheEntry
{
    JSLinearString* str;
    JSScript* script;
    JSScript* callerScript;
    jsbytecode* pc;
};

struct EvalCacheLookup
{
    explicit EvalCacheLookup(JSContext* cx) : str(cx), callerScript(cx) {}
    RootedLinearString str;
    RootedScript callerScript;
    JSVersion version;
    jsbytecode* pc;
};

struct EvalCacheHashPolicy
{
    typedef EvalCacheLookup Lookup;

    static HashNumber hash(const Lookup& l);
    static bool match(const EvalCacheEntry& entry, const EvalCacheLookup& l);
};

typedef HashSet<EvalCacheEntry, EvalCacheHashPolicy, SystemAllocPolicy> EvalCache;

struct LazyScriptHashPolicy
{
    struct Lookup {
        JSContext* cx;
        LazyScript* lazy;

        Lookup(JSContext* cx, LazyScript* lazy)
          : cx(cx), lazy(lazy)
        {}
    };

    static const size_t NumHashes = 3;

    static void hash(const Lookup& lookup, HashNumber hashes[NumHashes]);
    static bool match(JSScript* script, const Lookup& lookup);

    
    
    static void hash(JSScript* script, HashNumber hashes[NumHashes]);
    static bool match(JSScript* script, JSScript* lookup) { return script == lookup; }

    static void clear(JSScript** pscript) { *pscript = nullptr; }
    static bool isCleared(JSScript* script) { return !script; }
};

typedef FixedSizeHashSet<JSScript*, LazyScriptHashPolicy, 769> LazyScriptCache;

class PropertyIteratorObject;

class NativeIterCache
{
    static const size_t SIZE = size_t(1) << 8;

    
    PropertyIteratorObject* data[SIZE];

    static size_t getIndex(uint32_t key) {
        return size_t(key) % SIZE;
    }

  public:
    
    PropertyIteratorObject* last;

    NativeIterCache()
      : last(nullptr)
    {
        mozilla::PodArrayZero(data);
    }

    void purge() {
        last = nullptr;
        mozilla::PodArrayZero(data);
    }

    PropertyIteratorObject* get(uint32_t key) const {
        return data[getIndex(key)];
    }

    void set(uint32_t key, PropertyIteratorObject* iterobj) {
        data[getIndex(key)] = iterobj;
    }
};






class NewObjectCache
{
    
    static const unsigned MAX_OBJ_SIZE = 4 * sizeof(void*) + 16 * sizeof(Value);

    static void staticAsserts() {
        JS_STATIC_ASSERT(NewObjectCache::MAX_OBJ_SIZE == sizeof(JSObject_Slots16));
        JS_STATIC_ASSERT(gc::AllocKind::OBJECT_LAST == gc::AllocKind::OBJECT16_BACKGROUND);
    }

    struct Entry
    {
        
        const Class* clasp;

        












        gc::Cell* key;

        
        gc::AllocKind kind;

        
        uint32_t nbytes;

        



        char templateObject[MAX_OBJ_SIZE];
    };

    Entry entries[41];  

  public:

    typedef int EntryIndex;

    NewObjectCache() { mozilla::PodZero(this); }
    void purge() { mozilla::PodZero(this); }

    
    void clearNurseryObjects(JSRuntime* rt);

    



    inline bool lookupProto(const Class* clasp, JSObject* proto, gc::AllocKind kind, EntryIndex* pentry);
    inline bool lookupGlobal(const Class* clasp, js::GlobalObject* global, gc::AllocKind kind,
                             EntryIndex* pentry);

    bool lookupGroup(js::ObjectGroup* group, gc::AllocKind kind, EntryIndex* pentry) {
        return lookup(group->clasp(), group, kind, pentry);
    }

    




    inline NativeObject* newObjectFromHit(JSContext* cx, EntryIndex entry, js::gc::InitialHeap heap);

    
    void fillProto(EntryIndex entry, const Class* clasp, js::TaggedProto proto,
                   gc::AllocKind kind, NativeObject* obj);

    inline void fillGlobal(EntryIndex entry, const Class* clasp, js::GlobalObject* global,
                           gc::AllocKind kind, NativeObject* obj);

    void fillGroup(EntryIndex entry, js::ObjectGroup* group, gc::AllocKind kind,
                   NativeObject* obj)
    {
        MOZ_ASSERT(obj->group() == group);
        return fill(entry, group->clasp(), group, kind, obj);
    }

    
    void invalidateEntriesForShape(JSContext* cx, HandleShape shape, HandleObject proto);

  private:
    EntryIndex makeIndex(const Class* clasp, gc::Cell* key, gc::AllocKind kind) {
        uintptr_t hash = (uintptr_t(clasp) ^ uintptr_t(key)) + size_t(kind);
        return hash % mozilla::ArrayLength(entries);
    }

    bool lookup(const Class* clasp, gc::Cell* key, gc::AllocKind kind, EntryIndex* pentry) {
        *pentry = makeIndex(clasp, key, kind);
        Entry* entry = &entries[*pentry];

        
        return entry->clasp == clasp && entry->key == key;
    }

    void fill(EntryIndex entry_, const Class* clasp, gc::Cell* key, gc::AllocKind kind,
              NativeObject* obj) {
        MOZ_ASSERT(unsigned(entry_) < mozilla::ArrayLength(entries));
        MOZ_ASSERT(entry_ == makeIndex(clasp, key, kind));
        Entry* entry = &entries[entry_];

        entry->clasp = clasp;
        entry->key = key;
        entry->kind = kind;

        entry->nbytes = gc::Arena::thingSize(kind);
        js_memcpy(&entry->templateObject, obj, entry->nbytes);
    }

    static void copyCachedToObject(NativeObject* dst, NativeObject* src, gc::AllocKind kind) {
        js_memcpy(dst, src, gc::Arena::thingSize(kind));
        Shape::writeBarrierPost(dst->shape_, &dst->shape_);
        ObjectGroup::writeBarrierPost(dst->group_, &dst->group_);
    }
};








class FreeOp : public JSFreeOp
{
    Vector<void*, 0, SystemAllocPolicy> freeLaterList;
    ThreadType threadType;

  public:
    static FreeOp* get(JSFreeOp* fop) {
        return static_cast<FreeOp*>(fop);
    }

    explicit FreeOp(JSRuntime* rt, ThreadType thread = MainThread)
      : JSFreeOp(rt), threadType(thread)
    {}

    ~FreeOp() {
        for (size_t i = 0; i < freeLaterList.length(); i++)
            free_(freeLaterList[i]);
    }

    bool onBackgroundThread() {
        return threadType == BackgroundThread;
    }

    inline void free_(void* p);
    inline void freeLater(void* p);

    template <class T>
    inline void delete_(T* p) {
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

    js::ImmutablePropertyNamePtr* wellKnownSymbolDescriptions() {
        return &Symbol_iterator;
    }
};

namespace js {










struct WellKnownSymbols
{
    js::ImmutableSymbolPtr iterator;
    js::ImmutableSymbolPtr match;

    const ImmutableSymbolPtr& get(size_t u) const {
        MOZ_ASSERT(u < JS::WellKnownSymbolLimit);
        const ImmutableSymbolPtr* symbols = reinterpret_cast<const ImmutableSymbolPtr*>(this);
        return symbols[u];
    }

    const ImmutableSymbolPtr& get(JS::SymbolCode code) const {
        return get(size_t(code));
    }
};

#define NAME_OFFSET(name)       offsetof(JSAtomState, name)

inline HandlePropertyName
AtomStateOffsetToName(const JSAtomState& atomState, size_t offset)
{
    return *reinterpret_cast<js::ImmutablePropertyNamePtr*>((char*)&atomState + offset);
}




enum RuntimeLock {
    ExclusiveAccessLock,
    HelperThreadStateLock,
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
#ifdef DEBUG
    
    friend void js::AssertCurrentThreadCanLock(RuntimeLock which);
#endif

    






    JSRuntime* runtime_;

  public:
#ifdef JS_TRACE_LOGGING
    TraceLoggerThread*  traceLogger;
#endif

    
    js::jit::AutoFlushICache* autoFlushICache_;

  public:
    
    DtoaState*          dtoaState;

    







    int32_t suppressGC;

#ifdef DEBUG
    
    bool ionCompiling;

    
    bool gcSweeping;
#endif

    
    unsigned activeCompilations;

    explicit PerThreadData(JSRuntime* runtime);
    ~PerThreadData();

    bool init();

    bool associatedWith(const JSRuntime* rt) { return runtime_ == rt; }
    inline JSRuntime* runtimeFromMainThread();
    inline JSRuntime* runtimeIfOnOwnerThread();

    inline bool exclusiveThreadsPresent();
    inline void addActiveCompilation();
    inline void removeActiveCompilation();

    
    
    class MOZ_STACK_CLASS AutoEnterRuntime
    {
        PerThreadData* pt;

      public:
        AutoEnterRuntime(PerThreadData* pt, JSRuntime* rt)
          : pt(pt)
        {
            MOZ_ASSERT(!pt->runtime_);
            pt->runtime_ = rt;
        }

        ~AutoEnterRuntime() {
            pt->runtime_ = nullptr;
        }
    };

    js::jit::AutoFlushICache* autoFlushICache() const;
    void setAutoFlushICache(js::jit::AutoFlushICache* afc);

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::Simulator* simulator() const;
#endif
};

class AutoLockForExclusiveAccess;
} 

struct JSRuntime : public JS::shadow::Runtime,
                   public js::MallocProvider<JSRuntime>
{
    









    js::PerThreadData mainThread;

    



    uint8_t*            jitTop;

    




    JSContext*          jitJSContext;

     



    js::jit::JitActivation* jitActivation;

    
  private:
    mozilla::Atomic<uintptr_t, mozilla::Relaxed> jitStackLimit_;
    void resetJitStackLimit();

  public:
    void initJitStackLimit();

    uintptr_t jitStackLimit() const { return jitStackLimit_; }

    
    void* addressOfJitStackLimit() { return &jitStackLimit_; }
    static size_t offsetOfJitStackLimit() { return offsetof(JSRuntime, jitStackLimit_); }

    
    js::irregexp::RegExpStack regexpStack;

  private:
    friend class js::Activation;
    friend class js::ActivationIterator;
    friend class js::jit::JitActivation;
    friend class js::AsmJSActivation;
    friend class js::jit::CompileRuntime;
#ifdef DEBUG
    friend void js::AssertCurrentThreadCanLock(js::RuntimeLock which);
#endif

    



    js::Activation* activation_;

    



    js::Activation * volatile profilingActivation_;

    











    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> profilerSampleBufferGen_;
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> profilerSampleBufferLapCount_;

    
    js::AsmJSActivation * volatile asmJSActivationStack_;

  public:
    








    js::SavedFrame* asyncStackForNewActivations;

    


    JSString* asyncCauseForNewActivations;

    js::Activation* const* addressOfActivation() const {
        return &activation_;
    }
    static unsigned offsetOfActivation() {
        return offsetof(JSRuntime, activation_);
    }

    js::Activation* profilingActivation() const {
        return profilingActivation_;
    }
    void* addressOfProfilingActivation() {
        return (void*) &profilingActivation_;
    }
    static unsigned offsetOfProfilingActivation() {
        return offsetof(JSRuntime, profilingActivation_);
    }

    uint32_t profilerSampleBufferGen() {
        return profilerSampleBufferGen_;
    }
    void resetProfilerSampleBufferGen() {
        profilerSampleBufferGen_ = 0;
    }
    void setProfilerSampleBufferGen(uint32_t gen) {
        
        
        for (;;) {
            uint32_t curGen = profilerSampleBufferGen_;
            if (curGen >= gen)
                break;

            if (profilerSampleBufferGen_.compareExchange(curGen, gen))
                break;
        }
    }

    uint32_t profilerSampleBufferLapCount() {
        MOZ_ASSERT(profilerSampleBufferLapCount_ > 0);
        return profilerSampleBufferLapCount_;
    }
    void resetProfilerSampleBufferLapCount() {
        profilerSampleBufferLapCount_ = 1;
    }
    void updateProfilerSampleBufferLapCount(uint32_t lapCount) {
        MOZ_ASSERT(profilerSampleBufferLapCount_ > 0);

        
        
        for (;;) {
            uint32_t curLapCount = profilerSampleBufferLapCount_;
            if (curLapCount >= lapCount)
                break;

            if (profilerSampleBufferLapCount_.compareExchange(curLapCount, lapCount))
                break;
        }
    }

    js::AsmJSActivation* asmJSActivationStack() const {
        return asmJSActivationStack_;
    }
    static js::AsmJSActivation* innermostAsmJSActivation() {
        js::PerThreadData* ptd = js::TlsPerThreadData.get();
        return ptd ? ptd->runtimeFromMainThread()->asmJSActivationStack_ : nullptr;
    }

    js::Activation* activation() const {
        return activation_;
    }

    



    JSRuntime* parentRuntime;

  private:
    mozilla::Atomic<uint32_t, mozilla::Relaxed> interrupt_;

    
    JSAccumulateTelemetryDataCallback telemetryCallback;
  public:
    
    
    
    void addTelemetry(int id, uint32_t sample, const char* key = nullptr);

    void setTelemetryCallback(JSRuntime* rt, JSAccumulateTelemetryDataCallback callback);

    enum InterruptMode {
        RequestInterruptUrgent,
        RequestInterruptCanWait
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void requestInterrupt(InterruptMode mode);
    bool handleInterrupt(JSContext* cx);

    MOZ_ALWAYS_INLINE bool hasPendingInterrupt() const {
        return interrupt_;
    }

    
    void* addressOfInterruptUint32() {
        static_assert(sizeof(interrupt_) == sizeof(uint32_t), "Assumed by JIT callers");
        return &interrupt_;
    }

    
    bool handlingSignal;

    JSInterruptCallback interruptCallback;

#ifdef DEBUG
    void assertCanLock(js::RuntimeLock which);
#else
    void assertCanLock(js::RuntimeLock which) {}
#endif

  private:
    







    PRLock* exclusiveAccessLock;
    mozilla::DebugOnly<PRThread*> exclusiveAccessOwner;
    mozilla::DebugOnly<bool> mainThreadHasExclusiveAccess;

    
    size_t numExclusiveThreads;

    friend class js::AutoLockForExclusiveAccess;

  public:
    void setUsedByExclusiveThread(JS::Zone* zone);
    void clearUsedByExclusiveThread(JS::Zone* zone);

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

    
    const JSLocaleCallbacks* localeCallbacks;

    
    char* defaultLocale;

    
    JSVersion defaultVersion_;

    
    js::FutexRuntime fx;

  private:
    
    void* ownerThread_;
    size_t ownerThreadNative_;
    friend bool js::CurrentThreadCanAccessRuntime(JSRuntime* rt);
  public:

    size_t ownerThreadNative() const {
        return ownerThreadNative_;
    }

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 4 * 1024;
    js::LifoAlloc tempLifoAlloc;

  private:
    js::jit::JitRuntime* jitRuntime_;

    



    js::NativeObject* selfHostingGlobal_;

    static js::GlobalObject*
    createSelfHostingGlobal(JSContext* cx);

    
    js::InterpreterStack interpreterStack_;

    js::jit::JitRuntime* createJitRuntime(JSContext* cx);

  public:
    js::jit::JitRuntime* getJitRuntime(JSContext* cx) {
        return jitRuntime_ ? jitRuntime_ : createJitRuntime(cx);
    }
    js::jit::JitRuntime* jitRuntime() const {
        return jitRuntime_;
    }
    bool hasJitRuntime() const {
        return !!jitRuntime_;
    }
    js::InterpreterStack& interpreterStack() {
        return interpreterStack_;
    }

    
    
    

    bool initSelfHosting(JSContext* cx);
    void finishSelfHosting();
    void markSelfHostingGlobal(JSTracer* trc);
    bool isSelfHostingGlobal(JSObject* global) {
        return global == selfHostingGlobal_;
    }
    bool isSelfHostingCompartment(JSCompartment* comp);
    bool isSelfHostingZone(JS::Zone* zone);
    bool cloneSelfHostedFunctionScript(JSContext* cx, js::Handle<js::PropertyName*> name,
                                       js::Handle<JSFunction*> targetFun);
    bool cloneSelfHostedValue(JSContext* cx, js::Handle<js::PropertyName*> name,
                              js::MutableHandleValue vp);

    
    
    

    






    bool setDefaultLocale(const char* locale);

    
    void resetDefaultLocale();

    
    const char* getDefaultLocale();

    JSVersion defaultVersion() { return defaultVersion_; }
    void setDefaultVersion(JSVersion v) { defaultVersion_ = v; }

    
    const uintptr_t     nativeStackBase;

    
    size_t              nativeStackQuota[js::StackKindCount];

    
    JSContextCallback   cxCallback;
    void*              cxCallbackData;

    
    JSDestroyCompartmentCallback destroyCompartmentCallback;

    
    JSZoneCallback destroyZoneCallback;

    
    JSZoneCallback sweepZoneCallback;

    
    JSCompartmentNameCallback compartmentNameCallback;

    js::ActivityCallback  activityCallback;
    void*                activityCallbackArg;
    void triggerActivityCallback(bool active);

    
    unsigned            requestDepth;

#ifdef DEBUG
    unsigned            checkRequestDepth;

    





    JSContext*         activeContext;
#endif

    
    js::gc::GCRuntime   gc;

    
    bool                gcInitialized;

    bool isHeapBusy() { return gc.isHeapBusy(); }
    bool isHeapMajorCollecting() { return gc.isHeapMajorCollecting(); }
    bool isHeapMinorCollecting() { return gc.isHeapMinorCollecting(); }
    bool isHeapCollecting() { return gc.isHeapCollecting(); }
    bool isHeapCompacting() { return gc.isHeapCompacting(); }

    int gcZeal() { return gc.zeal(); }

    void lockGC() {
        assertCanLock(js::GCLock);
        gc.lockGC();
    }

    void unlockGC() {
        gc.unlockGC();
    }

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::Simulator* simulator_;
#endif

  public:
    void setNeedsIncrementalBarrier(bool needs) {
        needsIncrementalBarrier_ = needs;
    }

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::Simulator* simulator() const;
    uintptr_t* addressOfSimulatorStackLimit();
#endif

    
    js::ScriptAndCountsVector* scriptAndCountsVector;

    
    const js::Value     NaNValue;
    const js::Value     negativeInfinityValue;
    const js::Value     positiveInfinityValue;

    js::PropertyName*   emptyString;

    
    mozilla::LinkedList<JSContext> contextList;

    bool hasContexts() const {
        return !contextList.isEmpty();
    }

    mozilla::UniquePtr<js::SourceHook> sourceHook;

    
    js::SPSProfiler     spsProfiler;

    
    bool                profilingScripts;

    
  private:
    mozilla::Atomic<bool, mozilla::SequentiallyConsistent> suppressProfilerSampling;

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

    



    bool                allowRelazificationForTesting;

    
    mozilla::LinkedList<js::Debugger> debuggerList;

    



    JSCList             onNewGlobalObjectWatchers;

    
    void*               data;

#if defined(XP_MACOSX) && defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
    js::AsmJSMachExceptionHandler asmJSMachExceptionHandler;
#endif

  private:
    
    
    bool signalHandlersInstalled_;

    
    
    bool canUseSignalHandlers_;

  public:
    bool canUseSignalHandlers() const {
        return canUseSignalHandlers_;
    }
    void setCanUseSignalHandlers(bool enable) {
        canUseSignalHandlers_ = signalHandlersInstalled_ && enable;
    }

  private:
    js::FreeOp          defaultFreeOp_;

  public:
    js::FreeOp* defaultFreeOp() {
        return &defaultFreeOp_;
    }

    uint32_t            debuggerMutations;

    const JSSecurityCallbacks* securityCallbacks;
    const js::DOMCallbacks* DOMcallbacks;
    JSDestroyPrincipalsOp destroyPrincipals;

    
    const JSStructuredCloneCallbacks* structuredCloneCallbacks;

    
    JSErrorReporter     errorReporter;

    
    JS::AsmJSCacheOps   asmJSCacheOps;

    
    js::AsmJSModule*   linkedAsmJSModules;

    




    uint32_t            propertyRemovals;

#if !EXPOSE_INTL_API
    
    const char*         thousandsSeparator;
    const char*         decimalSeparator;
    const char*         numGrouping;
#endif

  private:
    js::MathCache* mathCache_;
    js::MathCache* createMathCache(JSContext* cx);
  public:
    js::MathCache* getMathCache(JSContext* cx) {
        return mathCache_ ? mathCache_ : createMathCache(cx);
    }
    js::MathCache* maybeGetMathCache() {
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
    js::frontend::ParseMapPool& parseMapPool() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        return parseMapPool_;
    }
    bool hasActiveCompilations() {
        return activeCompilations_ != 0;
    }
    void addActiveCompilation() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        activeCompilations_++;
    }
    void removeActiveCompilation() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        activeCompilations_--;
    }

    
    
    
    
    
    
    
    
    
  private:
    unsigned keepAtoms_;
    friend class js::AutoKeepAtoms;
  public:
    bool keepAtoms() {
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(this));
        return keepAtoms_ != 0 || exclusiveThreadsPresent();
    }

  private:
    const JSPrincipals* trustedPrincipals_;
  public:
    void setTrustedPrincipals(const JSPrincipals* p) { trustedPrincipals_ = p; }
    const JSPrincipals* trustedPrincipals() const { return trustedPrincipals_; }

  private:
    bool beingDestroyed_;
  public:
    bool isBeingDestroyed() const {
        return beingDestroyed_;
    }

  private:
    
    
    
    js::AtomSet* atoms_;

    
    
    
    
    JSCompartment* atomsCompartment_;

    
    
    
    
    js::SymbolRegistry symbolRegistry_;

  public:
    bool initializeAtoms(JSContext* cx);
    void finishAtoms();

    void sweepAtoms();

    js::AtomSet& atoms() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        return *atoms_;
    }
    JSCompartment* atomsCompartment() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        return atomsCompartment_;
    }

    bool isAtomsCompartment(JSCompartment* comp) {
        return comp == atomsCompartment_;
    }

    
    inline bool isAtomsZone(JS::Zone* zone);

    bool activeGCInAtomsZone();

    js::SymbolRegistry& symbolRegistry() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        return symbolRegistry_;
    }

    
    
    
    

    
    js::StaticStrings* staticStrings;

    
    JSAtomState* commonNames;

    
    
    
    js::FrozenAtomSet* permanentAtoms;

    bool transformToPermanentAtoms(JSContext* cx);

    
    
    js::WellKnownSymbols* wellKnownSymbols;

    const JSWrapObjectCallbacks*           wrapObjectCallbacks;
    js::PreserveWrapperCallback            preserveWrapperCallback;

    
    
    
  private:
    js::ScriptDataTable scriptDataTable_;
  public:
    js::ScriptDataTable& scriptDataTable() {
        MOZ_ASSERT(currentThreadHasExclusiveAccess());
        return scriptDataTable_;
    }

    bool                jitSupportsFloatingPoint;
    bool                jitSupportsSimd;

    
    js::jit::PcScriptCache* ionPcScriptCache;

    js::DefaultJSContextCallback defaultJSContextCallback;

    js::CTypesActivityCallback  ctypesActivityCallback;

  private:
    static mozilla::Atomic<size_t> liveRuntimesCount;

  public:
    static bool hasLiveRuntimes() {
        return liveRuntimesCount > 0;
    }

    explicit JSRuntime(JSRuntime* parentRuntime);
    ~JSRuntime();

    bool init(uint32_t maxbytes, uint32_t maxNurseryBytes);

    JSRuntime* thisFromCtor() { return this; }

    







    void updateMallocCounter(size_t nbytes);
    void updateMallocCounter(JS::Zone* zone, size_t nbytes);

    void reportAllocationOverflow() { js::ReportAllocationOverflow(nullptr); }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    






    JS_FRIEND_API(void*) onOutOfMemory(js::AllocFunction allocator, size_t nbytes,
                                       void* reallocPtr = nullptr, JSContext* maybecx = nullptr);

    
    JS_FRIEND_API(void*) onOutOfMemoryCanGC(js::AllocFunction allocator, size_t nbytes,
                                            void* reallocPtr = nullptr);

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::RuntimeSizes* runtime);

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

    const JS::RuntimeOptions& options() const {
        return options_;
    }
    JS::RuntimeOptions& options() {
        return options_;
    }

#ifdef DEBUG
  public:
    js::AutoEnterPolicy* enteredPolicy;
#endif

    
    JS::LargeAllocationFailureCallback largeAllocationFailureCallback;
    void* largeAllocationFailureCallbackData;

    
    JS::OutOfMemoryCallback oomCallback;
    void* oomCallbackData;

    




    static const unsigned LARGE_ALLOCATION = 25 * 1024 * 1024;

    template <typename T>
    T* pod_callocCanGC(size_t numElems) {
        T* p = pod_calloc<T>(numElems);
        if (MOZ_LIKELY(!!p))
            return p;
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            reportAllocationOverflow();
            return nullptr;
        }
        return (T*)onOutOfMemoryCanGC(js::AllocFunction::Calloc, numElems * sizeof(T));
    }

    template <typename T>
    T* pod_reallocCanGC(T* p, size_t oldSize, size_t newSize) {
        T* p2 = pod_realloc<T>(p, oldSize, newSize);
        if (MOZ_LIKELY(!!p2))
            return p2;
        if (newSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            reportAllocationOverflow();
            return nullptr;
        }
        return (T*)onOutOfMemoryCanGC(js::AllocFunction::Realloc, newSize * sizeof(T), p);
    }

    



    mozilla::MallocSizeOf debuggerMallocSizeOf;

    
    int64_t lastAnimationTime;

  public:

    


    struct Stopwatch {
        








        uint64_t iteration;

        



        bool isEmpty;

        


        js::PerformanceData performance;

        Stopwatch()
          : iteration(0)
          , isEmpty(true)
          , isActive_(false)
        { }

        






        void reset() {
            ++iteration;
            isEmpty = true;
        }

        










        bool setIsActive(bool value) {
            if (isActive_ != value)
                reset();

            if (value && !groups_.initialized()) {
                if (!groups_.init(128))
                    return false;
            }

            isActive_ = value;
            return true;
        }

        


        bool isActive() const {
            return isActive_;
        }

        
        
        
        
        
        struct MonotonicTimeStamp {
            MonotonicTimeStamp()
              : latestGood_(0)
            {}
            inline uint64_t monotonize(uint64_t stamp)
            {
                if (stamp <= latestGood_)
                    return latestGood_;
                latestGood_ = stamp;
                return stamp;
            }
          private:
            uint64_t latestGood_;
        };
        MonotonicTimeStamp systemTimeFix;
        MonotonicTimeStamp userTimeFix;

    private:
        














        typedef js::HashMap<void*, js::PerformanceGroup*,
                            js::DefaultHasher<void*>,
                            js::SystemAllocPolicy> Groups;

        Groups groups_;
        friend struct js::PerformanceGroupHolder;

        


        bool isActive_;
    };
    Stopwatch stopwatch;
};

namespace js {






static inline JSContext*
GetJSContextFromJitCode()
{
    JSContext* cx = js::TlsPerThreadData.get()->runtimeFromMainThread()->jitJSContext;
    MOZ_ASSERT(cx);
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
VersionCopyFlags(JSVersion* version, JSVersion from)
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
FreeOp::free_(void* p)
{
    js_free(p);
}

inline void
FreeOp::freeLater(void* p)
{
    
    
    MOZ_ASSERT(this != runtime()->defaultFreeOp());

    if (!freeLaterList.append(p))
        CrashAtUnhandlableOOM("FreeOp::freeLater");
}







class MOZ_STACK_CLASS AutoLockGC
{
  public:
    explicit AutoLockGC(JSRuntime* rt
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : runtime_(rt), wasUnlocked_(false)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        lock();
    }

    ~AutoLockGC() {
        unlock();
    }

    void lock() {
        runtime_->lockGC();
    }

    void unlock() {
        runtime_->unlockGC();
        wasUnlocked_ = true;
    }

#ifdef DEBUG
    bool wasUnlocked() {
        return wasUnlocked_;
    }
#endif

  private:
    JSRuntime* runtime_;
    mozilla::DebugOnly<bool> wasUnlocked_;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoLockGC(const AutoLockGC&) = delete;
    AutoLockGC& operator=(const AutoLockGC&) = delete;
};

class MOZ_STACK_CLASS AutoUnlockGC
{
  public:
    explicit AutoUnlockGC(AutoLockGC& lock
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : lock(lock)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        lock.unlock();
    }

    ~AutoUnlockGC() {
        lock.lock();
    }

  private:
    AutoLockGC& lock;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoUnlockGC(const AutoUnlockGC&) = delete;
    AutoUnlockGC& operator=(const AutoUnlockGC&) = delete;
};

class MOZ_STACK_CLASS AutoKeepAtoms
{
    PerThreadData* pt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoKeepAtoms(PerThreadData* pt
                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : pt(pt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (JSRuntime* rt = pt->runtimeIfOnOwnerThread()) {
            rt->keepAtoms_++;
        } else {
            
            
            MOZ_ASSERT(pt->exclusiveThreadsPresent());
        }
    }
    ~AutoKeepAtoms() {
        if (JSRuntime* rt = pt->runtimeIfOnOwnerThread()) {
            MOZ_ASSERT(rt->keepAtoms_);
            rt->keepAtoms_--;
            if (rt->gc.fullGCForAtomsRequested() && !rt->keepAtoms())
                rt->gc.triggerFullGCForAtoms();
        }
    }
};

inline JSRuntime*
PerThreadData::runtimeFromMainThread()
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
    return runtime_;
}

inline JSRuntime*
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
    MOZ_ASSERT(activeCompilations);
    activeCompilations--;
    runtime_->removeActiveCompilation();
}



static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Value* vec, size_t len)
{
    mozilla::PodZero(vec, len);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Value* beg, Value* end)
{
    mozilla::PodZero(beg, end - beg);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(jsid* beg, jsid* end)
{
    for (jsid* id = beg; id != end; ++id)
        *id = INT_TO_JSID(0);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(jsid* vec, size_t len)
{
    MakeRangeGCSafe(vec, vec + len);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Shape** beg, Shape** end)
{
    mozilla::PodZero(beg, end - beg);
}

static MOZ_ALWAYS_INLINE void
MakeRangeGCSafe(Shape** vec, size_t len)
{
    mozilla::PodZero(vec, len);
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToUndefined(Value* beg, Value* end)
{
    for (Value* v = beg; v != end; ++v)
        v->setUndefined();
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToUndefined(Value* vec, size_t len)
{
    SetValueRangeToUndefined(vec, vec + len);
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToNull(Value* beg, Value* end)
{
    for (Value* v = beg; v != end; ++v)
        v->setNull();
}

static MOZ_ALWAYS_INLINE void
SetValueRangeToNull(Value* vec, size_t len)
{
    SetValueRangeToNull(vec, vec + len);
}












class RuntimeAllocPolicy
{
    JSRuntime* const runtime;

  public:
    MOZ_IMPLICIT RuntimeAllocPolicy(JSRuntime* rt) : runtime(rt) {}

    template <typename T>
    T* pod_malloc(size_t numElems) {
        return runtime->pod_malloc<T>(numElems);
    }

    template <typename T>
    T* pod_calloc(size_t numElems) {
        return runtime->pod_calloc<T>(numElems);
    }

    template <typename T>
    T* pod_realloc(T* p, size_t oldSize, size_t newSize) {
        return runtime->pod_realloc<T>(p, oldSize, newSize);
    }

    void free_(void* p) { js_free(p); }
    void reportAllocOverflow() const {}
};

extern const JSSecurityCallbacks NullSecurityCallbacks;



class AutoEnterIonCompilation
{
  public:
    explicit AutoEnterIonCompilation(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;

#ifdef DEBUG
        PerThreadData* pt = js::TlsPerThreadData.get();
        MOZ_ASSERT(!pt->ionCompiling);
        pt->ionCompiling = true;
#endif
    }

    ~AutoEnterIonCompilation() {
#ifdef DEBUG
        PerThreadData* pt = js::TlsPerThreadData.get();
        MOZ_ASSERT(pt->ionCompiling);
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
