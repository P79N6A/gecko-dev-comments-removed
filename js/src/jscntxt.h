







































#ifndef jscntxt_h___
#define jscntxt_h___



#include <string.h>


#ifdef mozilla_mozalloc_macro_wrappers_h
#  define JS_UNDEFD_MOZALLOC_WRAPPERS

#  include "mozilla/mozalloc_undef_macro_wrappers.h"
#endif

#include "jsprvtd.h"
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

namespace JSC {
    class ExecutableAllocator;
}

namespace js {

#ifdef JS_METHODJIT
struct VMFrame;
#endif


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

namespace mjit {
class CallStackIterator;
}







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
    js::Value*     nativeVp;

    
    
    JSFrameRegs    bailedSlowNativeRegs;

    TracerState(JSContext *cx, TraceMonitor *tm, TreeFragment *ti,
                uintN &inlineCallCountp, VMSideExit** innermostNestedGuardp);
    ~TracerState();
};

#ifdef JS_METHODJIT
namespace mjit {
    struct Trampolines
    {
        void (* forceReturn)();
        JSC::ExecutablePool *forceReturnPool;
    };

    struct ThreadData
    {
        JSC::ExecutableAllocator *execPool;

        
        Trampolines trampolines;

        VMFrame *activeFrame;

        bool Initialize();
        void Finish();
    };
}
#endif 







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

















































class StackSegment
{
    
    JSContext           *cx;

    
    StackSegment        *previousInContext;

    
    StackSegment        *previousInMemory;

    
    JSStackFrame        *initialFrame;

    
    JSStackFrame        *suspendedFrame;

    
    JSFrameRegs         *suspendedRegs;

    
    JSObject            *initialVarObj;

    
    bool                saved;

    



#define NON_NULL_SUSPENDED_FRAME ((JSStackFrame *)0x1)

  public:
    StackSegment()
      : cx(NULL), previousInContext(NULL), previousInMemory(NULL),
        initialFrame(NULL), suspendedFrame(NON_NULL_SUSPENDED_FRAME),
        suspendedRegs(NULL), initialVarObj(NULL), saved(false)
    {
        JS_ASSERT(!inContext());
    }

    

    Value *previousSegmentEnd() const {
        return (Value *)this;
    }

    Value *getInitialArgBegin() const {
        return (Value *)(this + 1);
    }

    










    bool inContext() const {
        JS_ASSERT(!!cx == !!initialFrame);
        JS_ASSERT_IF(!cx, suspendedFrame == NON_NULL_SUSPENDED_FRAME && !saved);
        return cx;
    }

    bool isActive() const {
        JS_ASSERT_IF(!suspendedFrame, cx && !saved);
        JS_ASSERT_IF(!cx, suspendedFrame == NON_NULL_SUSPENDED_FRAME);
        return !suspendedFrame;
    }

    bool isSuspended() const {
        JS_ASSERT_IF(!cx || !suspendedFrame, !saved);
        JS_ASSERT_IF(!cx, suspendedFrame == NON_NULL_SUSPENDED_FRAME);
        return cx && suspendedFrame;
    }

    

    bool isSaved() const {
        JS_ASSERT_IF(saved, isSuspended());
        return saved;
    }

    

    void joinContext(JSContext *cx, JSStackFrame *f) {
        JS_ASSERT(!inContext());
        this->cx = cx;
        initialFrame = f;
        suspendedFrame = NULL;
        JS_ASSERT(isActive());
    }

    void leaveContext() {
        JS_ASSERT(isActive());
        this->cx = NULL;
        initialFrame = NULL;
        suspendedFrame = NON_NULL_SUSPENDED_FRAME;
        JS_ASSERT(!inContext());
    }

