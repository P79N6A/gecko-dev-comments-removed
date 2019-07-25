







































#ifndef jscntxt_h___
#define jscntxt_h___



#include <string.h>

#include "jsprvtd.h"
#include "jsarena.h"
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsdhash.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcchunk.h"
#include "jshashtable.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jspropertycache.h"
#include "jspropertytree.h"
#include "jsstaticcheck.h"
#include "jsutil.h"
#include "jsvector.h"
#include "prmjtime.h"

#include "vm/Stack.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif


namespace nanojit {

class Assembler;
class CodeAlloc;
class Fragment;
template<typename K> struct DefaultHash;
template<typename K, typename V, typename H> class HashMap;
template<typename T> class Seq;

}  

JS_BEGIN_EXTERN_C
struct DtoaState;
JS_END_EXTERN_C

namespace js {


static const size_t MONITOR_N_GLOBAL_STATES = 4;
static const size_t FRAGMENT_TABLE_SIZE = 512;
static const size_t MAX_GLOBAL_SLOTS = 4096;
static const size_t GLOBAL_SLOTS_BUFFER_SIZE = MAX_GLOBAL_SLOTS + 1;


class VMAllocator;
class FrameInfoCache;
struct FrameInfo;
struct VMSideExit;
struct TreeFragment;
struct TracerState;
template<typename T> class Queue;
typedef Queue<uint16> SlotList;
class TypeMap;
class LoopProfile;

#if defined(JS_JIT_SPEW) || defined(DEBUG)
struct FragPI;
typedef nanojit::HashMap<uint32, FragPI, nanojit::DefaultHash<uint32> > FragStatsMap;
#endif

namespace mjit {
class JaegerCompartment;
}

class WeakMapBase;







struct GSNCache {
    typedef HashMap<jsbytecode *,
                    jssrcnote *,
                    PointerHasher<jsbytecode *, 0>,
                    SystemAllocPolicy> Map;

    jsbytecode      *code;
    Map             map;
#ifdef JS_GSNMETER
    struct Stats {
        uint32          hits;
        uint32          misses;
        uint32          fills;
        uint32          purges;

        Stats() : hits(0), misses(0), fills(0), purges(0) { }
    };

    Stats           stats;
#endif

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
    




    volatile int32      interruptFlags;

#ifdef JS_THREADSAFE
    
    unsigned            requestDepth;
#endif

#ifdef JS_TRACER
    







    JSCompartment       *onTraceCompartment;
    JSCompartment       *recordingCompartment;
    JSCompartment       *profilingCompartment;

    
    uint32              maxCodeCacheBytes;

    static const uint32 DEFAULT_JIT_CACHE_SIZE = 16 * 1024 * 1024;
#endif

    
    StackSpace          stackSpace;

    



    bool                waiveGCQuota;

    



    GSNCache            gsnCache;

    
    PropertyCache       propertyCache;

    
    DtoaState           *dtoaState;

    
    jsuword             *nativeStackBase;

    
    PendingProxyOperation *pendingProxyOperation;

    ConservativeGCThreadData conservativeGC;

    ThreadData();
    ~ThreadData();

    bool init();
    
    void mark(JSTracer *trc) {
        stackSpace.mark(trc);
    }

    void purge(JSContext *cx) {
        gsnCache.purge();

        
        propertyCache.purge(cx);
    }

    
    void triggerOperationCallback(JSRuntime *rt);
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

    JSThread(void *id)
      : id(id),
        suspendCount(0)
# ifdef DEBUG
      , checkRequestDepth(0)
# endif        
    {
        JS_INIT_CLIST(&contextList);
    }

    ~JSThread() {
        
        JS_ASSERT(JS_CLIST_IS_EMPTY(&contextList));
    }

    bool init() {
        return data.init();
    }
};

#define JS_THREAD_DATA(cx)      (&(cx)->thread()->data)

extern JSThread *
js_CurrentThreadAndLockGC(JSRuntime *rt);






extern JSBool
js_InitContextThreadAndLockGC(JSContext *cx);




extern void
js_ClearContextThread(JSContext *cx);

#endif 

#ifdef DEBUG
# define FUNCTION_KIND_METER_LIST(_)                                          \
                        _(allfun), _(heavy), _(nofreeupvar), _(onlyfreevar),  \
                        _(flat), _(badfunarg),                                \
                        _(joinedsetmethod), _(joinedinitmethod),              \
                        _(joinedreplace), _(joinedsort), _(joinedmodulepat),  \
                        _(mreadbarrier), _(mwritebarrier), _(mwslotbarrier),  \
                        _(unjoined), _(indynamicscope)
# define identity(x)    x

struct JSFunctionMeter {
    int32 FUNCTION_KIND_METER_LIST(identity);
};

# undef identity

# define JS_FUNCTION_METER(cx,x) JS_RUNTIME_METER((cx)->runtime, functionMeter.x)
#else
# define JS_FUNCTION_METER(cx,x) ((void)0)
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

typedef void
(* JSActivityCallback)(void *arg, JSBool active);

namespace js {

typedef js::Vector<JSCompartment *, 0, js::SystemAllocPolicy> CompartmentVector;

}

struct JSRuntime {
    
