







#ifndef jscntxt_h
#define jscntxt_h

#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include <string.h>
#include <setjmp.h>

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"
#include "jsatom.h"
#include "jsclist.h"
#include "jsgc.h"

#include "ds/FixedSizeHash.h"
#include "ds/LifoAlloc.h"
#include "frontend/ParseMaps.h"
#include "gc/Nursery.h"
#include "gc/Statistics.h"
#include "gc/StoreBuffer.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/DateTime.h"
#include "vm/SPSProfiler.h"
#include "vm/Stack.h"
#include "vm/ThreadPool.h"

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

typedef Rooted<JSLinearString*> RootedLinearString;

struct CallsiteCloneKey {
    
    JSFunction *original;

    
    JSScript *script;

    
    uint32_t offset;

    CallsiteCloneKey(JSFunction *f, JSScript *s, uint32_t o) : original(f), script(s), offset(o) {}

    typedef CallsiteCloneKey Lookup;

    static inline uint32_t hash(CallsiteCloneKey key) {
        return uint32_t(size_t(key.script->code + key.offset) ^ size_t(key.original));
    }

    static inline bool match(const CallsiteCloneKey &a, const CallsiteCloneKey &b) {
        return a.script == b.script && a.offset == b.offset && a.original == b.original;
    }
};

typedef HashMap<CallsiteCloneKey,
                ReadBarriered<JSFunction>,
                CallsiteCloneKey,
                SystemAllocPolicy> CallsiteCloneTable;

JSFunction *CloneFunctionAtCallsite(JSContext *cx, HandleFunction fun,
                                    HandleScript script, jsbytecode *pc);

typedef HashSet<JSObject *> ObjectSet;
typedef HashSet<Shape *> ShapeSet;


class AutoCycleDetector
{
    JSContext *cx;
    RootedObject obj;
    bool cyclic;
    uint32_t hashsetGenerationAtInit;
    ObjectSet::AddPtr hashsetAddPointer;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoCycleDetector(JSContext *cx, HandleObject objArg
                      MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), obj(cx, objArg), cyclic(true)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoCycleDetector();

    bool init();

    bool foundCycle() { return cyclic; }
};


extern void
TraceCycleDetectionSet(JSTracer *trc, ObjectSet &set);

class MathCache;

namespace ion {
class IonActivation;
class IonRuntime;
struct PcScriptCache;
}

