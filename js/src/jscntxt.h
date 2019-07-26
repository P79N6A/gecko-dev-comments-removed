








#ifndef jscntxt_h___
#define jscntxt_h___

#include "mozilla/Attributes.h"
#include "mozilla/LinkedList.h"

#include <string.h>

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"
#include "jsatom.h"
#include "jsclist.h"
#include "jsgc.h"
#include "jspropertycache.h"
#include "jspropertytree.h"
#include "jsprototypes.h"
#include "jsutil.h"
#include "prmjtime.h"
#include "vm/threadpool.h"

#include "ds/LifoAlloc.h"
#include "gc/Statistics.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/Stack.h"
#include "vm/SPSProfiler.h"

#include "ion/PcScriptCache.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif

struct DtoaState;

extern void
js_ReportOutOfMemory(JSContext *cx);

extern void
js_ReportAllocationOverflow(JSContext *cx);

namespace js {

typedef HashSet<JSObject *> ObjectSet;


class AutoCycleDetector
{
    JSContext *cx;
    JSObject *obj;
    bool cyclic;
    uint32_t hashsetGenerationAtInit;
    ObjectSet::AddPtr hashsetAddPointer;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoCycleDetector(JSContext *cx, JSObject *obj
                      JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), obj(obj), cyclic(true)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoCycleDetector();

    bool init();

    bool foundCycle() { return cyclic; }
};


extern void
TraceCycleDetectionSet(JSTracer *trc, ObjectSet &set);

namespace mjit {
class JaegerRuntime;
}

class MathCache;

namespace ion {
class IonRuntime;
class IonActivation;
}

class WeakMapBase;
class InterpreterFrames;
class DebugScopes;
class WorkerThreadState;







struct GSNCache {
    typedef HashMap<jsbytecode *,
                    jssrcnote *,
                    PointerHasher<jsbytecode *, 0>,
                    SystemAllocPolicy> Map;

    jsbytecode      *code;
    Map             map;

    GSNCache() : code(NULL) { }

    void purge();
};

typedef Vector<ScriptAndCounts, 0, SystemAllocPolicy> ScriptAndCountsVector;

struct ConservativeGCData
{
    



    uintptr_t           *nativeStackTop;

#if defined(JSGC_ROOT_ANALYSIS) && (JS_STACK_GROWTH_DIRECTION < 0)
    





    uintptr_t           *oldStackMin, *oldStackEnd;
    uintptr_t           *oldStackData;
    size_t              oldStackCapacity; 
#endif

    union {
        jmp_buf         jmpbuf;
        uintptr_t       words[JS_HOWMANY(sizeof(jmp_buf), sizeof(uintptr_t))];
    } registerSnapshot;

    ConservativeGCData() {
        PodZero(this);
    }

    ~ConservativeGCData() {
#ifdef JS_THREADSAFE
        



        JS_ASSERT(!hasStackToScan());
#endif
    }

    JS_NEVER_INLINE void recordStackTop();

#ifdef JS_THREADSAFE
    void updateForRequestEnd() {
        nativeStackTop = NULL;
    }
#endif

    bool hasStackToScan() const {
        return !!nativeStackTop;
    }
};

class SourceDataCache
{
    typedef HashMap<ScriptSource *,
                    JSStableString *,
                    DefaultHasher<ScriptSource *>,
                    SystemAllocPolicy> Map;
    Map *map_;

  public:
    SourceDataCache() : map_(NULL) {}
    JSStableString *lookup(ScriptSource *ss);
    void put(ScriptSource *ss, JSStableString *);
    void purge();
};

struct EvalCacheLookup
{
    JSLinearString *str;
    JSFunction *caller;
    unsigned staticLevel;
    JSVersion version;
    JSCompartment *compartment;
};

struct EvalCacheHashPolicy
{
    typedef EvalCacheLookup Lookup;

    static HashNumber hash(const Lookup &l);
    static bool match(JSScript *script, const EvalCacheLookup &l);
};

typedef HashSet<JSScript *, EvalCacheHashPolicy, SystemAllocPolicy> EvalCache;

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
      : last(NULL) {
        PodArrayZero(data);
    }

