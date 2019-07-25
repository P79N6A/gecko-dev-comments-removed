






































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

namespace js {

typedef HashMap<JSFunction *,
                JSString *,
                DefaultHasher<JSFunction *>,
                SystemAllocPolicy> ToSourceCache;

namespace mjit {
class JaegerCompartment;
}


extern Class dummy_class;

} 

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

    bool                         needsBarrier_;
    js::GCMarker                 *gcIncrementalTracer;

    bool needsBarrier() {
        return needsBarrier_;
    }

    js::GCMarker *barrierTracer() {
        JS_ASSERT(needsBarrier_);
        if (gcIncrementalTracer)
            return gcIncrementalTracer;
        return createBarrierTracer();
    }

    uint32                       gcBytes;
    uint32                       gcTriggerBytes;
    size_t                       gcLastBytes;

    bool                         hold;
    bool                         isSystemCompartment;

    




    static const size_t TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 128 * 1024;
    js::LifoAlloc                typeLifoAlloc;
    bool                         activeAnalysis;
    bool                         activeInference;

    
    js::types::TypeCompartment   types;

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

    


    js::PropertyTree             propertyTree;

#ifdef DEBUG
    
    jsrefcount                   livePropTreeNodes;
    jsrefcount                   totalPropTreeNodes;
    jsrefcount                   propTreeKidsChunks;
    jsrefcount                   liveDictModeNodes;
#endif

    typedef js::ReadBarriered<js::EmptyShape> BarrieredEmptyShape;
    typedef js::ReadBarriered<const js::Shape> BarrieredShape;

    



    BarrieredEmptyShape          emptyArgumentsShape;
    BarrieredEmptyShape          emptyBlockShape;
    BarrieredEmptyShape          emptyCallShape;
    BarrieredEmptyShape          emptyDeclEnvShape;
    BarrieredEmptyShape          emptyEnumeratorShape;
    BarrieredEmptyShape          emptyWithShape;

    typedef js::HashSet<js::EmptyShape *,
                        js::DefaultHasher<js::EmptyShape *>,
                        js::SystemAllocPolicy> EmptyShapeSet;

    EmptyShapeSet                emptyShapes;

    








    BarrieredShape               initialRegExpShape;
    BarrieredShape               initialStringShape;

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
    bool wrap(JSContext *cx, js::HeapPtrString *strp);
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
                                                  js::GlobalObject *scriptGlobal);
    void clearBreakpointsIn(JSContext *cx, js::Debugger *dbg, JSScript *script, JSObject *handler);
    void clearTraps(JSContext *cx, JSScript *script);
    bool markTrapClosuresIteratively(JSTracer *trc);

  private:
    void sweepBreakpoints(JSContext *cx);

    js::GCMarker *createBarrierTracer();

  public:
    js::WatchpointMap *watchpointMap;
};

#define JS_PROPERTY_TREE(cx)    ((cx)->compartment->propertyTree)

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
        cx->setCompartment(target->compartment());
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

class AutoCompartment
{
  public:
    JSContext * const context;
    JSCompartment * const origin;
    JSObject * const target;
    JSCompartment * const destination;
  private:
    Maybe<DummyFrameGuard> frame;
    bool entered;

  public:
    AutoCompartment(JSContext *cx, JSObject *target);
    ~AutoCompartment();

    bool enter();
    void leave();

  private:
    
    AutoCompartment(const AutoCompartment &);
    AutoCompartment & operator=(const AutoCompartment &);
};






class ErrorCopier
{
    AutoCompartment &ac;
    JSObject *scope;

  public:
    ErrorCopier(AutoCompartment &ac, JSObject *scope) : ac(ac), scope(scope) {
        JS_ASSERT(scope->compartment() == ac.origin);
    }
    ~ErrorCopier();
};

class CompartmentsIter {
  private:
    JSCompartment **it, **end;

  public:
    CompartmentsIter(JSRuntime *rt) {
        it = rt->compartments.begin();
        end = rt->compartments.end();
    }

    bool done() const { return it == end; }

    void next() {
        JS_ASSERT(!done());
        it++;
    }

    JSCompartment *get() const {
        JS_ASSERT(!done());
        return *it;
    }

    operator JSCompartment *() const { return get(); }
    JSCompartment *operator->() const { return get(); }
};

} 

#endif
