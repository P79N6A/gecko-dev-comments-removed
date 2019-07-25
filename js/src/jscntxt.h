









































#ifndef jscntxt_h___
#define jscntxt_h___

#include "mozilla/Attributes.h"

#include <string.h>

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"
#include "jsatom.h"
#include "jsclist.h"
#include "jsgc.h"
#include "jspropertycache.h"
#include "jspropertytree.h"
#include "jsutil.h"
#include "prmjtime.h"

#include "ds/LifoAlloc.h"
#include "gc/Statistics.h"
#include "js/HashTable.h"
#include "js/Vector.h"
#include "vm/Stack.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif

JS_BEGIN_EXTERN_C
struct DtoaState;
JS_END_EXTERN_C

struct JSSharpInfo {
    bool hasGen;
    bool isSharp;

    JSSharpInfo() : hasGen(false), isSharp(false) {}
};

typedef js::HashMap<JSObject *, JSSharpInfo> JSSharpTable;

struct JSSharpObjectMap {
    unsigned     depth;
    uint32_t     sharpgen;
    JSSharpTable table;

    JSSharpObjectMap(JSContext *cx) : depth(0), sharpgen(0), table(js::TempAllocPolicy(cx)) {
        table.init();
    }
};

namespace js {

namespace mjit {
class JaegerCompartment;
}

class WeakMapBase;
class InterpreterFrames;







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

typedef Vector<ScriptAndCounts, 0, SystemAllocPolicy> ScriptAndCountsVector;

struct ConservativeGCData
{
    



    uintptr_t           *nativeStackTop;

    union {
        jmp_buf         jmpbuf;
        uintptr_t       words[JS_HOWMANY(sizeof(jmp_buf), sizeof(uintptr_t))];
    } registerSnapshot;

    




    unsigned requestThreshold;

    ConservativeGCData()
      : nativeStackTop(NULL), requestThreshold(0)
    {}

    ~ConservativeGCData() {
#ifdef JS_THREADSAFE
        



        JS_ASSERT(!hasStackToScan());
#endif
    }

    JS_NEVER_INLINE void recordStackTop();

#ifdef JS_THREADSAFE
    void updateForRequestEnd(unsigned suspendCount) {
        if (suspendCount)
            recordStackTop();
        else
            nativeStackTop = NULL;
    }
#endif

    bool hasStackToScan() const {
        return !!nativeStackTop;
    }
};

} 

struct JSRuntime : js::RuntimeFriendFields
{
    
    JSCompartment       *atomsCompartment;

    
    js::CompartmentVector compartments;

    
#ifdef JS_THREADSAFE
  public:
    void *ownerThread() const { return ownerThread_; }
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

    
    js::StackSpace stackSpace;

    
    static const size_t TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12;
    js::LifoAlloc tempLifoAlloc;

  private:
    



    JSC::ExecutableAllocator *execAlloc_;
    WTF::BumpPointerAllocator *bumpAlloc_;

    JSC::ExecutableAllocator *createExecutableAllocator(JSContext *cx);
    WTF::BumpPointerAllocator *createBumpPointerAllocator(JSContext *cx);

  public:
    JSC::ExecutableAllocator *getExecutableAllocator(JSContext *cx) {
        return execAlloc_ ? execAlloc_ : createExecutableAllocator(cx);
    }
    WTF::BumpPointerAllocator *getBumpPointerAllocator(JSContext *cx) {
        return bumpAlloc_ ? bumpAlloc_ : createBumpPointerAllocator(cx);
    }

    
    uintptr_t           nativeStackBase;

    
    size_t              nativeStackQuota;

    



    js::InterpreterFrames *interpreterFrames;

    
    JSContextCallback   cxCallback;

    
    JSCompartmentCallback compartmentCallback;

    js::ActivityCallback  activityCallback;
    void                 *activityCallbackArg;

#ifdef JS_THREADSAFE
    
    unsigned            suspendCount;

    
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
    void                *gcVerifyData;
    bool                gcChunkAllocationSinceLastGC;
    int64_t             gcNextFullGCTime;
    int64_t             gcJitReleaseTime;
    JSGCMode            gcMode;

    




    volatile uintptr_t  gcIsNeeded;
    volatile uintptr_t  gcFullIsNeeded;

    js::WeakMapBase     *gcWeakMapList;
    js::gcstats::Statistics gcStats;

    
    uint64_t            gcNumber;

    
    uint64_t            gcStartNumber;

    
    js::gcreason::Reason gcTriggerReason;

    
    bool                gcIsFull;

    



    bool                gcStrictCompartmentChecking;

    



    js::gc::State       gcIncrementalState;

    
    bool                gcCompartmentCreated;

    
    bool                gcLastMarkSlice;

    
    bool                gcIncrementalIsFull;

    




    volatile uintptr_t  gcInterFrameGC;

    
    int64_t             gcSliceBudget;

    



