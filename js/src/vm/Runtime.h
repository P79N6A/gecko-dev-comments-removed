





#ifndef vm_Runtime_h
#define vm_Runtime_h

#include "mozilla/Atomics.h"
#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Scoped.h"
#include "mozilla/ThreadLocal.h"

#include <setjmp.h>

#include "jsatom.h"
#include "jsclist.h"
#include "jsgc.h"
#ifdef DEBUG
# include "jsproxy.h"
#endif
#include "jsscript.h"

#include "ds/FixedSizeHash.h"
#include "frontend/ParseMaps.h"
#ifdef JSGC_GENERATIONAL
# include "gc/Nursery.h"
#endif
#include "gc/Statistics.h"
#ifdef JSGC_GENERATIONAL
# include "gc/StoreBuffer.h"
#endif
#ifdef XP_MACOSX
# include "jit/AsmJSSignalHandlers.h"
#endif
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/CommonPropertyNames.h"
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

namespace js {

class PerThreadData;
class ThreadSafeContext;


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

namespace WTF { class BumpPointerAllocator; }

namespace js {

typedef Rooted<JSLinearString*> RootedLinearString;

class Activation;
class ActivationIterator;
class AsmJSActivation;
class MathCache;
class WorkerThreadState;

namespace jit {
class IonRuntime;
class JitActivation;
struct PcScriptCache;
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
        nativeStackTop = nullptr;
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
    SourceDataCache() : map_(nullptr) {}
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

    bool lookupType(const Class *clasp, js::types::TypeObject *type, gc::AllocKind kind,
                    EntryIndex *pentry)
    {
        return lookup(clasp, type, kind, pentry);
    }

    




    inline JSObject *newObjectFromHit(JSContext *cx, EntryIndex entry, js::gc::InitialHeap heap);

    
    void fillProto(EntryIndex entry, const Class *clasp, js::TaggedProto proto, gc::AllocKind kind, JSObject *obj);

    inline void fillGlobal(EntryIndex entry, const Class *clasp, js::GlobalObject *global,
                           gc::AllocKind kind, JSObject *obj);

    void fillType(EntryIndex entry, const Class *clasp, js::types::TypeObject *type, gc::AllocKind kind,
                  JSObject *obj)
    {
        JS_ASSERT(obj->type() == type);
        return fill(entry, clasp, type, kind, obj);
    }

    
    void invalidateEntriesForShape(JSContext *cx, HandleShape shape, HandleObject proto);

  private:
    bool lookup(const Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry) {
        uintptr_t hash = (uintptr_t(clasp) ^ uintptr_t(key)) + kind;
        *pentry = hash % mozilla::ArrayLength(entries);

        Entry *entry = &entries[*pentry];

        
        return (entry->clasp == clasp && entry->key == key);
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








class PerThreadData : public PerThreadDataFriendFields,
                      public mozilla::LinkedListElement<PerThreadData>
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
    friend class js::jit::JitActivation;
    friend class js::AsmJSActivation;

    



    js::Activation *activation_;

    
    js::AsmJSActivation *asmJSActivationStack_;

  public:
    js::Activation *const *addressOfActivation() const {
        return &activation_;
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

    







    int32_t suppressGC;

    



    unsigned gcKeepAtoms;

    



    unsigned activeCompilations;

    PerThreadData(JSRuntime *runtime);
    ~PerThreadData();

    bool init();
    void addToThreadList();
    void removeFromThreadList();

    bool associatedWith(const JSRuntime *rt) { return runtime_ == rt; }
    inline JSRuntime *runtimeFromMainThread();
    inline JSRuntime *runtimeIfOnOwnerThread();
};

template<class Client>
struct MallocProvider
{
    void *malloc_(size_t bytes) {
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(bytes);
        void *p = js_malloc(bytes);
        return JS_LIKELY(!!p) ? p : client->onOutOfMemory(nullptr, bytes);
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
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return nullptr;
        }
        return (T *)malloc_(numElems * sizeof(T));
    }

    template <class T>
    T *pod_calloc(size_t numElems, JSCompartment *comp = nullptr, JSContext *cx = nullptr) {
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return nullptr;
        }
        return (T *)calloc_(numElems * sizeof(T));
    }