    JSCompartment       *atomsCompartment;
#ifdef JS_THREADSAFE
    bool                atomsCompartmentIsLocked;
#endif

    
    js::CompartmentVector compartments;

    
    JSRuntimeState      state;

    
    JSContextCallback   cxCallback;

    
    JSCompartmentCallback compartmentCallback;

    





    void setActivityCallback(JSActivityCallback cb, void *arg) {
        activityCallback = cb;
        activityCallbackArg = arg;
    }

    JSActivityCallback    activityCallback;
    void                 *activityCallbackArg;

    











    uint32              protoHazardShape;

    
    js::GCChunkSet      gcChunkSet;

    js::RootedValueMap  gcRootsHash;
    js::GCLocks         gcLocksHash;
    jsrefcount          gcKeepAtoms;
    uint32              gcBytes;
    uint32              gcTriggerBytes;
    size_t              gcLastBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;
    size_t              gcChunksWaitingToExpire;
    uint32              gcEmptyArenaPoolLifespan;
    uint32              gcNumber;
    js::GCMarker        *gcMarkingTracer;
    int64               gcJitReleaseTime;
    JSGCMode            gcMode;
    volatile bool       gcIsNeeded;
    js::WeakMapBase     *gcWeakMapList;

    
    void                *gcMarkStackObjs[js::OBJECT_MARK_STACK_SIZE / sizeof(void *)];
    void                *gcMarkStackRopes[js::ROPES_MARK_STACK_SIZE / sizeof(void *)];
    void                *gcMarkStackXMLs[js::XML_MARK_STACK_SIZE / sizeof(void *)];
    void                *gcMarkStackLarges[js::LARGE_MARK_STACK_SIZE / sizeof(void *)];

    



    JSCompartment       *gcTriggerCompartment;

    
    JSCompartment       *gcCurrentCompartment;

    





    bool                gcPoke;
    bool                gcMarkAndSweep;
    bool                gcRunning;
    bool                gcRegenShapes;

    

















#ifdef JS_GC_ZEAL
    int                 gcZeal_;
    int                 gcZealFrequency;
    int                 gcNextScheduled;
    bool                gcDebugCompartmentGC;

    int gcZeal() { return gcZeal_; }

