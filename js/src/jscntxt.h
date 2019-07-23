







































#ifndef jscntxt_h___
#define jscntxt_h___



#include "jsarena.h" 
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsversion.h"
#include "jsdhash.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsregexp.h"
#include "jsutil.h"
#include "jsarray.h"
#include "jstask.h"
#include "jsvector.h"







typedef struct JSGSNCache {
    jsbytecode      *code;
    JSDHashTable    table;
#ifdef JS_GSNMETER
    uint32          hits;
    uint32          misses;
    uint32          fills;
    uint32          purges;
# define GSN_CACHE_METER(cache,cnt) (++(cache)->cnt)
#else
# define GSN_CACHE_METER(cache,cnt)
#endif
} JSGSNCache;

#define js_FinishGSNCache(cache) js_PurgeGSNCache(cache)

extern void
js_PurgeGSNCache(JSGSNCache *cache);


#define JS_PURGE_GSN_CACHE(cx)      js_PurgeGSNCache(&JS_GSN_CACHE(cx))
#define JS_METER_GSN_CACHE(cx,cnt)  GSN_CACHE_METER(&JS_GSN_CACHE(cx), cnt)


namespace nanojit
{
    class Assembler;
    class CodeAlloc;
    class Fragment;
    class LirBuffer;
#ifdef DEBUG
    class LabelMap;
#endif
    template<typename K> struct DefaultHash;
    template<typename K, typename V, typename H> class HashMap;
    template<typename T> class Seq;
}


static const size_t MONITOR_N_GLOBAL_STATES = 4;
static const size_t FRAGMENT_TABLE_SIZE = 512;
static const size_t MAX_NATIVE_STACK_SLOTS = 4096;
static const size_t MAX_CALL_STACK_ENTRIES = 500;
static const size_t MAX_GLOBAL_SLOTS = 4096;
static const size_t GLOBAL_SLOTS_BUFFER_SIZE = MAX_GLOBAL_SLOTS + 1;


class TreeInfo;
class VMAllocator;
class TraceRecorder;
class FrameInfoCache;
struct REHashFn;
struct REHashKey;
struct FrameInfo;
struct VMSideExit;
struct TreeFragment;
struct InterpState;
template<typename T> class Queue;
typedef Queue<uint16> SlotList;
struct REFragment;
typedef nanojit::HashMap<REHashKey, REFragment*, REHashFn> REHashMap;

#if defined(JS_JIT_SPEW) || defined(DEBUG)
struct FragPI;
typedef nanojit::HashMap<uint32, FragPI, nanojit::DefaultHash<uint32> > FragStatsMap;
#endif


struct InterpState
{
    JSContext*     cx;                  
    double*        stackBase;           
    double*        sp;                  
    double*        eos;                 
    FrameInfo**    callstackBase;       
    void*          sor;                 
    FrameInfo**    rp;                  
    void*          eor;                 
    VMSideExit*    lastTreeExitGuard;   
    VMSideExit*    lastTreeCallGuard;   
                                        
    void*          rpAtLastTreeCall;    
    VMSideExit*    outermostTreeExitGuard; 
    TreeInfo*      outermostTree;       
    uintN*         inlineCallCountp;    
    VMSideExit**   innermostNestedGuardp;
    VMSideExit*    innermost;
    uint64         startTime;
    InterpState*   prev;

    
    
    
    uint32         builtinStatus;

    
    double*        deepBailSp;

    
    uintN          nativeVpLen;
    jsval*         nativeVp;

    InterpState(JSContext *cx, JSTraceMonitor *tm, TreeInfo *ti,
                uintN &inlineCallCountp, VMSideExit** innermostNestedGuardp);
    ~InterpState();
};







struct TraceNativeStorage
{
    double stack_global_buf[MAX_NATIVE_STACK_SLOTS + GLOBAL_SLOTS_BUFFER_SIZE];
    FrameInfo *callstack_buf[MAX_CALL_STACK_ENTRIES];

    double *stack() { return stack_global_buf; }
    double *global() { return stack_global_buf + MAX_NATIVE_STACK_SLOTS; }
    FrameInfo **callstack() { return callstack_buf; }
};


struct GlobalState {
    JSObject*               globalObj;
    uint32                  globalShape;
    SlotList*               globalSlots;
};






struct JSTraceMonitor {
    









    JSContext               *tracecx;

    




    TraceNativeStorage      storage;

    















    VMAllocator*            dataAlloc;   
    VMAllocator*            traceAlloc;  
    VMAllocator*            tempAlloc;   
    nanojit::CodeAlloc*     codeAlloc;   
    nanojit::Assembler*     assembler;
    nanojit::LirBuffer*     lirbuf;
    nanojit::LirBuffer*     reLirBuf;
    FrameInfoCache*         frameCache;
#ifdef DEBUG
    nanojit::LabelMap*      labels;
#endif

