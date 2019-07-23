







































#ifndef jscntxt_h___
#define jscntxt_h___



#include <string.h>

#include "jsarena.h" 
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsdhash.h"
#include "jsdtoa.h"
#include "jsgc.h"
#include "jshashtable.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jspropertycache.h"
#include "jspropertytree.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsregexp.h"
#include "jsutil.h"
#include "jsarray.h"
#include "jstask.h"
#include "jsvector.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif







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


namespace nanojit {

class Assembler;
class CodeAlloc;
class Fragment;
template<typename K> struct DefaultHash;
template<typename K, typename V, typename H> class HashMap;
template<typename T> class Seq;

}  

namespace js {


static const size_t MONITOR_N_GLOBAL_STATES = 4;
static const size_t FRAGMENT_TABLE_SIZE = 512;
static const size_t MAX_NATIVE_STACK_SLOTS = 4096;
static const size_t MAX_CALL_STACK_ENTRIES = 500;
static const size_t MAX_GLOBAL_SLOTS = 4096;
static const size_t GLOBAL_SLOTS_BUFFER_SIZE = MAX_GLOBAL_SLOTS + 1;


class VMAllocator;
class FrameInfoCache;
struct REHashFn;
struct REHashKey;
struct FrameInfo;
struct VMSideExit;
struct TreeFragment;
struct InterpState;
template<typename T> class Queue;
typedef Queue<uint16> SlotList;
class TypeMap;
struct REFragment;
typedef nanojit::HashMap<REHashKey, REFragment*, REHashFn> REHashMap;

#if defined(JS_JIT_SPEW) || defined(DEBUG)
struct FragPI;
typedef nanojit::HashMap<uint32, FragPI, nanojit::DefaultHash<uint32> > FragStatsMap;
#endif







class ContextAllocPolicy
{
    JSContext *cx;

  public:
    ContextAllocPolicy(JSContext *cx) : cx(cx) {}
    JSContext *context() const { return cx; }

    
    void *malloc(size_t bytes);
    void free(void *p);
    void *realloc(void *p, size_t bytes);
    void reportAllocOverflow() const;
};


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
    TreeFragment*  outermostTree;       
    uintN*         inlineCallCountp;    
    VMSideExit**   innermostNestedGuardp;
    VMSideExit*    innermost;
    uint64         startTime;
    InterpState*   prev;

    
    
    
    uint32         builtinStatus;

    
    double*        deepBailSp;

    
    uintN          nativeVpLen;
    jsval*         nativeVp;