class AsmJSActivation;
class InterpreterFrames;
class WeakMapBase;
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
        mozilla::PodZero(this);
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

    static void clear(JSScript **pscript) { *pscript = NULL; }
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
      : last(NULL)
    {
        mozilla::PodArrayZero(data);
    }

    void purge() {
        last = NULL;
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

    NewObjectCache() { mozilla::PodZero(this); }
    void purge() { mozilla::PodZero(this); }

    
    void clearNurseryObjects(JSRuntime *rt);

    



    inline bool lookupProto(Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupGlobal(Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupType(Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, EntryIndex *pentry);

    




    inline JSObject *newObjectFromHit(JSContext *cx, EntryIndex entry, js::gc::InitialHeap heap);

    
    void fillProto(EntryIndex entry, Class *clasp, js::TaggedProto proto, gc::AllocKind kind, JSObject *obj);
    inline void fillGlobal(EntryIndex entry, Class *clasp, js::GlobalObject *global, gc::AllocKind kind, JSObject *obj);
    inline void fillType(EntryIndex entry, Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, JSObject *obj);

    
    void invalidateEntriesForShape(JSContext *cx, HandleShape shape, HandleObject proto);

  private:
    inline bool lookup(Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry);
    inline void fill(EntryIndex entry, Class *clasp, gc::Cell *key, gc::AllocKind kind, JSObject *obj);
    static inline void copyCachedToObject(JSObject *dst, JSObject *src, gc::AllocKind kind);
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
#endif





    uint8_t             *ionTop;
    JSContext           *ionJSContext;
    uintptr_t            ionStackLimit;

    inline void setIonStackLimit(uintptr_t limit);

    







  private:
    friend class js::Activation;
    friend class js::ActivationIterator;
    friend class js::AsmJSActivation;

    



    js::Activation *activation_;

    
    js::AsmJSActivation *asmJSActivationStack_;

  public:
    static unsigned offsetOfActivation() {
        return offsetof(PerThreadData, activation_);
    }
    static unsigned offsetOfAsmJSActivationStackReadOnly() {
        return offsetof(PerThreadData, asmJSActivationStack_);
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

    







    int32_t             suppressGC;

    PerThreadData(JSRuntime *runtime);
    ~PerThreadData();

    bool init();

    bool associatedWith(const JSRuntime *rt) { return runtime_ == rt; }
};

template<class Client>
struct MallocProvider
{
    void *malloc_(size_t bytes) {
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(bytes);
        void *p = js_malloc(bytes);
        return JS_LIKELY(!!p) ? p : client->onOutOfMemory(NULL, bytes);
    }

    void *calloc_(size_t bytes) {
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(bytes);
        void *p = js_calloc(bytes);
        return JS_LIKELY(!!p) ? p : client->onOutOfMemory(reinterpret_cast<void *>(1), bytes);
    }

    void *realloc_(void *p, size_t oldBytes, size_t newBytes) {
        Client *client = static_cast<Client *>(this);
        



        if (newBytes > oldBytes)
            client->updateMallocCounter(newBytes - oldBytes);
        void *p2 = js_realloc(p, newBytes);
        return JS_LIKELY(!!p2) ? p2 : client->onOutOfMemory(p, newBytes);
    }

    void *realloc_(void *p, size_t bytes) {
        Client *client = static_cast<Client *>(this);
        



        if (!p)
            client->updateMallocCounter(bytes);
        void *p2 = js_realloc(p, bytes);
        return JS_LIKELY(!!p2) ? p2 : client->onOutOfMemory(p, bytes);
    }

    template <class T>
    T *pod_malloc() {
        return (T *)malloc_(sizeof(T));
    }

    template <class T>
    T *pod_calloc() {
        return (T *)calloc_(sizeof(T));
    }

    template <class T>
    T *pod_malloc(size_t numElems) {
        if (numElems & js::tl::MulOverflowMask<sizeof(T)>::result) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return NULL;
        }
        return (T *)malloc_(numElems * sizeof(T));
    }

    template <class T>
    T *pod_calloc(size_t numElems, JSCompartment *comp = NULL, JSContext *cx = NULL) {
        if (numElems & js::tl::MulOverflowMask<sizeof(T)>::result) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return NULL;
        }
        return (T *)calloc_(numElems * sizeof(T));
    }

    JS_DECLARE_NEW_METHODS(new_, malloc_, JS_ALWAYS_INLINE)
};

namespace gc {
class MarkingValidator;
} 

typedef Vector<JS::Zone *, 1, SystemAllocPolicy> ZoneVector;

} 