    TraceRecorder*          recorder;

    struct GlobalState      globalStates[MONITOR_N_GLOBAL_STATES];
    struct TreeFragment*    vmfragments[FRAGMENT_TABLE_SIZE];
    JSDHashTable            recordAttempts;

    



    uint32                  maxCodeCacheBytes;

    




    JSBool                  needFlush;

    



    JSBool                  useReservedObjects;
    JSObject                *reservedObjects;

    


    REHashMap*              reFragments;

    


    VMAllocator*            reTempAlloc;

#ifdef DEBUG
    
    nanojit::Seq<nanojit::Fragment*>* branches;
    uint32                  lastFragID;
    



    VMAllocator*            profAlloc;
    FragStatsMap*           profTab;
#endif

    
    void flush();

    
    void mark(JSTracer *trc);

    bool outOfMemory() const;
};

typedef struct InterpStruct InterpStruct;






#ifdef JS_TRACER
# define JS_ON_TRACE(cx)            (JS_TRACE_MONITOR(cx).tracecx != NULL)
#else
# define JS_ON_TRACE(cx)            JS_FALSE
#endif

#ifdef DEBUG
# define JS_EVAL_CACHE_METERING     1
# define JS_FUNCTION_METERING       1
#endif


#ifndef JS_EVAL_CACHE_SHIFT
# define JS_EVAL_CACHE_SHIFT        6
#endif
#define JS_EVAL_CACHE_SIZE          JS_BIT(JS_EVAL_CACHE_SHIFT)

#ifdef JS_EVAL_CACHE_METERING
# define EVAL_CACHE_METER_LIST(_)   _(probe), _(hit), _(step), _(noscope)
# define identity(x)                x

struct JSEvalCacheMeter {
    uint64 EVAL_CACHE_METER_LIST(identity);
};

# undef identity
#endif

#ifdef JS_FUNCTION_METERING
# define FUNCTION_KIND_METER_LIST(_)                                          \
                        _(allfun), _(heavy), _(nofreeupvar), _(onlyfreevar),  \
                        _(display), _(flat), _(setupvar), _(badfunarg)
# define identity(x)    x

struct JSFunctionMeter {
    int32 FUNCTION_KIND_METER_LIST(identity);
};

# undef identity
#endif

struct JSLocalRootChunk;

#define JSLRS_CHUNK_SHIFT       8
#define JSLRS_CHUNK_SIZE        JS_BIT(JSLRS_CHUNK_SHIFT)
#define JSLRS_CHUNK_MASK        JS_BITMASK(JSLRS_CHUNK_SHIFT)

struct JSLocalRootChunk {
    jsval               roots[JSLRS_CHUNK_SIZE];
    JSLocalRootChunk    *down;
};

struct JSLocalRootStack {
    uint32              scopeMark;
    uint32              rootCount;
    JSLocalRootChunk    *topChunk;
    JSLocalRootChunk    firstChunk;

    
    JSGCFreeLists       gcFreeLists;
};

const uint32 JSLRS_NULL_MARK = uint32(-1);

struct JSThreadData {
    JSGCFreeLists       gcFreeLists;

    




    bool                waiveGCQuota;

    



    JSGSNCache          gsnCache;

    
    JSPropertyCache     propertyCache;

    
    int64               rngSeed;

    
    JSLocalRootStack    *localRootStack;

#ifdef JS_TRACER
    
    JSTraceMonitor      traceMonitor;
#endif

    
    JSScript            *scriptsToGC[JS_EVAL_CACHE_SIZE];

#ifdef JS_EVAL_CACHE_METERING
    JSEvalCacheMeter    evalCacheMeter;
#endif

    




#define NATIVE_ENUM_CACHE_LOG2  8
#define NATIVE_ENUM_CACHE_MASK  JS_BITMASK(NATIVE_ENUM_CACHE_LOG2)
#define NATIVE_ENUM_CACHE_SIZE  JS_BIT(NATIVE_ENUM_CACHE_LOG2)

#define NATIVE_ENUM_CACHE_HASH(shape)                                         \
    ((((shape) >> NATIVE_ENUM_CACHE_LOG2) ^ (shape)) & NATIVE_ENUM_CACHE_MASK)

    jsuword             nativeEnumCache[NATIVE_ENUM_CACHE_SIZE];

    void init();
    void finish();
    void mark(JSTracer *trc);
    void purge(JSContext *cx);
    void purgeGCFreeLists();
};

#ifdef JS_THREADSAFE





struct JSThread {
    
    JSCList             contextList;

    
    jsword              id;

    
    JSTitle             *titleToShare;

    



    ptrdiff_t           gcThreadMallocBytes;

    


    JSFreePointerListTask *deallocatorTask;

    
    JSThreadData        data;
};






const size_t JS_GC_THREAD_MALLOC_LIMIT = 1 << 19;

