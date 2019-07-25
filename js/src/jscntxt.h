









































#ifndef jscntxt_h___
#define jscntxt_h___

#include "mozilla/Attributes.h"

#include <string.h>

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"
#include "jsatom.h"
#include "jsclist.h"
#include "jsdhash.h"
#include "jsgc.h"
#include "jsgcchunk.h"
#include "jspropertycache.h"
#include "jspropertytree.h"
#include "jsutil.h"
#include "prmjtime.h"

#include "ds/LifoAlloc.h"
#include "gc/Statistics.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/StackSpace.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif

JS_BEGIN_EXTERN_C
struct DtoaState;
JS_END_EXTERN_C

struct JSSharpObjectMap {
    jsrefcount  depth;
    uint32_t    sharpgen;
    JSHashTable *table;
};

namespace js {

namespace mjit {
class JaegerCompartment;
}

namespace ion {
class IonActivation;
}

class WeakMapBase;
class InterpreterFrames;

class ScriptOpcodeCounts;
struct ScriptOpcodeCountsPair;







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

inline GSNCache *
GetGSNCache(JSContext *cx);

struct PendingProxyOperation {
    PendingProxyOperation   *next;
    JSObject                *object;
};

struct ThreadData {
    JSRuntime           *rt;

    




    volatile int32_t    interruptFlags;

#ifdef JS_THREADSAFE
    
    unsigned            requestDepth;
#endif

    
    StackSpace          stackSpace;

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12;
    LifoAlloc           tempLifoAlloc;

  private:
    



    JSC::ExecutableAllocator    *execAlloc;
    WTF::BumpPointerAllocator   *bumpAlloc;
    js::RegExpPrivateCache      *repCache;

    JSC::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
    WTF::BumpPointerAllocator *createBumpPointerAllocator(JSContext *cx);
    js::RegExpPrivateCache *createRegExpPrivateCache(JSContext *cx);

  public:
    JSC::ExecutableAllocator *getOrCreateExecutableAllocator(JSContext *cx) {
        if (execAlloc)
            return execAlloc;

        return createExecutableAllocator(cx);
    }

    WTF::BumpPointerAllocator *getOrCreateBumpPointerAllocator(JSContext *cx) {
        if (bumpAlloc)
            return bumpAlloc;

        return createBumpPointerAllocator(cx);
    }

    js::RegExpPrivateCache *getRegExpPrivateCache() {
        return repCache;
    }
    js::RegExpPrivateCache *getOrCreateRegExpPrivateCache(JSContext *cx) {
        if (repCache)
            return repCache;

        return createRegExpPrivateCache(cx);
    }

    
    void purgeRegExpPrivateCache();

    



    GSNCache            gsnCache;

    
    PropertyCache       propertyCache;

    
    DtoaState           *dtoaState;

    
    uintptr_t           *nativeStackBase;

    
    PendingProxyOperation *pendingProxyOperation;

    ConservativeGCThreadData conservativeGC;

    
    
    uint8_t             *ionTop;
    JSContext           *ionJSContext;
    uintptr_t            ionStackLimit;

    
    ion::IonActivation  *ionActivation;

#ifdef DEBUG
    size_t              noGCOrAllocationCheck;
#endif

    ThreadData(JSRuntime *rt);
    ~ThreadData();

    bool init();

    void mark(JSTracer *trc);

    void purge(JSContext *cx) {
        tempLifoAlloc.freeUnused();
        gsnCache.purge();

        
        propertyCache.purge(cx);
    }

#ifdef JS_THREADSAFE
    void sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *normal, size_t *temporary,
                             size_t *regexpCode, size_t *stackCommitted);
#endif

    
    void triggerOperationCallback(JSRuntime *rt);

    



    InterpreterFrames *interpreterFrames;
};

} 

#ifdef JS_THREADSAFE





struct JSThread {
    typedef js::HashMap<void *,
                        JSThread *,
                        js::DefaultHasher<void *>,
                        js::SystemAllocPolicy> Map;

    
    JSCList             contextList;

    
    void                *id;

    
    unsigned            suspendCount;

# ifdef DEBUG
    unsigned            checkRequestDepth;
# endif

    
    js::ThreadData      data;

    JSThread(JSRuntime *rt, void *id)
      : id(id),
        suspendCount(0),
# ifdef DEBUG
        checkRequestDepth(0),
# endif
        data(rt)
    {
        JS_INIT_CLIST(&contextList);
    }