    JSContext *maybeContext() const {
        return cx;
    }

#undef NON_NULL_SUSPENDED_FRAME

    

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
        JS_ASSERT(!isSuspended());
        suspend(fp, regs);
        saved = true;
        JS_ASSERT(isSaved());
    }

    void restore() {
        JS_ASSERT(isSaved());
        saved = false;
        resume();
        JS_ASSERT(!isSuspended());
    }

    

    JSStackFrame *getInitialFrame() const {
        JS_ASSERT(inContext());
        return initialFrame;
    }

    inline JSStackFrame *getCurrentFrame() const;
    inline JSFrameRegs *getCurrentRegs() const;

    

    JSStackFrame *getSuspendedFrame() const {
        JS_ASSERT(isSuspended());
        return suspendedFrame;
    }

    JSFrameRegs *getSuspendedRegs() const {
        JS_ASSERT(isSuspended());
        return suspendedRegs;
    }

    

    void setPreviousInContext(StackSegment *seg) {
        previousInContext = seg;
    }

    StackSegment *getPreviousInContext() const  {
        return previousInContext;
    }

    void setPreviousInMemory(StackSegment *seg) {
        previousInMemory = seg;
    }

    StackSegment *getPreviousInMemory() const  {
        return previousInMemory;
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

static const size_t VALUES_PER_STACK_SEGMENT = sizeof(StackSegment) / sizeof(Value);
JS_STATIC_ASSERT(sizeof(StackSegment) % sizeof(Value) == 0);


class InvokeArgsGuard : public CallArgs
{
    friend class StackSpace;
    JSContext        *cx;  
    StackSegment     *seg;
    Value            *prevInvokeArgEnd;
#ifdef DEBUG
    StackSegment     *prevInvokeSegment;
    JSStackFrame     *prevInvokeFrame;
#endif
  public:
    inline InvokeArgsGuard() : cx(NULL), seg(NULL) {}
    inline InvokeArgsGuard(JSContext *cx, Value *vp, uintN argc);
    inline ~InvokeArgsGuard();
    bool pushed() const { return cx != NULL; }
};





struct InvokeArgsAlreadyOnTheStack : CallArgs
{
    InvokeArgsAlreadyOnTheStack(Value *vp, uintN argc) : CallArgs(vp + 2, argc) {}
};


class InvokeFrameGuard
{
    friend class StackSpace;
    JSContext        *cx;  
    JSStackFrame     *fp;
    JSFrameRegs      regs;
    JSFrameRegs      *prevRegs;
  public:
    InvokeFrameGuard() : cx(NULL), fp(NULL) {}
    JS_REQUIRES_STACK ~InvokeFrameGuard();
    bool pushed() const { return cx != NULL; }
    JSStackFrame *getFrame() { return fp; }
    JSFrameRegs &getRegs() { return regs; }
};


class ExecuteFrameGuard
{
    friend class StackSpace;
    JSContext        *cx;  
    StackSegment     *seg;
    Value            *vp;
    JSStackFrame     *fp;
    JSStackFrame     *down;
  public:
    ExecuteFrameGuard() : cx(NULL), vp(NULL), fp(NULL) {}
    JS_REQUIRES_STACK ~ExecuteFrameGuard();
    bool pushed() const { return cx != NULL; }
    Value *getvp() const { return vp; }
    JSStackFrame *getFrame() const { return fp; }
};





































































class StackSpace
{
    Value *base;
#ifdef XP_WIN
    mutable Value *commitEnd;
#endif
    Value *end;
    StackSegment *currentSegment;
#ifdef DEBUG
    




    StackSegment *invokeSegment;
    JSStackFrame *invokeFrame;
#endif
    Value        *invokeArgEnd;

    JS_REQUIRES_STACK bool pushSegmentForInvoke(JSContext *cx, uintN argc,
                                                InvokeArgsGuard &ag);
    JS_REQUIRES_STACK bool pushInvokeFrameSlow(JSContext *cx, const InvokeArgsGuard &ag,
                                               InvokeFrameGuard &fg);
    JS_REQUIRES_STACK void popInvokeFrameSlow(const CallArgs &args);
    JS_REQUIRES_STACK void popSegmentForInvoke(const InvokeArgsGuard &ag);

    
    friend class InvokeArgsGuard;
    JS_REQUIRES_STACK inline void popInvokeArgs(const InvokeArgsGuard &args);
    friend class InvokeFrameGuard;
    JS_REQUIRES_STACK void popInvokeFrame(const InvokeFrameGuard &ag);
    friend class ExecuteFrameGuard;
    JS_REQUIRES_STACK void popExecuteFrame(JSContext *cx);

    
    JS_REQUIRES_STACK
    inline Value *firstUnused() const;

    inline bool isCurrentAndActive(JSContext *cx) const;
    friend class AllFramesIter;
    StackSegment *getCurrentSegment() const { return currentSegment; }

    



    inline bool ensureSpace(JSContext *maybecx, Value *from, ptrdiff_t nvals) const;

#ifdef XP_WIN
    
    JS_FRIEND_API(bool) bumpCommit(Value *from, ptrdiff_t nvals) const;
#endif

  public:
    static const size_t CAPACITY_VALS   = 512 * 1024;
    static const size_t CAPACITY_BYTES  = CAPACITY_VALS * sizeof(Value);
    static const size_t COMMIT_VALS     = 16 * 1024;
    static const size_t COMMIT_BYTES    = COMMIT_VALS * sizeof(Value);

    
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
      (VALUES_PER_STACK_SEGMENT + VALUES_PER_STACK_FRAME );

    
    JS_REQUIRES_STACK void mark(JSTracer *trc);

    















    





    JS_REQUIRES_STACK
    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard &ag);

    
    bool getInvokeFrame(JSContext *cx, const CallArgs &args,
                        uintN nmissing, uintN nfixed,
                        InvokeFrameGuard &fg) const;

    JS_REQUIRES_STACK
    void pushInvokeFrame(JSContext *cx, const CallArgs &args, InvokeFrameGuard &fg);

    




    JS_REQUIRES_STACK
    bool getExecuteFrame(JSContext *cx, JSStackFrame *down,
                         uintN vplen, uintN nfixed,
                         ExecuteFrameGuard &fg) const;
    JS_REQUIRES_STACK
    void pushExecuteFrame(JSContext *cx, ExecuteFrameGuard &fg,
                          JSFrameRegs &regs, JSObject *initialVarObj);

    



    JS_REQUIRES_STACK
    inline JSStackFrame *getInlineFrame(JSContext *cx, Value *sp,
                                        uintN nmissing, uintN nfixed) const;

    JS_REQUIRES_STACK
    inline JSStackFrame *getInlineFrameUnchecked(JSContext *cx, Value *sp,
                                                 uintN nmissing) const;

    JS_REQUIRES_STACK
    inline void pushInlineFrame(JSContext *cx, JSStackFrame *fp, jsbytecode *pc,
                                JSStackFrame *newfp);

    JS_REQUIRES_STACK
    inline void popInlineFrame(JSContext *cx, JSStackFrame *up, JSStackFrame *down);

    



    JS_REQUIRES_STACK
    void getSynthesizedSlowNativeFrame(JSContext *cx, StackSegment *&seg, JSStackFrame *&fp);

    JS_REQUIRES_STACK
    void pushSynthesizedSlowNativeFrame(JSContext *cx, StackSegment *seg, JSStackFrame *fp,
                                        JSFrameRegs &regs);

    JS_REQUIRES_STACK
    void popSynthesizedSlowNativeFrame(JSContext *cx);

    


    inline bool ensureSpace(JSContext *maybecx, Value *start, Value *from,
                            Value *& limit, uint32 nslots) const;

    



    inline Value *makeStackLimit(Value *start) const;

};