    JS_DECLARE_NEW_METHODS(new_, malloc_, JS_ALWAYS_INLINE)
};

namespace gc {
class MarkingValidator;
} 

typedef Vector<JS::Zone *, 4, SystemAllocPolicy> ZoneVector;

class AutoLockForExclusiveAccess;
class AutoPauseWorkersForTracing;
class ThreadDataIter;

void RecomputeStackLimit(JSRuntime *rt, StackKind kind);

} 

struct JSRuntime : public JS::shadow::Runtime,
                   public js::MallocProvider<JSRuntime>
{
    









    js::PerThreadData mainThread;

    



    mozilla::LinkedList<js::PerThreadData> threadList;

    



#ifdef JS_THREADSAFE
    mozilla::Atomic<int32_t, mozilla::Relaxed> interrupt;
#else
    int32_t interrupt;
#endif

    
    bool handlingSignal;

    
    JSOperationCallback operationCallback;

  private:
    



#ifdef JS_THREADSAFE
    PRLock *operationCallbackLock;
    PRThread *operationCallbackOwner;
#else
    bool operationCallbackLockTaken;
#endif 
  public:

    class AutoLockForOperationCallback {
        JSRuntime *rt;
      public:
        AutoLockForOperationCallback(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) : rt(rt) {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
            JS_ASSERT(!rt->currentThreadOwnsOperationCallbackLock());
#ifdef JS_THREADSAFE
            PR_Lock(rt->operationCallbackLock);
            rt->operationCallbackOwner = PR_GetCurrentThread();
#else
            rt->operationCallbackLockTaken = true;
#endif 
        }
        ~AutoLockForOperationCallback() {
            JS_ASSERT(rt->currentThreadOwnsOperationCallbackLock());
#ifdef JS_THREADSAFE
            rt->operationCallbackOwner = nullptr;
            PR_Unlock(rt->operationCallbackLock);
#else
            rt->operationCallbackLockTaken = false;
#endif 
        }

        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    };

    bool currentThreadOwnsOperationCallbackLock() {
#if defined(JS_THREADSAFE)
        return operationCallbackOwner == PR_GetCurrentThread();
#else
        return operationCallbackLockTaken;
#endif
    }

#if defined(JS_THREADSAFE) && defined(JS_ION)
# define JS_WORKER_THREADS

    js::WorkerThreadState *workerThreadState;

  private:
    







    PRLock *exclusiveAccessLock;
    mozilla::DebugOnly<PRThread *> exclusiveAccessOwner;
    mozilla::DebugOnly<bool> mainThreadHasExclusiveAccess;
    mozilla::DebugOnly<bool> exclusiveThreadsPaused;

    
    size_t numExclusiveThreads;

    friend class js::AutoLockForExclusiveAccess;
    friend class js::AutoPauseWorkersForTracing;
    friend class js::ThreadDataIter;

  public:
    void setUsedByExclusiveThread(JS::Zone *zone);
    void clearUsedByExclusiveThread(JS::Zone *zone);

#endif 

    bool currentThreadHasExclusiveAccess() {
#if defined(JS_WORKER_THREADS) && defined(DEBUG)
        return (!numExclusiveThreads && mainThreadHasExclusiveAccess) ||
            exclusiveThreadsPaused ||
            exclusiveAccessOwner == PR_GetCurrentThread();
#else
        return true;
#endif
    }

    bool exclusiveThreadsPresent() const {
#ifdef JS_WORKER_THREADS
        return numExclusiveThreads > 0;
#else
        return false;
#endif
    }

    
    JS::Zone            *systemZone;

    
    js::ZoneVector      zones;

    
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
    WTF::BumpPointerAllocator *bumpAlloc_;
    js::jit::IonRuntime *ionRuntime_;

    JSObject *selfHostingGlobal_;

    
    js::InterpreterStack interpreterStack_;

    JSC::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
    WTF::BumpPointerAllocator *createBumpPointerAllocator(JSContext *cx);
    js::jit::IonRuntime *createIonRuntime(JSContext *cx);

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
    js::jit::IonRuntime *getIonRuntime(JSContext *cx) {
        return ionRuntime_ ? ionRuntime_ : createIonRuntime(cx);
    }
    js::jit::IonRuntime *ionRuntime() {
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
    bool maybeWrappedSelfHostedFunction(JSContext *cx, js::HandleId name,
                                        js::MutableHandleValue funVal);

    
    
    

    






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
                gcZeal() == js::gc::ZealGenerationalGCValue ||
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

    void                *gcCallbackData;

  private:
    



    volatile ptrdiff_t  gcMallocBytes;

  public:
    void setNeedsBarrier(bool needs) {
        needsBarrier_ = needs;
    }

    struct ExtraTracer {
        JSTraceDataOp op;
        void *data;

        ExtraTracer()
          : op(nullptr), data(nullptr)
        {}
        ExtraTracer(JSTraceDataOp op, void *data)
          : op(op), data(data)
        {}
    };

    





    typedef js::Vector<ExtraTracer, 4, js::SystemAllocPolicy> ExtraTracerVector;
    ExtraTracerVector   gcBlackRootTracers;
    ExtraTracer         gcGrayRootTracer;

    



    size_t              gcSystemPageSize;

    
    size_t              gcSystemAllocGranularity;

    
    js::ScriptAndCountsVector *scriptAndCountsVector;

    
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

    
    bool                alwaysPreserveCode;

    
    bool                hadOutOfMemory;

    
    bool                haveCreatedContext;

    
    mozilla::LinkedList<js::Debugger> debuggerList;

    



    JSCList             onNewGlobalObjectWatchers;

    
    void                *data;

    
    PRLock              *gcLock;

    js::GCHelperThread  gcHelperThread;

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

    js::GSNCache        gsnCache;
    js::NewObjectCache  newObjectCache;
    js::NativeIterCache nativeIterCache;
    js::SourceDataCache sourceDataCache;
    js::EvalCache       evalCache;
    js::LazyScriptCache lazyScriptCache;

    js::DateTimeInfo    dateTimeInfo;

    js::ConservativeGCData conservativeGC;

    
    
  private:
    js::frontend::ParseMapPool parseMapPool_;
  public:
    js::frontend::ParseMapPool &parseMapPool() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return parseMapPool_;
    }

  private:
    const JSPrincipals  *trustedPrincipals_;
  public:
    void setTrustedPrincipals(const JSPrincipals *p) { trustedPrincipals_ = p; }
    const JSPrincipals *trustedPrincipals() const { return trustedPrincipals_; }

    
    
    
    
  private:
    js::AtomSet atoms_;
    JSCompartment *atomsCompartment_;
    bool beingDestroyed_;
  public:
    js::AtomSet &atoms() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return atoms_;
    }
    JSCompartment *atomsCompartment() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return atomsCompartment_;
    }

    bool isAtomsCompartment(JSCompartment *comp) {
        return comp == atomsCompartment_;
    }

    bool isBeingDestroyed() const {
        return beingDestroyed_;
    }

    
    inline bool isAtomsZone(JS::Zone *zone);

    bool activeGCInAtomsZone();

    union {
        



        JSAtomState atomState;

        js::FixedHeapPtr<js::PropertyName> firstCachedName;
    };

    
    js::StaticStrings   staticStrings;

    JSWrapObjectCallback                   wrapObjectCallback;
    JSSameCompartmentWrapObjectCallback    sameCompartmentWrapObjectCallback;
    JSPreWrapCallback                      preWrapObjectCallback;
    js::PreserveWrapperCallback            preserveWrapperCallback;

    
    
    
  private:
    js::ScriptDataTable scriptDataTable_;
  public:
    js::ScriptDataTable &scriptDataTable() {
        JS_ASSERT(currentThreadHasExclusiveAccess());
        return scriptDataTable_;
    }

