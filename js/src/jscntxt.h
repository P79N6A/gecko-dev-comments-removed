







































#ifndef jscntxt_h___
#define jscntxt_h___



#include <string.h>


#ifdef mozilla_mozalloc_macro_wrappers_h
#  define JS_UNDEFD_MOZALLOC_WRAPPERS

#  include "mozilla/mozalloc_undef_macro_wrappers.h"
#endif

#include "jsarena.h" 
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsdhash.h"
#include "jsdtoa.h"
#include "jsgc.h"
#include "jsgcchunk.h"
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
#include "prmjtime.h"

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
static const size_t MAX_SLOW_NATIVE_EXTRA_SLOTS = 16;


class VMAllocator;
class FrameInfoCache;
struct REHashFn;
struct REHashKey;
struct FrameInfo;
struct VMSideExit;
struct TreeFragment;
struct TracerState;
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


struct TracerState 
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
    TracerState*   prev;

    
    
    
    uint32         builtinStatus;

    
    double*        deepBailSp;

    
    uintN          nativeVpLen;
    jsval*         nativeVp;

    
    
    JSFrameRegs    bailedSlowNativeRegs;

    TracerState(JSContext *cx, TraceMonitor *tm, TreeFragment *ti,
                uintN &inlineCallCountp, VMSideExit** innermostNestedGuardp);
    ~TracerState();
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
    
    JSContext           *cx;

    
    CallStack           *previousInContext;

    
    CallStack           *previousInThread;

    
    JSStackFrame        *initialFrame;

    
    JSStackFrame        *suspendedFrame;

    
    JSFrameRegs         *suspendedRegs;

    
    bool                saved;

    
    jsval               *initialArgEnd;

    
    JSObject            *initialVarObj;

  public:
    CallStack()
      : cx(NULL), previousInContext(NULL), previousInThread(NULL),
        initialFrame(NULL), suspendedFrame(NULL), saved(false),
        initialArgEnd(NULL), initialVarObj(NULL)
    {}

    

    jsval *previousCallStackEnd() const {
        return (jsval *)this;
    }

    jsval *getInitialArgBegin() const {
        return (jsval *)(this + 1);
    }

    










    bool inContext() const {
        JS_ASSERT(!!cx == !!initialFrame);
        JS_ASSERT_IF(!initialFrame, !suspendedFrame && !saved);
        return cx;
    }

    bool isActive() const {
        JS_ASSERT_IF(suspendedFrame, inContext());
        return initialFrame && !suspendedFrame;
    }

    bool isSuspended() const {
        JS_ASSERT_IF(!suspendedFrame, !saved);
        JS_ASSERT_IF(suspendedFrame, inContext());
        return suspendedFrame;
    }

    
    bool isSaved() const {
        JS_ASSERT_IF(saved, isSuspended());
        return saved;
    }

    

    void joinContext(JSContext *cx, JSStackFrame *f) {
        JS_ASSERT(!inContext());
        this->cx = cx;
        initialFrame = f;
        JS_ASSERT(isActive());
    }

    void leaveContext() {
        JS_ASSERT(isActive());
        this->cx = NULL;
        initialFrame = NULL;
        JS_ASSERT(!inContext());
    }

    JSContext *maybeContext() const {
        return cx;
    }

    

    void suspend(JSStackFrame *fp, JSFrameRegs *regs) {
        JS_ASSERT(isActive());
        JS_ASSERT(fp && contains(fp));
        suspendedFrame = fp;
        JS_ASSERT(isSuspended());
        suspendedRegs = regs;
    }

    void resume() {
        JS_ASSERT(isSuspended());
        suspendedFrame = NULL;
        JS_ASSERT(isActive());
    }

    

    void save(JSStackFrame *fp, JSFrameRegs *regs) {
        JS_ASSERT(!isSaved());
        suspend(fp, regs);
        saved = true;
        JS_ASSERT(isSaved());
    }

    void restore() {
        JS_ASSERT(isSaved());
        saved = false;
        resume();
        JS_ASSERT(!isSaved());
    }

    

    void setInitialArgEnd(jsval *v) {
        JS_ASSERT(!inContext() && !initialArgEnd);
        initialArgEnd = v;
    }

    jsval *getInitialArgEnd() const {
        JS_ASSERT(!inContext() && initialArgEnd);
        return initialArgEnd;
    }

    

    JSStackFrame *getInitialFrame() const {
        JS_ASSERT(inContext());
        return initialFrame;
    }

    inline JSStackFrame *getCurrentFrame() const;

    

    JSStackFrame *getSuspendedFrame() const {
        JS_ASSERT(isSuspended());
        return suspendedFrame;
    }

    JSFrameRegs *getSuspendedRegs() const {
        JS_ASSERT(isSuspended());
        return suspendedRegs;
    }

    jsval *getSuspendedSP() const {
        JS_ASSERT(isSuspended());
        return suspendedRegs->sp;
    }

    

    void setPreviousInContext(CallStack *cs) {
        previousInContext = cs;
    }

    CallStack *getPreviousInContext() const  {
        return previousInContext;
    }

    void setPreviousInThread(CallStack *cs) {
        previousInThread = cs;
    }

    CallStack *getPreviousInThread() const  {
        return previousInThread;
    }

    void setInitialVarObj(JSObject *obj) {
        JS_ASSERT(inContext());
        initialVarObj = obj;
    }

    JSObject *getInitialVarObj() const {
        JS_ASSERT(inContext());
        return initialVarObj;
    }