JS_STATIC_ASSERT(StackSpace::CAPACITY_VALS % StackSpace::COMMIT_VALS == 0);











class FrameRegsIter
{
    StackSegment      *curseg;
    JSStackFrame      *curfp;
    Value             *cursp;
    jsbytecode        *curpc;

    void initSlow();
    void incSlow(JSStackFrame *up, JSStackFrame *down);
    static inline Value *contiguousDownFrameSP(JSStackFrame *up);

  public:
    JS_REQUIRES_STACK inline FrameRegsIter(JSContext *cx);

    bool done() const { return curfp == NULL; }
    inline FrameRegsIter &operator++();

    JSStackFrame *fp() const { return curfp; }
    Value *sp() const { return cursp; }
    jsbytecode *pc() const { return curpc; }
};




class AllFramesIter
{
public:
    AllFramesIter(JSContext *cx);

    bool done() const { return curfp == NULL; }
    AllFramesIter& operator++();

    JSStackFrame *fp() const { return curfp; }

private:
    StackSegment *curcs;
    JSStackFrame *curfp;
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


#ifndef JS_EVAL_CACHE_SHIFT
# define JS_EVAL_CACHE_SHIFT        6
#endif
#define JS_EVAL_CACHE_SIZE          JS_BIT(JS_EVAL_CACHE_SHIFT)

#ifdef DEBUG
# define EVAL_CACHE_METER_LIST(_)   _(probe), _(hit), _(step), _(noscope)
# define identity(x)                x

struct JSEvalCacheMeter {
    uint64 EVAL_CACHE_METER_LIST(identity);
};

# undef identity
#endif

#ifdef DEBUG
# define FUNCTION_KIND_METER_LIST(_)                                          \
                        _(allfun), _(heavy), _(nofreeupvar), _(onlyfreevar),  \
                        _(display), _(flat), _(setupvar), _(badfunarg),       \
                        _(joinedsetmethod), _(joinedinitmethod),              \
                        _(joinedreplace), _(joinedsort), _(joinedmodulepat),  \
                        _(mreadbarrier), _(mwritebarrier), _(mwslotbarrier),  \
                        _(unjoined)
# define identity(x)    x

struct JSFunctionMeter {
    int32 FUNCTION_KIND_METER_LIST(identity);
};

# undef identity

# define JS_FUNCTION_METER(cx,x) JS_RUNTIME_METER((cx)->runtime, functionMeter.x)
#else
# define JS_FUNCTION_METER(cx,x) ((void)0)
#endif


#define NATIVE_ITER_CACHE_LOG2  8
#define NATIVE_ITER_CACHE_MASK  JS_BITMASK(NATIVE_ITER_CACHE_LOG2)
#define NATIVE_ITER_CACHE_SIZE  JS_BIT(NATIVE_ITER_CACHE_LOG2)

struct JSPendingProxyOperation {
    JSPendingProxyOperation *next;
    JSObject *object;
};

struct JSThreadData {
    