    bool                gcIncrementalEnabled;

    





#ifdef DEBUG
    struct SavedGCRoot {
        void *thing;
        JSGCTraceKind kind;

        SavedGCRoot(void *thing, JSGCTraceKind kind) : thing(thing), kind(kind) {}
    };
    js::Vector<SavedGCRoot, 0, js::SystemAllocPolicy> gcSavedRoots;
#endif







    bool                gcPoke;
    bool                gcRunning;

    



















#ifdef JS_GC_ZEAL
    int                 gcZeal_;
    int                 gcZealFrequency;
    int                 gcNextScheduled;
    bool                gcDebugCompartmentGC;
    bool                gcDeterministicOnly;

    int gcZeal() { return gcZeal_; }

    bool needZealousGC() {
        if (gcNextScheduled > 0 && --gcNextScheduled == 0) {
            if (gcZeal() == js::gc::ZealAllocValue)
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
    js::GCSliceCallback gcSliceCallback;
    JSFinalizeCallback  gcFinalizeCallback;

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

    JSAtom              *emptyString;

    
    JSCList             contextList;

    bool hasContexts() const {
        return !JS_CLIST_IS_EMPTY(&contextList);
    }

    
    JSDebugHooks        debugHooks;

    
    bool                debugMode;

    
    bool                profilingScripts;

    
    JSBool              hadOutOfMemory;

    



    JSCList             debuggerList;

    
    void                *data;

#ifdef JS_THREADSAFE
    
    PRLock              *gcLock;

    js::GCHelperThread  gcHelperThread;
#endif 

    uint32_t            debuggerMutations;

    const JSSecurityCallbacks *securityCallbacks;
    JSDestroyPrincipalsOp destroyPrincipals;

    
    const JSStructuredCloneCallbacks *structuredCloneCallbacks;

    
    JSAccumulateTelemetryDataCallback telemetryCallback;

    




    int32_t             propertyRemovals;

    
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

    






    JSObject            *anynameObject;
    JSObject            *functionNamespaceObject;

    



    bool                waiveGCQuota;

    



    js::GSNCache        gsnCache;

    
    js::PropertyCache   propertyCache;

    
    DtoaState           *dtoaState;

    
    js::PendingProxyOperation *pendingProxyOperation;

    js::ConservativeGCData conservativeGC;

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

#ifdef DEBUG
    size_t              noGCOrAllocationCheck;
#endif

    




    int32_t             inOOMReport;

    bool                jitHardening;

    JSRuntime();
    ~JSRuntime();

    bool init(uint32_t maxbytes);

    JSRuntime *thisFromCtor() { return this; }

    



    void* malloc_(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(cx, bytes);
        void *p = ::js_malloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(NULL, bytes, cx);
    }

    



    void* calloc_(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(cx, bytes);
        void *p = ::js_calloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(reinterpret_cast<void *>(1), bytes, cx);
    }

    void* realloc_(void* p, size_t oldBytes, size_t newBytes, JSContext *cx = NULL) {
        JS_ASSERT(oldBytes < newBytes);
        updateMallocCounter(cx, newBytes - oldBytes);
        void *p2 = ::js_realloc(p, newBytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, newBytes, cx);
    }

    void* realloc_(void* p, size_t bytes, JSContext *cx = NULL) {
        



        if (!p)
            updateMallocCounter(cx, bytes);
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

    







    void updateMallocCounter(JSContext *cx, size_t nbytes);

    


    JS_FRIEND_API(void) onTooMuchMalloc();

    







    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);

    void triggerOperationCallback();

    void setJitHardening(bool enabled);
    bool getJitHardening() const {
        return jitHardening;
    }

    void sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *normal, size_t *temporary,
                             size_t *regexpCode, size_t *stackCommitted, size_t *gcMarker);
};


#define JS_PROPERTY_CACHE(cx)   (cx->runtime->propertyCache)

#define JS_KEEP_ATOMS(rt)   (rt)->gcKeepAtoms++;
#define JS_UNKEEP_ATOMS(rt) (rt)->gcKeepAtoms--;

#ifdef JS_ARGUMENT_FORMATTER_DEFINED





struct JSArgumentFormatMap {
    const char          *format;
    size_t              length;
    JSArgumentFormatter formatter;
    JSArgumentFormatMap *next;
};
#endif

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
static const unsigned MASK         = 0x0FFF; 
static const unsigned HAS_XML      = 0x1000; 
static const unsigned FULL_MASK    = 0x3FFF;
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

static inline unsigned
VersionFlagsToOptions(JSVersion version)
{
    unsigned copts = VersionHasXML(version) ? JSOPTION_XML : 0;
    JS_ASSERT((copts & JSCOMPILEOPTION_MASK) == copts);
    return copts;
}

static inline JSVersion
OptionFlagsToVersion(unsigned options, JSVersion version)
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

struct JSContext : js::ContextFriendFields
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

    
    unsigned            runOptions;          

