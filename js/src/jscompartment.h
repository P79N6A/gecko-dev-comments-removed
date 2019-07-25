






































#ifndef jscompartment_h___
#define jscompartment_h___

#include "jsclist.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcstats.h"
#include "jsobj.h"
#include "vm/GlobalObject.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251) /* Silence warning about JS_FRIEND_API and data members. */
#endif

namespace JSC { class ExecutableAllocator; }
namespace WTF { class BumpPointerAllocator; }

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
    VMSideExit**   innermostNestedGuardp;
    VMSideExit*    innermost;
    uint64         startTime;
    TracerState*   prev;

    
    
    
    uint32         builtinStatus;

    
    double*        deepBailSp;

    
    uintN          nativeVpLen;
    js::Value*     nativeVp;

    TracerState(JSContext *cx, TraceMonitor *tm, TreeFragment *ti,
                VMSideExit** innermostNestedGuardp);
    ~TracerState();
};







struct TraceNativeStorage
{
    
    static const size_t MAX_NATIVE_STACK_SLOTS  = 4096;
    static const size_t MAX_CALL_STACK_ENTRIES  = 500;

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

    void logFragProfile();
#endif

    TraceMonitor();
    ~TraceMonitor();

    bool init(JSRuntime* rt);

    bool ontrace() const {
        return !!tracecx;
    }

    
    void flush();

    
    void sweep(JSContext *cx);

    
    void mark(JSTracer *trc);

    bool outOfMemory() const;

    JS_FRIEND_API(void) getCodeAllocStats(size_t &total, size_t &frag_size, size_t &free_size) const;
    JS_FRIEND_API(size_t) getVMAllocatorsMainSize(JSUsableSizeFun usf) const;
    JS_FRIEND_API(size_t) getVMAllocatorsReserveSize(JSUsableSizeFun usf) const;
    JS_FRIEND_API(size_t) getTraceMonitorSize(JSUsableSizeFun usf) const;
};

namespace mjit {
class JaegerCompartment;
}
}


extern JSClass js_dummy_class;

#ifndef JS_EVAL_CACHE_SHIFT
# define JS_EVAL_CACHE_SHIFT        6
#endif


#define JS_EVAL_CACHE_SIZE          JS_BIT(JS_EVAL_CACHE_SHIFT)

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

class MathCache;








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

struct ScriptFilenameEntry
{
    bool marked;
    char filename[1];
};

struct ScriptFilenameHasher
{
    typedef const char *Lookup;
    static HashNumber hash(const char *l) { return JS_HashString(l); }
    static bool match(const ScriptFilenameEntry *e, const char *l) {
        return strcmp(e->filename, l) == 0;
    }
};

typedef HashSet<ScriptFilenameEntry *,
                ScriptFilenameHasher,
                SystemAllocPolicy> ScriptFilenameTable;

} 

struct JS_FRIEND_API(JSCompartment) {
    JSRuntime                    *rt;
    JSPrincipals                 *principals;

    js::gc::ArenaLists           arenas;

    uint32                       gcBytes;
    uint32                       gcTriggerBytes;
    size_t                       gcLastBytes;

    bool                         hold;
    bool                         isSystemCompartment;

    




    JSArenaPool                  pool;
    bool                         activeAnalysis;
    bool                         activeInference;

    
    js::types::TypeCompartment   types;

#ifdef JS_TRACER
  private:
    



    js::TraceMonitor             *traceMonitor_;
#endif

  public:
    
    JSScript                     *evalCache[JS_EVAL_CACHE_SIZE];

    void                         *data;
    bool                         active;  
    bool                         hasDebugModeCodeToDrop;
    js::WrapperMap               crossCompartmentWrappers;

#ifdef JS_METHODJIT
  private:
    
    js::mjit::JaegerCompartment  *jaegerCompartment_;
    





  public:
    bool hasJaegerCompartment() {
        return !!jaegerCompartment_;
    }

    js::mjit::JaegerCompartment *jaegerCompartment() const {
        JS_ASSERT(jaegerCompartment_);
        return jaegerCompartment_;
    }

    bool ensureJaegerCompartmentExists(JSContext *cx);

    void getMjitCodeStats(size_t& method, size_t& regexp, size_t& unused) const;
#endif
    WTF::BumpPointerAllocator    *regExpAllocator;

    


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

  private:
    enum { DebugFromC = 1, DebugFromJS = 2 };

    uintN                        debugModeBits;  

  public:
    js::NativeIterCache          nativeIterCache;

    typedef js::Maybe<js::ToSourceCache> LazyToSourceCache;
    LazyToSourceCache            toSourceCache;

    js::ScriptFilenameTable      scriptFilenameTable;

    JSCompartment(JSRuntime *rt);
    ~JSCompartment();

    bool init(JSContext *cx);

    
    void markCrossCompartmentWrappers(JSTracer *trc);