struct JSRuntime : public JS::shadow::Runtime,
                   public js::MallocProvider<JSRuntime>
{
    









    js::PerThreadData   mainThread;

    



    volatile int32_t    interrupt;

#ifdef JS_THREADSAFE
  private:
    



    PRLock *operationCallbackLock;
#ifdef DEBUG
    PRThread *operationCallbackOwner;
#endif
  public:
#endif 

    class AutoLockForOperationCallback {
#ifdef JS_THREADSAFE
        JSRuntime *rt;
      public:
        AutoLockForOperationCallback(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) : rt(rt) {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
            PR_Lock(rt->operationCallbackLock);
#ifdef DEBUG
            rt->operationCallbackOwner = PR_GetCurrentThread();
#endif
        }
        ~AutoLockForOperationCallback() {
            JS_ASSERT(rt->operationCallbackOwner == PR_GetCurrentThread());
#ifdef DEBUG
            rt->operationCallbackOwner = NULL;
#endif
            PR_Unlock(rt->operationCallbackLock);
        }
#else 
      public:
        AutoLockForOperationCallback(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        }
#endif 

        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    };

    bool currentThreadOwnsOperationCallbackLock() {
#if defined(JS_THREADSAFE) && defined(DEBUG)
        return operationCallbackOwner == PR_GetCurrentThread();
#else
        return true;
#endif
    }

    
    JSCompartment       *atomsCompartment;

    
    JS::Zone            *systemZone;

    
    js::ZoneVector      zones;

    
    size_t              numCompartments;

    
    JSLocaleCallbacks *localeCallbacks;

    
    char *defaultLocale;

    
    JSVersion defaultVersion_;

    
#ifdef JS_THREADSAFE
  public:
    void *ownerThread() const { return ownerThread_; }
    void clearOwnerThread();
    void setOwnerThread();
    JS_FRIEND_API(void) abortIfWrongThread() const;
#ifdef DEBUG
    JS_FRIEND_API(void) assertValidThread() const;
#else
    void assertValidThread() const {}
#endif
  private:
    void                *ownerThread_;
  public:
#else
  public:
    void abortIfWrongThread() const {}
    void assertValidThread() const {}
#endif

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 4 * 1024;
    js::LifoAlloc tempLifoAlloc;

    



    js::LifoAlloc freeLifoAlloc;

  private:
    



    JSC::ExecutableAllocator *execAlloc_;
    WTF::BumpPointerAllocator *bumpAlloc_;
    js::ion::IonRuntime *ionRuntime_;

    JSObject *selfHostingGlobal_;

    
    js::InterpreterStack interpreterStack_;

    JSC::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
    WTF::BumpPointerAllocator *createBumpPointerAllocator(JSContext *cx);
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
    js::ion::IonRuntime *getIonRuntime(JSContext *cx) {
        return ionRuntime_ ? ionRuntime_ : createIonRuntime(cx);
    }
    js::ion::IonRuntime *ionRuntime() {
        return ionRuntime_;
    }
    bool hasIonRuntime() const {
        return !!ionRuntime_;
    }
    js::InterpreterStack &interpreterStack() {
        return interpreterStack_;
    }

    
    
    

    bool initSelfHosting(JSContext *cx);
    void finishSelfHosting();
    void markSelfHostingGlobal(JSTracer *trc);
    bool isSelfHostingGlobal(js::HandleObject global) {
        return global == selfHostingGlobal_;
    }
    bool cloneSelfHostedFunctionScript(JSContext *cx, js::Handle<js::PropertyName*> name,
                                       js::Handle<JSFunction*> targetFun);
    bool cloneSelfHostedValue(JSContext *cx, js::Handle<js::PropertyName*> name,
                              js::MutableHandleValue vp);
    bool maybeWrappedSelfHostedFunction(JSContext *cx, js::Handle<js::PropertyName*> name,
                                        js::MutableHandleValue funVal);

    
    
    

    






    bool setDefaultLocale(const char *locale);

    
    void resetDefaultLocale();

    
    const char *getDefaultLocale();

    JSVersion defaultVersion() { return defaultVersion_; }
    void setDefaultVersion(JSVersion v) { defaultVersion_ = v; }

    
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
    unsigned            gcKeepAtoms;
    volatile size_t     gcBytes;
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
    uint64_t            gcDecommitThreshold;

    
    bool                gcShouldCleanUpEverything;

    



    bool                gcGrayBitsValid;

    




    volatile uintptr_t  gcIsNeeded;

    js::gcstats::Statistics gcStats;

    
    uint64_t            gcNumber;

    
    uint64_t            gcStartNumber;

    
    bool                gcIsIncremental;

    
    bool                gcIsFull;

    
    JS::gcreason::Reason gcTriggerReason;

    



    bool                gcStrictCompartmentChecking;

#ifdef DEBUG
    





    uintptr_t           gcDisableStrictProxyCheckingCount;
#else
    uintptr_t           unused1;
#endif

    



    js::gc::State       gcIncrementalState;

    
    bool                gcLastMarkSlice;

    
    bool                gcSweepOnBackgroundThread;

    
    bool                gcFoundBlackGrayEdges;

    
    JS::Zone            *gcSweepingZones;

    
    unsigned            gcZoneGroupIndex;

    


    JS::Zone            *gcZoneGroups;
    JS::Zone            *gcCurrentZoneGroup;
    int                 gcSweepPhase;
    JS::Zone            *gcSweepZone;
    int                 gcSweepKindIndex;
    bool                gcAbortSweepAfterCurrentGroup;

    


    js::gc::ArenaHeader *gcArenasAllocatedDuringSweep;

#ifdef DEBUG
    js::gc::MarkingValidator *gcMarkingValidator;
#endif

    




    volatile uintptr_t  gcInterFrameGC;

    
    int64_t             gcSliceBudget;

    



    bool                gcIncrementalEnabled;

    


    bool                gcGenerationalEnabled;

    




    bool                gcManipulatingDeadZones;

    







    unsigned            gcObjectsMarkedInDeadZones;

    bool                gcPoke;

    volatile js::HeapState heapState;

    bool isHeapBusy() { return heapState != js::Idle; }
    bool isHeapMajorCollecting() { return heapState == js::MajorCollecting; }
    bool isHeapMinorCollecting() { return heapState == js::MinorCollecting; }
    bool isHeapCollecting() { return isHeapMajorCollecting() || isHeapMinorCollecting(); }

#ifdef JSGC_GENERATIONAL
    js::Nursery                  gcNursery;
    js::gc::StoreBuffer          gcStoreBuffer;
#endif

    























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
    bool                gcFullCompartmentChecks;

    JSGCCallback        gcCallback;
    JS::GCSliceCallback gcSliceCallback;
    JSFinalizeCallback  gcFinalizeCallback;

    js::AnalysisPurgeCallback analysisPurgeCallback;
    uint64_t            analysisPurgeTriggerBytes;

  private:
    



    volatile ptrdiff_t  gcMallocBytes;

  public:
    void setNeedsBarrier(bool needs) {
        needsBarrier_ = needs;
    }

    bool needsBarrier() const {
        return needsBarrier_;
    }

    struct ExtraTracer {
        JSTraceDataOp op;
        void *data;

        ExtraTracer()
          : op(NULL), data(NULL)
        {}
        ExtraTracer(JSTraceDataOp op, void *data)
          : op(op), data(data)
        {}
    };

    





    typedef js::Vector<ExtraTracer, 4, js::SystemAllocPolicy> ExtraTracerVector;
    ExtraTracerVector   gcBlackRootTracers;
    ExtraTracer         gcGrayRootTracer;

    
    js::AutoGCRooter   *autoGCRooters;

    



    size_t              gcSystemPageSize;

    
    size_t              gcSystemAllocGranularity;

    
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

    
    bool                hadOutOfMemory;

    
    mozilla::LinkedList<js::Debugger> debuggerList;

    



    JSCList             onNewGlobalObjectWatchers;

    
    void                *data;

    
    PRLock              *gcLock;

    js::GCHelperThread  gcHelperThread;

#ifdef XP_MACOSX
    js::AsmJSMachExceptionHandler asmJSMachExceptionHandler;
#endif

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

#if !ENABLE_INTL_API
    
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

    js::GSNCache        gsnCache;
    js::NewObjectCache  newObjectCache;
    js::NativeIterCache nativeIterCache;
    js::SourceDataCache sourceDataCache;
    js::EvalCache       evalCache;
    js::LazyScriptCache lazyScriptCache;

    js::DateTimeInfo    dateTimeInfo;

    js::ConservativeGCData conservativeGC;

    
    js::frontend::ParseMapPool parseMapPool;

    




    unsigned activeCompilations;

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

    js::ScriptDataTable scriptDataTable;

#ifdef DEBUG
    size_t              noGCOrAllocationCheck;
#endif

    bool                jitHardening;

    bool                jitSupportsFloatingPoint;

    
    
    void resetIonStackLimit() {
        AutoLockForOperationCallback lock(this);
        mainThread.setIonStackLimit(mainThread.nativeStackLimit);
    }

    
    js::ion::PcScriptCache *ionPcScriptCache;

    js::ThreadPool threadPool;

    js::CTypesActivityCallback  ctypesActivityCallback;

    
    
    uint32_t parallelWarmup;

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

    void setGCMaxMallocBytes(size_t value);

    void resetGCMallocBytes() { gcMallocBytes = ptrdiff_t(gcMaxMallocBytes); }

    







    void updateMallocCounter(size_t nbytes);
    void updateMallocCounter(JS::Zone *zone, size_t nbytes);

    void reportAllocationOverflow() { js_ReportAllocationOverflow(NULL); }

    bool isTooMuchMalloc() const {
        return gcMallocBytes <= 0;
    }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes);
    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);

    void triggerOperationCallback();

    void setJitHardening(bool enabled);
    bool getJitHardening() const {
        return jitHardening;
    }

    void sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::RuntimeSizes *runtime);

  private:

    JSUseHelperThreads useHelperThreads_;
    int32_t requestedHelperThreadCount;

  public:

    bool useHelperThreads() const {
#ifdef JS_THREADSAFE
        return useHelperThreads_ == JS_USE_HELPER_THREADS;
#else
        return false;
#endif
    }

    void requestHelperThreadCount(size_t count) {
        requestedHelperThreadCount = count;
    }

    
    size_t helperThreadCount() const {
#ifdef JS_THREADSAFE
        if (requestedHelperThreadCount < 0)
            return js::GetCPUCount() - 1;
        return requestedHelperThreadCount;
#else
        return 0;
#endif
    }