#define JS_THREAD_DATA(cx)      (&(cx)->thread->data)

struct JSThreadsHashEntry {
    JSDHashEntryHdr     base;
    JSThread            *thread;
};

extern JSThread *
js_CurrentThread(JSRuntime *rt);






extern JSBool
js_InitContextThread(JSContext *cx);




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

typedef enum JSBuiltinFunctionId {
    JSBUILTIN_ObjectToIterator,
    JSBUILTIN_CallIteratorNext,
    JSBUILTIN_LIMIT
} JSBuiltinFunctionId;

typedef struct JSPropertyTreeEntry {
    JSDHashEntryHdr     hdr;
    JSScopeProperty     *child;
} JSPropertyTreeEntry;

typedef struct JSSetSlotRequest JSSetSlotRequest;

struct JSSetSlotRequest {
    JSObject            *obj;           
    JSObject            *pobj;          
    uint16              slot;           
    JSPackedBool        cycle;          
    JSSetSlotRequest    *next;          
};

struct JSRuntime {
    
    JSRuntimeState      state;

    
    JSContextCallback   cxCallback;

    











    uint32              protoHazardShape;

    
    JSGCChunkInfo       *gcChunkList;
    JSGCArenaList       gcArenaList[FINALIZE_LIMIT];
    JSGCDoubleArenaList gcDoubleArenaList;
    JSDHashTable        gcRootsHash;
    JSDHashTable        *gcLocksHash;
    jsrefcount          gcKeepAtoms;
    size_t              gcBytes;
    size_t              gcLastBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;
    uint32              gcEmptyArenaPoolLifespan;
    uint32              gcLevel;
    uint32              gcNumber;
    JSTracer            *gcMarkingTracer;
    uint32              gcTriggerFactor;
    size_t              gcTriggerBytes;
    volatile JSBool     gcIsNeeded;
    volatile JSBool     gcFlushCodeCaches;

    





    JSPackedBool        gcPoke;
    JSPackedBool        gcRunning;
    JSPackedBool        gcRegenShapes;

    








    uint8               gcRegenShapesScopeFlag;

#ifdef JS_GC_ZEAL
    jsrefcount          gcZeal;
#endif

    JSGCCallback        gcCallback;

    



    ptrdiff_t           gcMallocBytes;

    




    JSGCArenaInfo       *gcUntracedArenaStackTop;
#ifdef DEBUG
    size_t              gcTraceLaterCount;
#endif

    



    js::Vector<JSObject*, 0, js::SystemAllocPolicy> gcIteratorTable;

    



    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    





    JSSetSlotRequest    *setSlotRequests;

    
    jsval               NaNValue;
    jsval               negativeInfinityValue;
    jsval               positiveInfinityValue;

#ifdef JS_THREADSAFE
    JSLock              *deflatedStringCacheLock;
#endif
    JSHashTable         *deflatedStringCache;
#ifdef DEBUG
    uint32              deflatedStringCacheBytes;
#endif

    JSString            *emptyString;

    





    JSObject            *builtinFunctions[JSBUILTIN_LIMIT];

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

    
    JSCList             trapList;
    JSCList             watchPointList;

    
    void                *data;

#ifdef JS_THREADSAFE
    
    PRLock              *gcLock;
    PRCondVar           *gcDone;
    PRCondVar           *requestDone;
    uint32              requestCount;
    JSThread            *gcThread;

    
    PRLock              *rtLock;
#ifdef DEBUG
    jsword              rtLockOwner;
#endif

    
    PRCondVar           *stateChange;

    









    PRCondVar           *titleSharingDone;
    JSTitle             *titleSharingTodo;








#define NO_TITLE_SHARING_TODO   ((JSTitle *) 0xfeedbeef)

    





    PRLock              *debuggerLock;

    JSDHashTable        threads;
#endif 
    uint32              debuggerMutations;

    



    JSSecurityCallbacks *securityCallbacks;

    





    JSDHashTable        propertyTreeHash;
    JSScopeProperty     *propertyFreeList;
    JSArenaPool         propertyArenaPool;
    int32               propertyRemovals;

    
    struct JSHashTable  *scriptFilenameTable;
    JSCList             scriptFilenamePrefixes;
#ifdef JS_THREADSAFE
    PRLock              *scriptFilenameTableLock;
#endif

    
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

    






    JSObject            *anynameObject;
    JSObject            *functionNamespaceObject;

#ifndef JS_THREADSAFE
    JSThreadData        threadData;

#define JS_THREAD_DATA(cx)      (&(cx)->runtime->threadData)
#endif

    












    volatile uint32     shapeGen;

    
    JSAtomState         atomState;

#ifdef JS_THREADSAFE
    JSBackgroundThread    *deallocatorThread;
#endif

    




#ifdef JS_DUMP_ENUM_CACHE_STATS
    int32               nativeEnumProbes;
    int32               nativeEnumMisses;
# define ENUM_CACHE_METER(name)     JS_ATOMIC_INCREMENT(&cx->runtime->name)
#else
# define ENUM_CACHE_METER(name)     ((void) 0)
#endif

#ifdef JS_DUMP_LOOP_STATS
    