    ~JSThread() {
        
        JS_ASSERT(JS_CLIST_IS_EMPTY(&contextList));
    }

    bool init() {
        return data.init();
    }

    JS_FRIEND_API(void) sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *normal,
                                            size_t *temporary, size_t *regexpCode,
                                            size_t *stackCommitted);
};

#define JS_THREAD_DATA(cx)      (&(cx)->thread()->data)

extern JSThread *
js_CurrentThreadAndLockGC(JSRuntime *rt);






extern JSBool
js_InitContextThreadAndLockGC(JSContext *cx);




extern void
js_ClearContextThread(JSContext *cx);

#endif 

typedef enum JSDestroyContextMode {
    JSDCM_NO_GC,
    JSDCM_MAYBE_GC,
    JSDCM_FORCE_GC,
    JSDCM_NEW_FAILED
} JSDestroyContextMode;

typedef enum JSRuntimeState {
    JSRTS_DOWN,
    JSRTS_LAUNCHING,
    JSRTS_UP,
    JSRTS_LANDING
} JSRuntimeState;

typedef struct JSPropertyTreeEntry {
    JSDHashEntryHdr     hdr;
    js::Shape           *child;
} JSPropertyTreeEntry;

namespace js {

typedef Vector<ScriptOpcodeCountsPair, 0, SystemAllocPolicy> ScriptOpcodeCountsVector;

}

struct JSRuntime
{
    
    JSCompartment       *atomsCompartment;

    
    js::CompartmentVector compartments;

    
    JSRuntimeState      state;

    
#ifdef JS_THREADSAFE
  public:
    void clearOwnerThread();
    void setOwnerThread();
    JS_FRIEND_API(bool) onOwnerThread() const;
  private:
    void                *ownerThread_;
  public:
#else
  public:
    bool onOwnerThread() const { return true; }
#endif

    
    JSContextCallback   cxCallback;

    
    JSCompartmentCallback compartmentCallback;

    js::ActivityCallback  activityCallback;
    void                 *activityCallbackArg;

    

    




    js::GCChunkSet      gcChunkSet;

    






    js::gc::Chunk       *gcSystemAvailableChunkListHead;
    js::gc::Chunk       *gcUserAvailableChunkListHead;
    js::gc::ChunkPool   gcChunkPool;

    js::RootedValueMap  gcRootsHash;
    js::GCLocks         gcLocksHash;
    jsrefcount          gcKeepAtoms;
    size_t              gcBytes;
    size_t              gcTriggerBytes;
    size_t              gcLastBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;

    




    volatile uint32_t   gcNumArenasFreeCommitted;
    uint32_t            gcNumber;
    js::GCMarker        *gcIncrementalTracer;
    void                *gcVerifyData;
    bool                gcChunkAllocationSinceLastGC;
    int64_t             gcNextFullGCTime;
    int64_t             gcJitReleaseTime;
    JSGCMode            gcMode;
    volatile uintptr_t  gcBarrierFailed;
    volatile uintptr_t  gcIsNeeded;
    js::WeakMapBase     *gcWeakMapList;
    js::gcstats::Statistics gcStats;

    
    js::gcstats::Reason gcTriggerReason;

    
    uintptr_t           gcMarkStackArray[js::MARK_STACK_LENGTH];

    



    JSCompartment       *gcTriggerCompartment;

    
    JSCompartment       *gcCurrentCompartment;

    



    JSCompartment       *gcCheckCompartment;

    





    bool                gcPoke;
    bool                gcMarkAndSweep;
    bool                gcRunning;

    




















#ifdef JS_GC_ZEAL
    int                 gcZeal_;
    int                 gcZealFrequency;
    int                 gcNextScheduled;
    bool                gcDebugCompartmentGC;

    int gcZeal() { return gcZeal_; }

    bool needZealousGC() {
        if (gcNextScheduled > 0 && --gcNextScheduled == 0) {
            if (gcZeal() >= js::gc::ZealAllocThreshold && gcZeal() < js::gc::ZealVerifierThreshold)
                gcNextScheduled = gcZealFrequency;
            return true;
        }
        return false;
    }
#else
    int gcZeal() { return 0; }
    bool needZealousGC() { return false; }
#endif

    JSGCCallback        gcCallback;
    JSGCFinishedCallback gcFinishedCallback;

  private:
    



    volatile ptrdiff_t  gcMallocBytes;

  public:
    