    bool needZealousGC() {
        if (gcNextScheduled > 0 && --gcNextScheduled == 0) {
            if (gcZeal() >= 2)
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

  private:
    



    volatile ptrdiff_t  gcMallocBytes;

  public:
    js::GCChunkAllocator    *gcChunkAllocator;

    void setCustomGCChunkAllocator(js::GCChunkAllocator *allocator) {
        JS_ASSERT(allocator);
        JS_ASSERT(state == JSRTS_DOWN);
        gcChunkAllocator = allocator;
    }

    



    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    
    js::Value           NaNValue;
    js::Value           negativeInfinityValue;
    js::Value           positiveInfinityValue;

    JSFlatString        *emptyString;

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

    


    JSBool              debugMode;

#ifdef JS_TRACER
    
    bool debuggerInhibitsJIT() const {
        return (globalDebugHooks.interruptHook ||
                globalDebugHooks.callHook);
    }
#endif

    
    JSCList             trapList;
    JSCList             watchPointList;

    
    void                *data;

#ifdef JS_THREADSAFE
    
    PRLock              *gcLock;
    PRCondVar           *gcDone;
    PRCondVar           *requestDone;
    uint32              requestCount;
    JSThread            *gcThread;

    js::GCHelperThread  gcHelperThread;

    
    PRLock              *rtLock;
#ifdef DEBUG
    void *              rtLockOwner;
#endif

    
    PRCondVar           *stateChange;

    





    PRLock              *debuggerLock;

    JSThread::Map       threads;
#endif 
    uint32              debuggerMutations;

    



    JSSecurityCallbacks *securityCallbacks;

    
    const JSStructuredCloneCallbacks *structuredCloneCallbacks;

    




    int32               propertyRemovals;

    
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
    
    volatile int32      interruptCounter;
#else
    js::ThreadData      threadData;

#define JS_THREAD_DATA(cx)      (&(cx)->runtime->threadData)
#endif

    












    volatile uint32     shapeGen;

    
    JSAtomState         atomState;

    




#ifdef JS_DUMP_ENUM_CACHE_STATS
    int32               nativeEnumProbes;
    int32               nativeEnumMisses;
# define ENUM_CACHE_METER(name)     JS_ATOMIC_INCREMENT(&cx->runtime->name)
#else
# define ENUM_CACHE_METER(name)     ((void) 0)
#endif

#ifdef DEBUG
    
    jsrefcount          inlineCalls;
    jsrefcount          nativeCalls;
    jsrefcount          nonInlineCalls;
    jsrefcount          constructs;

    






    const char          *propTreeStatFilename;
    const char          *propTreeDumpFilename;

    bool meterEmptyShapes() const { return propTreeStatFilename || propTreeDumpFilename; }

    
    jsrefcount          liveStrings;
    jsrefcount          totalStrings;
    jsrefcount          liveDependentStrings;
    jsrefcount          totalDependentStrings;
    jsrefcount          badUndependStrings;
    double              lengthSum;
    double              lengthSquaredSum;
    double              strdepLengthSum;
    double              strdepLengthSquaredSum;

    
    jsrefcount          liveScripts;
    jsrefcount          totalScripts;
    jsrefcount          liveEmptyScripts;
    jsrefcount          totalEmptyScripts;
    jsrefcount          highWaterLiveScripts;
#endif 

#ifdef JS_SCOPE_DEPTH_METER
    



    JSBasicStats        protoLookupDepthStats;
    JSBasicStats        scopeSearchDepthStats;

    



    JSBasicStats        hostenvScopeDepthStats;
    JSBasicStats        lexicalScopeDepthStats;
#endif

#ifdef JS_GCMETER
    js::gc::JSGCStats           gcStats;
    js::gc::JSGCArenaStats      globalArenaStats[js::gc::FINALIZE_LIMIT];
#endif

#ifdef DEBUG
    



    const char          *functionMeterFilename;
    JSFunctionMeter     functionMeter;
    char                lastScriptFilename[1024];

    typedef js::HashMap<JSFunction *,
                        int32,
                        js::DefaultHasher<JSFunction *>,
                        js::SystemAllocPolicy> FunctionCountMap;

    FunctionCountMap    methodReadBarrierCountMap;
    FunctionCountMap    unjoinedFunctionCountMap;
#endif

    JSWrapObjectCallback wrapObjectCallback;
    JSPreWrapCallback    preWrapObjectCallback;

#ifdef JS_METHODJIT
    
    size_t               mjitDataSize;
#endif

    




    int32               inOOMReport;

#if defined(MOZ_GCTIMER) || defined(JSGC_TESTPILOT)
    struct GCData {
        



        uint64      firstEnter;
        bool        firstEnterValid;

        void setFirstEnter(uint64 v) {
            JS_ASSERT(!firstEnterValid);
            firstEnter = v;
            firstEnterValid = true;
        }

#ifdef JSGC_TESTPILOT
        bool        infoEnabled;

        bool isTimerEnabled() {
            return infoEnabled;
        }

        



        static const size_t INFO_LIMIT = 64;
        JSGCInfo    info[INFO_LIMIT];
        size_t      start;
        size_t      count;
#else 
        bool isTimerEnabled() {
            return true;
        }
#endif
    } gcData;
#endif

    JSRuntime();
    ~JSRuntime();

    bool init(uint32 maxbytes);

    void setGCLastBytes(size_t lastBytes);
    void reduceGCTriggerBytes(uint32 amount);

    



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

#ifdef DEBUG
# define JS_RUNTIME_METER(rt, which)    JS_ATOMIC_INCREMENT(&(rt)->which)
# define JS_RUNTIME_UNMETER(rt, which)  JS_ATOMIC_DECREMENT(&(rt)->which)
#else
# define JS_RUNTIME_METER(rt, which)
# define JS_RUNTIME_UNMETER(rt, which)
#endif

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

class AutoGCRooter;
struct AutoResolving;

static inline bool
OptionsHasXML(uint32 options)
{
    return !!(options & JSOPTION_XML);
}

static inline bool
OptionsHasAnonFunFix(uint32 options)
{
    return !!(options & JSOPTION_ANONFUNFIX);
}

static inline bool
OptionsSameVersionFlags(uint32 self, uint32 other)
{
    static const uint32 mask = JSOPTION_XML | JSOPTION_ANONFUNFIX;
    return !((self & mask) ^ (other & mask));
}









namespace VersionFlags {
static const uintN MASK         = 0x0FFF; 
static const uintN HAS_XML      = 0x1000; 
static const uintN ANONFUNFIX   = 0x2000; 
static const uintN FULL_MASK    = 0x3FFF;
}

static inline JSVersion
VersionNumber(JSVersion version)
{
    return JSVersion(uint32(version) & VersionFlags::MASK);
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

static inline bool
VersionHasAnonFunFix(JSVersion version)
{
    return !!(version & VersionFlags::ANONFUNFIX);
}

static inline void
VersionSetXML(JSVersion *version, bool enable)
{
    if (enable)
        *version = JSVersion(uint32(*version) | VersionFlags::HAS_XML);
    else
        *version = JSVersion(uint32(*version) & ~VersionFlags::HAS_XML);
}

static inline void
VersionSetAnonFunFix(JSVersion *version, bool enable)
{
    if (enable)
        *version = JSVersion(uint32(*version) | VersionFlags::ANONFUNFIX);
    else
        *version = JSVersion(uint32(*version) & ~VersionFlags::ANONFUNFIX);
}

static inline JSVersion
VersionExtractFlags(JSVersion version)
{
    return JSVersion(uint32(version) & ~VersionFlags::MASK);
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
    uintN copts = (VersionHasXML(version) ? JSOPTION_XML : 0) |
                  (VersionHasAnonFunFix(version) ? JSOPTION_ANONFUNFIX : 0);
    JS_ASSERT((copts & JSCOMPILEOPTION_MASK) == copts);
    return copts;
}

static inline JSVersion
OptionFlagsToVersion(uintN options, JSVersion version)
{
    VersionSetXML(&version, OptionsHasXML(options));
    VersionSetAnonFunFix(&version, OptionsHasAnonFunFix(options));
    return version;
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
    
    JSLocaleCallbacks   *localeCallbacks;

    js::AutoResolving   *resolvingList;

    



    JSPackedBool        generatingError;

    
    jsuword             stackLimit;

    
    JSRuntime *const    runtime;

    
    JSCompartment       *compartment;

    
    js::ContextStack    stack;

    
    bool hasfp() const                { return stack.hasfp(); }
    js::StackFrame* fp() const        { return stack.fp(); }
    js::StackFrame* maybefp() const   { return stack.maybefp(); }
    js::FrameRegs& regs() const       { return stack.regs(); }
    js::FrameRegs* maybeRegs() const  { return stack.maybeRegs(); }

    
    void resetCompartment();

    
    void wrapPendingException();

    
    JSArenaPool         tempPool;

    
    JSArenaPool         regExpPool;

    
    JSObject            *globalObject;

    
    JSSharpObjectMap    sharpObjectMap;
    js::BusyArraysSet   busyArrays;

    
    JSArgumentFormatMap *argumentFormatMap;

    
    char                *lastMessage;
#ifdef DEBUG
    void                *logfp;
    jsbytecode          *logPrevPc;
#endif

    
    JSErrorReporter     errorReporter;

    
    JSOperationCallback operationCallback;

    
    void                *data;
    void                *data2;

    inline js::RegExpStatics *regExpStatics();

  public:
    



    bool canSetDefaultVersion() const {
        return !stack.hasfp() && !hasVersionOverride;
    }

    
    void overrideVersion(JSVersion newVersion) {
        JS_ASSERT(!canSetDefaultVersion());
        versionOverride = newVersion;
        hasVersionOverride = true;
    }

    
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

    



    bool maybeOverrideVersion(JSVersion newVersion) {
        if (canSetDefaultVersion()) {
            setDefaultVersion(newVersion);
            return false;
        }
        overrideVersion(newVersion);
        return true;
    }

    



    void maybeMigrateVersionOverride() {
        JS_ASSERT(stack.empty());
        if (JS_UNLIKELY(isVersionOverridden())) {
            defaultVersion = versionOverride;
            clearVersionOverride();
        }
    }

    







    JSVersion findVersion() const {
        if (hasVersionOverride)
            return versionOverride;

        if (stack.hasfp()) {
            
            js::StackFrame *f = fp();
            while (f && !f->isScriptFrame())
                f = f->prev();
            if (f)
                return f->script()->getVersion();
        }

        return defaultVersion;
    }

    void setRunOptions(uintN ropts) {
        JS_ASSERT((ropts & JSRUNOPTION_MASK) == ropts);
        runOptions = ropts;
    }

    
    void setCompileOptions(uintN newcopts) {
        JS_ASSERT((newcopts & JSCOMPILEOPTION_MASK) == newcopts);
        if (JS_LIKELY(getCompileOptions() == newcopts))
            return;
        JSVersion version = findVersion();
        JSVersion newVersion = js::OptionFlagsToVersion(newcopts, version);
        maybeOverrideVersion(newVersion);
    }

    uintN getRunOptions() const { return runOptions; }
    uintN getCompileOptions() const { return js::VersionFlagsToOptions(findVersion()); }
    uintN allOptions() const { return getRunOptions() | getCompileOptions(); }

    bool hasRunOption(uintN ropt) const {
        JS_ASSERT((ropt & JSRUNOPTION_MASK) == ropt);
        return !!(runOptions & ropt);
    }

    bool hasStrictOption() const { return hasRunOption(JSOPTION_STRICT); }
    bool hasWErrorOption() const { return hasRunOption(JSOPTION_WERROR); }
    bool hasAtLineOption() const { return hasRunOption(JSOPTION_ATLINE); }

#ifdef JS_THREADSAFE
  private:
    JSThread            *thread_;
  public:
    JSThread *thread() const { return thread_; }

    void setThread(JSThread *thread);
    static const size_t threadOffset() { return offsetof(JSContext, thread_); }

    unsigned            outstandingRequests;


    JSCList             threadLinks;        

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

    
    js::AutoGCRooter   *autoGCRooters;

    
    const JSDebugHooks  *debugHooks;

    
    JSSecurityCallbacks *securityCallbacks;

    
    uintN               resolveFlags;

    
    int64               rngSeed;

    
    js::Value           iterValue;

#ifdef JS_TRACER
    









    bool                 traceJitEnabled;
#endif

#ifdef JS_METHODJIT
    bool                 methodJitEnabled;
    bool                 profilingEnabled;

    inline js::mjit::JaegerCompartment *jaegerCompartment();
#endif

    
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

#ifdef DEBUG
    void assertValidStackDepth(uintN depth) {
        JS_ASSERT(0 <= regs().sp - fp()->base());
        JS_ASSERT(depth <= uintptr_t(regs().sp - fp()->base()));
    }
#else
    void assertValidStackDepth(uintN ) {}
#endif

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

#ifdef DEBUG
    



    bool stackIterAssertionEnabled;
#endif

  private:
    





    JS_FRIEND_API(void) checkMallocGCPressure(void *p);
}; 

namespace js {










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

#ifdef JS_THREADSAFE
# define JS_THREAD_ID(cx)       ((cx)->thread() ? (cx)->thread()->id : 0)
#endif

#if defined JS_THREADSAFE && defined DEBUG

class AutoCheckRequestDepth {
    JSContext *cx;
  public:
    AutoCheckRequestDepth(JSContext *cx) : cx(cx) { cx->thread()->checkRequestDepth++; }

    ~AutoCheckRequestDepth() {
        JS_ASSERT(cx->thread()->checkRequestDepth != 0);
        cx->thread()->checkRequestDepth--;
    }
};

# define CHECK_REQUEST(cx)                                                    \
    JS_ASSERT((cx)->thread());                                                \
    JS_ASSERT((cx)->thread()->data.requestDepth || (cx)->thread() == (cx)->runtime->gcThread); \
    AutoCheckRequestDepth _autoCheckRequestDepth(cx);

#else
# define CHECK_REQUEST(cx)          ((void) 0)
# define CHECK_REQUEST_THREAD(cx)   ((void) 0)
#endif

static inline JSAtom **
FrameAtomBase(JSContext *cx, js::StackFrame *fp)
{
    return fp->hasImacropc()
           ? cx->runtime->atomState.commonAtomsStart()
           : fp->script()->atomMap.vector;
}

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

class AutoGCRooter {
  public:
    AutoGCRooter(JSContext *cx, ptrdiff_t tag)
      : down(cx->autoGCRooters), tag(tag), context(cx)
    {
        JS_ASSERT(this != cx->autoGCRooters);
        CHECK_REQUEST(cx);
        cx->autoGCRooters = this;
    }

    ~AutoGCRooter() {
        JS_ASSERT(this == context->autoGCRooters);
        CHECK_REQUEST(context);
        context->autoGCRooters = down;
    }

    
    inline void trace(JSTracer *trc);

#ifdef __GNUC__
# pragma GCC visibility push(default)
#endif
    friend JS_FRIEND_API(void) MarkContext(JSTracer *trc, JSContext *acx);
    friend void MarkRuntime(JSTracer *trc);
#ifdef __GNUC__
# pragma GCC visibility pop
#endif

  protected:
    AutoGCRooter * const down;

    






    ptrdiff_t tag;

    JSContext * const context;

    enum {
        JSVAL =        -1, 
        SHAPE =        -2, 
        PARSER =       -3, 
        SCRIPT =       -4, 
        ENUMERATOR =   -5, 
        IDARRAY =      -6, 
        DESCRIPTORS =  -7, 
        NAMESPACES =   -8, 
        XML =          -9, 
        OBJECT =      -10, 
        ID =          -11, 
        VALVECTOR =   -12, 
        DESCRIPTOR =  -13, 
        STRING =      -14, 
        IDVECTOR =    -15, 
        BINDINGS =    -16, 
        SHAPEVECTOR = -17  
    };

    private:
    
    AutoGCRooter(AutoGCRooter &ida);
    void operator=(AutoGCRooter &ida);
};


class AutoValueRooter : private AutoGCRooter
{
  public:
    explicit AutoValueRooter(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(js::NullValue())
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    AutoValueRooter(JSContext *cx, const Value &v
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(v)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    AutoValueRooter(JSContext *cx, jsval v
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(js::Valueify(v))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    





    void set(Value v) {
        JS_ASSERT(tag == JSVAL);
        val = v;
    }

    void set(jsval v) {
        JS_ASSERT(tag == JSVAL);
        val = js::Valueify(v);
    }

    const Value &value() const {
        JS_ASSERT(tag == JSVAL);
        return val;
    }

    Value *addr() {
        JS_ASSERT(tag == JSVAL);
        return &val;
    }

    const jsval &jsval_value() const {
        JS_ASSERT(tag == JSVAL);
        return Jsvalify(val);
    }

    jsval *jsval_addr() {
        JS_ASSERT(tag == JSVAL);
        return Jsvalify(&val);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    Value val;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectRooter : private AutoGCRooter {
  public:
    AutoObjectRooter(JSContext *cx, JSObject *obj = NULL
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, OBJECT), obj(obj)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setObject(JSObject *obj) {
        this->obj = obj;
    }

    JSObject * object() const {
        return obj;
    }

    JSObject ** addr() {
        return &obj;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    JSObject *obj;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoStringRooter : private AutoGCRooter {
  public:
    AutoStringRooter(JSContext *cx, JSString *str = NULL
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, STRING), str(str)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setString(JSString *str) {
        this->str = str;
    }

    JSString * string() const {
        return str;
    }

    JSString ** addr() {
        return &str;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSString *str;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoArrayRooter : private AutoGCRooter {
  public:
    AutoArrayRooter(JSContext *cx, size_t len, Value *vec
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(vec)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag >= 0);
    }

    AutoArrayRooter(JSContext *cx, size_t len, jsval *vec
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(Valueify(vec))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag >= 0);
    }

    void changeLength(size_t newLength) {
        tag = ptrdiff_t(newLength);
        JS_ASSERT(tag >= 0);
    }

    void changeArray(Value *newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    Value *array;

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoShapeRooter : private AutoGCRooter {
  public:
    AutoShapeRooter(JSContext *cx, const js::Shape *shape
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, SHAPE), shape(shape)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    const js::Shape * const shape;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoScriptRooter : private AutoGCRooter {
  public:
    AutoScriptRooter(JSContext *cx, JSScript *script
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, SCRIPT), script(script)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setScript(JSScript *script) {
        this->script = script;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSScript *script;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdRooter : private AutoGCRooter
{
  public:
    explicit AutoIdRooter(JSContext *cx, jsid id = INT_TO_JSID(0)
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ID), id_(id)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    jsid id() {
        return id_;
    }

    jsid * addr() {
        return &id_;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    jsid id_;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdArray : private AutoGCRooter {
  public:
    AutoIdArray(JSContext *cx, JSIdArray *ida JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, IDARRAY), idArray(ida)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoIdArray() {
        if (idArray)
            JS_DestroyIdArray(context, idArray);
    }
    bool operator!() {
        return idArray == NULL;
    }
    jsid operator[](size_t i) const {
        JS_ASSERT(idArray);
        JS_ASSERT(i < size_t(idArray->length));
        return idArray->vector[i];
    }
    size_t length() const {
         return idArray->length;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    JSIdArray *steal() {
        JSIdArray *copy = idArray;
        idArray = NULL;
        return copy;
    }

  protected:
    inline void trace(JSTracer *trc);

  private:
    JSIdArray * idArray;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoIdArray(AutoIdArray &ida);
    void operator=(AutoIdArray &ida);
};


class AutoEnumStateRooter : private AutoGCRooter
{
  public:
    AutoEnumStateRooter(JSContext *cx, JSObject *obj
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ENUMERATOR), obj(obj), stateValue()
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
    }

    ~AutoEnumStateRooter() {
        if (!stateValue.isNull()) {
            DebugOnly<JSBool> ok =
                obj->enumerate(context, JSENUMERATE_DESTROY, &stateValue, 0);
            JS_ASSERT(ok);
        }
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    const Value &state() const { return stateValue; }
    Value *addr() { return &stateValue; }

  protected:
    void trace(JSTracer *trc);

    JSObject * const obj;

  private:
    Value stateValue;
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
    friend void MarkRuntime(JSTracer *trc);

  private:
    JSXML * const xml;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

class AutoBindingsRooter : private AutoGCRooter {
  public:
    AutoBindingsRooter(JSContext *cx, Bindings &bindings
                       JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, BINDINGS), bindings(bindings)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Bindings &bindings;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoLockGC {
  public:
    explicit AutoLockGC(JSRuntime *rt
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_LOCK_GC(rt);
    }
    ~AutoLockGC() { JS_UNLOCK_GC(rt); }

  private:
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
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
        JS_LOCK(cx, &cx->runtime->atomState.lock);
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = true;
#endif
    }
    ~AutoLockAtomsCompartment() {
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = false;
#endif
        JS_UNLOCK(cx, &cx->runtime->atomState.lock);
    }
};

class AutoUnlockAtomsCompartment {
    JSContext *cx;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoUnlockAtomsCompartment(JSContext *cx
                                 JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = false;
#endif
        JS_UNLOCK(cx, &cx->runtime->atomState.lock);
    }
    ~AutoUnlockAtomsCompartment() {
        JS_LOCK(cx, &cx->runtime->atomState.lock);
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = true;
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

class AutoArenaAllocator {
    JSArenaPool *pool;
    void        *mark;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoArenaAllocator(JSArenaPool *pool
                                JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : pool(pool), mark(JS_ARENA_MARK(pool))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoArenaAllocator() { JS_ARENA_RELEASE(pool, mark); }

    template <typename T>
    T *alloc(size_t elems) {
        void *ptr;
        JS_ARENA_ALLOCATE(ptr, pool, elems * sizeof(T));
        return static_cast<T *>(ptr);
    }
};

class AutoReleasePtr {
    JSContext   *cx;
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleasePtr operator=(const AutoReleasePtr &other);

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

    AutoReleaseNullablePtr operator=(const AutoReleaseNullablePtr &other);

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

class AutoLocalNameArray {
  public:
    explicit AutoLocalNameArray(JSContext *cx, JSFunction *fun
                                JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : context(cx),
        mark(JS_ARENA_MARK(&cx->tempPool)),
        names(fun->script()->bindings.getLocalNameArray(cx, &cx->tempPool)),
        count(fun->script()->bindings.countLocalNames())
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoLocalNameArray() {
        JS_ARENA_RELEASE(&context->tempPool, mark);
    }

    operator bool() const { return !!names; }

    uint32 length() const { return count; }

    const jsuword &operator [](unsigned i) const { return names[i]; }

  private:
    JSContext   *context;
    void        *mark;
    jsuword     *names;
    uint32      count;

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template <class RefCountable>
class AlreadyIncRefed
{
    typedef RefCountable *****ConvertibleToBool;

    RefCountable *obj;

  public:
    explicit AlreadyIncRefed(RefCountable *obj) : obj(obj) {}

    bool null() const { return obj == NULL; }
    operator ConvertibleToBool() const { return (ConvertibleToBool)obj; }

    RefCountable *operator->() const { JS_ASSERT(!null()); return obj; }
    RefCountable &operator*() const { JS_ASSERT(!null()); return *obj; }
    RefCountable *get() const { return obj; }
};

template <class RefCountable>
class NeedsIncRef
{
    typedef RefCountable *****ConvertibleToBool;

    RefCountable *obj;

  public:
    explicit NeedsIncRef(RefCountable *obj) : obj(obj) {}

    bool null() const { return obj == NULL; }
    operator ConvertibleToBool() const { return (ConvertibleToBool)obj; }

    RefCountable *operator->() const { JS_ASSERT(!null()); return obj; }
    RefCountable &operator*() const { JS_ASSERT(!null()); return *obj; }
    RefCountable *get() const { return obj; }
};

template <class RefCountable>
class AutoRefCount
{
    typedef RefCountable *****ConvertibleToBool;

    JSContext *const cx;
    RefCountable *obj;

    AutoRefCount(const AutoRefCount &);
    void operator=(const AutoRefCount &);

  public:
    explicit AutoRefCount(JSContext *cx)
      : cx(cx), obj(NULL)
    {}

    AutoRefCount(JSContext *cx, NeedsIncRef<RefCountable> aobj)
      : cx(cx), obj(aobj.get())
    {
        if (obj)
            obj->incref(cx);
    }

    AutoRefCount(JSContext *cx, AlreadyIncRefed<RefCountable> aobj)
      : cx(cx), obj(aobj.get())
    {}

    ~AutoRefCount() {
        if (obj)
            obj->decref(cx);
    }

    void reset(NeedsIncRef<RefCountable> aobj) {
        if (obj)
            obj->decref(cx);
        obj = aobj.get();
        if (obj)
            obj->incref(cx);
    }

    void reset(AlreadyIncRefed<RefCountable> aobj) {
        if (obj)
            obj->decref(cx);
        obj = aobj.get();
    }

    bool null() const { return obj == NULL; }
    operator ConvertibleToBool() const { return (ConvertibleToBool)obj; }

    RefCountable *operator->() const { JS_ASSERT(!null()); return obj; }
    RefCountable &operator*() const { JS_ASSERT(!null()); return *obj; }
    RefCountable *get() const { return obj; }
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

} 





extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode);

static JS_INLINE JSContext *
js_ContextFromLinkField(JSCList *link)
{
    JS_ASSERT(link);
    return (JSContext *) ((uint8 *) link - offsetof(JSContext, link));
}





extern JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp);






extern JS_FRIEND_API(JSContext *)
js_NextActiveContext(JSRuntime *, JSContext *);





typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

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


JS_FRIEND_API(void)
js_ReportOverRecursed(JSContext *maybecx);

extern JS_FRIEND_API(void)
js_ReportAllocationOverflow(JSContext *cx);

#define JS_CHECK_RECURSION(cx, onerror)                                       \
    JS_BEGIN_MACRO                                                            \
        int stackDummy_;                                                      \
                                                                              \
        if (!JS_CHECK_STACK_SIZE(cx->stackLimit, &stackDummy_)) {             \
            js_ReportOverRecursed(cx);                                        \
            onerror;                                                          \
        }                                                                     \
    JS_END_MACRO





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



JS_FRIEND_API(void)
TriggerOperationCallback(JSContext *cx);

void
TriggerAllOperationCallbacks(JSRuntime *rt);

} 

extern js::StackFrame *
js_GetScriptedCaller(JSContext *cx, js::StackFrame *fp);

extern jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx);

extern bool
js_CurrentPCIsInImacro(JSContext *cx);

namespace js {

class RegExpStatics;

extern JS_FORCES_STACK JS_FRIEND_API(void)
LeaveTrace(JSContext *cx);

} 







static JS_FORCES_STACK JS_INLINE js::StackFrame *
js_GetTopStackFrame(JSContext *cx)
{
    js::LeaveTrace(cx);
    return cx->maybefp();
}

static JS_INLINE JSBool
js_IsPropertyCacheDisabled(JSContext *cx)
{
    return cx->runtime->shapeGen >= js::SHAPE_OVERFLOW_BIT;
}

static JS_INLINE uint32
js_RegenerateShapeForGC(JSRuntime *rt)
{
    JS_ASSERT(rt->gcRunning);
    JS_ASSERT(rt->gcRegenShapes);

    




    uint32 shape = rt->shapeGen;
    shape = (shape + 1) | (shape & js::SHAPE_OVERFLOW_BIT);
    rt->shapeGen = shape;
    return shape;
}

namespace js {

template<class T>
class AutoVectorRooter : protected AutoGCRooter
{
  public:
    explicit AutoVectorRooter(JSContext *cx, ptrdiff_t tag
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, tag), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool append(const T &v) { return vector.append(v); }

    
    void infallibleAppend(const T &v) { vector.infallibleAppend(v); }

    void popBack() { vector.popBack(); }
    T popCopy() { return vector.popCopy(); }

    bool growBy(size_t inc) {
        size_t oldLength = vector.length();
        if (!vector.growByUninitialized(inc))
            return false;
        MakeRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool resize(size_t newLength) {
        size_t oldLength = vector.length();
        if (newLength <= oldLength) {
            vector.shrinkBy(oldLength - newLength);
            return true;
        }
        if (!vector.growByUninitialized(newLength - oldLength))
            return false;
        MakeRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    T &operator[](size_t i) { return vector[i]; }
    const T &operator[](size_t i) const { return vector[i]; }

    const T *begin() const { return vector.begin(); }
    T *begin() { return vector.begin(); }

    const T *end() const { return vector.end(); }
    T *end() { return vector.end(); }

    const T &back() const { return vector.back(); }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    typedef Vector<T, 8> VectorImpl;
    VectorImpl vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueVector : public AutoVectorRooter<Value>
{
  public:
    explicit AutoValueVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Value>(cx, VALVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    const jsval *jsval_begin() const { return Jsvalify(begin()); }
    jsval *jsval_begin() { return Jsvalify(begin()); }

    const jsval *jsval_end() const { return Jsvalify(end()); }
    jsval *jsval_end() { return Jsvalify(end()); }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdVector : public AutoVectorRooter<jsid>
{
  public:
    explicit AutoIdVector(JSContext *cx
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<jsid>(cx, IDVECTOR)
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

JSIdArray *
NewIdArray(JSContext *cx, jsint length);

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