    void purge() {
        last = NULL;
        PodArrayZero(data);
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
    static inline void staticAsserts();

    struct Entry
    {
        
        Class *clasp;

        












        gc::Cell *key;

        
        gc::AllocKind kind;

        
        uint32_t nbytes;

        



        char templateObject[MAX_OBJ_SIZE];
    };

    Entry entries[41];  

  public:

    typedef int EntryIndex;

    NewObjectCache() { PodZero(this); }
    void purge() { PodZero(this); }

    



    inline bool lookupProto(Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupGlobal(Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupType(Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, EntryIndex *pentry);

    




    inline JSObject *newObjectFromHit(JSContext *cx, EntryIndex entry);

    
    inline void fillProto(EntryIndex entry, Class *clasp, js::TaggedProto proto, gc::AllocKind kind, JSObject *obj);
    inline void fillGlobal(EntryIndex entry, Class *clasp, js::GlobalObject *global, gc::AllocKind kind, JSObject *obj);
    inline void fillType(EntryIndex entry, Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, JSObject *obj);

    
    void invalidateEntriesForShape(JSContext *cx, Shape *shape, JSObject *proto);

  private:
    inline bool lookup(Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry);
    inline void fill(EntryIndex entry, Class *clasp, gc::Cell *key, gc::AllocKind kind, JSObject *obj);
    static inline void copyCachedToObject(JSObject *dst, JSObject *src);
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

    inline void free_(void* p);

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
#define PROPERTYNAME_FIELD(idpart, id, text) js::FixedHeapPtr<js::PropertyName> id;
    FOR_EACH_COMMON_PROPERTYNAME(PROPERTYNAME_FIELD)
#undef PROPERTYNAME_FIELD
#define PROPERTYNAME_FIELD(name, code, init) js::FixedHeapPtr<js::PropertyName> name;
    JS_FOR_EACH_PROTOTYPE(PROPERTYNAME_FIELD)
#undef PROPERTYNAME_FIELD
};

#define NAME_OFFSET(name)       offsetof(JSAtomState, name)
#define OFFSET_TO_NAME(rt,off)  (*(js::FixedHeapPtr<js::PropertyName>*)((char*)&(rt)->atomState + (off)))

namespace js {












class PerThreadData : public js::PerThreadDataFriendFields
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

    bool                gcRelaxRootChecks;
    int                 gcAssertNoGCDepth;
#endif

    PerThreadData(JSRuntime *runtime);

    bool associatedWith(const JSRuntime *rt) { return runtime_ == rt; }
};

} 

struct JSRuntime : js::RuntimeFriendFields
{
    








    js::PerThreadData mainThread;

    
    JSCompartment       *atomsCompartment;

    
    js::CompartmentVector compartments;

    
#ifdef JS_THREADSAFE
  public:
    void *ownerThread() const { return ownerThread_; }
    void clearOwnerThread();
    void setOwnerThread();
    JS_FRIEND_API(void) abortIfWrongThread() const;
    JS_FRIEND_API(void) assertValidThread() const;
  private:
    void                *ownerThread_;
  public:
#else
  public:
    void abortIfWrongThread() const {}
    void assertValidThread() const {}
#endif

    
    js::StackSpace stackSpace;

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 4 * 1024;
    js::LifoAlloc tempLifoAlloc;

    



    js::LifoAlloc freeLifoAlloc;

  private:
    



    JSC::ExecutableAllocator *execAlloc_;
    WTF::BumpPointerAllocator *bumpAlloc_;
#ifdef JS_METHODJIT
    js::mjit::JaegerRuntime *jaegerRuntime_;
#endif
    js::ion::IonRuntime *ionRuntime_;

    JSObject *selfHostedGlobal_;

    JSC::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
    WTF::BumpPointerAllocator *createBumpPointerAllocator(JSContext *cx);
    js::mjit::JaegerRuntime *createJaegerRuntime(JSContext *cx);
    js::ion::IonRuntime *createIonRuntime(JSContext *cx);

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
    WTF::BumpPointerAllocator *getBumpPointerAllocator(JSContext *cx) {
        return bumpAlloc_ ? bumpAlloc_ : createBumpPointerAllocator(cx);
    }
#ifdef JS_METHODJIT
    js::mjit::JaegerRuntime *getJaegerRuntime(JSContext *cx) {
        return jaegerRuntime_ ? jaegerRuntime_ : createJaegerRuntime(cx);
    }
    bool hasJaegerRuntime() const {
        return jaegerRuntime_;
    }
    js::mjit::JaegerRuntime &jaegerRuntime() {
        JS_ASSERT(hasJaegerRuntime());
        return *jaegerRuntime_;
    }
#endif
    js::ion::IonRuntime *getIonRuntime(JSContext *cx) {
        return ionRuntime_ ? ionRuntime_ : createIonRuntime(cx);
    }

    bool initSelfHosting(JSContext *cx);
    void markSelfHostedGlobal(JSTracer *trc);
    bool isSelfHostedGlobal(js::HandleObject global) {
        return global == selfHostedGlobal_;
    }
    bool getUnclonedSelfHostedValue(JSContext *cx, js::Handle<js::PropertyName*> name,
                                    js::MutableHandleValue vp);
    bool cloneSelfHostedFunctionScript(JSContext *cx, js::Handle<js::PropertyName*> name,
                                       js::Handle<JSFunction*> targetFun);
    bool cloneSelfHostedValue(JSContext *cx, js::Handle<js::PropertyName*> name,
                              js::HandleObject holder, js::MutableHandleValue vp);

    
    uintptr_t           nativeStackBase;

    
    size_t              nativeStackQuota;

    



    js::InterpreterFrames *interpreterFrames;

    
    JSContextCallback   cxCallback;

    
    JSDestroyCompartmentCallback destroyCompartmentCallback;

    
    JSCompartmentNameCallback compartmentNameCallback;

    js::ActivityCallback  activityCallback;
    void                 *activityCallbackArg;

#ifdef JS_THREADSAFE
    
    unsigned            requestDepth;

# ifdef DEBUG
    unsigned            checkRequestDepth;
# endif
#endif

    

    




    js::GCChunkSet      gcChunkSet;

    






    js::gc::Chunk       *gcSystemAvailableChunkListHead;
    js::gc::Chunk       *gcUserAvailableChunkListHead;
    js::gc::ChunkPool   gcChunkPool;

    js::RootedValueMap  gcRootsHash;
    js::GCLocks         gcLocksHash;
    unsigned            gcKeepAtoms;
    size_t              gcBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;

    




    volatile uint32_t   gcNumArenasFreeCommitted;
    js::GCMarker        gcMarker;
    void                *gcVerifyPreData;
    void                *gcVerifyPostData;
    bool                gcChunkAllocationSinceLastGC;
    int64_t             gcNextFullGCTime;
    int64_t             gcLastGCTime;
    int64_t             gcJitReleaseTime;
    JSGCMode            gcMode;
    size_t              gcAllocationThreshold;
    bool                gcHighFrequencyGC;
    uint64_t            gcHighFrequencyTimeThreshold;
    uint64_t            gcHighFrequencyLowLimitBytes;
    uint64_t            gcHighFrequencyHighLimitBytes;
    double              gcHighFrequencyHeapGrowthMax;
    double              gcHighFrequencyHeapGrowthMin;
    double              gcLowFrequencyHeapGrowth;
    bool                gcDynamicHeapGrowth;
    bool                gcDynamicMarkSlice;

    
    bool                gcShouldCleanUpEverything;

    




    volatile uintptr_t  gcIsNeeded;

    js::WeakMapBase     *gcWeakMapList;
    js::gcstats::Statistics gcStats;

    
    uint64_t            gcNumber;

    
    uint64_t            gcStartNumber;

    
    int                 gcIsIncremental;

    
    bool                gcIsFull;

    
    js::gcreason::Reason gcTriggerReason;

    



    bool                gcStrictCompartmentChecking;

    





    uintptr_t           gcDisableStrictProxyCheckingCount;

    



    js::gc::State       gcIncrementalState;

    
    bool                gcLastMarkSlice;

    
    bool                gcSweepOnBackgroundThread;

    
    JSCompartment       *gcSweepingCompartments;

    


    int                 gcSweepPhase;
    ptrdiff_t           gcSweepCompartmentIndex;
    int                 gcSweepKindIndex;

    


    js::gc::ArenaHeader *gcArenasAllocatedDuringSweep;

    




    volatile uintptr_t  gcInterFrameGC;

    
    int64_t             gcSliceBudget;

    



    bool                gcIncrementalEnabled;

    




    bool                gcExactScanningEnabled;


    




    bool                gcManipulatingDeadCompartments;

    







    unsigned            gcObjectsMarkedInDeadCompartments;

    bool                gcPoke;

    enum HeapState {
        Idle,       
        Tracing,    
        Collecting  
    };

    HeapState           heapState;

    bool isHeapBusy() { return heapState != Idle; }

    bool isHeapCollecting() { return heapState == Collecting; }

    























#ifdef JS_GC_ZEAL
    int                 gcZeal_;
    int                 gcZealFrequency;
    int                 gcNextScheduled;
    bool                gcDeterministicOnly;
    int                 gcIncrementalLimit;

    js::Vector<JSObject *, 0, js::SystemAllocPolicy> gcSelectedForMarking;

    int gcZeal() { return gcZeal_; }

    bool needZealousGC() {
        if (gcNextScheduled > 0 && --gcNextScheduled == 0) {
            if (gcZeal() == js::gc::ZealAllocValue ||
                gcZeal() == js::gc::ZealPurgeAnalysisValue ||
                (gcZeal() >= js::gc::ZealIncrementalRootsThenFinish &&
                 gcZeal() <= js::gc::ZealIncrementalMultipleSlices))
            {
                gcNextScheduled = gcZealFrequency;
            }
            return true;
        }
        return false;
    }
#else
    int gcZeal() { return 0; }
    bool needZealousGC() { return false; }
#endif

    bool                gcValidate;

    JSGCCallback        gcCallback;
    js::GCSliceCallback gcSliceCallback;
    JSFinalizeCallback  gcFinalizeCallback;

    js::AnalysisPurgeCallback analysisPurgeCallback;
    uint64_t            analysisPurgeTriggerBytes;

  private:
    



    volatile ptrdiff_t  gcMallocBytes;

  public:
    





    JSTraceDataOp       gcBlackRootsTraceOp;
    void                *gcBlackRootsData;
    JSTraceDataOp       gcGrayRootsTraceOp;
    void                *gcGrayRootsData;

    
    js::AutoGCRooter   *autoGCRooters;

    
    js::ScriptAndCountsVector *scriptAndCountsVector;

    
    js::Value           NaNValue;
    js::Value           negativeInfinityValue;
    js::Value           positiveInfinityValue;

    js::PropertyName    *emptyString;

    
    mozilla::LinkedList<JSContext> contextList;

    bool hasContexts() const {
        return !contextList.isEmpty();
    }

    JS_SourceHook       sourceHook;

    
    JSDebugHooks        debugHooks;

    
    bool                debugMode;

    
    js::SPSProfiler     spsProfiler;

    
    bool                profilingScripts;

    
    bool                alwaysPreserveCode;

    
    JSBool              hadOutOfMemory;

    



    JSCList             debuggerList;

    



    JSCList             onNewGlobalObjectWatchers;

    
    js::DebugScopes     *debugScopes;

    
    JSObject            *liveArrayBuffers;

    
    void                *data;

    
    PRLock              *gcLock;

    js::GCHelperThread  gcHelperThread;

#ifdef JS_THREADSAFE
# ifdef JS_ION
    js::WorkerThreadState *workerThreadState;
# endif

    js::SourceCompressorThread sourceCompressorThread;
#endif

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

    




    uint32_t            propertyRemovals;

    
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

  private:
    js::MathCache *mathCache_;
    js::MathCache *createMathCache(JSContext *cx);
  public:
    js::MathCache *getMathCache(JSContext *cx) {
        return mathCache_ ? mathCache_ : createMathCache(cx);
    }

    js::GSNCache        gsnCache;
    js::PropertyCache   propertyCache;
    js::NewObjectCache  newObjectCache;
    js::NativeIterCache nativeIterCache;
    js::SourceDataCache sourceDataCache;
    js::EvalCache       evalCache;

    
    DtoaState           *dtoaState;

    js::ConservativeGCData conservativeGC;

  private:
    JSPrincipals        *trustedPrincipals_;
  public:
    void setTrustedPrincipals(JSPrincipals *p) { trustedPrincipals_ = p; }
    JSPrincipals *trustedPrincipals() const { return trustedPrincipals_; }

    
    js::AtomSet         atoms;

    union {
        



        JSAtomState atomState;

        js::FixedHeapPtr<js::PropertyName> firstCachedName;
    };

    
    js::StaticStrings   staticStrings;

    JSWrapObjectCallback                   wrapObjectCallback;
    JSSameCompartmentWrapObjectCallback    sameCompartmentWrapObjectCallback;
    JSPreWrapCallback                      preWrapObjectCallback;
    js::PreserveWrapperCallback            preserveWrapperCallback;

    js::ScriptFilenameTable scriptFilenameTable;

#ifdef DEBUG
    size_t              noGCOrAllocationCheck;
#endif

    




    int32_t             inOOMReport;

    bool                jitHardening;

    
    
    uint8_t             *ionTop;
    JSContext           *ionJSContext;
    uintptr_t            ionStackLimit;

    void resetIonStackLimit() {
        ionStackLimit = nativeStackLimit;
    }

    
    js::ion::IonActivation  *ionActivation;

    
    js::ion::PcScriptCache *ionPcScriptCache;

    js::ThreadPool threadPool;

  private:
    
    
    
    
    
    
    
    
    
    
    
    
    js::Value            ionReturnOverride_;

  public:
    bool hasIonReturnOverride() const {
        return !ionReturnOverride_.isMagic();
    }
    js::Value takeIonReturnOverride() {
        js::Value v = ionReturnOverride_;
        ionReturnOverride_ = js::MagicValue(JS_ARG_POISON);
        return v;
    }
    void setIonReturnOverride(const js::Value &v) {
        JS_ASSERT(!hasIonReturnOverride());
        ionReturnOverride_ = v;
    }

    JSRuntime(JSUseHelperThreads useHelperThreads);
    ~JSRuntime();

    bool init(uint32_t maxbytes);

    JSRuntime *thisFromCtor() { return this; }

    



    void* malloc_(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(cx, bytes);
        void *p = js_malloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(NULL, bytes, cx);
    }

    



    void* calloc_(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(cx, bytes);
        void *p = js_calloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(reinterpret_cast<void *>(1), bytes, cx);
    }

    void* realloc_(void* p, size_t oldBytes, size_t newBytes, JSContext *cx = NULL) {
        JS_ASSERT(oldBytes < newBytes);
        updateMallocCounter(cx, newBytes - oldBytes);
        void *p2 = js_realloc(p, newBytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, newBytes, cx);
    }

    void* realloc_(void* p, size_t bytes, JSContext *cx = NULL) {
        



        if (!p)
            updateMallocCounter(cx, bytes);
        void *p2 = js_realloc(p, bytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, bytes, cx);
    }

    template <class T>
    T *pod_malloc(JSContext *cx = NULL) {
        return (T *)malloc_(sizeof(T), cx);
    }

    template <class T>
    T *pod_calloc(JSContext *cx = NULL) {
        return (T *)calloc_(sizeof(T), cx);
    }

    template <class T>
    T *pod_malloc(size_t numElems, JSContext *cx = NULL) {
        if (numElems & js::tl::MulOverflowMask<sizeof(T)>::result) {
            js_ReportAllocationOverflow(cx);
            return NULL;
        }
        return (T *)malloc_(numElems * sizeof(T), cx);
    }

    template <class T>
    T *pod_calloc(size_t numElems, JSContext *cx = NULL) {
        if (numElems & js::tl::MulOverflowMask<sizeof(T)>::result) {
            js_ReportAllocationOverflow(cx);
            return NULL;
        }
        return (T *)calloc_(numElems * sizeof(T), cx);
    }

    JS_DECLARE_NEW_METHODS(new_, malloc_, JS_ALWAYS_INLINE)

    void setGCMaxMallocBytes(size_t value);

    void resetGCMallocBytes() { gcMallocBytes = ptrdiff_t(gcMaxMallocBytes); }

    







    void updateMallocCounter(JSContext *cx, size_t nbytes);

    bool isTooMuchMalloc() const {
        return gcMallocBytes <= 0;
    }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);

    void triggerOperationCallback();

    void setJitHardening(bool enabled);
    bool getJitHardening() const {
        return jitHardening;
    }

    void sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf, JS::RuntimeSizes *runtime);
    size_t sizeOfExplicitNonHeap();