#ifdef DEBUG
  public:
    js::AutoEnterPolicy *enteredPolicy;
#endif
};


#define JS_KEEP_ATOMS(rt)   (rt)->gcKeepAtoms++;
#define JS_UNKEEP_ATOMS(rt) (rt)->gcKeepAtoms--;

namespace js {

struct AutoResolving;









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
        runtime()->gcHelperThread.freeLater(p);
        return;
    }
    js_free(p);
}

class ForkJoinSlice;

















struct ThreadSafeContext : js::ContextFriendFields,
                           public MallocProvider<ThreadSafeContext>
{
  public:
    enum ContextKind {
        Context_JS,
        Context_ForkJoin
    };

  private:
    ContextKind contextKind_;

  public:
    PerThreadData *perThreadData;

    explicit ThreadSafeContext(JSRuntime *rt, PerThreadData *pt, ContextKind kind);

    bool isJSContext() const;
    JSContext *asJSContext();

    bool isForkJoinSlice() const;
    ForkJoinSlice *asForkJoinSlice();

#ifdef JSGC_GENERATIONAL
    inline bool hasNursery() const {
        return isJSContext();
    }

    inline js::Nursery &nursery() {
        JS_ASSERT(hasNursery());
        return runtime_->gcNursery;
    }
#endif

    
    StaticStrings &staticStrings() { return runtime_->staticStrings; }

    










  protected:
    Allocator *allocator_;

  public:
    static size_t offsetOfAllocator() { return offsetof(ThreadSafeContext, allocator_); }

    inline Allocator *const allocator();

    
    inline AllowGC allowGC() const;

    template <typename T>
    inline bool isInsideCurrentZone(T thing) const;

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtime_->onOutOfMemory(p, nbytes, isJSContext() ? asJSContext() : NULL);
    }
    inline void updateMallocCounter(size_t nbytes) {
        
        runtime_->updateMallocCounter(zone_, nbytes);
    }
    void reportAllocationOverflow() {
        js_ReportAllocationOverflow(isJSContext() ? asJSContext() : NULL);
    }
};

} 