    JSTraceDataOp       gcBlackRootsTraceOp;
    void                *gcBlackRootsData;
    JSTraceDataOp       gcGrayRootsTraceOp;
    void                *gcGrayRootsData;

    
    js::ScriptOpcodeCountsVector *scriptPCCounters;

    
    js::Value           NaNValue;
    js::Value           negativeInfinityValue;
    js::Value           positiveInfinityValue;

    JSAtom              *emptyString;

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

    
    bool                debugMode;

    
    bool                profilingScripts;

    
    JSBool              hadOutOfMemory;

    



    JSCList             debuggerList;

    
    void                *data;

#ifdef JS_THREADSAFE
    
    PRLock              *gcLock;
    PRCondVar           *gcDone;
    PRCondVar           *requestDone;
    uint32_t            requestCount;
    JSThread            *gcThread;

    js::GCHelperThread  gcHelperThread;

    
    PRLock              *rtLock;
#ifdef DEBUG
    void *              rtLockOwner;
#endif

    
    PRCondVar           *stateChange;

    





    JSThread::Map       threads;
#endif 

    uint32_t            debuggerMutations;

    



    JSSecurityCallbacks *securityCallbacks;

    
    const JSStructuredCloneCallbacks *structuredCloneCallbacks;

    
    JSAccumulateTelemetryDataCallback telemetryCallback;

    




    int32_t             propertyRemovals;

    
    struct JSHashTable  *scriptFilenameTable;
#ifdef JS_THREADSAFE
    PRLock              *scriptFilenameTableLock;
#endif

    
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

    






    JSObject            *anynameObject;
    JSObject            *functionNamespaceObject;

#ifdef JS_THREADSAFE
    
    volatile int32_t    interruptCounter;
#else
    js::ThreadData      threadData;

#define JS_THREAD_DATA(cx)      (&(cx)->runtime->threadData)
#endif

  private:
    JSPrincipals        *trustedPrincipals_;
  public:
    void setTrustedPrincipals(JSPrincipals *p) { trustedPrincipals_ = p; }
    JSPrincipals *trustedPrincipals() const { return trustedPrincipals_; }

    
    JSAtomState         atomState;

    
    js::StaticStrings   staticStrings;

    JSWrapObjectCallback wrapObjectCallback;
    JSPreWrapCallback    preWrapObjectCallback;
    js::PreserveWrapperCallback preserveWrapperCallback;

    




    int32_t             inOOMReport;

    JSRuntime();
    ~JSRuntime();

    bool init(uint32_t maxbytes);

    JSRuntime *thisFromCtor() { return this; }

    void setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind);
    void reduceGCTriggerBytes(size_t amount);

    



    void* malloc_(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(bytes);
        void *p = ::js_malloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(NULL, bytes, cx);
    }

    



    void* calloc_(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(bytes);
        void *p = ::js_calloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(reinterpret_cast<void *>(1), bytes, cx);
    }

    void* realloc_(void* p, size_t oldBytes, size_t newBytes, JSContext *cx = NULL) {
        JS_ASSERT(oldBytes < newBytes);
        updateMallocCounter(newBytes - oldBytes);
        void *p2 = ::js_realloc(p, newBytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, newBytes, cx);
    }

    void* realloc_(void* p, size_t bytes, JSContext *cx = NULL) {
        



        if (!p)
            updateMallocCounter(bytes);
        void *p2 = ::js_realloc(p, bytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, bytes, cx);
    }

    inline void free_(void* p) {
        
        js::Foreground::free_(p);
    }

    JS_DECLARE_NEW_METHODS(malloc_, JS_ALWAYS_INLINE)
    JS_DECLARE_DELETE_METHODS(free_, JS_ALWAYS_INLINE)

    bool isGCMallocLimitReached() const { return gcMallocBytes <= 0; }

    void resetGCMallocBytes() { gcMallocBytes = ptrdiff_t(gcMaxMallocBytes); }

    void setGCMaxMallocBytes(size_t value) {
        



        gcMaxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
        resetGCMallocBytes();
    }

    







    void updateMallocCounter(size_t nbytes) {
        
        ptrdiff_t newCount = gcMallocBytes - ptrdiff_t(nbytes);
        gcMallocBytes = newCount;
        if (JS_UNLIKELY(newCount <= 0))
            onTooMuchMalloc();
    }

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);
};


#define JS_PROPERTY_CACHE(cx)   (JS_THREAD_DATA(cx)->propertyCache)