  private:
    JSUseHelperThreads useHelperThreads_;
  public:
    bool useHelperThreads() const {
#ifdef JS_THREADSAFE
        return useHelperThreads_ == JS_USE_HELPER_THREADS;
#else
        return false;
#endif
    }
};


#define JS_KEEP_ATOMS(rt)   (rt)->gcKeepAtoms++;
#define JS_UNKEEP_ATOMS(rt) (rt)->gcKeepAtoms--;

namespace js {

struct AutoResolving;

static inline bool
OptionsHasAllowXML(uint32_t options)
{
    return !!(options & JSOPTION_ALLOW_XML);
}

static inline bool
OptionsHasMoarXML(uint32_t options)
{
    return !!(options & JSOPTION_MOAR_XML);
}

static inline bool
OptionsSameVersionFlags(uint32_t self, uint32_t other)
{
    static const uint32_t mask = JSOPTION_MOAR_XML;
    return !((self & mask) ^ (other & mask));
}









namespace VersionFlags {
static const unsigned MASK      = 0x0FFF; 
static const unsigned ALLOW_XML = 0x1000; 
static const unsigned MOAR_XML  = 0x2000; 
static const unsigned FULL_MASK = 0x3FFF;
} 

static inline JSVersion
VersionNumber(JSVersion version)
{
    return JSVersion(uint32_t(version) & VersionFlags::MASK);
}

static inline bool
VersionHasAllowXML(JSVersion version)
{
    return !!(version & VersionFlags::ALLOW_XML);
}

static inline bool
VersionHasMoarXML(JSVersion version)
{
    return !!(version & VersionFlags::MOAR_XML);
}


static inline bool
VersionShouldParseXML(JSVersion version)
{
    return VersionHasMoarXML(version) || VersionNumber(version) >= JSVERSION_1_6;
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

static inline unsigned
VersionFlagsToOptions(JSVersion version)
{
    unsigned copts = (VersionHasAllowXML(version) ? JSOPTION_ALLOW_XML : 0) |
                     (VersionHasMoarXML(version) ? JSOPTION_MOAR_XML : 0);
    JS_ASSERT((copts & JSCOMPILEOPTION_MASK) == copts);
    return copts;
}

static inline JSVersion
OptionFlagsToVersion(unsigned options, JSVersion version)
{
    uint32_t v = version;
    v &= ~(VersionFlags::ALLOW_XML | VersionFlags::MOAR_XML);
    if (OptionsHasAllowXML(options))
        v |= VersionFlags::ALLOW_XML;
    if (OptionsHasMoarXML(options))
        v |= VersionFlags::MOAR_XML;
    return JSVersion(v);
}

static inline bool
VersionIsKnown(JSVersion version)
{
    return VersionNumber(version) != JSVERSION_UNKNOWN;
}

inline void
FreeOp::free_(void* p) {
    if (shouldFreeLater()) {
        runtime()->gcHelperThread.freeLater(p);
        return;
    }
    js_free(p);
}

} 

struct JSContext : js::ContextFriendFields,
                   public mozilla::LinkedListElement<JSContext>
{
    explicit JSContext(JSRuntime *rt);
    JSContext *thisDuringConstruction() { return this; }
    ~JSContext();

  private:
    
    JSVersion           defaultVersion;      
    JSVersion           versionOverride;     
    bool                hasVersionOverride;

    
    JSBool              throwing;            
    js::Value           exception;           

    
    unsigned            runOptions;          

  public:
    int32_t             reportGranularity;  

    
    JSLocaleCallbacks   *localeCallbacks;

    js::AutoResolving   *resolvingList;

    
    bool                generatingError;

    
    JSCompartment       *compartment;

    inline void setCompartment(JSCompartment *c) { compartment = c; }

    














  private:
    unsigned            enterCompartmentDepth_;
  public:
    bool hasEnteredCompartment() const {
        return enterCompartmentDepth_ > 0;
    }

    void enterCompartment(JSCompartment *c) {
        enterCompartmentDepth_++;
        compartment = c;
        if (throwing)
            wrapPendingException();
    }

    inline void leaveCompartment(JSCompartment *oldCompartment) {
        JS_ASSERT(hasEnteredCompartment());
        enterCompartmentDepth_--;

        







        if (hasEnteredCompartment() || !defaultCompartmentObject_)
            compartment = oldCompartment;
        else
            compartment = defaultCompartmentObject_->compartment();

        if (throwing)
            wrapPendingException();
    }

    
  private:
    struct SavedFrameChain {
        SavedFrameChain(JSCompartment *comp, unsigned count)
          : compartment(comp), enterCompartmentCount(count) {}
        JSCompartment *compartment;
        unsigned enterCompartmentCount;
    };
    typedef js::Vector<SavedFrameChain, 1, js::SystemAllocPolicy> SaveStack;
    SaveStack           savedFrameChains_;
  public:
    bool saveFrameChain();
    void restoreFrameChain();

    




  private:
    JSObject *defaultCompartmentObject_;
  public:
    inline void setDefaultCompartmentObject(JSObject *obj);
    inline void setDefaultCompartmentObjectIfUnset(JSObject *obj);
    JSObject *maybeDefaultCompartmentObject() const { return defaultCompartmentObject_; }

    
    js::ContextStack    stack;

    
    inline js::Handle<js::GlobalObject*> global() const;

    
    inline bool hasfp() const               { return stack.hasfp(); }
    inline js::StackFrame* fp() const       { return stack.fp(); }
    inline js::StackFrame* maybefp() const  { return stack.maybefp(); }
    inline js::FrameRegs& regs() const      { return stack.regs(); }
    inline js::FrameRegs* maybeRegs() const { return stack.maybeRegs(); }

    
    void wrapPendingException();

  private:
    
    js::frontend::ParseMapPool *parseMapPool_;

  public:
    
    js::ObjectSet       cycleDetectorSet;

    
    JSErrorReporter     errorReporter;

    
    JSOperationCallback operationCallback;

    
    void                *data;
    void                *data2;

    inline js::RegExpStatics *regExpStatics();

  public:
    js::frontend::ParseMapPool &parseMapPool() {
        JS_ASSERT(parseMapPool_);
        return *parseMapPool_;
    }

    inline bool ensureParseMapPool();

    



    inline bool canSetDefaultVersion() const;

    
    inline void overrideVersion(JSVersion newVersion);

    
    void setDefaultVersion(JSVersion version) {
        defaultVersion = version;
    }

    void clearVersionOverride() { hasVersionOverride = false; }
    JSVersion getDefaultVersion() const { return defaultVersion; }
    bool isVersionOverridden() const { return hasVersionOverride; }

    JSVersion getVersionOverride() const {
        JS_ASSERT(isVersionOverridden());
        return versionOverride;
    }

    



    inline bool maybeOverrideVersion(JSVersion newVersion);

    



    void maybeMigrateVersionOverride() {
        JS_ASSERT(stack.empty());
        if (JS_UNLIKELY(isVersionOverridden())) {
            defaultVersion = versionOverride;
            clearVersionOverride();
        }
    }

    







    inline JSVersion findVersion() const;

    void setRunOptions(unsigned ropts) {
        JS_ASSERT((ropts & JSRUNOPTION_MASK) == ropts);
        runOptions = ropts;
    }

    
    inline void setCompileOptions(unsigned newcopts);

    unsigned getRunOptions() const { return runOptions; }
    inline unsigned getCompileOptions() const;
    inline unsigned allOptions() const;

    bool hasRunOption(unsigned ropt) const {
        JS_ASSERT((ropt & JSRUNOPTION_MASK) == ropt);
        return !!(runOptions & ropt);
    }

    bool hasStrictOption() const { return hasRunOption(JSOPTION_STRICT); }
    bool hasWErrorOption() const { return hasRunOption(JSOPTION_WERROR); }
    bool hasAtLineOption() const { return hasRunOption(JSOPTION_ATLINE); }

    js::LifoAlloc &tempLifoAlloc() { return runtime->tempLifoAlloc; }
    inline js::LifoAlloc &analysisLifoAlloc();
    inline js::LifoAlloc &typeLifoAlloc();

    inline js::PropertyTree &propertyTree();

    js::PropertyCache &propertyCache() { return runtime->propertyCache; }

#ifdef JS_THREADSAFE
    unsigned            outstandingRequests;


#endif

    
    unsigned               resolveFlags;

    
    int64_t             rngSeed;

    
    js::Value           iterValue;

#ifdef JS_METHODJIT
    bool                 methodJitEnabled;

    js::mjit::JaegerRuntime &jaegerRuntime() { return runtime->jaegerRuntime(); }
#endif

    inline bool typeInferenceEnabled() const;

    
    void updateJITEnabled();

#ifdef MOZ_TRACE_JSCALLS
    
    JSFunctionCallback    functionCallback;

    void doFunctionCallback(const JSFunction *fun,
                            const JSScript *scr,
                            int entering) const
    {
        if (functionCallback)
            functionCallback(fun, scr, this, entering);
    }
#endif

    DSTOffsetCache dstOffsetCache;

    
    js::PropertyIteratorObject *enumerators;

  private:
    
    JSGenerator *innermostGenerator_;
  public:
    JSGenerator *innermostGenerator() const { return innermostGenerator_; }
    void enterGenerator(JSGenerator *gen);
    void leaveGenerator(JSGenerator *gen);

    inline void* malloc_(size_t bytes) {
        return runtime->malloc_(bytes, this);
    }

    inline void* calloc_(size_t bytes) {
        return runtime->calloc_(bytes, this);
    }

    inline void* realloc_(void* p, size_t bytes) {
        return runtime->realloc_(p, bytes, this);
    }

    inline void* realloc_(void* p, size_t oldBytes, size_t newBytes) {
        return runtime->realloc_(p, oldBytes, newBytes, this);
    }

    template <class T> T *pod_malloc() {
        return runtime->pod_malloc<T>(this);
    }

    template <class T> T *pod_calloc() {
        return runtime->pod_calloc<T>(this);
    }

    template <class T> T *pod_malloc(size_t numElems) {
        return runtime->pod_malloc<T>(numElems, this);
    }

    template <class T>
    T *pod_calloc(size_t numElems) {
        return runtime->pod_calloc<T>(numElems, this);
    }

    JS_DECLARE_NEW_METHODS(new_, malloc_, JS_ALWAYS_INLINE)

    void purge();

    bool isExceptionPending() {
        return throwing;
    }

    js::Value getPendingException() {
        JS_ASSERT(throwing);
        return exception;
    }

    void setPendingException(js::Value v);

    void clearPendingException() {
        throwing = false;
        exception.setUndefined();
    }

    JSAtomState & names() { return runtime->atomState; }

#ifdef DEBUG
    



    bool stackIterAssertionEnabled;
#endif

    




    unsigned activeCompilations;

    



    bool runningWithTrustedPrincipals() const;

    JS_FRIEND_API(size_t) sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const;

    void mark(JSTracer *trc);

  private:
    





    JS_FRIEND_API(void) checkMallocGCPressure(void *p);
}; 

namespace js {

struct AutoResolving {
  public:
    enum Kind {
        LOOKUP,
        WATCH
    };