    volatile jsword     interruptFlags;

    JSGCFreeLists       gcFreeLists;

    
    js::StackSpace      stackSpace;

    




    bool                waiveGCQuota;

    



    JSGSNCache          gsnCache;

    
    js::PropertyCache   propertyCache;

#ifdef JS_TRACER
    
    js::TraceMonitor    traceMonitor;
#endif

#ifdef JS_METHODJIT
    js::mjit::ThreadData jmData;
#endif

    
    JSScript            *scriptsToGC[JS_EVAL_CACHE_SIZE];

#ifdef DEBUG
    JSEvalCacheMeter    evalCacheMeter;
#endif

    
    DtoaState           *dtoaState;

    




    struct {
        jsdouble d;
        jsint    base;
        JSString *s;        
    } dtoaCache;

    
    JSObject            *cachedNativeIterators[NATIVE_ITER_CACHE_SIZE];

    
    JSObject            *lastNativeIterator;

    
    jsuword             *nativeStackBase;

    
    JSPendingProxyOperation *pendingProxyOperation;

    js::ConservativeGCThreadData conservativeGC;

    bool init();
    void finish();
    void mark(JSTracer *trc);
    void purge(JSContext *cx);

    static const jsword INTERRUPT_OPERATION_CALLBACK = 0x1;

    void triggerOperationCallback() {
        





        JS_ATOMIC_SET_MASK(&interruptFlags, INTERRUPT_OPERATION_CALLBACK);
    }
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

    


    JSContext           *requestContext;

    
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

struct GCPtrHasher
{
    typedef void *Lookup;
    
    static HashNumber hash(void *key) {
        return HashNumber(uintptr_t(key) >> JS_GCTHING_ZEROBITS);
    }

    static bool match(void *l, void *k) {
        return l == k;
    }
};

typedef HashMap<void *, uint32, GCPtrHasher, SystemAllocPolicy> GCLocks;

struct RootInfo {
    RootInfo() {}
    RootInfo(const char *name, JSGCRootType type) : name(name), type(type) {}
    const char *name;
    JSGCRootType type;
};

typedef js::HashMap<void *,
                    RootInfo,
                    js::DefaultHasher<void *>,
                    js::SystemAllocPolicy> RootedValueMap;


JS_STATIC_ASSERT(sizeof(HashNumber) == 4);
    
struct WrapperHasher
{
    typedef Value Lookup;