    bool wrap(JSContext *cx, js::Value *vp);
    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, JSObject **objp);
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, js::PropertyOp *op);
    bool wrap(JSContext *cx, js::StrictPropertyOp *op);
    bool wrap(JSContext *cx, js::PropertyDescriptor *desc);
    bool wrap(JSContext *cx, js::AutoIdVector &props);

    void markTypes(JSTracer *trc);
    void sweep(JSContext *cx, bool releaseTypes);
    void purge(JSContext *cx);

    void setGCLastBytes(size_t lastBytes, JSGCInvocationKind gckind);
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

    



    js::GlobalObjectSet              debuggees;

  public:
    js::BreakpointSiteMap            breakpointSites;

  private:
    JSCompartment *thisForCtor() { return this; }

  public:
    js::MathCache *getMathCache(JSContext *cx) {
        return mathCache ? mathCache : allocMathCache(cx);
    }

#ifdef JS_TRACER
    bool hasTraceMonitor() {
        return !!traceMonitor_;
    }

    js::TraceMonitor *allocAndInitTraceMonitor(JSContext *cx);

    js::TraceMonitor *traceMonitor() const {
        JS_ASSERT(traceMonitor_);
        return traceMonitor_;
    }
#endif

    size_t backEdgeCount(jsbytecode *pc) const;
    size_t incBackEdgeCount(jsbytecode *pc);

    






    bool debugMode() const { return !!debugModeBits; }

    




    bool hasScriptsOnStack(JSContext *cx);

  private:
    
    void updateForDebugMode(JSContext *cx);

  public:
    js::GlobalObjectSet &getDebuggees() { return debuggees; }
    bool addDebuggee(JSContext *cx, js::GlobalObject *global);
    void removeDebuggee(JSContext *cx, js::GlobalObject *global,
                        js::GlobalObjectSet::Enum *debuggeesEnum = NULL);
    bool setDebugModeFromC(JSContext *cx, bool b);

    js::BreakpointSite *getBreakpointSite(jsbytecode *pc);
    js::BreakpointSite *getOrCreateBreakpointSite(JSContext *cx, JSScript *script, jsbytecode *pc,
                                                  JSObject *scriptObject);
    void clearBreakpointsIn(JSContext *cx, js::Debugger *dbg, JSScript *script, JSObject *handler);
    void clearTraps(JSContext *cx, JSScript *script);
    bool markTrapClosuresIteratively(JSTracer *trc);

  private:
    void sweepBreakpoints(JSContext *cx);

  public:
    js::WatchpointMap *watchpointMap;
};

#define JS_PROPERTY_TREE(cx)    ((cx)->compartment->propertyTree)






static inline bool
JS_ON_TRACE(const JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_THREAD_DATA(cx)->onTraceCompartment)
        return JS_THREAD_DATA(cx)->onTraceCompartment->traceMonitor()->ontrace();
#endif
    return false;
}

#ifdef JS_TRACER
static inline js::TraceMonitor *
JS_TRACE_MONITOR_ON_TRACE(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    return JS_THREAD_DATA(cx)->onTraceCompartment->traceMonitor();
}






static inline js::TraceMonitor *
JS_TRACE_MONITOR_FROM_CONTEXT(JSContext *cx)
{
    return cx->compartment->traceMonitor();
}





static inline js::TraceMonitor *
JS_TRACE_MONITOR_FROM_CONTEXT_WITH_LAZY_INIT(JSContext *cx)
{
    if (!cx->compartment->hasTraceMonitor())
        return cx->compartment->allocAndInitTraceMonitor(cx);
        
    return cx->compartment->traceMonitor();
}
#endif

static inline js::TraceRecorder *
TRACE_RECORDER(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_THREAD_DATA(cx)->recordingCompartment)
        return JS_THREAD_DATA(cx)->recordingCompartment->traceMonitor()->recorder;
#endif
    return NULL;
}

static inline js::LoopProfile *
TRACE_PROFILER(JSContext *cx)
{
#ifdef JS_TRACER
    if (JS_THREAD_DATA(cx)->profilingCompartment)
        return JS_THREAD_DATA(cx)->profilingCompartment->traceMonitor()->profile;
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

inline void
JSContext::setCompartment(JSCompartment *compartment)
{
    this->compartment = compartment;
    this->inferenceEnabled = compartment ? compartment->types.inferenceEnabled : false;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace js {

class PreserveCompartment {
  protected:
    JSContext *cx;
  private:
    JSCompartment *oldCompartment;
    bool oldInferenceEnabled;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
     PreserveCompartment(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM) : cx(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        oldCompartment = cx->compartment;
        oldInferenceEnabled = cx->inferenceEnabled;
    }

    ~PreserveCompartment() {
        
        cx->compartment = oldCompartment;
        cx->inferenceEnabled = oldInferenceEnabled;
    }
};

class SwitchToCompartment : public PreserveCompartment {
  public:
    SwitchToCompartment(JSContext *cx, JSCompartment *newCompartment
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : PreserveCompartment(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->setCompartment(newCompartment);
    }

    SwitchToCompartment(JSContext *cx, JSObject *target JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : PreserveCompartment(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->setCompartment(target->getCompartment());
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