struct JSContext : js::ThreadSafeContext,
                   public mozilla::LinkedListElement<JSContext>
{
    explicit JSContext(JSRuntime *rt);
    ~JSContext();

    JSRuntime *runtime() const { return runtime_; }
    JSCompartment *compartment() const { return compartment_; }

    inline JS::Zone *zone() const {
        JS_ASSERT_IF(!compartment(), !zone_);
        JS_ASSERT_IF(compartment(), js::GetCompartmentZone(compartment()) == zone_);
        return zone_;
    }
    js::PerThreadData &mainThread() const { return runtime()->mainThread; }

  private:
    
    bool                throwing;            
    js::Value           exception;           

    
    unsigned            options_;            

  public:
    int32_t             reportGranularity;  

    js::AutoResolving   *resolvingList;

    
    bool                generatingError;

    inline void setCompartment(JSCompartment *comp);

    














  private:
    unsigned            enterCompartmentDepth_;
  public:
    bool hasEnteredCompartment() const {
        return enterCompartmentDepth_ > 0;
    }
#ifdef DEBUG
    unsigned getEnterCompartmentDepth() const {
        return enterCompartmentDepth_;
    }
#endif

    inline void enterCompartment(JSCompartment *c);
    inline void leaveCompartment(JSCompartment *oldCompartment);

    
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

    



    inline js::Handle<js::GlobalObject*> global() const;

    
    void wrapPendingException();

    
    js::ObjectSet       cycleDetectorSet;

    
    JSErrorReporter     errorReporter;

    
    JSOperationCallback operationCallback;

    
    void                *data;
    void                *data2;

    inline js::RegExpStatics *regExpStatics();

  public:

    







    JSVersion findVersion() const;

    void setOptions(unsigned opts) {
        JS_ASSERT((opts & JSOPTION_MASK) == opts);
        options_ = opts;
    }

    unsigned options() const { return options_; }

    bool hasOption(unsigned opt) const {
        JS_ASSERT((opt & JSOPTION_MASK) == opt);
        return !!(options_ & opt);
    }

    bool hasExtraWarningsOption() const { return hasOption(JSOPTION_EXTRA_WARNINGS); }
    bool hasWErrorOption() const { return hasOption(JSOPTION_WERROR); }

    js::LifoAlloc &tempLifoAlloc() { return runtime()->tempLifoAlloc; }
    inline js::LifoAlloc &analysisLifoAlloc();
    inline js::LifoAlloc &typeLifoAlloc();

    inline js::PropertyTree &propertyTree();

#ifdef JS_THREADSAFE
    unsigned            outstandingRequests;


#endif

    
    unsigned               resolveFlags;

    
    js::Value           iterValue;

    bool jitIsBroken;

    inline bool typeInferenceEnabled() const;

    void updateJITEnabled();

    
    bool currentlyRunning() const;

    bool currentlyRunningInInterpreter() const {
        return mainThread().activation()->isInterpreter();
    }
    bool currentlyRunningInJit() const {
        return mainThread().activation()->isJit();
    }
    js::StackFrame *interpreterFrame() const {
        return mainThread().activation()->asInterpreter()->current();
    }
    js::FrameRegs &interpreterRegs() const {
        return mainThread().activation()->asInterpreter()->regs();
    }

    





    enum MaybeAllowCrossCompartment {
        DONT_ALLOW_CROSS_COMPARTMENT = false,
        ALLOW_CROSS_COMPARTMENT = true
    };
    inline JSScript *currentScript(jsbytecode **pc = NULL,
                                   MaybeAllowCrossCompartment = DONT_ALLOW_CROSS_COMPARTMENT) const;

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

  private:
    
    JSGenerator *innermostGenerator_;
  public:
    JSGenerator *innermostGenerator() const { return innermostGenerator_; }
    void enterGenerator(JSGenerator *gen);
    void leaveGenerator(JSGenerator *gen);

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtime()->onOutOfMemory(p, nbytes, this);
    }
    void updateMallocCounter(size_t nbytes) {
        runtime()->updateMallocCounter(zone(), nbytes);
    }
    void reportAllocationOverflow() {
        js_ReportAllocationOverflow(this);
    }

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

    JSAtomState & names() { return runtime()->atomState; }