#ifdef DEBUG
    size_t              noGCOrAllocationCheck;
#endif

    bool                jitHardening;

    bool                jitSupportsFloatingPoint;

    
    
    void resetIonStackLimit() {
        AutoLockForOperationCallback lock(this);
        mainThread.setIonStackLimit(mainThread.nativeStackLimit[js::StackForUntrustedScript]);
    }

    
    js::jit::PcScriptCache *ionPcScriptCache;

    js::ThreadPool threadPool;

    js::DefaultJSContextCallback defaultJSContextCallback;

    js::CTypesActivityCallback  ctypesActivityCallback;

    
    
    uint32_t parallelWarmup;

  private:
    
    
    
    
    
    
    
    
    
    
    
    
    js::Value            ionReturnOverride_;

    static mozilla::Atomic<size_t> liveRuntimesCount;

  public:
    static bool hasLiveRuntimes() {
        return liveRuntimesCount > 0;
    }

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

    void reportAllocationOverflow() { js_ReportAllocationOverflow(nullptr); }

    bool isTooMuchMalloc() const {
        return gcMallocBytes <= 0;
    }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes);
    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);

    
    
    enum OperationCallbackTrigger {
        TriggerCallbackMainThread,
        TriggerCallbackAnyThread,
        TriggerCallbackAnyThreadDontStopIon
    };

    void triggerOperationCallback(OperationCallbackTrigger trigger);

    void setJitHardening(bool enabled);
    bool getJitHardening() const {
        return jitHardening;
    }

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::RuntimeSizes *runtime);

  private:

    JSUseHelperThreads useHelperThreads_;
    int32_t requestedHelperThreadCount;

    
    bool useHelperThreadsForIonCompilation_;
    bool useHelperThreadsForParsing_;

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
#ifdef JS_WORKER_THREADS
        if (requestedHelperThreadCount < 0) {
            unsigned ncpus = js::GetCPUCount();
            return ncpus == 1 ? 0 : ncpus;
        }
        return requestedHelperThreadCount;
