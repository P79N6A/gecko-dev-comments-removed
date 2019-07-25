






































#ifndef jscompartment_h___
#define jscompartment_h___

#include "jscntxt.h"
#include "jsgc.h"
#include "jsmath.h"
#include "jsobj.h"
#include "jsfun.h"
#include "jsgcstats.h"
#include "jsclist.h"
#include "jsxml.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251) /* Silence warning about JS_FRIEND_API and data members. */
#endif

namespace JSC {

class ExecutableAllocator;

}

namespace js {


typedef HashMap<jsbytecode*,
                size_t,
                DefaultHasher<jsbytecode*>,
                SystemAllocPolicy> RecordAttemptMap;


typedef HashMap<jsbytecode*,
                LoopProfile*,
                DefaultHasher<jsbytecode*>,
                SystemAllocPolicy> LoopProfileMap;

class Oracle;

typedef HashSet<JSScript *,
                DefaultHasher<JSScript *>,
                SystemAllocPolicy> TracedScriptSet;

typedef HashMap<JSFunction *,
                JSString *,
                DefaultHasher<JSFunction *>,
                SystemAllocPolicy> ToSourceCache;

struct TraceMonitor;


struct TracerState
{
    JSContext*     cx;                  
    TraceMonitor*  traceMonitor;        
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






struct TraceMonitor {
    









    JSContext               *tracecx;

    




    js::TracerState     *tracerState;
    js::VMSideExit      *bailExit;

    
    unsigned                iterationCounter;

    




    TraceNativeStorage      *storage;

    























    VMAllocator*            dataAlloc;
    VMAllocator*            traceAlloc;
    VMAllocator*            tempAlloc;
    nanojit::CodeAlloc*     codeAlloc;
    nanojit::Assembler*     assembler;
    FrameInfoCache*         frameCache;

    
    uintN                   flushEpoch;

    Oracle*                 oracle;
    TraceRecorder*          recorder;

    
    LoopProfile*            profile;

    GlobalState             globalStates[MONITOR_N_GLOBAL_STATES];
    TreeFragment            *vmfragments[FRAGMENT_TABLE_SIZE];
    RecordAttemptMap*       recordAttempts;

    
    LoopProfileMap*         loopProfiles;

    



    uint32                  maxCodeCacheBytes;

    




    JSBool                  needFlush;

    
    
    
    TypeMap*                cachedTempTypeMap;

    
    TracedScriptSet         tracedScripts;

#ifdef DEBUG
    
    nanojit::Seq<nanojit::Fragment*>* branches;
    uint32                  lastFragID;
    



    VMAllocator*            profAlloc;
    FragStatsMap*           profTab;
#endif

    bool ontrace() const {
        return !!tracecx;
    }

    
    void flush();

    
    void sweep(JSContext *cx);

    
    void mark(JSTracer *trc);

    bool outOfMemory() const;
};

namespace mjit {
class JaegerCompartment;
}
}


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

namespace js {

class NativeIterCache {
    static const size_t SIZE = size_t(1) << 8;
    
    
    JSObject            *data[SIZE];

    static size_t getIndex(uint32 key) {
        return size_t(key) % SIZE;
    }

  public:
    
    JSObject            *last;

    NativeIterCache()
      : last(NULL) {
        PodArrayZero(data);
    }

    void purge() {
        PodArrayZero(data);
        last = NULL;
    }

    JSObject *get(uint32 key) const {
        return data[getIndex(key)];
    }

    void set(uint32 key, JSObject *iterobj) {
        data[getIndex(key)] = iterobj;
    }
};








class DtoaCache {
    double        d;
    jsint         base;
    JSFixedString *s;      
  public:
    DtoaCache() : s(NULL) {}
    void purge() { s = NULL; }

    JSFixedString *lookup(jsint base, double d) {
        return this->s && base == this->base && d == this->d ? this->s : NULL;
    }

    void cache(jsint base, double d, JSFixedString *s) {
        this->base = base;
        this->d = d;
        this->s = s;
    }

};

} 

struct JS_FRIEND_API(JSCompartment) {
    JSRuntime                    *rt;
    JSPrincipals                 *principals;
    js::gc::Chunk                *chunk;

    js::gc::ArenaList            arenas[js::gc::FINALIZE_LIMIT];
    js::gc::FreeLists            freeLists;

    uint32                       gcBytes;
    uint32                       gcTriggerBytes;
    size_t                       gcLastBytes;

    bool                         hold;

#ifdef JS_GCMETER
    js::gc::JSGCArenaStats       compartmentStats[js::gc::FINALIZE_LIMIT];
#endif

#ifdef JS_TRACER
    
    js::TraceMonitor             traceMonitor;
#endif

    
    JSScript                     *scriptsToGC[JS_EVAL_CACHE_SIZE];

#ifdef DEBUG
    JSEvalCacheMeter             evalCacheMeter;
#endif

    void                         *data;
    bool                         active;  
    js::WrapperMap               crossCompartmentWrappers;

#ifdef JS_METHODJIT
    js::mjit::JaegerCompartment  *jaegerCompartment;
#endif

    


    js::PropertyTree             propertyTree;

#ifdef DEBUG
    
    jsrefcount                   livePropTreeNodes;
    jsrefcount                   totalPropTreeNodes;
    jsrefcount                   propTreeKidsChunks;
    jsrefcount                   liveDictModeNodes;
#endif

    