    JSBasicStats        loopStats;
#endif

#if defined DEBUG || defined JS_DUMP_PROPTREE_STATS
    
    jsrefcount          inlineCalls;
    jsrefcount          nativeCalls;
    jsrefcount          nonInlineCalls;
    jsrefcount          constructs;

    
    jsrefcount          claimAttempts;
    jsrefcount          claimedTitles;
    jsrefcount          deadContexts;
    jsrefcount          deadlocksAvoided;
    jsrefcount          liveScopes;
    jsrefcount          sharedTitles;
    jsrefcount          totalScopes;
    jsrefcount          liveScopeProps;
    jsrefcount          liveScopePropsPreSweep;
    jsrefcount          totalScopeProps;
    jsrefcount          livePropTreeNodes;
    jsrefcount          duplicatePropTreeNodes;
    jsrefcount          totalPropTreeNodes;
    jsrefcount          propTreeKidsChunks;
    jsrefcount          middleDeleteFixups;

    
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
#endif 

#ifdef JS_SCOPE_DEPTH_METER
    



    JSBasicStats        protoLookupDepthStats;
    JSBasicStats        scopeSearchDepthStats;

    



    JSBasicStats        hostenvScopeDepthStats;
    JSBasicStats        lexicalScopeDepthStats;
#endif

#ifdef JS_GCMETER
    JSGCStats           gcStats;
#endif

#ifdef JS_FUNCTION_METERING
    JSFunctionMeter     functionMeter;
    char                lastScriptFilename[1024];
#endif

    JSRuntime();
    ~JSRuntime();

    bool init(uint32 maxbytes);

    void setGCTriggerFactor(uint32 factor);
    void setGCLastBytes(size_t lastBytes);

    void* malloc(size_t bytes) { return ::js_malloc(bytes); }

    void* calloc(size_t bytes) { return ::js_calloc(bytes); }

    void* realloc(void* p, size_t bytes) { return ::js_realloc(p, bytes); }

    void free(void* p) { ::js_free(p); }

    bool isGCMallocLimitReached() const { return gcMallocBytes <= 0; }

    void resetGCMallocBytes() { gcMallocBytes = ptrdiff_t(gcMaxMallocBytes); }

    void setGCMaxMallocBytes(size_t value) {
        



        gcMaxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
        resetGCMallocBytes();
    }
};


#define JS_GSN_CACHE(cx)        (JS_THREAD_DATA(cx)->gsnCache)
#define JS_PROPERTY_CACHE(cx)   (JS_THREAD_DATA(cx)->propertyCache)
#define JS_TRACE_MONITOR(cx)    (JS_THREAD_DATA(cx)->traceMonitor)
#define JS_SCRIPTS_TO_GC(cx)    (JS_THREAD_DATA(cx)->scriptsToGC)

#ifdef JS_EVAL_CACHE_METERING
# define EVAL_CACHE_METER(x)    (JS_THREAD_DATA(cx)->evalCacheMeter.x++)
#else
# define EVAL_CACHE_METER(x)    ((void) 0)
#endif

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

struct JSStackHeader {
    uintN               nslots;
    JSStackHeader       *down;
};

#define JS_STACK_SEGMENT(sh)    ((jsval *)(sh) + 2)







typedef struct JSResolvingKey {
    JSObject            *obj;
    jsid                id;
} JSResolvingKey;

typedef struct JSResolvingEntry {
    JSDHashEntryHdr     hdr;
    JSResolvingKey      key;
    uint32              flags;
} JSResolvingEntry;

#define JSRESFLAG_LOOKUP        0x1     /* resolving id from lookup */
#define JSRESFLAG_WATCH         0x2     /* resolving id from watch */
















#define JSTVU_SINGLE        (-1)    /* u.value or u.<gcthing> is single jsval
                                       or non-JSString GC-thing pointer */
#define JSTVU_TRACE         (-2)    

#define JSTVU_SPROP         (-3)    /* u.sprop roots property tree node */
#define JSTVU_WEAK_ROOTS    (-4)    /* u.weakRoots points to saved weak roots */
#define JSTVU_COMPILER      (-5)    /* u.compiler roots JSCompiler* */
#define JSTVU_SCRIPT        (-6)    /* u.script roots JSScript* */
#define JSTVU_ENUMERATOR    (-7)    /* a pointer to JSTempValueRooter points
                                       to an instance of JSAutoEnumStateRooter
                                       with u.object storing the enumeration
                                       object */