#ifdef DEBUG
    JS_REQUIRES_STACK bool contains(const JSStackFrame *fp) const;
#endif

};

static const size_t VALUES_PER_CALL_STACK = sizeof(CallStack) / sizeof(jsval);
JS_STATIC_ASSERT(sizeof(CallStack) % sizeof(jsval) == 0);






class InvokeArgsGuard
{
    friend class StackSpace;
    JSContext       *cx;
    CallStack       *cs;  
    jsval           *vp;
    uintN           argc;
  public:
    inline InvokeArgsGuard();
    inline InvokeArgsGuard(jsval *vp, uintN argc);
    inline ~InvokeArgsGuard();
    jsval *getvp() const { return vp; }
    uintN getArgc() const { JS_ASSERT(vp != NULL); return argc; }
};


class InvokeFrameGuard
{
    friend class StackSpace;
    JSContext       *cx;  
    CallStack       *cs;
    JSStackFrame    *fp;
  public:
    InvokeFrameGuard();
    JS_REQUIRES_STACK ~InvokeFrameGuard();
    JSStackFrame *getFrame() const { return fp; }
};


class ExecuteFrameGuard
{
    friend class StackSpace;
    JSContext       *cx;  
    CallStack       *cs;
    jsval           *vp;
    JSStackFrame    *fp;
    JSStackFrame    *down;
  public:
    ExecuteFrameGuard();
    JS_REQUIRES_STACK ~ExecuteFrameGuard();
    jsval *getvp() const { return vp; }
    JSStackFrame *getFrame() const { return fp; }
};






































































class StackSpace
{
    jsval *base;
#ifdef XP_WIN
    mutable jsval *commitEnd;
#endif
    jsval *end;
    CallStack *currentCallStack;

    
    friend class InvokeArgsGuard;
    JS_REQUIRES_STACK inline void popInvokeArgs(JSContext *cx, jsval *vp);
    friend class InvokeFrameGuard;
    JS_REQUIRES_STACK void popInvokeFrame(JSContext *cx, CallStack *maybecs);
    friend class ExecuteFrameGuard;
    JS_REQUIRES_STACK void popExecuteFrame(JSContext *cx);

    
    JS_REQUIRES_STACK
    inline jsval *firstUnused() const;

    inline void assertIsCurrent(JSContext *cx) const;
#ifdef DEBUG
    CallStack *getCurrentCallStack() const { return currentCallStack; }
#endif

    



    inline bool ensureSpace(JSContext *maybecx, jsval *from, ptrdiff_t nvals) const;

#ifdef XP_WIN
    
    JS_FRIEND_API(bool) bumpCommit(jsval *from, ptrdiff_t nvals) const;
#endif