    static HashNumber hash(Value key) {
        uint64 bits = JSVAL_BITS(Jsvalify(key));
        return (uint32)bits ^ (uint32)(bits >> 32);
    }

    static bool match(const Value &l, const Value &k) {
        return l == k;
    }
};

typedef HashMap<Value, Value, WrapperHasher, SystemAllocPolicy> WrapperMap;

class AutoValueVector;
class AutoIdVector;

} 

struct JSCompartment {
    JSRuntime *rt;
    JSPrincipals *principals;
    void *data;
    bool marked;
    js::WrapperMap crossCompartmentWrappers;
    bool debugMode;

    
    JSCList scripts;

    JSCompartment(JSRuntime *cx);
    ~JSCompartment();

    bool init();

    bool wrap(JSContext *cx, js::Value *vp);
    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, JSObject **objp);
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, js::PropertyOp *op);
    bool wrap(JSContext *cx, js::PropertyDescriptor *desc);
    bool wrap(JSContext *cx, js::AutoIdVector &props);
    bool wrapException(JSContext *cx);

    void sweep(JSContext *cx);

#ifdef JS_METHODJIT
    bool addScript(JSContext *cx, JSScript *script);
    void removeScript(JSScript *script);
#endif
    void purge(JSContext *cx);
};

typedef void
(* JSActivityCallback)(void *arg, JSBool active);

struct JSRuntime {
    
    JSCompartment       *defaultCompartment;

    
    js::Vector<JSCompartment *, 0, js::SystemAllocPolicy> compartments;

    
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

    
    js::GCChunkInfoVector gcFreeArenaChunks;
#ifdef DEBUG
    JSGCArena           *gcEmptyArenaList;
#endif
    JSGCArenaList       gcArenaList[FINALIZE_LIMIT];
    js::RootedValueMap  gcRootsHash;
    js::GCLocks         gcLocksHash;
    jsrefcount          gcKeepAtoms;
    size_t              gcBytes;
    size_t              gcLastBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;
    uint32              gcEmptyArenaPoolLifespan;
    uint32              gcNumber;
    js::GCMarker        *gcMarkingTracer;
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

    
    js::Value           NaNValue;
    js::Value           negativeInfinityValue;
    js::Value           positiveInfinityValue;

    js::DeflatedStringCache *deflatedStringCache;

    JSString            *emptyString;

    
    JSCList             contextList;

    
    JSDebugHooks        globalDebugHooks;

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

    JSC::ExecutableAllocator *regExpAllocator;

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
#define JS_METHODJIT_DATA(cx)   (JS_THREAD_DATA(cx)->jmData)
#define JS_SCRIPTS_TO_GC(cx)    (JS_THREAD_DATA(cx)->scriptsToGC)

#ifdef DEBUG
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

namespace js {

class RegExp;

class RegExpStatics
{
    js::Vector<int, 20>         matchPairs;
    JSContext                   *cx;
    JSString                    *input;
    uintN                       flags;

    bool createDependent(size_t start, size_t end, Value *out) const;

    size_t pairCount() const {
        JS_ASSERT(matchPairs.length() % 2 == 0);
        return matchPairs.length() / 2;
    }
    




    bool makeMatch(size_t checkValidIndex, size_t pairNum, Value *out) const;
    static const uintN allFlags = JSREG_FOLD | JSREG_GLOB | JSREG_STICKY | JSREG_MULTILINE;
    friend class RegExp;

  public:
    explicit RegExpStatics(JSContext *cx) : matchPairs(cx), cx(cx) { clear(); }
    void clone(const RegExpStatics &other);

    

    void setMultiline(bool enabled) {
        if (enabled)
            flags = flags | JSREG_MULTILINE;
        else
            flags = flags & ~JSREG_MULTILINE;
    }

    void clear() {
        input = 0;
        flags = 0;
        matchPairs.clear();
    }

    void checkInvariants() {
        if (pairCount() > 0) {
            JS_ASSERT(input);
            JS_ASSERT(get(0, 0) <= get(0, 1));
            JS_ASSERT(get(0, 1) <= int(input->length()));
        }
    }