    InterpState(JSContext *cx, TraceMonitor *tm, TreeFragment *ti,
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















class CallStack
{
#ifdef DEBUG
    
    JSContext           *cx;
#endif

    
    JSStackFrame        *suspendedFrame;

    
    bool                saved;

    
    CallStack           *previous;

    
    JSObject            *initialVarObj;

    
    JSStackFrame        *initialFrame;

  public:
    CallStack(JSContext *cx)
      :
#ifdef DEBUG
        cx(cx),
#endif
        suspendedFrame(NULL), saved(false), previous(NULL),
        initialVarObj(NULL), initialFrame(NULL)
    {}

#ifdef DEBUG
    bool contains(JSStackFrame *fp);
#endif

    void suspend(JSStackFrame *fp) {
        JS_ASSERT(fp && !isSuspended() && contains(fp));
        suspendedFrame = fp;
    }

    void resume() {
        JS_ASSERT(suspendedFrame);
        suspendedFrame = NULL;
    }

    JSStackFrame *getSuspendedFrame() const {
        JS_ASSERT(suspendedFrame);
        return suspendedFrame;
    }

    bool isSuspended() const { return !!suspendedFrame; }

    void setPrevious(CallStack *cs) { previous = cs; }
    CallStack *getPrevious() const  { return previous; }

    void setInitialVarObj(JSObject *o) { initialVarObj = o; }
    JSObject *getInitialVarObj() const { return initialVarObj; }

    void setInitialFrame(JSStackFrame *f) { initialFrame = f; }
    JSStackFrame *getInitialFrame() const { return initialFrame; }

    








    void save(JSStackFrame *fp) {
        suspend(fp);
        saved = true;
    }

    void restore() {
        saved = false;
        resume();
    }

    bool isSaved() const {
        JS_ASSERT_IF(saved, isSuspended());
        return saved;
    }
};


typedef HashMap<jsbytecode*,
                size_t,
                DefaultHasher<jsbytecode*>,
                SystemAllocPolicy> RecordAttemptMap;






struct TraceMonitor {
    









    JSContext               *tracecx;

    




    TraceNativeStorage      *storage;

    



























    VMAllocator*            dataAlloc;
    VMAllocator*            traceAlloc;
    VMAllocator*            tempAlloc;
    VMAllocator*            reTempAlloc;
    nanojit::CodeAlloc*     codeAlloc;
    nanojit::Assembler*     assembler;
    FrameInfoCache*         frameCache;

    TraceRecorder*          recorder;

    GlobalState             globalStates[MONITOR_N_GLOBAL_STATES];
    TreeFragment*           vmfragments[FRAGMENT_TABLE_SIZE];
    RecordAttemptMap*       recordAttempts;

    



    uint32                  maxCodeCacheBytes;

    




    JSBool                  needFlush;

    


    REHashMap*              reFragments;

    
    
    
    TypeMap*                cachedTempTypeMap;

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

} 






#ifdef JS_TRACER
# define JS_ON_TRACE(cx)            (JS_TRACE_MONITOR(cx).tracecx != NULL)
#else
# define JS_ON_TRACE(cx)            JS_FALSE
#endif

#ifdef DEBUG_brendan
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

    
    js::PropertyCache   propertyCache;

    
    JSLocalRootStack    *localRootStack;

#ifdef JS_TRACER
    
    js::TraceMonitor    traceMonitor;
#endif

    
    JSScript            *scriptsToGC[JS_EVAL_CACHE_SIZE];

#ifdef JS_EVAL_CACHE_METERING
    JSEvalCacheMeter    evalCacheMeter;
#endif

    
    DtoaState           *dtoaState;

    




#define NATIVE_ENUM_CACHE_LOG2  8
#define NATIVE_ENUM_CACHE_MASK  JS_BITMASK(NATIVE_ENUM_CACHE_LOG2)
#define NATIVE_ENUM_CACHE_SIZE  JS_BIT(NATIVE_ENUM_CACHE_LOG2)

#define NATIVE_ENUM_CACHE_HASH(shape)                                         \
    ((((shape) >> NATIVE_ENUM_CACHE_LOG2) ^ (shape)) & NATIVE_ENUM_CACHE_MASK)

    jsuword             nativeEnumCache[NATIVE_ENUM_CACHE_SIZE];

    bool init();
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

    






    bool                gcWaiting;

    


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


struct JSClassProtoCache {
    void purge() { js::PodArrayZero(entries); }

#ifdef JS_PROTO_CACHE_METERING
    struct Stats {
        int32       probe, hit;
    };
# define PROTO_CACHE_METER(cx, x)                                             \
    ((void) (JS_ATOMIC_INCREMENT(&(cx)->runtime->classProtoCacheStats.x)))
#else
# define PROTO_CACHE_METER(cx, x)  ((void) 0)
#endif

  private:
    struct GlobalAndProto {
        JSObject    *global;
        JSObject    *proto;
    };

    GlobalAndProto  entries[JSProto_LIMIT - JSProto_Object];

#ifdef __GNUC__
# pragma GCC visibility push(default)
#endif
    friend JSBool js_GetClassPrototype(JSContext *cx, JSObject *scope,
                                       JSProtoKey protoKey, JSObject **protop,
                                       JSClass *clasp);
#ifdef __GNUC__
# pragma GCC visibility pop
#endif
};

struct JSRuntime {
    
    JSRuntimeState      state;

    
    JSContextCallback   cxCallback;

    











    uint32              protoHazardShape;

    
    JSGCChunkInfo       *gcChunkList;
    JSGCArenaList       gcArenaList[FINALIZE_LIMIT];
    JSGCDoubleArenaList gcDoubleArenaList;
    JSDHashTable        gcRootsHash;
    JSDHashTable        gcLocksHash;
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

    
    JSGCArena           *gcUnmarkedArenaStackTop;
#ifdef DEBUG
    size_t              gcMarkLaterCount;
#endif

    



    js::Vector<JSObject*, 0, js::SystemAllocPolicy> gcIteratorTable;

    



    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    





    JSSetSlotRequest    *setSlotRequests;

    
    jsval               NaNValue;
    jsval               negativeInfinityValue;
    jsval               positiveInfinityValue;

    js::DeflatedStringCache *deflatedStringCache;

    JSString            *emptyString;

    





    JSObject            *builtinFunctions[JSBUILTIN_LIMIT];

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

#ifdef JS_TRACER
    
    bool debuggerInhibitsJIT() const {
        return (globalDebugHooks.interruptHandler ||
                globalDebugHooks.callHook ||
                globalDebugHooks.objectHook);
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

    




    js::PropertyTree    propertyTree;

#define JS_PROPERTY_TREE(cx) ((cx)->runtime->propertyTree)

    




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

    JSEmptyScope          *emptyArgumentsScope;
    JSEmptyScope          *emptyBlockScope;

    




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

#ifdef DEBUG
    
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

#ifdef JS_PROTO_CACHE_METERING
    JSClassProtoCache::Stats classProtoCacheStats;
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
#define JSRESOLVE_INFER         0xffff  /* infer bits from current bytecode */

extern const JSDebugHooks js_NullDebugHooks;  

namespace js {
class AutoGCRooter;
}

struct JSContext
{
    explicit JSContext(JSRuntime *rt) : runtime(rt), busyArrays(this) {}

    



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

    
    JS_REQUIRES_STACK
    JSArenaPool         stackPool;

    JS_REQUIRES_STACK
    JSStackFrame        *fp;

    
    JSArenaPool         tempPool;

    
    JSObject            *globalObject;

    
    JSWeakRoots         weakRoots;

    
    JSRegExpStatics     regExpStatics;

    
    JSSharpObjectMap    sharpObjectMap;
    js::HashSet<JSObject *> busyArrays;

    
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

  private:
#ifdef __GNUC__
# pragma GCC visibility push(default)
#endif
    friend void js_TraceContext(JSTracer *, JSContext *);
#ifdef __GNUC__
# pragma GCC visibility pop
#endif

    
    js::CallStack       *currentCallStack;

  public:
    
    js::CallStack *activeCallStack() const {
        JS_ASSERT(currentCallStack && !currentCallStack->isSaved());
        return currentCallStack;
    }

    
    void pushCallStack(js::CallStack *newcs) {
        if (fp)
            currentCallStack->suspend(fp);
        else
            JS_ASSERT_IF(currentCallStack, currentCallStack->isSaved());
        newcs->setPrevious(currentCallStack);
        currentCallStack = newcs;
        JS_ASSERT(!newcs->isSuspended() && !newcs->isSaved());
    }

    
    void popCallStack() {
        JS_ASSERT(!currentCallStack->isSuspended() && !currentCallStack->isSaved());
        currentCallStack = currentCallStack->getPrevious();
        if (currentCallStack && !currentCallStack->isSaved()) {
            JS_ASSERT(fp);
            currentCallStack->resume();
        }
    }

    
    void saveActiveCallStack() {
        JS_ASSERT(fp && currentCallStack && !currentCallStack->isSuspended());
        currentCallStack->save(fp);
        fp = NULL;
    }

    
    void restoreCallStack() {
        JS_ASSERT(!fp && currentCallStack && currentCallStack->isSuspended());
        fp = currentCallStack->getSuspendedFrame();
        currentCallStack->restore();
    }

    



    js::CallStack *containingCallStack(JSStackFrame *target);

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

    
    js::AutoGCRooter   *autoGCRooters;

    
    const JSDebugHooks  *debugHooks;

    
    JSSecurityCallbacks *securityCallbacks;

    
    JSArenaPool         regexpPool;

    
    uintN               resolveFlags;

    
    int64               rngSeed;

#ifdef JS_TRACER
    




    js::InterpState     *interpState;
    js::VMSideExit      *bailExit;

    









    bool                 jitEnabled;
#endif

    JSClassProtoCache    classProtoCache;

    
    void updateJITEnabled() {
#ifdef JS_TRACER
        jitEnabled = ((options & JSOPTION_JIT) &&
                      (debugHooks == &js_NullDebugHooks ||
                       (debugHooks == &runtime->globalDebugHooks &&
                        !runtime->debuggerInhibitsJIT())));
#endif
    }

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

    bool isConstructing();

    void purge();

private:

    





    void checkMallocGCPressure(void *p);
};

JS_ALWAYS_INLINE JSObject *
JSStackFrame::varobj(js::CallStack *cs)
{
    JS_ASSERT(cs->contains(this));
    return fun ? callobj : cs->getInitialVarObj();
}

JS_ALWAYS_INLINE JSObject *
JSStackFrame::varobj(JSContext *cx)
{
    JS_ASSERT(cx->activeCallStack()->contains(this));
    return fun ? callobj : cx->activeCallStack()->getInitialVarObj();
}

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

namespace js {

class AutoGCRooter {
  public:
    AutoGCRooter(JSContext *cx, ptrdiff_t tag)
      : down(cx->autoGCRooters), tag(tag), context(cx)
    {
        JS_ASSERT(this != cx->autoGCRooters);
        cx->autoGCRooters = this;
    }

    ~AutoGCRooter() {
        JS_ASSERT(this == context->autoGCRooters);
        context->autoGCRooters = down;
    }

    inline void trace(JSTracer *trc);

#ifdef __GNUC__
# pragma GCC visibility push(default)
#endif
    friend void ::js_TraceContext(JSTracer *trc, JSContext *acx);
#ifdef __GNUC__
# pragma GCC visibility pop
#endif

  protected:
    AutoGCRooter * const down;

    






    ptrdiff_t tag;

    JSContext * const context;

    enum {
        JSVAL =        -1, 
        SPROP =        -2, 
        WEAKROOTS =    -3, 
        COMPILER =     -4, 
        SCRIPT =       -5, 
        ENUMERATOR =   -6, 
        IDARRAY =      -7, 
        DESCRIPTORS =  -8, 
        NAMESPACES =   -9, 
        XML =         -10, 
        OBJECT =      -11, 
        ID =          -12, 
        VECTOR =      -13  
    };
};

class AutoSaveWeakRoots : private AutoGCRooter
{
  public:
    explicit AutoSaveWeakRoots(JSContext *cx
                               JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, WEAKROOTS), savedRoots(cx->weakRoots)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSWeakRoots savedRoots;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};


class AutoValueRooter : private AutoGCRooter
{
  public:
    explicit AutoValueRooter(JSContext *cx, jsval v = JSVAL_NULL
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(v)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    AutoValueRooter(JSContext *cx, JSString *str
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(STRING_TO_JSVAL(str))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    AutoValueRooter(JSContext *cx, JSObject *obj
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(OBJECT_TO_JSVAL(obj))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setObject(JSObject *obj) {
        JS_ASSERT(tag == JSVAL);
        val = OBJECT_TO_JSVAL(obj);
    }

    void setString(JSString *str) {
        JS_ASSERT(tag == JSVAL);
        JS_ASSERT(str);
        val = STRING_TO_JSVAL(str);
    }

    void setDouble(jsdouble *dp) {
        JS_ASSERT(tag == JSVAL);
        JS_ASSERT(dp);
        val = DOUBLE_TO_JSVAL(dp);
    }

    jsval value() const {
        JS_ASSERT(tag == JSVAL);
        return val;
    }

    jsval *addr() {
        JS_ASSERT(tag == JSVAL);
        return &val;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    jsval val;
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

  private:
    JSObject *obj;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoArrayRooter : private AutoGCRooter {
  public:
    AutoArrayRooter(JSContext *cx, size_t len, jsval *vec
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(vec)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag >= 0);
    }

    void changeLength(size_t newLength) {
        tag = ptrdiff_t(newLength);
        JS_ASSERT(tag >= 0);
    }

    void changeArray(jsval *newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    jsval *array;

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoScopePropertyRooter : private AutoGCRooter {
  public:
    AutoScopePropertyRooter(JSContext *cx, JSScopeProperty *sprop
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, SPROP), sprop(sprop)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSScopeProperty * const sprop;
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
      : AutoGCRooter(cx, ID), idval(id)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    jsid id() {
        return idval;
    }

    jsid * addr() {
        return &idval;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    jsid idval;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdArray : private AutoGCRooter {
  public:
    AutoIdArray(JSContext *cx, JSIdArray *ida
                  JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ida ? ida->length : 0), idArray(ida)
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

  protected:
    inline void trace(JSTracer *trc);

  private:
    JSIdArray * const idArray;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoIdArray(AutoIdArray &ida);
    void operator=(AutoIdArray &ida);
};


class AutoEnumStateRooter : private AutoGCRooter
{
  public:
    AutoEnumStateRooter(JSContext *cx, JSObject *obj
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ENUMERATOR), obj(obj), stateValue(JSVAL_NULL)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
    }

    ~AutoEnumStateRooter() {
        if (!JSVAL_IS_NULL(stateValue)) {
#ifdef DEBUG
            JSBool ok =
#endif
            obj->enumerate(context, JSENUMERATE_DESTROY, &stateValue, 0);
            JS_ASSERT(ok);
        }
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    jsval state() const { return stateValue; }
    jsval * addr() { return &stateValue; }

  protected:
    void trace(JSTracer *trc) {
        JS_CALL_OBJECT_TRACER(trc, obj, "js::AutoEnumStateRooter.obj");
        js_MarkEnumeratorState(trc, obj, stateValue);
    }

    JSObject * const obj;

  private:
    jsval stateValue;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef JS_HAS_XML_SUPPORT
class AutoXMLRooter : private AutoGCRooter {
  public:
    AutoXMLRooter(JSContext *cx, JSXML *xml)
      : AutoGCRooter(cx, XML), xml(xml)
    {
        JS_ASSERT(xml);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSXML * const xml;
};
#endif 

class AutoLockGC {
private:
    JSRuntime *rt;
public:
    explicit AutoLockGC(JSRuntime *rt) : rt(rt) { JS_LOCK_GC(rt); }
    ~AutoLockGC() { JS_UNLOCK_GC(rt); }
};

class AutoUnlockGC {
private:
    JSRuntime *rt;
public:
    explicit AutoUnlockGC(JSRuntime *rt) : rt(rt) { JS_UNLOCK_GC(rt); }
    ~AutoUnlockGC() { JS_LOCK_GC(rt); }
};

class AutoKeepAtoms {
    JSRuntime *rt;
  public:
    explicit AutoKeepAtoms(JSRuntime *rt) : rt(rt) { JS_KEEP_ATOMS(rt); }
    ~AutoKeepAtoms() { JS_UNKEEP_ATOMS(rt); }
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




extern uint32
js_CountThreadRequests(JSContext *cx);







extern void
js_WaitForGC(JSRuntime *rt);

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
                        bool charArgs, va_list ap);
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

namespace js {

#ifdef JS_TRACER







JS_FORCES_STACK JS_FRIEND_API(void)
DeepBail(JSContext *cx);
#endif

static JS_FORCES_STACK JS_INLINE void
LeaveTrace(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_ON_TRACE(cx))
        DeepBail(cx);
#endif
}

static JS_INLINE void
LeaveTraceIfGlobalObject(JSContext *cx, JSObject *obj)
{
    if (!obj->fslots[JSSLOT_PARENT])
        LeaveTrace(cx);
}

static JS_INLINE JSBool
CanLeaveTrace(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));
#ifdef JS_TRACER
    return cx->bailExit != NULL;
#else
    return JS_FALSE;
#endif
}

}       







static JS_FORCES_STACK JS_INLINE JSStackFrame *
js_GetTopStackFrame(JSContext *cx)
{
    js::LeaveTrace(cx);
    return cx->fp;
}

static JS_INLINE JSBool
js_IsPropertyCacheDisabled(JSContext *cx)
{
    return cx->runtime->shapeGen >= js::SHAPE_OVERFLOW_BIT;
}

static JS_INLINE uint32
js_RegenerateShapeForGC(JSContext *cx)
{
    JS_ASSERT(cx->runtime->gcRunning);
    JS_ASSERT(cx->runtime->gcRegenShapes);

    




    uint32 shape = cx->runtime->shapeGen;
    shape = (shape + 1) | (shape & js::SHAPE_OVERFLOW_BIT);
    cx->runtime->shapeGen = shape;
    return shape;
}

namespace js {

inline void *
ContextAllocPolicy::malloc(size_t bytes)
{
    return cx->malloc(bytes);
}

inline void
ContextAllocPolicy::free(void *p)
{
    cx->free(p);
}

inline void *
ContextAllocPolicy::realloc(void *p, size_t bytes)
{
    return cx->realloc(p, bytes);
}

inline void
ContextAllocPolicy::reportAllocOverflow() const
{
    js_ReportAllocationOverflow(cx);
}

class AutoValueVector : private AutoGCRooter
{
  public:
    explicit AutoValueVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, VECTOR), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool push(jsval v) { return vector.append(v); }
    bool push(JSString *str) { return push(STRING_TO_JSVAL(str)); }
    bool push(JSObject *obj) { return push(OBJECT_TO_JSVAL(obj)); }
    bool push(jsdouble *dp) { return push(DOUBLE_TO_JSVAL(dp)); }

    void pop() { vector.popBack(); }

    bool resize(size_t newLength) {
        if (!vector.resize(newLength))
            return false;
        return true;
    }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    jsval &operator[](size_t i) { return vector[i]; }
    jsval operator[](size_t i) const { return vector[i]; }

    const jsval *buffer() const { return vector.begin(); }
    jsval *buffer() { return vector.begin(); }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Vector<jsval, 8> vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

}

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