  public:
    int32_t             reportGranularity;  

    
    JSLocaleCallbacks   *localeCallbacks;

    js::AutoResolving   *resolvingList;

    
    bool                generatingError;

    
    JSCompartment       *compartment;

    inline void setCompartment(JSCompartment *compartment);

    
    js::ContextStack    stack;

    
    inline bool hasfp() const               { return stack.hasfp(); }
    inline js::StackFrame* fp() const       { return stack.fp(); }
    inline js::StackFrame* maybefp() const  { return stack.maybefp(); }
    inline js::FrameRegs& regs() const      { return stack.regs(); }
    inline js::FrameRegs* maybeRegs() const { return stack.maybeRegs(); }

    
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
    inline js::LifoAlloc &typeLifoAlloc();

#ifdef JS_THREADSAFE
    unsigned            outstandingRequests;


#endif


#ifdef JSGC_ROOT_ANALYSIS

    



    js::Root<js::gc::Cell*> *thingGCRooters[js::THING_ROOT_COUNT];

#ifdef DEBUG
    







    js::CheckRoot *checkGCRooters;
#endif

#endif 

    
    unsigned               resolveFlags;

    
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

    
    inline void assertValidStackDepth(unsigned depth);

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

    




    unsigned activeCompilations;

#ifdef DEBUG
    



    bool stackIterAssertionEnabled;
#endif

    



    bool runningWithTrustedPrincipals() const;

    JS_FRIEND_API(size_t) sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const;

    static inline JSContext *fromLinkField(JSCList *link) {
        JS_ASSERT(link);
        return reinterpret_cast<JSContext *>(uintptr_t(link) - offsetof(JSContext, link));
    }

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

  private:
    JSXML * const xml;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

#ifdef JS_THREADSAFE
# define JS_LOCK_GC(rt)    PR_Lock((rt)->gcLock)
# define JS_UNLOCK_GC(rt)  PR_Unlock((rt)->gcLock)
#else
# define JS_LOCK_GC(rt)
# define JS_UNLOCK_GC(rt)
#endif

class AutoLockGC
{
  public:
    explicit AutoLockGC(JSRuntime *rt = NULL
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : runtime(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        
#ifdef JS_THREADSAFE
        if (rt)
            JS_LOCK_GC(rt);
#endif
    }

    ~AutoLockGC()
    {
#ifdef JS_THREADSAFE
        if (runtime)
            JS_UNLOCK_GC(runtime);
#endif
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
    JSCList *begin;
    JSCList *end;

public:
    explicit ContextIter(JSRuntime *rt) {
        end = &rt->contextList;
        begin = end->next;
    }

    bool done() const {
        return begin == end;
    }

    void next() {
        JS_ASSERT(!done());
        begin = begin->next;
    }

    JSContext *get() const {
        JS_ASSERT(!done());
        return JSContext::fromLinkField(begin);
    }

    operator JSContext *() const {
        return get();
    }

    JSContext *operator ->() const {
        return get();
    }
};

} 





extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

typedef enum JSDestroyContextMode {
    JSDCM_NO_GC,
    JSDCM_MAYBE_GC,
    JSDCM_FORCE_GC,
    JSDCM_NEW_FAILED
} JSDestroyContextMode;

extern void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode);

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, unsigned flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, unsigned flags, JSErrorCallback callback,
                       void *userRef, const unsigned errorNumber,
                       JSBool charArgs, va_list ap);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const unsigned errorNumber,
                        char **message, JSErrorReport *reportp,
                        bool charArgs, va_list ap);
#endif

namespace js {


extern void
ReportUsageError(JSContext *cx, JSObject *callee, const char *msg);

} 

extern void
js_ReportOutOfMemory(JSContext *cx);

extern JS_FRIEND_API(void)
js_ReportAllocationOverflow(JSContext *cx);





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);




extern JSBool
js_ReportIsNullOrUndefined(JSContext *cx, int spindex, const js::Value &v,
                           JSString *fallback);

extern void
js_ReportMissingArg(JSContext *cx, const js::Value &v, unsigned arg);






extern JSBool
js_ReportValueErrorFlags(JSContext *cx, unsigned flags, const unsigned errorNumber,
                         int spindex, const js::Value &v, JSString *fallback,
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






#define JS_CHECK_OPERATION_LIMIT(cx)                                          \
    (JS_ASSERT_REQUEST_DEPTH(cx),                                             \
     (!cx->runtime->interrupt || js_InvokeOperationCallback(cx)))





extern JSBool
js_InvokeOperationCallback(JSContext *cx);

extern JSBool
js_HandleExecutionInterrupt(JSContext *cx);

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
    js::Value *start_;
    unsigned length_;

  public:
    AutoValueArray(JSContext *cx, js::Value *start, unsigned length
                   JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, VALARRAY), start_(start), length_(length)
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