#ifdef DEBUG
    



    bool stackIterAssertionEnabled;
#endif

    



    bool runningWithTrustedPrincipals() const;

    JS_FRIEND_API(size_t) sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

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
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : context(cx), object(obj), id(id), kind(kind), link(cx->resolvingList)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
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
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

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

class AutoUnlockGC
{
  private:
#ifdef JS_THREADSAFE
    JSRuntime *rt;
#endif
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoUnlockGC(JSRuntime *rt
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
#ifdef JS_THREADSAFE
      : rt(rt)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        JS_UNLOCK_GC(rt);
    }
    ~AutoUnlockGC() { JS_LOCK_GC(rt); }
};

class MOZ_STACK_CLASS AutoKeepAtoms
{
    JSRuntime *rt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoKeepAtoms(JSRuntime *rt
                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        JS_KEEP_ATOMS(rt);
    }
    ~AutoKeepAtoms() { JS_UNKEEP_ATOMS(rt); }
};





static const unsigned ARGS_LENGTH_MAX = 500 * 1000;

} 

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, unsigned flags
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx), mSaved(cx->resolveFlags)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    unsigned mSaved;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
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

enum ErrorArgumentsType {
    ArgumentsAreUnicode,
    ArgumentsAreASCII
};

inline void
PerThreadData::setIonStackLimit(uintptr_t limit)
{
    JS_ASSERT(runtime_->currentThreadOwnsOperationCallbackLock());
    ionStackLimit = limit;
}

} 

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, unsigned flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, unsigned flags, JSErrorCallback callback,
                       void *userRef, const unsigned errorNumber,
                       js::ErrorArgumentsType argumentsType, va_list ap);