#define JS_KEEP_ATOMS(rt)   JS_ATOMIC_INCREMENT(&(rt)->gcKeepAtoms);
#define JS_UNKEEP_ATOMS(rt) JS_ATOMIC_DECREMENT(&(rt)->gcKeepAtoms);

#ifdef JS_ARGUMENT_FORMATTER_DEFINED





struct JSArgumentFormatMap {
    const char          *format;
    size_t              length;
    JSArgumentFormatter formatter;
    JSArgumentFormatMap *next;
};
#endif

extern const JSDebugHooks js_NullDebugHooks;  

namespace js {

template <typename T> class Root;
class CheckRoot;

struct AutoResolving;

static inline bool
OptionsHasXML(uint32_t options)
{
    return !!(options & JSOPTION_XML);
}

static inline bool
OptionsSameVersionFlags(uint32_t self, uint32_t other)
{
    static const uint32_t mask = JSOPTION_XML;
    return !((self & mask) ^ (other & mask));
}









namespace VersionFlags {
static const uintN MASK         = 0x0FFF; 
static const uintN HAS_XML      = 0x1000; 
static const uintN FULL_MASK    = 0x3FFF;
}

static inline JSVersion
VersionNumber(JSVersion version)
{
    return JSVersion(uint32_t(version) & VersionFlags::MASK);
}

static inline bool
VersionHasXML(JSVersion version)
{
    return !!(version & VersionFlags::HAS_XML);
}


static inline bool
VersionShouldParseXML(JSVersion version)
{
    return VersionHasXML(version) || VersionNumber(version) >= JSVERSION_1_6;
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

static inline uintN
VersionFlagsToOptions(JSVersion version)
{
    uintN copts = VersionHasXML(version) ? JSOPTION_XML : 0;
    JS_ASSERT((copts & JSCOMPILEOPTION_MASK) == copts);
    return copts;
}

static inline JSVersion
OptionFlagsToVersion(uintN options, JSVersion version)
{
    return VersionSetXML(version, OptionsHasXML(options));
}

static inline bool
VersionIsKnown(JSVersion version)
{
    return VersionNumber(version) != JSVERSION_UNKNOWN;
}

typedef HashSet<JSObject *,
                DefaultHasher<JSObject *>,
                SystemAllocPolicy> BusyArraysSet;

} 

struct JSContext
{
    explicit JSContext(JSRuntime *rt);
    JSContext *thisDuringConstruction() { return this; }
    ~JSContext();

    
    JSCList             link;

  private:
    
    JSVersion           defaultVersion;      
    JSVersion           versionOverride;     
    bool                hasVersionOverride;

    
    JSBool              throwing;           
    js::Value           exception;          

    
    uintN               runOptions;            

  public:
    int32_t             reportGranularity;  

    
    JSLocaleCallbacks   *localeCallbacks;

    js::AutoResolving   *resolvingList;

    



    JSPackedBool        generatingError;

    
    uintptr_t           stackLimit;

    
    JSRuntime *const    runtime;

    
    JSCompartment       *compartment;

    inline void setCompartment(JSCompartment *compartment);

#ifdef JS_THREADSAFE
  private:
    JSThread            *thread_;
  public:
    JSThread *thread() const { return thread_; }

    void setThread(JSThread *thread);
    static const size_t threadOffset() { return offsetof(JSContext, thread_); }
#endif

    
    js::ContextStack    stack;

    
    inline bool hasfp() const;
    inline js::StackFrame* fp() const;
    inline js::StackFrame* maybefp() const;
    inline js::FrameRegs& regs() const;
    inline js::FrameRegs* maybeRegs() const;

    
    void resetCompartment();

    
    void wrapPendingException();

  private:
    
    js::ParseMapPool    *parseMapPool_;

  public:
    
    JSObject            *globalObject;

    
    JSSharpObjectMap    sharpObjectMap;
    js::BusyArraysSet   busyArrays;

    
    JSArgumentFormatMap *argumentFormatMap;

    
    char                *lastMessage;

    
    JSErrorReporter     errorReporter;

    
    JSOperationCallback operationCallback;

    
    void                *data;
    void                *data2;

    inline js::RegExpStatics *regExpStatics();