  public:
    static const size_t CAPACITY_VALS   = 512 * 1024;
    static const size_t CAPACITY_BYTES  = CAPACITY_VALS * sizeof(jsval);
    static const size_t COMMIT_VALS     = 16 * 1024;
    static const size_t COMMIT_BYTES    = COMMIT_VALS * sizeof(jsval);

    
    bool init();
    void finish();

#ifdef DEBUG
    template <class T>
    bool contains(T *t) const {
        char *v = (char *)t;
        JS_ASSERT(size_t(-1) - uintptr_t(t) >= sizeof(T));
        return v >= (char *)base && v + sizeof(T) <= (char *)end;
    }
#endif

    





    inline bool ensureEnoughSpaceToEnterTrace();

    
    static const ptrdiff_t MAX_TRACE_SPACE_VALS =
      MAX_NATIVE_STACK_SLOTS + MAX_CALL_STACK_ENTRIES * VALUES_PER_STACK_FRAME +
      (VALUES_PER_CALL_STACK + VALUES_PER_STACK_FRAME );

    
    JS_REQUIRES_STACK void mark(JSTracer *trc);

    















    





    JS_REQUIRES_STACK
    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard &ag);

    
    bool getInvokeFrame(JSContext *cx, const InvokeArgsGuard &ag,
                        uintN nmissing, uintN nfixed,
                        InvokeFrameGuard &fg) const;

    JS_REQUIRES_STACK
    void pushInvokeFrame(JSContext *cx, const InvokeArgsGuard &ag,
                         InvokeFrameGuard &fg, JSFrameRegs &regs);

    




    JS_REQUIRES_STACK
    bool getExecuteFrame(JSContext *cx, JSStackFrame *down,
                         uintN vplen, uintN nfixed,
                         ExecuteFrameGuard &fg) const;
    JS_REQUIRES_STACK
    void pushExecuteFrame(JSContext *cx, ExecuteFrameGuard &fg,
                          JSFrameRegs &regs, JSObject *initialVarObj);

    



    JS_REQUIRES_STACK
    inline JSStackFrame *getInlineFrame(JSContext *cx, jsval *sp,
                                        uintN nmissing, uintN nfixed) const;

    JS_REQUIRES_STACK
    inline void pushInlineFrame(JSContext *cx, JSStackFrame *fp, jsbytecode *pc,
                                JSStackFrame *newfp);

    JS_REQUIRES_STACK
    inline void popInlineFrame(JSContext *cx, JSStackFrame *up, JSStackFrame *down);

    



    JS_REQUIRES_STACK
    void getSynthesizedSlowNativeFrame(JSContext *cx, CallStack *&cs, JSStackFrame *&fp);

    JS_REQUIRES_STACK
    void pushSynthesizedSlowNativeFrame(JSContext *cx, CallStack *cs, JSStackFrame *fp,
                                        JSFrameRegs &regs);

    JS_REQUIRES_STACK
    void popSynthesizedSlowNativeFrame(JSContext *cx);

    
    JS_REQUIRES_STACK
    JS_FRIEND_API(bool) pushInvokeArgsFriendAPI(JSContext *, uintN, InvokeArgsGuard &);
};

JS_STATIC_ASSERT(StackSpace::CAPACITY_VALS % StackSpace::COMMIT_VALS == 0);











class FrameRegsIter
{
    CallStack         *curcs;
    JSStackFrame      *curfp;
    jsval             *cursp;
    jsbytecode        *curpc;

  public:
    JS_REQUIRES_STACK FrameRegsIter(JSContext *cx);

    bool done() const { return curfp == NULL; }
    FrameRegsIter &operator++();

    JSStackFrame *fp() const { return curfp; }
    jsval *sp() const { return cursp; }
    jsbytecode *pc() const { return curpc; }
};


typedef HashMap<jsbytecode*,
                size_t,
                DefaultHasher<jsbytecode*>,
                SystemAllocPolicy> RecordAttemptMap;

class Oracle;






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

    Oracle*                 oracle;
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