    js::EmptyShape               *emptyArgumentsShape;
    js::EmptyShape               *emptyBlockShape;
    js::EmptyShape               *emptyCallShape;
    js::EmptyShape               *emptyDeclEnvShape;
    js::EmptyShape               *emptyEnumeratorShape;
    js::EmptyShape               *emptyWithShape;

    typedef js::HashSet<js::EmptyShape *,
                        js::DefaultHasher<js::EmptyShape *>,
                        js::SystemAllocPolicy> EmptyShapeSet;

    EmptyShapeSet                emptyShapes;

    








    const js::Shape              *initialRegExpShape;
    const js::Shape              *initialStringShape;

    bool                         debugMode;  
    JSCList                      scripts;    

    JSC::ExecutableAllocator     *regExpAllocator;

    js::NativeIterCache          nativeIterCache;

    typedef js::LazilyConstructed<js::ToSourceCache> LazyToSourceCache;
    LazyToSourceCache            toSourceCache;

    JSCompartment(JSRuntime *rt);
    ~JSCompartment();

    bool init();

    
    void markCrossCompartmentWrappers(JSTracer *trc);

    bool wrap(JSContext *cx, js::Value *vp);
    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, JSObject **objp);
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, js::PropertyOp *op);
    bool wrap(JSContext *cx, js::StrictPropertyOp *op);
    bool wrap(JSContext *cx, js::PropertyDescriptor *desc);
    bool wrap(JSContext *cx, js::AutoIdVector &props);

    void sweep(JSContext *cx, uint32 releaseInterval);
    void purge(JSContext *cx);
    void finishArenaLists();
    void finalizeObjectArenaLists(JSContext *cx, JSGCInvocationKind gckind);
    void finalizeStringArenaLists(JSContext *cx, JSGCInvocationKind gckind);
    void finalizeShapeArenaLists(JSContext *cx, JSGCInvocationKind gckind);
    bool arenaListsAreEmpty();

    void setGCLastBytes(size_t lastBytes);
    void reduceGCTriggerBytes(uint32 amount);

    js::DtoaCache dtoaCache;

  private:
    js::MathCache                *mathCache;

    js::MathCache *allocMathCache(JSContext *cx);

    typedef js::HashMap<jsbytecode*,
                        size_t,
                        js::DefaultHasher<jsbytecode*>,
                        js::SystemAllocPolicy> BackEdgeMap;

    BackEdgeMap                  backEdgeTable;

    JSCompartment *thisForCtor() { return this; }
  public:
    js::MathCache *getMathCache(JSContext *cx) {
        return mathCache ? mathCache : allocMathCache(cx);
    }

    size_t backEdgeCount(jsbytecode *pc) const;
    size_t incBackEdgeCount(jsbytecode *pc);
};

#define JS_SCRIPTS_TO_GC(cx)    ((cx)->compartment->scriptsToGC)
#define JS_PROPERTY_TREE(cx)    ((cx)->compartment->propertyTree)

#ifdef DEBUG
#define JS_COMPARTMENT_METER(x) x
#else
#define JS_COMPARTMENT_METER(x)
#endif






static inline bool
JS_ON_TRACE(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_THREAD_DATA(cx)->onTraceCompartment)
        return JS_THREAD_DATA(cx)->onTraceCompartment->traceMonitor.ontrace();
#endif
    return false;
}

#ifdef JS_TRACER
static inline js::TraceMonitor *
JS_TRACE_MONITOR_ON_TRACE(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    return &JS_THREAD_DATA(cx)->onTraceCompartment->traceMonitor;
}






static inline js::TraceMonitor *
JS_TRACE_MONITOR_FROM_CONTEXT(JSContext *cx)
{
    return &cx->compartment->traceMonitor;
}
#endif

static inline js::TraceRecorder *
TRACE_RECORDER(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_THREAD_DATA(cx)->recordingCompartment)
        return JS_THREAD_DATA(cx)->recordingCompartment->traceMonitor.recorder;
#endif
    return NULL;
}

static inline js::LoopProfile *
TRACE_PROFILER(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_THREAD_DATA(cx)->profilingCompartment)
        return JS_THREAD_DATA(cx)->profilingCompartment->traceMonitor.profile;
#endif
    return NULL;
}

namespace js {
static inline MathCache *
GetMathCache(JSContext *cx)
{
    return cx->compartment->getMathCache(cx);
}
}

#ifdef DEBUG
# define EVAL_CACHE_METER(x)    (cx->compartment->evalCacheMeter.x++)
#else
# define EVAL_CACHE_METER(x)    ((void) 0)
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace js {

class PreserveCompartment {
  protected:
    JSContext *cx;
  private:
    JSCompartment *oldCompartment;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
     PreserveCompartment(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM) : cx(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        oldCompartment = cx->compartment;
    }

    ~PreserveCompartment() {
        cx->compartment = oldCompartment;
    }
};

class SwitchToCompartment : public PreserveCompartment {
  public:
    SwitchToCompartment(JSContext *cx, JSCompartment *newCompartment
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : PreserveCompartment(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->compartment = newCompartment;
    }

    SwitchToCompartment(JSContext *cx, JSObject *target JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : PreserveCompartment(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->compartment = target->getCompartment();
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AssertCompartmentUnchanged {
  protected:
    JSContext * const cx;
    JSCompartment * const oldCompartment;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
     AssertCompartmentUnchanged(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM)
     : cx(cx), oldCompartment(cx->compartment) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AssertCompartmentUnchanged() {
        JS_ASSERT(cx->compartment == oldCompartment);
    }
};

}

#endif