    void reset(JSString *newInput, bool newMultiline) {
        clear();
        input = newInput;
        setMultiline(newMultiline);
        checkInvariants();
    }

    void setInput(JSString *newInput) {
        input = newInput;
    }

    

    JSString *getInput() const { return input; }
    uintN getFlags() const { return flags; }
    bool multiline() const { return flags & JSREG_MULTILINE; }
    bool matched() const { JS_ASSERT(pairCount() > 0); return get(0, 1) - get(0, 0) > 0; }
    size_t getParenCount() const { JS_ASSERT(pairCount() > 0); return pairCount() - 1; }

    void mark(JSTracer *trc) const {
        if (input)
            JS_CALL_STRING_TRACER(trc, input, "res->input");
    }

    size_t getParenLength(size_t parenNum) const {
        if (pairCount() <= parenNum + 1)
            return 0;
        return get(parenNum + 1, 1) - get(parenNum + 1, 0);
    }

    int get(size_t pairNum, bool which) const {
        JS_ASSERT(pairNum < pairCount());
        return matchPairs[2 * pairNum + which];
    }

    

    bool createInput(Value *out) const;
    bool createLastMatch(Value *out) const { return makeMatch(0, 0, out); }
    bool createLastParen(Value *out) const;
    bool createLeftContext(Value *out) const;
    bool createRightContext(Value *out) const;

    bool createParen(size_t parenNum, Value *out) const {
        return makeMatch((parenNum + 1) * 2, parenNum + 1, out);
    }

    

    void getParen(size_t num, JSSubString *out) const;
    void getLastMatch(JSSubString *out) const;
    void getLastParen(JSSubString *out) const;
    void getLeftContext(JSSubString *out) const;
    void getRightContext(JSSubString *out) const;
};

}

struct JSContext
{
    explicit JSContext(JSRuntime *rt);

    
    JSCList             link;

    
    uint16              version;

    
    uint32              options;            

    
    JSLocaleCallbacks   *localeCallbacks;

    





    JSDHashTable        *resolvingTable;

    



    JSPackedBool        generatingError;

    
    JSBool              throwing;           
    js::Value           exception;          

    
    jsuword             stackLimit;

    
    size_t              scriptStackQuota;

    
    JSRuntime *const    runtime;

    
    JSCompartment       *compartment;

    
    JS_REQUIRES_STACK
    JSStackFrame        *fp;

    



    JS_REQUIRES_STACK
    JSFrameRegs         *regs;

  public:
    friend class js::StackSpace;
    friend bool js::Interpret(JSContext *, JSStackFrame *, uintN);

    
    void setCurrentFrame(JSStackFrame *fp) {
        this->fp = fp;
    }

    void setCurrentRegs(JSFrameRegs *regs) {
        this->regs = regs;
    }

    
    JSArenaPool         tempPool;

    
    JSArenaPool         regExpPool;

    
    JSObject            *globalObject;

    
    JSWeakRoots         weakRoots;

    
    js::RegExpStatics   regExpStatics;

    
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
    
    js::StackSegment *currentSegment;

  public:
    void assertSegmentsInSync() const {
#ifdef DEBUG
        if (fp) {
            JS_ASSERT(currentSegment->isActive());
            if (js::StackSegment *prev = currentSegment->getPreviousInContext())
                JS_ASSERT(!prev->isActive());
        } else {
            JS_ASSERT_IF(currentSegment, !currentSegment->isActive());
        }
#endif
    }

    
    bool hasActiveSegment() const {
        assertSegmentsInSync();
        return !!fp;
    }

    
    js::StackSegment *activeSegment() const {
        JS_ASSERT(hasActiveSegment());
        return currentSegment;
    }

    
    js::StackSegment *getCurrentSegment() const {
        assertSegmentsInSync();
        return currentSegment;
    }

    
    void pushSegmentAndFrame(js::StackSegment *newseg, JSStackFrame *newfp,
                             JSFrameRegs &regs);

    
    void popSegmentAndFrame();

    
    void saveActiveSegment();

    
    void restoreSegment();

    



    js::StackSegment *containingSegment(const JSStackFrame *target);

    