#define NATIVE_ITER_CACHE_LOG2  8
#define NATIVE_ITER_CACHE_MASK  JS_BITMASK(NATIVE_ITER_CACHE_LOG2)
#define NATIVE_ITER_CACHE_SIZE  JS_BIT(NATIVE_ITER_CACHE_LOG2)

struct JSPendingProxyOperation {
    JSPendingProxyOperation *next;
    JSObject *object;
};

struct JSThreadData {
    JSGCFreeLists       gcFreeLists;

    
    js::StackSpace      stackSpace;

    




    bool                waiveGCQuota;

    



    JSGSNCache          gsnCache;

    
    js::PropertyCache   propertyCache;

#ifdef JS_TRACER
    
    js::TraceMonitor    traceMonitor;
#endif

    
    JSScript            *scriptsToGC[JS_EVAL_CACHE_SIZE];

#ifdef JS_EVAL_CACHE_METERING
    JSEvalCacheMeter    evalCacheMeter;
#endif

    
    DtoaState           *dtoaState;

    




    struct {
        jsdouble d;
        jsint    base;
        JSString *s;        
    } dtoaCache;

    
    JSObject            *cachedNativeIterators[NATIVE_ITER_CACHE_SIZE];

    
    jsuword             *nativeStackBase;

    
    JSPendingProxyOperation *pendingProxyOperation;

    js::ConservativeGCThreadData conservativeGC;

    bool init();
    void finish();
    void mark(JSTracer *trc);
    void purge(JSContext *cx);
};

#ifdef JS_THREADSAFE





struct JSThread {
    typedef js::HashMap<void *,
                        JSThread *,
                        js::DefaultHasher<void *>,
                        js::SystemAllocPolicy> Map;

    
    JSCList             contextList;

    
    void                *id;

    
    JSTitle             *titleToShare;

    



    ptrdiff_t           gcThreadMallocBytes;

    






    bool                gcWaiting;

    



    uint32              contextsInRequests;

    
    JSThreadData        data;
};






const size_t JS_GC_THREAD_MALLOC_LIMIT = 1 << 19;

#define JS_THREAD_DATA(cx)      (&(cx)->thread->data)

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

typedef struct JSPropertyTreeEntry {
    JSDHashEntryHdr     hdr;
    JSScopeProperty     *child;
} JSPropertyTreeEntry;


namespace js {

typedef Vector<JSGCChunkInfo *, 32, SystemAllocPolicy> GCChunks;

struct GCPtrHasher
{
    typedef void *Lookup;
    
    static HashNumber hash(void *key) {
        return HashNumber(uintptr_t(key) >> JSVAL_TAGBITS);
    }

    static bool match(void *l, void *k) {
        return l == k;
    }
};

typedef HashMap<void *, const char *, GCPtrHasher, SystemAllocPolicy> GCRoots;
typedef HashMap<void *, uint32, GCPtrHasher, SystemAllocPolicy> GCLocks;

struct WrapperHasher
{
    typedef jsval Lookup;
    
    static HashNumber hash(jsval key) {
        return GCPtrHasher::hash(JSVAL_TO_GCTHING(key));
    }

    static bool match(jsval l, jsval k) {
        return l == k;
    }
};

typedef HashMap<jsval, jsval, WrapperHasher, SystemAllocPolicy> WrapperMap;

class AutoValueVector;

} 

struct JSCompartment {
    JSRuntime *rt;
    JSPrincipals *principals;
    bool marked;
    js::WrapperMap crossCompartmentWrappers;

    JSCompartment(JSRuntime *cx);
    ~JSCompartment();

    bool init();

    bool wrap(JSContext *cx, jsval *vp);
    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, JSObject **objp);
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, JSPropertyOp *op);
    bool wrap(JSContext *cx, JSPropertyDescriptor *desc);
    bool wrap(JSContext *cx, js::AutoValueVector &props);
    bool wrapException(JSContext *cx);

    void sweep(JSContext *cx);
};

struct JSRuntime {
    
    JSCompartment       *defaultCompartment;

    
    js::Vector<JSCompartment *, 0, js::SystemAllocPolicy> compartments;

    
    JSRuntimeState      state;

    
    JSContextCallback   cxCallback;

    