  public:
    js::ParseMapPool &parseMapPool() {
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

    void setRunOptions(uintN ropts) {
        JS_ASSERT((ropts & JSRUNOPTION_MASK) == ropts);
        runOptions = ropts;
    }

    
    inline void setCompileOptions(uintN newcopts);

    uintN getRunOptions() const { return runOptions; }
    inline uintN getCompileOptions() const;
    inline uintN allOptions() const;

    bool hasRunOption(uintN ropt) const {
        JS_ASSERT((ropt & JSRUNOPTION_MASK) == ropt);
        return !!(runOptions & ropt);
    }

    bool hasStrictOption() const { return hasRunOption(JSOPTION_STRICT); }
    bool hasWErrorOption() const { return hasRunOption(JSOPTION_WERROR); }
    bool hasAtLineOption() const { return hasRunOption(JSOPTION_ATLINE); }
    bool hasJITHardeningOption() const { return !hasRunOption(JSOPTION_SOFTEN); }

    js::LifoAlloc &tempLifoAlloc() { return JS_THREAD_DATA(this)->tempLifoAlloc; }
    inline js::LifoAlloc &typeLifoAlloc();

#ifdef JS_THREADSAFE
    




    bool                atomsCompartmentIsLocked;

    unsigned            outstandingRequests;


    JSCList             threadLinks;        
#endif

    
    js::AutoGCRooter   *autoGCRooters;

#ifdef JSGC_ROOT_ANALYSIS

    



    js::Root<js::gc::Cell*> *thingGCRooters[js::THING_ROOT_COUNT];

#ifdef DEBUG
    







    js::CheckRoot *checkGCRooters;
#endif

#endif 

    
    const JSDebugHooks  *debugHooks;

    
    JSSecurityCallbacks *securityCallbacks;

    
    uintN               resolveFlags;

    
    int64_t             rngSeed;

    
    js::Value           iterValue;

#ifdef JS_METHODJIT
    bool                 methodJitEnabled;

    inline js::mjit::JaegerCompartment *jaegerCompartment();
#endif

    bool                 inferenceEnabled;

    bool typeInferenceEnabled() { return inferenceEnabled; }

    
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

    
    JSObject *enumerators;

  private:
    





    js::Vector<JSGenerator *, 2, js::SystemAllocPolicy> genStack;

  public:
    
    JSGenerator *generatorFor(js::StackFrame *fp) const;

    
    inline bool ensureGeneratorStackSpace();

    bool enterGenerator(JSGenerator *gen) {
        return genStack.append(gen);
    }

    void leaveGenerator(JSGenerator *gen) {
        JS_ASSERT(genStack.back() == gen);
        genStack.popBack();
    }

#ifdef JS_THREADSAFE
    



    js::GCHelperThread *gcBackgroundFree;
#endif

    js::ThreadData *threadData() { return JS_THREAD_DATA(this); }

    inline void* malloc_(size_t bytes) {
        return runtime->malloc_(bytes, this);
    }

    inline void* mallocNoReport(size_t bytes) {
        JS_ASSERT(bytes != 0);
        return runtime->malloc_(bytes, NULL);
    }

    inline void* calloc_(size_t bytes) {
        JS_ASSERT(bytes != 0);
        return runtime->calloc_(bytes, this);
    }

    inline void* realloc_(void* p, size_t bytes) {
        return runtime->realloc_(p, bytes, this);
    }

    inline void* realloc_(void* p, size_t oldBytes, size_t newBytes) {
        return runtime->realloc_(p, oldBytes, newBytes, this);
    }

    inline void free_(void* p) {
#ifdef JS_THREADSAFE
        if (gcBackgroundFree) {
            gcBackgroundFree->freeLater(p);
            return;
        }
#endif
        runtime->free_(p);
    }

    JS_DECLARE_NEW_METHODS(malloc_, inline)
    JS_DECLARE_DELETE_METHODS(free_, inline)

    void purge();

    
    inline void assertValidStackDepth(uintN depth);

    bool isExceptionPending() {
        return throwing;
    }

    js::Value getPendingException() {
        JS_ASSERT(throwing);
        return exception;
    }

    void setPendingException(js::Value v);

    void clearPendingException() {
        this->throwing = false;
        this->exception.setUndefined();
    }

    




    uintN activeCompilations;

#ifdef DEBUG
    



    bool stackIterAssertionEnabled;
#endif

    



    bool runningWithTrustedPrincipals() const;

    JS_FRIEND_API(size_t) sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const;

    static inline JSContext *fromLinkField(JSCList *link) {
        JS_ASSERT(link);
        return reinterpret_cast<JSContext *>(uintptr_t(link) - offsetof(JSContext, link));
    }

#ifdef JS_THREADSAFE
    static inline JSContext *fromThreadLinks(JSCList *link) {
        JS_ASSERT(link);
        return reinterpret_cast<JSContext *>(uintptr_t(link) - offsetof(JSContext, threadLinks));
    }
#endif

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

    AutoResolving(JSContext *cx, JSObject *obj, jsid id, Kind kind = LOOKUP
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
    JSObject            *const object;
    jsid                const id;
    Kind                const kind;
    AutoResolving       *const link;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
















































template <> struct RootMethods<const jsid>
{
    static jsid initial() { return JSID_VOID; }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(jsid id) { return IsPoisonedId(id); }
};

template <> struct RootMethods<jsid>
{
    static jsid initial() { return JSID_VOID; }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(jsid id) { return IsPoisonedId(id); }
};

template <> struct RootMethods<const Value>
{
    static Value initial() { return UndefinedValue(); }
    static ThingRootKind kind() { return THING_ROOT_VALUE; }
    static bool poisoned(const Value &v) { return IsPoisonedValue(v); }
};

template <> struct RootMethods<Value>
{
    static Value initial() { return UndefinedValue(); }
    static ThingRootKind kind() { return THING_ROOT_VALUE; }
    static bool poisoned(const Value &v) { return IsPoisonedValue(v); }
};

template <typename T>
struct RootMethods<T *>
{
    static T *initial() { return NULL; }
    static ThingRootKind kind() { return T::rootKind(); }
    static bool poisoned(T *v) { return IsPoisonedPtr(v); }
};











template <typename T>
class Root
{
  public:
    Root(JSContext *cx, const T *ptr
         JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
#ifdef JSGC_ROOT_ANALYSIS
        ThingRootKind kind = RootMethods<T>::kind();
        this->stack = reinterpret_cast<Root<T>**>(&cx->thingGCRooters[kind]);
        this->prev = *stack;
        *stack = this;
#endif

        JS_ASSERT(!RootMethods<T>::poisoned(*ptr));

        this->ptr = ptr;

        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~Root()
    {
#ifdef JSGC_ROOT_ANALYSIS
        JS_ASSERT(*stack == this);
        *stack = prev;
#endif
    }

#ifdef JSGC_ROOT_ANALYSIS
    Root<T> *previous() { return prev; }
#endif

    const T *address() const { return ptr; }

  private:

#ifdef JSGC_ROOT_ANALYSIS
    Root<T> **stack, *prev;
#endif
    const T *ptr;

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<typename T> template <typename S>
inline
Handle<T>::Handle(const Root<S> &root)
{
    testAssign<S>();
    ptr = reinterpret_cast<const T *>(root.address());
}

typedef Root<JSObject*>          RootObject;
typedef Root<JSFunction*>        RootFunction;
typedef Root<Shape*>             RootShape;
typedef Root<BaseShape*>         RootBaseShape;
typedef Root<types::TypeObject*> RootTypeObject;
typedef Root<JSString*>          RootString;
typedef Root<JSAtom*>            RootAtom;
typedef Root<jsid>               RootId;
typedef Root<Value>              RootValue;


class CheckRoot
{
#if defined(DEBUG) && defined(JSGC_ROOT_ANALYSIS)

    CheckRoot **stack, *prev;
    const uint8_t *ptr;

  public:
    template <typename T>
    CheckRoot(JSContext *cx, const T *ptr
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        this->stack = &cx->checkGCRooters;
        this->prev = *stack;
        *stack = this;
        this->ptr = static_cast<const uint8_t*>(ptr);
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~CheckRoot()
    {
        JS_ASSERT(*stack == this);
        *stack = prev;
    }

    CheckRoot *previous() { return prev; }

    bool contains(const uint8_t *v, size_t len) {
        return ptr >= v && ptr < v + len;
    }

#else 

  public:
    template <typename T>
    CheckRoot(JSContext *cx, const T *ptr
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

#endif 

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};


template <typename T>
class RootedVar
{
  public:
    RootedVar(JSContext *cx)
        : ptr(RootMethods<T>::initial()), root(cx, &ptr)
    {}

    RootedVar(JSContext *cx, T initial)
        : ptr(initial), root(cx, &ptr)
    {}

    operator T () { return ptr; }
    T operator ->() { return ptr; }
    T * address() { return &ptr; }
    const T * address() const { return &ptr; }
    T raw() { return ptr; }

    T & operator =(T value)
    {
        JS_ASSERT(!RootMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

  private:
    T ptr;
    Root<T> root;
};

template <typename T> template <typename S>
inline
Handle<T>::Handle(const RootedVar<S> &root)
{
    ptr = reinterpret_cast<const T *>(root.address());
}

typedef RootedVar<JSObject*>          RootedVarObject;
typedef RootedVar<JSFunction*>        RootedVarFunction;
typedef RootedVar<Shape*>             RootedVarShape;
typedef RootedVar<BaseShape*>         RootedVarBaseShape;
typedef RootedVar<types::TypeObject*> RootedVarTypeObject;
typedef RootedVar<JSString*>          RootedVarString;
typedef RootedVar<JSAtom*>            RootedVarAtom;
typedef RootedVar<jsid>               RootedVarId;
typedef RootedVar<Value>              RootedVarValue;

#ifdef JS_HAS_XML_SUPPORT
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
    friend void JS::MarkRuntime(JSTracer *trc);

  private:
    JSXML * const xml;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

class AutoUnlockGC {
  private:
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoUnlockGC(JSRuntime *rt
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_UNLOCK_GC(rt);
    }
    ~AutoUnlockGC() { JS_LOCK_GC(rt); }
};

class AutoLockAtomsCompartment {
  private:
    JSContext *cx;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoLockAtomsCompartment(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_THREADSAFE
        JS_ASSERT(!cx->atomsCompartmentIsLocked);
        JS_LOCK(cx, &cx->runtime->atomState.lock);
        cx->atomsCompartmentIsLocked = true;
#endif
    }

    ~AutoLockAtomsCompartment() {
#ifdef JS_THREADSAFE
        JS_ASSERT(cx->atomsCompartmentIsLocked);
        cx->atomsCompartmentIsLocked = false;
        JS_UNLOCK(cx, &cx->runtime->atomState.lock);
#endif
    }
};

class AutoUnlockAtomsCompartmentWhenLocked {
    JSContext *cx;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoUnlockAtomsCompartmentWhenLocked(JSContext *cx
                                         JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(NULL)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
 #ifdef JS_THREADSAFE
        if (cx->atomsCompartmentIsLocked) {
            this->cx = cx;
            cx->atomsCompartmentIsLocked = false;
            JS_UNLOCK(cx, &cx->runtime->atomState.lock);
        }
#endif
    }

    ~AutoUnlockAtomsCompartmentWhenLocked() {
#ifdef JS_THREADSAFE
        if (cx) {
            JS_ASSERT(!cx->atomsCompartmentIsLocked);
            JS_LOCK(cx, &cx->runtime->atomState.lock);
            cx->atomsCompartmentIsLocked = true;
        }
#endif
    }
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
    JSContext   *cx;
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleasePtr(const AutoReleasePtr &other) MOZ_DELETE;
    AutoReleasePtr operator=(const AutoReleasePtr &other) MOZ_DELETE;

  public:
    explicit AutoReleasePtr(JSContext *cx, void *ptr
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), ptr(ptr)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoReleasePtr() { cx->free_(ptr); }
};




class AutoReleaseNullablePtr {
    JSContext   *cx;
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleaseNullablePtr(const AutoReleaseNullablePtr &other) MOZ_DELETE;
    AutoReleaseNullablePtr operator=(const AutoReleaseNullablePtr &other) MOZ_DELETE;

  public:
    explicit AutoReleaseNullablePtr(JSContext *cx, void *ptr
                                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), ptr(ptr)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    void reset(void *ptr2) {
        if (ptr)
            cx->free_(ptr);
        ptr = ptr2;
    }
    ~AutoReleaseNullablePtr() { if (ptr) cx->free_(ptr); }
};

} 

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, uintN flags
                       JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx), mSaved(cx->resolveFlags)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    uintN mSaved;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

extern js::ThreadData *
js_CurrentThreadData(JSRuntime *rt);

extern JSBool
js_InitThreads(JSRuntime *rt);

extern void
js_FinishThreads(JSRuntime *rt);

extern void
js_PurgeThreads(JSContext *cx);

extern void
js_PurgeThreads_PostGlobalSweep(JSContext *cx);

namespace js {

#ifdef JS_THREADSAFE


class ThreadDataIter : public JSThread::Map::Range
{
  public:
    ThreadDataIter(JSRuntime *rt) : JSThread::Map::Range(rt->threads.all()) {}

    ThreadData *threadData() const {
        return &front().value->data;
    }
};

#else 

class ThreadDataIter
{
    JSRuntime *runtime;
    bool done;
  public:
    ThreadDataIter(JSRuntime *rt) : runtime(rt), done(false) {}

    bool empty() const {
        return done;
    }

    void popFront() {
        JS_ASSERT(!done);
        done = true;
    }

    ThreadData *threadData() const {
        JS_ASSERT(!done);
        return &runtime->threadData;
    }
};

#endif  





class ThreadContextRange {
    JSCList *begin;
    JSCList *end;

public:
    explicit ThreadContextRange(JSContext *cx) {
#ifdef JS_THREADSAFE
        end = &cx->thread()->contextList;
#else
        end = &cx->runtime->contextList;
#endif
        begin = end->next;
    }

    bool empty() const { return begin == end; }
    void popFront() { JS_ASSERT(!empty()); begin = begin->next; }

    JSContext *front() const {
#ifdef JS_THREADSAFE
        return JSContext::fromThreadLinks(begin);
#else
        return JSContext::fromLinkField(begin);
#endif
    }
};

} 





extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode);





extern JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp);






extern JS_FRIEND_API(JSContext *)
js_NextActiveContext(JSRuntime *, JSContext *);

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **message, JSErrorReport *reportp,
                        bool charArgs, va_list ap);
#endif

extern void
js_ReportOutOfMemory(JSContext *cx);

extern JS_FRIEND_API(void)
js_ReportAllocationOverflow(JSContext *cx);





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);




extern JSBool
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, const js::Value &v,
                           JSString *fallback);

extern void
js_ReportMissingArg(JSContext *cx, const js::Value &v, uintN arg);






extern JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, const js::Value &v, JSString *fallback,
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
# define JS_ASSERT_REQUEST_DEPTH(cx)  (JS_ASSERT((cx)->thread()),             \
                                       JS_ASSERT((cx)->thread()->data.requestDepth >= 1))
#else
# define JS_ASSERT_REQUEST_DEPTH(cx)  ((void) 0)
#endif






#define JS_CHECK_OPERATION_LIMIT(cx)                                          \
    (JS_ASSERT_REQUEST_DEPTH(cx),                                             \
     (!JS_THREAD_DATA(cx)->interruptFlags || js_InvokeOperationCallback(cx)))





extern JSBool
js_InvokeOperationCallback(JSContext *cx);

extern JSBool
js_HandleExecutionInterrupt(JSContext *cx);

namespace js {



void
TriggerOperationCallback(JSContext *cx);

void
TriggerAllOperationCallbacks(JSRuntime *rt);

} 






extern js::StackFrame *
js_GetScriptedCaller(JSContext *cx, js::StackFrame *fp);

extern jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx);

extern JSScript *
js_GetCurrentScript(JSContext* cx);

namespace js {

#ifdef JS_METHODJIT
namespace mjit {
    void ExpandInlineFrames(JSCompartment *compartment);
}
#endif

} 


enum FrameExpandKind {
    FRAME_EXPAND_NONE = 0,
    FRAME_EXPAND_ALL = 1
};

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
MakeRangeGCSafe(const Shape **beg, const Shape **end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(const Shape **vec, size_t len)
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

class AutoShapeVector : public AutoVectorRooter<const Shape *>
{
  public:
    explicit AutoShapeVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<const Shape *>(cx, SHAPEVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueArray : public AutoGCRooter
{
    const js::Value *start_;
    unsigned length_;

  public:
    AutoValueArray(JSContext *cx, const js::Value *start, unsigned length
                   JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, VALARRAY), start_(start), length_(length)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    const Value *start() const { return start_; }
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
    void *realloc_(void *p, size_t bytes) { return runtime->realloc_(p, bytes); }
    void free_(void *p) { runtime->free_(p); }
    void reportAllocOverflow() const {}
};




class ContextAllocPolicy
{
    JSContext *const cx;

  public:
    ContextAllocPolicy(JSContext *cx) : cx(cx) {}
    JSContext *context() const { return cx; }
    void *malloc_(size_t bytes) { return cx->malloc_(bytes); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) { return cx->realloc_(p, oldBytes, bytes); }
    void free_(void *p) { cx->free_(p); }
    void reportAllocOverflow() const { js_ReportAllocationOverflow(cx); }
};

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