    JSStackFrame *findFrameAtLevel(uintN targetLevel) {
        JSStackFrame *fp = this->fp;
        while (true) {
            JS_ASSERT(fp && fp->hasScript());
            if (fp->getScript()->staticLevel == targetLevel)
                break;
            fp = fp->down;
        }
        return fp;
    }
 
#ifdef JS_THREADSAFE
    JSThread            *thread;
    jsrefcount          requestDepth;
    
    jsrefcount          outstandingRequests;
    JSContext           *prevRequestContext;
    jsrefcount          prevRequestDepth;
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

    
    uintN               resolveFlags;

    
    int64               rngSeed;

    
    js::Value           iterValue;

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

#ifdef MOZ_TRACE_JSCALLS
    
    JSFunctionCallback    functionCallback;

    void doFunctionCallback(const JSFunction *fun,
                            const JSScript *scr,
                            JSBool entering) const
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
        JS_ASSERT(0 <= regs->sp - fp->base());
        JS_ASSERT(depth <= uintptr_t(regs->sp - fp->base()));
    }
#else
    void assertValidStackDepth(uintN ) {}
#endif

private:

    





    JS_FRIEND_API(void) checkMallocGCPressure(void *p);
};

static inline void
js_TraceRegExpStatics(JSTracer *trc, JSContext *acx)
{
    acx->regExpStatics.mark(trc);
}

JS_ALWAYS_INLINE JSObject *
JSStackFrame::varobj(js::StackSegment *seg) const
{
    JS_ASSERT(seg->contains(this));
    return hasFunction() ? maybeCallObj() : seg->getInitialVarObj();
}

JS_ALWAYS_INLINE JSObject *
JSStackFrame::varobj(JSContext *cx) const
{
    JS_ASSERT(cx->activeSegment()->contains(this));
    return hasFunction() ? maybeCallObj() : cx->activeSegment()->getInitialVarObj();
}

JS_ALWAYS_INLINE jsbytecode *
JSStackFrame::pc(JSContext *cx) const
{
    JS_ASSERT(cx->containingSegment(this) != NULL);
    return (cx->fp == this) ? cx->regs->pc : savedPC;
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
    jsbytecode *pc = fp->hasIMacroPC() ? fp->getIMacroPC() : fp->pc(cx);
    return uintN(pc - fp->getScript()->code);
}

static inline JSAtom **
FrameAtomBase(JSContext *cx, JSStackFrame *fp)
{
    return fp->hasIMacroPC()
           ? COMMON_ATOMS_START(&cx->runtime->atomState)
           : fp->getScript()->atomMap.vector;
}

namespace js {

class AutoGCRooter {
  public:
    AutoGCRooter(JSContext *cx, ptrdiff_t tag)
      : down(cx->autoGCRooters), tag(tag), context(cx)
    {
        JS_ASSERT(this != cx->autoGCRooters);
#ifdef JS_THREADSAFE
        JS_ASSERT(cx->requestDepth != 0);
#endif
        cx->autoGCRooters = this;
    }

    ~AutoGCRooter() {
        JS_ASSERT(this == context->autoGCRooters);
#ifdef JS_THREADSAFE
        JS_ASSERT(context->requestDepth != 0);
#endif
        context->autoGCRooters = down;
    }

    
    inline void trace(JSTracer *trc);

#ifdef __GNUC__
# pragma GCC visibility push(default)
#endif
    friend void ::js_TraceContext(JSTracer *trc, JSContext *acx);
    friend void ::js_TraceRuntime(JSTracer *trc);
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
        VALVECTOR =   -13, 
        DESCRIPTOR =  -14, 
        STRING =      -15, 
        IDVECTOR =    -16  
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
    friend void ::js_TraceRuntime(JSTracer *trc);

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
    friend void ::js_TraceRuntime(JSTracer *trc);

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

class AutoScopePropertyRooter : private AutoGCRooter {
  public:
    AutoScopePropertyRooter(JSContext *cx, JSScopeProperty *sprop
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, SPROP), sprop(sprop)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void ::js_TraceRuntime(JSTracer *trc);

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
    friend void ::js_TraceRuntime(JSTracer *trc);

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
#ifdef DEBUG
            JSBool ok =
#endif
            obj->enumerate(context, JSENUMERATE_DESTROY, &stateValue, 0);
            JS_ASSERT(ok);
        }
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    const Value &state() const { return stateValue; }
    Value *addr() { return &stateValue; }

