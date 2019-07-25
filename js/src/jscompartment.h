






































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






struct TraceMonitor {
    









    JSContext               *tracecx;

    




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
    TreeFragment*           vmfragments[FRAGMENT_TABLE_SIZE];
    RecordAttemptMap*       recordAttempts;

    
    LoopProfileMap*         loopProfiles;

    



    uint32                  maxCodeCacheBytes;

    




    JSBool                  needFlush;

    


    REHashMap*              reFragments;

    
    
    
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

    
    void sweep();

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

struct JS_FRIEND_API(JSCompartment) {
    JSRuntime                    *rt;
    JSPrincipals                 *principals;
    js::gc::Chunk                *chunk;

    js::gc::ArenaList            arenas[js::gc::FINALIZE_LIMIT];
    js::gc::FreeLists            freeLists;

#ifdef JS_GCMETER
    js::gc::JSGCArenaStats       compartmentStats[js::gc::FINALIZE_LIMIT];
#endif

#ifdef JS_TRACER
    
    js::TraceMonitor traceMonitor;
#endif

    
    JSScript                     *scriptsToGC[JS_EVAL_CACHE_SIZE];

#ifdef DEBUG
    JSEvalCacheMeter    evalCacheMeter;
#endif

    void                         *data;
    bool                         marked;
    bool                         active;  
    js::WrapperMap               crossCompartmentWrappers;

#ifdef JS_METHODJIT
    js::mjit::JaegerCompartment  *jaegerCompartment;
#endif

    bool                         debugMode;  
    JSCList                      scripts;    

    






    JSObject                     *anynameObject;
    JSObject                     *functionNamespaceObject;

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

    void sweep(JSContext *cx, uint32 releaseInterval);
    void purge(JSContext *cx);
    void finishArenaLists();
    bool arenaListsAreEmpty();

  private:
    js::MathCache                *mathCache;

    js::MathCache *allocMathCache(JSContext *cx);

  public:
    js::MathCache *getMathCache(JSContext *cx) {
        return mathCache ? mathCache : allocMathCache(cx);
    }
};

#define JS_TRACE_MONITOR(cx)    (cx->compartment->traceMonitor)
#define JS_SCRIPTS_TO_GC(cx)    (cx->compartment->scriptsToGC)

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
    SwitchToCompartment(JSContext *cx, JSCompartment *newCompartment) : PreserveCompartment(cx) {
        cx->compartment = newCompartment;
    }

    SwitchToCompartment(JSContext *cx, JSObject *target) : PreserveCompartment(cx) {
        cx->compartment = target->getCompartment();
    }
};

}

#endif 