#define JS_PUSH_TEMP_ROOT_COMMON(cx,x,tvr,cnt,kind)                           \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters != (tvr));                           \
        (tvr)->count = (cnt);                                                 \
        (tvr)->u.kind = (x);                                                  \
        (tvr)->down = (cx)->tempValueRooters;                                 \
        (cx)->tempValueRooters = (tvr);                                       \
    JS_END_MACRO

#define JS_POP_TEMP_ROOT(cx,tvr)                                              \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters == (tvr));                           \
        (cx)->tempValueRooters = (tvr)->down;                                 \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT(cx,cnt,arr,tvr)                                     \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((int)(cnt) >= 0);                                           \
        JS_PUSH_TEMP_ROOT_COMMON(cx, arr, tvr, (ptrdiff_t) (cnt), array);     \
    JS_END_MACRO

#define JS_PUSH_SINGLE_TEMP_ROOT(cx,val,tvr)                                  \
    JS_PUSH_TEMP_ROOT_COMMON(cx, val, tvr, JSTVU_SINGLE, value)

#define JS_PUSH_TEMP_ROOT_OBJECT(cx,obj,tvr)                                  \
    JS_PUSH_TEMP_ROOT_COMMON(cx, obj, tvr, JSTVU_SINGLE, object)

#define JS_PUSH_TEMP_ROOT_STRING(cx,str,tvr)                                  \
    JS_PUSH_SINGLE_TEMP_ROOT(cx, str ? STRING_TO_JSVAL(str) : JSVAL_NULL, tvr)

#define JS_PUSH_TEMP_ROOT_XML(cx,xml_,tvr)                                    \
    JS_PUSH_TEMP_ROOT_COMMON(cx, xml_, tvr, JSTVU_SINGLE, xml)

#define JS_PUSH_TEMP_ROOT_TRACE(cx,trace_,tvr)                                \
    JS_PUSH_TEMP_ROOT_COMMON(cx, trace_, tvr, JSTVU_TRACE, trace)

#define JS_PUSH_TEMP_ROOT_SPROP(cx,sprop_,tvr)                                \
    JS_PUSH_TEMP_ROOT_COMMON(cx, sprop_, tvr, JSTVU_SPROP, sprop)

#define JS_PUSH_TEMP_ROOT_WEAK_COPY(cx,weakRoots_,tvr)                        \
    JS_PUSH_TEMP_ROOT_COMMON(cx, weakRoots_, tvr, JSTVU_WEAK_ROOTS, weakRoots)

#define JS_PUSH_TEMP_ROOT_COMPILER(cx,pc,tvr)                                 \
    JS_PUSH_TEMP_ROOT_COMMON(cx, pc, tvr, JSTVU_COMPILER, compiler)

#define JS_PUSH_TEMP_ROOT_SCRIPT(cx,script_,tvr)                              \
    JS_PUSH_TEMP_ROOT_COMMON(cx, script_, tvr, JSTVU_SCRIPT, script)

#define JSRESOLVE_INFER         0xffff  /* infer bits from current bytecode */

struct JSContext {
    



    volatile jsint      operationCallbackFlag;

    
    JSCList             link;

#if JS_HAS_XML_SUPPORT
    





    uint8               xmlSettingFlags;
    uint8               padding;
#else
    uint16              padding;
#endif

    


#define JS_DISPLAY_SIZE 16U

    JSStackFrame        *display[JS_DISPLAY_SIZE];

    
    uint16              version;

    
    uint32              options;            

    
    JSLocaleCallbacks   *localeCallbacks;

    





    JSDHashTable        *resolvingTable;

    



    JSPackedBool        generatingError;

    
    JSPackedBool        insideGCMarkCallback;

    
    JSPackedBool        throwing;           
    jsval               exception;          

    
    jsuword             stackLimit;

    
    size_t              scriptStackQuota;

    
    JSRuntime * const   runtime;

    explicit JSContext(JSRuntime *rt) : runtime(rt) {}

    
    JS_REQUIRES_STACK
    JSArenaPool         stackPool;

    JS_REQUIRES_STACK
    JSStackFrame        *fp;

    
    JSArenaPool         tempPool;

    
    JSObject            *globalObject;

    
    JSWeakRoots         weakRoots;

    
    JSRegExpStatics     regExpStatics;

    
    JSSharpObjectMap    sharpObjectMap;
    JSHashTable         *busyArrayTable;

    
    JSArgumentFormatMap *argumentFormatMap;

    
    char                *lastMessage;
#ifdef DEBUG
    void                *tracefp;
    jsbytecode          *tracePrevPc;
#endif

    
    JSErrorReporter     errorReporter;

    
    JSOperationCallback operationCallback;

    
    uintN               interpLevel;

    
    void                *data;
    void                *data2;

    
    JSStackFrame        *dormantFrameChain; 
#ifdef JS_THREADSAFE
    JSThread            *thread;
    jsrefcount          requestDepth;
    
    jsrefcount          outstandingRequests;
    JSTitle             *lockedSealedTitle; 