#else
        return 0;
#endif
    }

    void setCanUseHelperThreadsForIonCompilation(bool value) {
        useHelperThreadsForIonCompilation_ = value;
    }
    bool useHelperThreadsForIonCompilation() const {
        return useHelperThreadsForIonCompilation_;
    }

    void setCanUseHelperThreadsForParsing(bool value) {
        useHelperThreadsForParsing_ = value;
    }
    bool useHelperThreadsForParsing() const {
        return useHelperThreadsForParsing_;
    }

#ifdef DEBUG
  public:
    js::AutoEnterPolicy *enteredPolicy;
#endif
};

namespace js {









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
    explicit AutoLockGC(JSRuntime *rt = nullptr
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
    PerThreadData *pt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoKeepAtoms(PerThreadData *pt
                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : pt(pt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        pt->gcKeepAtoms++;
    }
    ~AutoKeepAtoms() {
        pt->gcKeepAtoms--;
    }
};

inline void
PerThreadData::setIonStackLimit(uintptr_t limit)
{
    JS_ASSERT(runtime_->currentThreadOwnsOperationCallbackLock());
    ionStackLimit = limit;
}

inline JSRuntime *
PerThreadData::runtimeFromMainThread()
{
    JS_ASSERT(js::CurrentThreadCanAccessRuntime(runtime_));
    return runtime_;
}

inline JSRuntime *
PerThreadData::runtimeIfOnOwnerThread()
{
    return js::CurrentThreadCanAccessRuntime(runtime_) ? runtime_ : nullptr;
}



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




class ThreadDataIter {
    PerThreadData *iter;

public:
    explicit ThreadDataIter(JSRuntime *rt) {
#ifdef JS_WORKER_THREADS
        
        
        JS_ASSERT(rt->exclusiveThreadsPaused);
#endif
        iter = rt->threadList.getFirst();
    }

    bool done() const {
        return !iter;
    }

    void next() {
        JS_ASSERT(!done());
        iter = iter->getNext();
    }

    PerThreadData *get() const {
        JS_ASSERT(!done());
        return iter;
    }

    operator PerThreadData *() const {
        return get();
    }

    PerThreadData *operator ->() const {
        return get();
    }
};

extern const JSSecurityCallbacks NullSecurityCallbacks;

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