  protected:
    void trace(JSTracer *trc) {
        JS_CALL_OBJECT_TRACER(trc, obj, "js::AutoEnumStateRooter.obj");
    }

    JSObject * const obj;

  private:
    Value stateValue;
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
    friend void ::js_TraceRuntime(JSTracer *trc);

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

class AutoArenaAllocator {
    JSArenaPool *pool;
    void        *mark;
  public:
    explicit AutoArenaAllocator(JSArenaPool *pool) : pool(pool) { mark = JS_ARENA_MARK(pool); }
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
    AutoReleasePtr operator=(const AutoReleasePtr &other);
  public:
    explicit AutoReleasePtr(JSContext *cx, void *ptr) : cx(cx), ptr(ptr) {}
    ~AutoReleasePtr() { cx->free(ptr); }
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

extern JS_FRIEND_API(void)
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





#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) < (cx)->stackLimit)
#else
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) > (cx)->stackLimit)
#endif

#ifdef JS_THREADSAFE
# define JS_ASSERT_REQUEST_DEPTH(cx)  JS_ASSERT((cx)->requestDepth >= 1)
#else
# define JS_ASSERT_REQUEST_DEPTH(cx)  ((void) 0)
#endif






#define JS_CHECK_OPERATION_LIMIT(cx)                                          \
    (JS_ASSERT_REQUEST_DEPTH(cx),                                             \
     (!(JS_THREAD_DATA(cx)->interruptFlags & JSThreadData::INTERRUPT_OPERATION_CALLBACK) || js_InvokeOperationCallback(cx)))





extern JSBool
js_InvokeOperationCallback(JSContext *cx);

extern JSBool
js_HandleExecutionInterrupt(JSContext *cx);


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
    if (!obj->parent)
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

extern void
SetPendingException(JSContext *cx, const Value &v);

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
        : AutoGCRooter(cx, VALVECTOR), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool append(const Value &v) { return vector.append(v); }

    void popBack() { vector.popBack(); }

    bool growBy(size_t inc) {
        
        size_t oldLength = vector.length();
        if (!vector.growByUninitialized(inc))
            return false;
        MakeValueRangeGCSafe(vector.begin() + oldLength, vector.end());
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
        MakeValueRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    Value &operator[](size_t i) { return vector[i]; }
    const Value &operator[](size_t i) const { return vector[i]; }

    const Value *begin() const { return vector.begin(); }
    Value *begin() { return vector.begin(); }

    const Value *end() const { return vector.end(); }
    Value *end() { return vector.end(); }

    const jsval *jsval_begin() const { return Jsvalify(begin()); }
    jsval *jsval_begin() { return Jsvalify(begin()); }

    const jsval *jsval_end() const { return Jsvalify(end()); }
    jsval *jsval_end() { return Jsvalify(end()); }

    const Value &back() const { return vector.back(); }

    friend void AutoGCRooter::trace(JSTracer *trc);
    
  private:
    Vector<Value, 8> vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdVector : private AutoGCRooter
{
  public:
    explicit AutoIdVector(JSContext *cx
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, IDVECTOR), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool append(jsid id) { return vector.append(id); }

    void popBack() { vector.popBack(); }

    bool growBy(size_t inc) {
        
        size_t oldLength = vector.length();
        if (!vector.growByUninitialized(inc))
            return false;
        MakeIdRangeGCSafe(vector.begin() + oldLength, vector.end());
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
        MakeIdRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    jsid &operator[](size_t i) { return vector[i]; }
    const jsid &operator[](size_t i) const { return vector[i]; }

    const jsid *begin() const { return vector.begin(); }
    jsid *begin() { return vector.begin(); }

    const jsid *end() const { return vector.end(); }
    jsid *end() { return vector.end(); }

    const jsid &back() const { return vector.back(); }

    friend void AutoGCRooter::trace(JSTracer *trc);
    
  private:
    Vector<jsid, 8> vector;
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