    AutoResolving(JSContext *cx, HandleObject obj, HandleId id, Kind kind = LOOKUP
                  JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : context(cx), object(obj), id(id), kind(kind), link(cx->resolvingList)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
        cx->resolvingList = this;
    }

    ~AutoResolving() {
        JS_ASSERT(context->resolvingList == this);
        context->resolvingList = link;
    }

    bool alreadyStarted() const {
        return link && alreadyStartedSlow();
    }

  private:
    bool alreadyStartedSlow() const;

    JSContext           *const context;
    HandleObject        object;
    HandleId            id;
    Kind                const kind;
    AutoResolving       *const link;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#if JS_HAS_XML_SUPPORT
class AutoXMLRooter : private AutoGCRooter {
  public:
    AutoXMLRooter(JSContext *cx, JSXML *xml
                  JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, XML), xml(xml)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(xml);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSXML * const xml;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

#ifdef JS_THREADSAFE
# define JS_LOCK_GC(rt)    PR_Lock((rt)->gcLock)
# define JS_UNLOCK_GC(rt)  PR_Unlock((rt)->gcLock)
#else
# define JS_LOCK_GC(rt)    do { } while (0)
# define JS_UNLOCK_GC(rt)  do { } while (0)
#endif

class AutoLockGC
{
  public:
    explicit AutoLockGC(JSRuntime *rt = NULL
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : runtime(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        
        if (rt)
            JS_LOCK_GC(rt);
    }

    ~AutoLockGC()
    {
        if (runtime)
            JS_UNLOCK_GC(runtime);
    }

    bool locked() const {
        return !!runtime;
    }

    void lock(JSRuntime *rt) {
        JS_ASSERT(rt);
        JS_ASSERT(!runtime);
        runtime = rt;
        JS_LOCK_GC(rt);
    }

  private:
    JSRuntime *runtime;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoUnlockGC {
  private:
#ifdef JS_THREADSAFE
    JSRuntime *rt;
#endif
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoUnlockGC(JSRuntime *rt
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
#ifdef JS_THREADSAFE
      : rt(rt)
#endif
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_UNLOCK_GC(rt);
    }
    ~AutoUnlockGC() { JS_LOCK_GC(rt); }
};

class AutoKeepAtoms {
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoKeepAtoms(JSRuntime *rt
                           JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_KEEP_ATOMS(rt);
    }
    ~AutoKeepAtoms() { JS_UNKEEP_ATOMS(rt); }
};

class AutoReleasePtr {
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleasePtr(const AutoReleasePtr &other) MOZ_DELETE;
    AutoReleasePtr operator=(const AutoReleasePtr &other) MOZ_DELETE;

  public:
    explicit AutoReleasePtr(void *ptr
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(ptr)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    void forget() { ptr = NULL; }
    ~AutoReleasePtr() { js_free(ptr); }
};




class AutoReleaseNullablePtr {
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleaseNullablePtr(const AutoReleaseNullablePtr &other) MOZ_DELETE;
    AutoReleaseNullablePtr operator=(const AutoReleaseNullablePtr &other) MOZ_DELETE;

  public:
    explicit AutoReleaseNullablePtr(void *ptr
                                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(ptr)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    void reset(void *ptr2) {
        if (ptr)
            js_free(ptr);
        ptr = ptr2;
    }
    ~AutoReleaseNullablePtr() { if (ptr) js_free(ptr); }
};

} 

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, unsigned flags
                       JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx), mSaved(cx->resolveFlags)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    unsigned mSaved;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

namespace js {




class ContextIter {
    JSContext *iter;

public:
    explicit ContextIter(JSRuntime *rt) {
        iter = rt->contextList.getFirst();
    }

    bool done() const {
        return !iter;
    }

    void next() {
        JS_ASSERT(!done());
        iter = iter->getNext();
    }

    JSContext *get() const {
        JS_ASSERT(!done());
        return iter;
    }

    operator JSContext *() const {
        return get();
    }

    JSContext *operator ->() const {
        return get();
    }
};





extern JSContext *
NewContext(JSRuntime *rt, size_t stackChunkSize);

enum DestroyContextMode {
    DCM_NO_GC,
    DCM_FORCE_GC,
    DCM_NEW_FAILED
};

extern void
DestroyContext(JSContext *cx, DestroyContextMode mode);

} 

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, unsigned flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, unsigned flags, JSErrorCallback callback,
                       void *userRef, const unsigned errorNumber,
                       JSBool charArgs, va_list ap);

extern bool
js_ReportErrorNumberUCArray(JSContext *cx, unsigned flags, JSErrorCallback callback,
                            void *userRef, const unsigned errorNumber,
                            const jschar **args);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const unsigned errorNumber,
                        char **message, JSErrorReport *reportp,
                        bool charArgs, va_list ap);
#endif

namespace js {


extern void
ReportUsageError(JSContext *cx, HandleObject callee, const char *msg);







extern bool
PrintError(JSContext *cx, FILE *file, const char *message, JSErrorReport *report,
           bool reportWarnings);
} 





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);




extern JSBool
js_ReportIsNullOrUndefined(JSContext *cx, int spindex, js::HandleValue v,
                           js::HandleString fallback);

extern void
js_ReportMissingArg(JSContext *cx, js::HandleValue v, unsigned arg);






extern JSBool
js_ReportValueErrorFlags(JSContext *cx, unsigned flags, const unsigned errorNumber,
                         int spindex, js::HandleValue v, js::HandleString fallback,
                         const char *arg1, const char *arg2);

#define js_ReportValueError(cx,errorNumber,spindex,v,fallback)                \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, NULL, NULL))

#define js_ReportValueError2(cx,errorNumber,spindex,v,fallback,arg1)          \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, NULL))

#define js_ReportValueError3(cx,errorNumber,spindex,v,fallback,arg1,arg2)     \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, arg2))

extern JSErrorFormatString js_ErrorFormatString[JSErr_Limit];

#ifdef JS_THREADSAFE
# define JS_ASSERT_REQUEST_DEPTH(cx)  JS_ASSERT((cx)->runtime->requestDepth >= 1)
#else
# define JS_ASSERT_REQUEST_DEPTH(cx)  ((void) 0)
#endif





extern JSBool
js_InvokeOperationCallback(JSContext *cx);

extern JSBool
js_HandleExecutionInterrupt(JSContext *cx);

extern jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx);






static MOZ_ALWAYS_INLINE bool
JS_CHECK_OPERATION_LIMIT(JSContext *cx)
{
    JS_ASSERT_REQUEST_DEPTH(cx);
    return !cx->runtime->interrupt || js_InvokeOperationCallback(cx);
}

namespace js {

#ifdef JS_METHODJIT
namespace mjit {
    void ExpandInlineFrames(JSCompartment *compartment);
}
#endif

} 

namespace js {



static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *vec, size_t len)
{
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *beg, Value *end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *beg, jsid *end)
{
    for (jsid *id = beg; id != end; ++id)
        *id = INT_TO_JSID(0);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *vec, size_t len)
{
    MakeRangeGCSafe(vec, vec + len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Shape **beg, Shape **end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Shape **vec, size_t len)
{
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setUndefined();
}

static JS_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *vec, size_t len)
{
    SetValueRangeToUndefined(vec, vec + len);
}

static JS_ALWAYS_INLINE void
SetValueRangeToNull(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setNull();
}

static JS_ALWAYS_INLINE void
SetValueRangeToNull(Value *vec, size_t len)
{
    SetValueRangeToNull(vec, vec + len);
}

class AutoObjectVector : public AutoVectorRooter<JSObject *>
{
  public:
    explicit AutoObjectVector(JSContext *cx
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSObject *>(cx, OBJVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoStringVector : public AutoVectorRooter<JSString *>
{
  public:
    explicit AutoStringVector(JSContext *cx
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSString *>(cx, STRINGVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoShapeVector : public AutoVectorRooter<Shape *>
{
  public:
    explicit AutoShapeVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Shape *>(cx, SHAPEVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueArray : public AutoGCRooter
{
    js::Value *start_;
    unsigned length_;
    SkipRoot skip;

  public:
    AutoValueArray(JSContext *cx, js::Value *start, unsigned length
                   JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, VALARRAY), start_(start), length_(length), skip(cx, start, length)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    Value *start() { return start_; }
    unsigned length() const { return length_; }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};












class RuntimeAllocPolicy
{
    JSRuntime *const runtime;

  public:
    RuntimeAllocPolicy(JSRuntime *rt) : runtime(rt) {}
    RuntimeAllocPolicy(JSContext *cx) : runtime(cx->runtime) {}
    void *malloc_(size_t bytes) { return runtime->malloc_(bytes); }
    void *calloc_(size_t bytes) { return runtime->calloc_(bytes); }
    void *realloc_(void *p, size_t bytes) { return runtime->realloc_(p, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const {}
};




class ContextAllocPolicy
{
    JSContext *const cx;

  public:
    ContextAllocPolicy(JSContext *cx) : cx(cx) {}
    JSContext *context() const { return cx; }
    void *malloc_(size_t bytes) { return cx->malloc_(bytes); }
    void *calloc_(size_t bytes) { return cx->calloc_(bytes); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) { return cx->realloc_(p, oldBytes, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const { js_ReportAllocationOverflow(cx); }
};

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