    JSCList             threadLinks;        

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

    
    JSStackHeader       *stackHeaders;

    
    JSTempValueRooter   *tempValueRooters;

    
    JSDebugHooks        *debugHooks;

    
    JSSecurityCallbacks *securityCallbacks;

    
    JSArenaPool         regexpPool;

    
    uintN               resolveFlags;

#ifdef JS_TRACER
    




    InterpState         *interpState;
    VMSideExit          *bailExit;
#endif

#ifdef JS_THREADSAFE
    inline void createDeallocatorTask() {
        JS_ASSERT(!thread->deallocatorTask);
        if (runtime->deallocatorThread && !runtime->deallocatorThread->busy())
            thread->deallocatorTask = new JSFreePointerListTask();
    }

    inline void submitDeallocatorTask() {
        if (thread->deallocatorTask) {
            runtime->deallocatorThread->schedule(thread->deallocatorTask);
            thread->deallocatorTask = NULL;
        }
    }
#endif

    ptrdiff_t &getMallocCounter() {
#ifdef JS_THREADSAFE
        return thread->gcThreadMallocBytes;
#else
        return runtime->gcMallocBytes;
#endif
    }

    



    inline void updateMallocCounter(void *p, size_t nbytes) {
        JS_ASSERT(ptrdiff_t(nbytes) >= 0);
        ptrdiff_t &counter = getMallocCounter();
        counter -= ptrdiff_t(nbytes);
        if (!p || counter <= 0)
            checkMallocGCPressure(p);
    }

    



    inline void updateMallocCounter(size_t nbytes) {
        JS_ASSERT(ptrdiff_t(nbytes) >= 0);
        ptrdiff_t &counter = getMallocCounter();
        counter -= ptrdiff_t(nbytes);
        if (counter <= 0) {
            



            checkMallocGCPressure(reinterpret_cast<void *>(jsuword(1)));
        }
    }

    inline void* malloc(size_t bytes) {
        JS_ASSERT(bytes != 0);
        void *p = runtime->malloc(bytes);
        updateMallocCounter(p, bytes);
        return p;
    }

    inline void* mallocNoReport(size_t bytes) {
        JS_ASSERT(bytes != 0);
        void *p = runtime->malloc(bytes);
        if (!p)
            return NULL;
        updateMallocCounter(bytes);
        return p;
    }

    inline void* calloc(size_t bytes) {
        JS_ASSERT(bytes != 0);
        void *p = runtime->calloc(bytes);
        updateMallocCounter(p, bytes);
        return p;
    }

    inline void* realloc(void* p, size_t bytes) {
        void *orig = p;
        p = runtime->realloc(p, bytes);

        



        updateMallocCounter(p, orig ? 0 : bytes);
        return p;
    }

#ifdef JS_THREADSAFE
    inline void free(void* p) {
        if (!p)
            return;
        if (thread) {
            JSFreePointerListTask* task = thread->deallocatorTask;
            if (task) {
                task->add(p);
                return;
            }
        }
        runtime->free(p);
    }
#else
    inline void free(void* p) {
        if (!p)
            return;
        runtime->free(p);
    }
#endif

    




#define CREATE_BODY(parms)                                                    \
    void *memory = this->malloc(sizeof(T));                                   \
    if (!memory)                                                              \
        return NULL;                                                          \
    return new(memory) T parms;

    template <class T>
    JS_ALWAYS_INLINE T *create() {
        CREATE_BODY(())
    }

    template <class T, class P1>
    JS_ALWAYS_INLINE T *create(const P1 &p1) {
        CREATE_BODY((p1))
    }

    template <class T, class P1, class P2>
    JS_ALWAYS_INLINE T *create(const P1 &p1, const P2 &p2) {
        CREATE_BODY((p1, p2))
    }

    template <class T, class P1, class P2, class P3>
    JS_ALWAYS_INLINE T *create(const P1 &p1, const P2 &p2, const P3 &p3) {
        CREATE_BODY((p1, p2, p3))
    }
#undef CREATE_BODY

    template <class T>
    JS_ALWAYS_INLINE void destroy(T *p) {
        p->~T();
        this->free(p);
    }

private:

    





    void checkMallocGCPressure(void *p);
};

#ifdef JS_THREADSAFE
# define JS_THREAD_ID(cx)       ((cx)->thread ? (cx)->thread->id : 0)
#endif

#ifdef __cplusplus

static inline JSAtom **
FrameAtomBase(JSContext *cx, JSStackFrame *fp)
{
    return fp->imacpc
           ? COMMON_ATOMS_START(&cx->runtime->atomState)
           : fp->script->atomMap.vector;
}