    uint32              protoHazardShape;

    
    js::GCChunks        gcChunks;
    size_t              gcChunkCursor;
#ifdef DEBUG
    JSGCArena           *gcEmptyArenaList;
#endif
    JSGCArenaList       gcArenaList[FINALIZE_LIMIT];
    JSGCDoubleArenaList gcDoubleArenaList;
    js::GCRoots         gcRootsHash;
    js::GCLocks         gcLocksHash;
    jsrefcount          gcKeepAtoms;
    size_t              gcBytes;
    size_t              gcLastBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;
    uint32              gcEmptyArenaPoolLifespan;
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

#ifdef JS_THREADSAFE
    JSBackgroundThread  gcHelperThread;
#endif

    js::GCChunkAllocator    *gcChunkAllocator;
    
    void setCustomGCChunkAllocator(js::GCChunkAllocator *allocator) {
        JS_ASSERT(allocator);
        JS_ASSERT(state == JSRTS_DOWN);
        gcChunkAllocator = allocator;
    }

    



    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    
    jsval               NaNValue;
    jsval               negativeInfinityValue;
    jsval               positiveInfinityValue;

    js::DeflatedStringCache *deflatedStringCache;

    JSString            *emptyString;

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

#ifdef JS_TRACER
    
    bool debuggerInhibitsJIT() const {
        return (globalDebugHooks.interruptHook ||
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
    void *              rtLockOwner;
#endif

    
    PRCondVar           *stateChange;

    









    PRCondVar           *titleSharingDone;
    JSTitle             *titleSharingTodo;








#define NO_TITLE_SHARING_TODO   ((JSTitle *) 0xfeedbeef)

    





    PRLock              *debuggerLock;

    JSThread::Map       threads;
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

    



    JSEmptyScope          *emptyArgumentsScope;
    JSEmptyScope          *emptyBlockScope;
    JSEmptyScope          *emptyCallScope;
    JSEmptyScope          *emptyDeclEnvScope;
    JSEmptyScope          *emptyEnumeratorScope;
    JSEmptyScope          *emptyWithScope;

    




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

    JSWrapObjectCallback wrapObjectCallback;

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

struct JSRegExpStatics {
    JSContext   *cx;
    JSString    *input;         
    JSBool      multiline;      
    JSSubString lastMatch;      
    JSSubString lastParen;      
    JSSubString leftContext;    
    JSSubString rightContext;   
    js::Vector<JSSubString> parens; 

    JSRegExpStatics(JSContext *cx) : cx(cx), parens(cx) {}

    bool copy(const JSRegExpStatics& other);
    void clearRoots();
    void clear();
};

struct JSContext
{
    explicit JSContext(JSRuntime *rt);

    



    volatile jsint      operationCallbackFlag;

    
    JSCList             link;

    


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

    
    JSRuntime *const    runtime;

    
    JSCompartment       *compartment;

    
    JS_REQUIRES_STACK
    JSStackFrame        *fp;

    



    JS_REQUIRES_STACK
    JSFrameRegs         *regs;

  private:
    friend class js::StackSpace;
    friend JSBool js_Interpret(JSContext *);

    
    void setCurrentFrame(JSStackFrame *fp) {
        this->fp = fp;
    }

    void setCurrentRegs(JSFrameRegs *regs) {
        this->regs = regs;
    }

  public:
    
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
    
    js::CallStack       *currentCallStack;

  public:
    void assertCallStacksInSync() const {
#ifdef DEBUG
        if (fp) {
            JS_ASSERT(currentCallStack->isActive());
            if (js::CallStack *prev = currentCallStack->getPreviousInContext())
                JS_ASSERT(!prev->isActive());
        } else {
            JS_ASSERT_IF(currentCallStack, !currentCallStack->isActive());
        }
#endif
    }

    
    bool hasActiveCallStack() const {
        assertCallStacksInSync();
        return fp;
    }

    
    js::CallStack *activeCallStack() const {
        JS_ASSERT(hasActiveCallStack());
        return currentCallStack;
    }

    
    js::CallStack *getCurrentCallStack() const {
        assertCallStacksInSync();
        return currentCallStack;
    }

    
    void pushCallStackAndFrame(js::CallStack *newcs, JSStackFrame *newfp,
                               JSFrameRegs &regs);

    
    void popCallStackAndFrame();

    
    void saveActiveCallStack();

    
    void restoreCallStack();

    



    js::CallStack *containingCallStack(const JSStackFrame *target);

#ifdef JS_THREADSAFE
    JSThread            *thread;
    jsrefcount          requestDepth;
    
    jsrefcount          outstandingRequests;
# ifdef DEBUG
    unsigned            checkRequestDepth;
# endif    

    JSTitle             *lockedSealedTitle; 

    JSCList             threadLinks;        

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

    
    js::AutoGCRooter   *autoGCRooters;

    
    const JSDebugHooks  *debugHooks;

    
    JSSecurityCallbacks *securityCallbacks;

    
    JSArenaPool         regexpPool;

    
    uintN               resolveFlags;

    
    int64               rngSeed;

    
    jsval               iterValue;

#ifdef JS_TRACER
    




    js::TracerState     *tracerState;
    js::VMSideExit      *bailExit;

    









    bool                 jitEnabled;
#endif

    
    void updateJITEnabled() {
#ifdef JS_TRACER
        jitEnabled = ((options & JSOPTION_JIT) &&
                      (debugHooks == &js_NullDebugHooks ||
                       (debugHooks == &runtime->globalDebugHooks &&
                        !runtime->debuggerInhibitsJIT())));
#endif
    }

    DSTOffsetCache dstOffsetCache;

    
    JSObject *enumerators;

  private:
    





    js::Vector<JSGenerator *, 2, js::SystemAllocPolicy> genStack;

  public:
    
    JSGenerator *generatorFor(JSStackFrame *fp) const;

    
    inline bool ensureGeneratorStackSpace();

    bool enterGenerator(JSGenerator *gen) {
        return genStack.append(gen);
    }

    void leaveGenerator(JSGenerator *gen) {
        JS_ASSERT(genStack.back() == gen);
        genStack.popBack();
    }

#ifdef JS_THREADSAFE
    


    js::BackgroundSweepTask *gcSweepTask;
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

    inline void free(void* p) {
#ifdef JS_THREADSAFE
        if (gcSweepTask) {
            gcSweepTask->freeLater(p);
            return;
        }
#endif
        runtime->free(p);
    }

    




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

    js::StackSpace &stack() const {
        return JS_THREAD_DATA(this)->stackSpace;
    }

#ifdef DEBUG
    void assertValidStackDepth(uintN depth) {
        JS_ASSERT(0 <= regs->sp - StackBase(fp));
        JS_ASSERT(depth <= uintptr_t(regs->sp - StackBase(fp)));
    }
#else
    void assertValidStackDepth(uintN ) {}
#endif

private:

    





    void checkMallocGCPressure(void *p);
};

JS_ALWAYS_INLINE JSObject *
JSStackFrame::varobj(js::CallStack *cs) const
{
    JS_ASSERT(cs->contains(this));
    return fun ? callobj : cs->getInitialVarObj();
}

JS_ALWAYS_INLINE JSObject *
JSStackFrame::varobj(JSContext *cx) const
{
    JS_ASSERT(cx->activeCallStack()->contains(this));
    return fun ? callobj : cx->activeCallStack()->getInitialVarObj();
}

JS_ALWAYS_INLINE jsbytecode *
JSStackFrame::pc(JSContext *cx) const
{
    JS_ASSERT(cx->containingCallStack(this) != NULL);
    return cx->fp == this ? cx->regs->pc : savedPC;
}





namespace js {

JS_ALWAYS_INLINE void
StackSpace::popInvokeArgs(JSContext *cx, jsval *vp)
{
    JS_ASSERT(!currentCallStack->inContext());
    currentCallStack = currentCallStack->getPreviousInThread();
}

JS_ALWAYS_INLINE
InvokeArgsGuard::InvokeArgsGuard()
  : cx(NULL), cs(NULL), vp(NULL)
{}

JS_ALWAYS_INLINE
InvokeArgsGuard::InvokeArgsGuard(jsval *vp, uintN argc)
  : cx(NULL), cs(NULL), vp(vp), argc(argc)
{}

JS_ALWAYS_INLINE
InvokeArgsGuard::~InvokeArgsGuard()
{
    if (!cs)
        return;
    JS_ASSERT(cs == cx->stack().getCurrentCallStack());
    cx->stack().popInvokeArgs(cx, vp);
}

} 

#ifdef JS_THREADSAFE
# define JS_THREAD_ID(cx)       ((cx)->thread ? (cx)->thread->id : 0)
#endif

#if defined JS_THREADSAFE && defined DEBUG

namespace js {

class AutoCheckRequestDepth {
    JSContext *cx;
  public:
    AutoCheckRequestDepth(JSContext *cx) : cx(cx) { cx->checkRequestDepth++; }

    ~AutoCheckRequestDepth() {
        JS_ASSERT(cx->checkRequestDepth != 0);
        cx->checkRequestDepth--;
    }
};

}

# define CHECK_REQUEST(cx)                                                  \
    JS_ASSERT((cx)->requestDepth || (cx)->thread == (cx)->runtime->gcThread);\
    AutoCheckRequestDepth _autoCheckRequestDepth(cx);

#else
# define CHECK_REQUEST(cx)       ((void)0)
#endif

static inline uintN
FramePCOffset(JSContext *cx, JSStackFrame* fp)
{
    return uintN((fp->imacpc ? fp->imacpc : fp->pc(cx)) - fp->script->code);
}

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
        PARSER =       -4, 
        SCRIPT =       -5, 
        ENUMERATOR =   -6, 
        IDARRAY =      -7, 
        DESCRIPTORS =  -8, 
        NAMESPACES =   -9, 
        XML =         -10, 
        OBJECT =      -11, 
        ID =          -12, 
        VECTOR =      -13, 
        DESCRIPTOR =  -14  
    };

    private:
    
    AutoGCRooter(AutoGCRooter &ida);
    void operator=(AutoGCRooter &ida);
};

class AutoPreserveWeakRoots : private AutoGCRooter
{
  public:
    explicit AutoPreserveWeakRoots(JSContext *cx
                                   JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, WEAKROOTS), savedRoots(cx->weakRoots)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoPreserveWeakRoots()
    {
        context->weakRoots = savedRoots;
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

    void set(jsval v) {
        JS_ASSERT(tag == JSVAL);
        val = v;
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

namespace js {

#ifdef JS_THREADSAFE


class ThreadDataIter : public JSThread::Map::Range
{
  public:
    ThreadDataIter(JSRuntime *rt) : JSThread::Map::Range(rt->threads.all()) {}

    JSThreadData *threadData() const {
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

    JSThreadData *threadData() const {
        JS_ASSERT(!done);
        return &runtime->threadData;
    }
};

#endif  

} 






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




extern JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp);

extern void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation);





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




void
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

    bool append(jsval v) { return vector.append(v); }
    bool append(JSString *str) { return append(STRING_TO_JSVAL(str)); }
    bool append(JSObject *obj) { return append(OBJECT_TO_JSVAL(obj)); }
    bool append(jsdouble *dp) { return append(DOUBLE_TO_JSVAL(dp)); }

    void popBack() { vector.popBack(); }

    bool resize(size_t newLength) {
        return vector.resize(newLength);
    }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    jsval &operator[](size_t i) { return vector[i]; }
    jsval operator[](size_t i) const { return vector[i]; }

    const jsval *begin() const { return vector.begin(); }
    jsval *begin() { return vector.begin(); }

    const jsval *end() const { return vector.end(); }
    jsval *end() { return vector.end(); }

    jsval back() const { return end()[-1]; }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Vector<jsval, 8> vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

JSIdArray *
NewIdArray(JSContext *cx, jsint length);

}

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#ifdef JS_UNDEFD_MOZALLOC_WRAPPERS
#  include "mozilla/mozalloc_macro_wrappers.h"
#endif

#endif