extern bool
js_ReportErrorNumberUCArray(JSContext *cx, unsigned flags, JSErrorCallback callback,
                            void *userRef, const unsigned errorNumber,
                            const jschar **args);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const unsigned errorNumber,
                        char **message, JSErrorReport *reportp,
                        js::ErrorArgumentsType argumentsType, va_list ap);
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

extern const JSErrorFormatString js_ErrorFormatString[JSErr_Limit];

#ifdef JS_THREADSAFE
# define JS_ASSERT_REQUEST_DEPTH(cx)  JS_ASSERT((cx)->runtime()->requestDepth >= 1)
#else
# define JS_ASSERT_REQUEST_DEPTH(cx)  ((void) 0)
#endif





extern JSBool
js_InvokeOperationCallback(JSContext *cx);

extern JSBool
js_HandleExecutionInterrupt(JSContext *cx);






static MOZ_ALWAYS_INLINE bool
JS_CHECK_OPERATION_LIMIT(JSContext *cx)
{
    JS_ASSERT_REQUEST_DEPTH(cx);
    return !cx->runtime()->interrupt || js_InvokeOperationCallback(cx);
}

namespace js {



static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *vec, size_t len)
{
    mozilla::PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *beg, Value *end)
{
    mozilla::PodZero(beg, end - beg);
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
    mozilla::PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Shape **vec, size_t len)
{
    mozilla::PodZero(vec, len);
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

class AutoStringVector : public AutoVectorRooter<JSString *>
{
  public:
    explicit AutoStringVector(JSContext *cx
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSString *>(cx, STRINGVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoShapeVector : public AutoVectorRooter<Shape *>
{
  public:
    explicit AutoShapeVector(JSContext *cx
                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Shape *>(cx, SHAPEVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueArray : public AutoGCRooter
{
    Value *start_;
    unsigned length_;
    SkipRoot skip;

  public:
    AutoValueArray(JSContext *cx, Value *start, unsigned length
                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, VALARRAY), start_(start), length_(length), skip(cx, start, length)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    Value *start() { return start_; }
    unsigned length() const { return length_; }

    MutableHandleValue handleAt(unsigned i)
    {
        JS_ASSERT(i < length_);
        return MutableHandleValue::fromMarkedLocation(&start_[i]);
    }
    HandleValue handleAt(unsigned i) const
    {
        JS_ASSERT(i < length_);
        return HandleValue::fromMarkedLocation(&start_[i]);
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectObjectHashMap : public AutoHashMapRooter<JSObject *, JSObject *>
{
  public:
    explicit AutoObjectObjectHashMap(JSContext *cx
                                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashMapRooter<JSObject *, JSObject *>(cx, OBJOBJHASHMAP)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectUnsigned32HashMap : public AutoHashMapRooter<JSObject *, uint32_t>
{
  public:
    explicit AutoObjectUnsigned32HashMap(JSContext *cx
                                         MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashMapRooter<JSObject *, uint32_t>(cx, OBJU32HASHMAP)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectHashSet : public AutoHashSetRooter<JSObject *>
{
  public:
    explicit AutoObjectHashSet(JSContext *cx
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashSetRooter<JSObject *>(cx, OBJHASHSET)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoAssertNoException
{
#ifdef DEBUG
    JSContext *cx;
    bool hadException;
#endif

  public:
    AutoAssertNoException(JSContext *cx)
#ifdef DEBUG
      : cx(cx),
        hadException(cx->isExceptionPending())
#endif
    {
    }

    ~AutoAssertNoException()
    {
        JS_ASSERT_IF(!hadException, !cx->isExceptionPending());
    }
};












class RuntimeAllocPolicy
{
    JSRuntime *const runtime;

  public:
    RuntimeAllocPolicy(JSRuntime *rt) : runtime(rt) {}
    RuntimeAllocPolicy(JSContext *cx) : runtime(cx->runtime()) {}
    void *malloc_(size_t bytes) { return runtime->malloc_(bytes); }
    void *calloc_(size_t bytes) { return runtime->calloc_(bytes); }
    void *realloc_(void *p, size_t bytes) { return runtime->realloc_(p, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const {}
};




class ContextAllocPolicy
{
    JSContext *const cx_;

  public:
    ContextAllocPolicy(JSContext *cx) : cx_(cx) {}
    JSContext *context() const { return cx_; }
    void *malloc_(size_t bytes) { return cx_->malloc_(bytes); }
    void *calloc_(size_t bytes) { return cx_->calloc_(bytes); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) { return cx_->realloc_(p, oldBytes, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const { js_ReportAllocationOverflow(cx_); }
};


JSBool intrinsic_ToObject(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_IsCallable(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_ThrowError(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_NewDenseArray(JSContext *cx, unsigned argc, Value *vp);

JSBool intrinsic_UnsafeSetElement(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_UnsafeSetReservedSlot(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_UnsafeGetReservedSlot(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_NewObjectWithClassPrototype(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_HaveSameClass(JSContext *cx, unsigned argc, Value *vp);

JSBool intrinsic_ShouldForceSequential(JSContext *cx, unsigned argc, Value *vp);
JSBool intrinsic_NewParallelArray(JSContext *cx, unsigned argc, Value *vp);

#ifdef DEBUG
JSBool intrinsic_Dump(JSContext *cx, unsigned argc, Value *vp);
#endif

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
