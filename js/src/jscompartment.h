






































#ifndef jscompartment_h___
#define jscompartment_h___

#include "mozilla/Attributes.h"

#include "jsclist.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsobj.h"
#include "jsscope.h"
#include "vm/GlobalObject.h"
#include "vm/RegExpObject.h"

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

namespace ion {
    class IonCompartment;
}

} 

#ifndef JS_EVAL_CACHE_SHIFT
# define JS_EVAL_CACHE_SHIFT        6
#endif


#define JS_EVAL_CACHE_SIZE          JS_BIT(JS_EVAL_CACHE_SHIFT)

namespace js {

class NativeIterCache {
    static const size_t SIZE = size_t(1) << 8;
    
    
    JSObject            *data[SIZE];

    static size_t getIndex(uint32_t key) {
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

    JSObject *get(uint32_t key) const {
        return data[getIndex(key)];
    }

    void set(uint32_t key, JSObject *iterobj) {
        data[getIndex(key)] = iterobj;
    }
};

class MathCache;








class DtoaCache {
    double        d;
    int         base;
    JSFixedString *s;      
  public:
    DtoaCache() : s(NULL) {}
    void purge() { s = NULL; }

    JSFixedString *lookup(int base, double d) {
        return this->s && base == this->base && d == this->d ? this->s : NULL;
    }

    void cache(int base, double d, JSFixedString *s) {
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


JS_STATIC_ASSERT(sizeof(HashNumber) == 4);

struct WrapperHasher
{
    typedef Value Lookup;

    static HashNumber hash(Value key) {
        uint64_t bits = JSVAL_TO_IMPL(key).asBits;
        return uint32_t(bits) ^ uint32_t(bits >> 32);
    }

    static bool match(const Value &l, const Value &k) { return l == k; }
};

typedef HashMap<Value, ReadBarrieredValue, WrapperHasher, SystemAllocPolicy> WrapperMap;

} 

namespace JS {
struct TypeInferenceSizes;
}

struct JSCompartment
{
    JSRuntime                    *rt;
    JSPrincipals                 *principals;

    js::gc::ArenaLists           arenas;

    bool                         needsBarrier_;

    bool needsBarrier() {
        return needsBarrier_;
    }

    js::GCMarker *barrierTracer() {
        JS_ASSERT(needsBarrier_);
        return &rt->gcMarker;
    }

    size_t                       gcBytes;
    size_t                       gcTriggerBytes;
    size_t                       gcMaxMallocBytes;

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

    size_t sizeOfMjitCode() const;
#endif

    js::RegExpCompartment        regExps;

    size_t sizeOfShapeTable(JSMallocSizeOfFun mallocSizeOf);
    void sizeOfTypeInferenceData(JS::TypeInferenceSizes *stats, JSMallocSizeOfFun mallocSizeOf);

    


    js::PropertyTree             propertyTree;

    
    js::BaseShapeSet             baseShapes;
    void sweepBaseShapeTable(JSContext *cx);

    
    js::InitialShapeSet          initialShapes;
    void sweepInitialShapeTable(JSContext *cx);

    
    js::types::TypeObjectSet     newTypeObjects;
    js::types::TypeObjectSet     lazyTypeObjects;
    void sweepNewTypeObjectTable(JSContext *cx, js::types::TypeObjectSet &table);

    js::types::TypeObject        *emptyTypeObject;

    
    inline js::types::TypeObject *getEmptyType(JSContext *cx);

    js::types::TypeObject *getLazyType(JSContext *cx, JSObject *proto);

    
    js::NewObjectCache           newObjectCache;

    





    size_t                       gcMallocAndFreeBytes;
    size_t                       gcTriggerMallocAndFreeBytes;

  private:
    




    ptrdiff_t                    gcMallocBytes;

    enum { DebugFromC = 1, DebugFromJS = 2 };

    unsigned                     debugModeBits;  

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

    void mark(JSTracer *trc);
    void markTypes(JSTracer *trc);
    void discardJitCode(JSContext *cx);
    void sweep(JSContext *cx, bool releaseTypes);
    void purge();

    void setGCLastBytes(size_t lastBytes, size_t lastMallocBytes, js::JSGCInvocationKind gckind);
    void reduceGCTriggerBytes(size_t amount);

    void resetGCMallocBytes();
    void setGCMaxMallocBytes(size_t value);
    void updateMallocCounter(size_t nbytes) {
        ptrdiff_t oldCount = gcMallocBytes;
        ptrdiff_t newCount = oldCount - ptrdiff_t(nbytes);
        gcMallocBytes = newCount;
        if (JS_UNLIKELY(newCount <= 0 && oldCount > 0))
            onTooMuchMalloc();
    }

    void onTooMuchMalloc();

    void mallocInCompartment(size_t nbytes) {
        gcMallocAndFreeBytes += nbytes;
    }

    void freeInCompartment(size_t nbytes) {
        JS_ASSERT(gcMallocAndFreeBytes >= nbytes);
        gcMallocAndFreeBytes -= nbytes;
    }

    js::DtoaCache dtoaCache;

  private:
    js::MathCache                *mathCache;

    js::MathCache *allocMathCache(JSContext *cx);

    



    js::GlobalObjectSet              debuggees;

  private:
    JSCompartment *thisForCtor() { return this; }

  public:
    js::MathCache *getMathCache(JSContext *cx) {
        return mathCache ? mathCache : allocMathCache(cx);
    }

    






    bool debugMode() const { return !!debugModeBits; }

    
    bool hasScriptsOnStack();

  private:
    
    void updateForDebugMode(JSContext *cx);

  public:
    js::GlobalObjectSet &getDebuggees() { return debuggees; }
    bool addDebuggee(JSContext *cx, js::GlobalObject *global);
    void removeDebuggee(JSContext *cx, js::GlobalObject *global,
                        js::GlobalObjectSet::Enum *debuggeesEnum = NULL);
    bool setDebugModeFromC(JSContext *cx, bool b);

    void clearBreakpointsIn(JSContext *cx, js::Debugger *dbg, JSObject *handler);
    void clearTraps(JSContext *cx);

  private:
    void sweepBreakpoints(JSContext *cx);

  public:
    js::WatchpointMap *watchpointMap;
	
#ifdef JS_ION
  private:
    js::ion::IonCompartment *ionCompartment_;

  public:
    bool ensureIonCompartmentExists(JSContext *cx);
    js::ion::IonCompartment *ionCompartment() {
        return ionCompartment_;
    }
#endif

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
    AutoCompartment(const AutoCompartment &) MOZ_DELETE;
    AutoCompartment & operator=(const AutoCompartment &) MOZ_DELETE;
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