class JSAutoTempValueRooter
{
  public:
    JSAutoTempValueRooter(JSContext *cx, size_t len, jsval *vec
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_PUSH_TEMP_ROOT(mContext, len, vec, &mTvr);
    }
    explicit JSAutoTempValueRooter(JSContext *cx, jsval v = JSVAL_NULL
                                   JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_PUSH_SINGLE_TEMP_ROOT(mContext, v, &mTvr);
    }
    JSAutoTempValueRooter(JSContext *cx, JSString *str
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_PUSH_TEMP_ROOT_STRING(mContext, str, &mTvr);
    }
    JSAutoTempValueRooter(JSContext *cx, JSObject *obj
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_PUSH_TEMP_ROOT_OBJECT(mContext, obj, &mTvr);
    }
    JSAutoTempValueRooter(JSContext *cx, JSScopeProperty *sprop
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_PUSH_TEMP_ROOT_SPROP(mContext, sprop, &mTvr);
    }

    ~JSAutoTempValueRooter() {
        JS_POP_TEMP_ROOT(mContext, &mTvr);
    }

    jsval value() { return mTvr.u.value; }
    jsval *addr() { return &mTvr.u.value; }

  protected:
    JSContext *mContext;

  private:
#ifndef AIX
    static void *operator new(size_t);
    static void operator delete(void *, size_t);
#endif

    JSTempValueRooter mTvr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class JSAutoTempIdRooter
{
  public:
    explicit JSAutoTempIdRooter(JSContext *cx, jsid id = INT_TO_JSID(0)
                                JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_PUSH_SINGLE_TEMP_ROOT(mContext, ID_TO_VALUE(id), &mTvr);
    }

    ~JSAutoTempIdRooter() {
        JS_POP_TEMP_ROOT(mContext, &mTvr);
    }

    jsid id() { return (jsid) mTvr.u.value; }
    jsid * addr() { return (jsid *) &mTvr.u.value; }

  private:
    JSContext *mContext;
    JSTempValueRooter mTvr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class JSAutoIdArray {
  public:
    JSAutoIdArray(JSContext *cx, JSIdArray *ida
                  JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : cx(cx), idArray(ida) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        if (ida)
            JS_PUSH_TEMP_ROOT(cx, ida->length, ida->vector, &tvr);
    }
    ~JSAutoIdArray() {
        if (idArray) {
            JS_POP_TEMP_ROOT(cx, &tvr);
            JS_DestroyIdArray(cx, idArray);
        }
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
  private:
    JSContext * const cx;
    JSIdArray * const idArray;
    JSTempValueRooter tvr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};


class JSAutoEnumStateRooter : public JSTempValueRooter
{
  public:
    JSAutoEnumStateRooter(JSContext *cx, JSObject *obj, jsval *statep
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx), mStatep(statep)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
        JS_ASSERT(statep);
        JS_PUSH_TEMP_ROOT_COMMON(cx, obj, this, JSTVU_ENUMERATOR, object);
    }

    ~JSAutoEnumStateRooter() {
        JS_POP_TEMP_ROOT(mContext, this);
    }

    void mark(JSTracer *trc) {
        JS_CALL_OBJECT_TRACER(trc, u.object, "enumerator_obj");
        js_MarkEnumeratorState(trc, u.object, *mStatep);
    }

  private:
    JSContext   *mContext;
    jsval       *mStatep;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, uintN flags
                       JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx), mSaved(cx->resolveFlags) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    uintN mSaved;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#endif




























#define JS_HAS_OPTION(cx,option)        (((cx)->options & (option)) != 0)
#define JS_HAS_STRICT_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_STRICT)
#define JS_HAS_WERROR_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_WERROR)
#define JS_HAS_COMPILE_N_GO_OPTION(cx)  JS_HAS_OPTION(cx, JSOPTION_COMPILE_N_GO)
#define JS_HAS_ATLINE_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_ATLINE)

#define JSVERSION_MASK                  0x0FFF  /* see JSVersion in jspubtd.h */
#define JSVERSION_HAS_XML               0x1000  /* flag induced by XML option */
#define JSVERSION_ANONFUNFIX            0x2000  /* see jsapi.h, the comments
                                                   for JSOPTION_ANONFUNFIX */

#define JSVERSION_NUMBER(cx)            ((JSVersion)((cx)->version &          \
                                                     JSVERSION_MASK))
#define JS_HAS_XML_OPTION(cx)           ((cx)->version & JSVERSION_HAS_XML || \
                                         JSVERSION_NUMBER(cx) >= JSVERSION_1_6)

extern JSThreadData *
js_CurrentThreadData(JSRuntime *rt);

extern JSBool
js_InitThreads(JSRuntime *rt);

extern void
js_FinishThreads(JSRuntime *rt);

extern void
js_PurgeThreads(JSContext *cx);

extern void
js_TraceThreads(JSRuntime *rt, JSTracer *trc);






extern void
js_SyncOptionsToVersion(JSContext *cx);





extern void
js_OnVersionChange(JSContext *cx);





extern void
js_SetVersion(JSContext *cx, JSVersion version);





extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode);





extern JSBool
js_ValidContextPointer(JSRuntime *rt, JSContext *cx);

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

#ifdef JS_THREADSAFE




uint32
js_CountThreadRequests(JSContext *cx);







extern void
js_WaitForGC(JSRuntime *rt);










uint32
js_DiscountRequestsForGC(JSContext *cx);




void
js_RecountRequestsAfterGC(JSRuntime *rt, uint32 requestDebit);

#else 

# define js_WaitForGC(rt)    ((void) 0)

#endif




extern JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp);

extern void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation);









extern JSBool
js_EnterLocalRootScope(JSContext *cx);

#define js_LeaveLocalRootScope(cx) \
    js_LeaveLocalRootScopeWithResult(cx, JSVAL_NULL)

extern void
js_LeaveLocalRootScopeWithResult(JSContext *cx, jsval rval);

extern void
js_ForgetLocalRoot(JSContext *cx, jsval v);

extern int
js_PushLocalRoot(JSContext *cx, JSLocalRootStack *lrs, jsval v);





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
                        JSBool *warningp, JSBool charArgs, va_list ap);
#endif

extern void
js_ReportOutOfMemory(JSContext *cx);




extern void
js_ReportOutOfScriptQuota(JSContext *cx);

extern void
js_ReportOverRecursed(JSContext *cx);

extern void
js_ReportAllocationOverflow(JSContext *cx);

#define JS_CHECK_RECURSION(cx, onerror)                                       \
    JS_BEGIN_MACRO                                                            \
        int stackDummy_;                                                      \
                                                                              \
        if (!JS_CHECK_STACK_SIZE(cx, stackDummy_)) {                          \
            js_ReportOverRecursed(cx);                                        \
            onerror;                                                          \
        }                                                                     \
    JS_END_MACRO





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);




extern JSBool
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback);

extern void
js_ReportMissingArg(JSContext *cx, jsval *vp, uintN arg);






extern JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, jsval v, JSString *fallback,
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








#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) < (cx)->stackLimit)
#else
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) > (cx)->stackLimit)
#endif






#define JS_CHECK_OPERATION_LIMIT(cx) \
    (!(cx)->operationCallbackFlag || js_InvokeOperationCallback(cx))





extern JSBool
js_InvokeOperationCallback(JSContext *cx);

#ifndef JS_THREADSAFE
# define js_TriggerAllOperationCallbacks(rt, gcLocked) \
    js_TriggerAllOperationCallbacks (rt)
#endif

void
js_TriggerAllOperationCallbacks(JSRuntime *rt, JSBool gcLocked);

extern JSStackFrame *
js_GetScriptedCaller(JSContext *cx, JSStackFrame *fp);

extern jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx);

extern bool
js_CurrentPCIsInImacro(JSContext *cx);

#ifdef JS_TRACER







JS_FORCES_STACK JS_FRIEND_API(void)
js_DeepBail(JSContext *cx);
#endif

static JS_FORCES_STACK JS_INLINE void
js_LeaveTrace(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_ON_TRACE(cx))
        js_DeepBail(cx);
#endif
}

static JS_INLINE void
js_LeaveTraceIfGlobalObject(JSContext *cx, JSObject *obj)
{
    if (!obj->fslots[JSSLOT_PARENT])
        js_LeaveTrace(cx);
}

static JS_INLINE JSBool
js_CanLeaveTrace(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));
#ifdef JS_TRACER
    return cx->bailExit != NULL;
#else
    return JS_FALSE;
#endif
}







static JS_FORCES_STACK JS_INLINE JSStackFrame *
js_GetTopStackFrame(JSContext *cx)
{
    js_LeaveTrace(cx);
    return cx->fp;
}

static JS_INLINE JSBool
js_IsPropertyCacheDisabled(JSContext *cx)
{
    return cx->runtime->shapeGen >= SHAPE_OVERFLOW_BIT;
}

static JS_INLINE uint32
js_RegenerateShapeForGC(JSContext *cx)
{
    JS_ASSERT(cx->runtime->gcRunning);
    JS_ASSERT(cx->runtime->gcRegenShapes);

    




    uint32 shape = cx->runtime->shapeGen;
    shape = (shape + 1) | (shape & SHAPE_OVERFLOW_BIT);
    cx->runtime->shapeGen = shape;
    return shape;
}

namespace js {







class ContextAllocPolicy
{
    JSContext *mCx;

  public:
    ContextAllocPolicy(JSContext *cx) : mCx(cx) {}
    JSContext *context() const { return mCx; }

    void *malloc(size_t bytes) { return mCx->malloc(bytes); }
    void free(void *p) { mCx->free(p); }
    void *realloc(void *p, size_t bytes) { return mCx->realloc(p, bytes); }
    void reportAllocOverflow() const { js_ReportAllocationOverflow(mCx); }
};

}

#endif
