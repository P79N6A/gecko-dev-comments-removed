






#ifndef jscompartment_h___
#define jscompartment_h___

#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/Util.h"

#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsobj.h"

#include "gc/StoreBuffer.h"
#include "gc/FindSCCs.h"
#include "vm/GlobalObject.h"
#include "vm/RegExpObject.h"
#include "vm/Shape.h"

namespace js {

namespace ion {
class IonCompartment;
}

struct NativeIterator;








class DtoaCache {
    double       d;
    int          base;
    JSFlatString *s;      

  public:
    DtoaCache() : s(NULL) {}
    void purge() { s = NULL; }

    JSFlatString *lookup(int base, double d) {
        return this->s && base == this->base && d == this->d ? this->s : NULL;
    }

    void cache(int base, double d, JSFlatString *s) {
        this->base = base;
        this->d = d;
        this->s = s;
    }
};


JS_STATIC_ASSERT(sizeof(HashNumber) == 4);

struct CrossCompartmentKey
{
    enum Kind {
        ObjectWrapper,
        StringWrapper,
        DebuggerScript,
        DebuggerObject,
        DebuggerEnvironment
    };

    Kind kind;
    JSObject *debugger;
    js::gc::Cell *wrapped;

    CrossCompartmentKey()
      : kind(ObjectWrapper), debugger(NULL), wrapped(NULL) {}
    CrossCompartmentKey(JSObject *wrapped)
      : kind(ObjectWrapper), debugger(NULL), wrapped(wrapped) {}
    CrossCompartmentKey(JSString *wrapped)
      : kind(StringWrapper), debugger(NULL), wrapped(wrapped) {}
    CrossCompartmentKey(Value wrapped)
      : kind(wrapped.isString() ? StringWrapper : ObjectWrapper),
        debugger(NULL),
        wrapped((js::gc::Cell *)wrapped.toGCThing()) {}
    CrossCompartmentKey(const RootedValue &wrapped)
      : kind(wrapped.get().isString() ? StringWrapper : ObjectWrapper),
        debugger(NULL),
        wrapped((js::gc::Cell *)wrapped.get().toGCThing()) {}
    CrossCompartmentKey(Kind kind, JSObject *dbg, js::gc::Cell *wrapped)
      : kind(kind), debugger(dbg), wrapped(wrapped) {}
};

struct WrapperHasher
{
    typedef CrossCompartmentKey Lookup;

    static HashNumber hash(const CrossCompartmentKey &key) {
        JS_ASSERT(!IsPoisonedPtr(key.wrapped));
        return uint32_t(uintptr_t(key.wrapped)) | uint32_t(key.kind);
    }

    static bool match(const CrossCompartmentKey &l, const CrossCompartmentKey &k) {
        return l.kind == k.kind && l.debugger == k.debugger && l.wrapped == k.wrapped;
    }
};

typedef HashMap<CrossCompartmentKey, ReadBarrieredValue,
                WrapperHasher, SystemAllocPolicy> WrapperMap;

} 

namespace JS {
struct TypeInferenceSizes;
}

namespace js {
class AutoDebugModeGC;
class DebugScopes;
}

namespace js {










class Allocator
{
    JSCompartment*const compartment;

  public:
    explicit Allocator(JSCompartment *compartment);

    js::gc::ArenaLists arenas;

    inline void *parallelNewGCThing(gc::AllocKind thingKind, size_t thingSize);

    inline void *malloc_(size_t bytes);
    inline void *calloc_(size_t bytes);
    inline void *realloc_(void *p, size_t bytes);
    inline void *realloc_(void *p, size_t oldBytes, size_t newBytes);
    template <class T> inline T *pod_malloc();
    template <class T> inline T *pod_calloc();
    template <class T> inline T *pod_malloc(size_t numElems);
    template <class T> inline T *pod_calloc(size_t numElems);
    JS_DECLARE_NEW_METHODS(new_, malloc_, JS_ALWAYS_INLINE)
};

} 

struct JSCompartment : private JS::shadow::Zone, public js::gc::GraphNodeBase<JSCompartment>
{
    JSRuntime                    *rt;
    JSPrincipals                 *principals;

  private:
    friend struct JSRuntime;
    friend struct JSContext;
    js::ReadBarriered<js::GlobalObject> global_;

    unsigned                     enterCompartmentDepth;

  public:
    void enter() { enterCompartmentDepth++; }
    void leave() { enterCompartmentDepth--; }

    










    inline js::GlobalObject *maybeGlobal() const;

    void initGlobal(js::GlobalObject &global) {
        JS_ASSERT(global.compartment() == this);
        JS_ASSERT(!global_);
        global_ = &global;
    }

  public:
    js::Allocator                    allocator;

    




    void adoptWorkerAllocator(js::Allocator *workerAllocator);

#ifdef JSGC_GENERATIONAL
    js::gc::Nursery              gcNursery;
    js::gc::StoreBuffer          gcStoreBuffer;
#endif

  private:
    bool                         ionUsingBarriers_;
  public:

    bool needsBarrier() const {
        return needsBarrier_;
    }

    bool compileBarriers(bool needsBarrier) const {
        return needsBarrier || rt->gcZeal() == js::gc::ZealVerifierPreValue;
    }

    bool compileBarriers() const {
        return compileBarriers(needsBarrier());
    }

    enum ShouldUpdateIon {
        DontUpdateIon,
        UpdateIon
    };

    void setNeedsBarrier(bool needs, ShouldUpdateIon updateIon);

    static size_t OffsetOfNeedsBarrier() {
        return offsetof(JSCompartment, needsBarrier_);
    }

    js::GCMarker *barrierTracer() {
        JS_ASSERT(needsBarrier_);
        return &rt->gcMarker;
    }

  public:
    enum CompartmentGCState {
        NoGC,
        Mark,
        MarkGray,
        Sweep,
        Finished
    };

  private:
    bool                         gcScheduled;
    CompartmentGCState           gcState;
    bool                         gcPreserveCode;

  public:
    bool isCollecting() const {
        if (rt->isHeapCollecting())
            return gcState != NoGC;
        else
            return needsBarrier();
    }

    bool isPreservingCode() const {
        return gcPreserveCode;
    }

    



    bool requireGCTracer() const {
        return rt->isHeapCollecting() && gcState != NoGC;
    }

    void setGCState(CompartmentGCState state) {
        JS_ASSERT(rt->isHeapBusy());
        gcState = state;
    }

    void scheduleGC() {
        JS_ASSERT(!rt->isHeapBusy());
        gcScheduled = true;
    }

    void unscheduleGC() {
        gcScheduled = false;
    }

    bool isGCScheduled() const {
        return gcScheduled;
    }

    void setPreservingCode(bool preserving) {
        gcPreserveCode = preserving;
    }

    bool wasGCStarted() const {
        return gcState != NoGC;
    }

    bool isGCMarking() {
        if (rt->isHeapCollecting())
            return gcState == Mark || gcState == MarkGray;
        else
            return needsBarrier();
    }

    bool isGCMarkingBlack() {
        return gcState == Mark;
    }

    bool isGCMarkingGray() {
        return gcState == MarkGray;
    }

    bool isGCSweeping() {
        return gcState == Sweep;
    }

    bool isGCFinished() {
        return gcState == Finished;
    }

    size_t                       gcBytes;
    size_t                       gcTriggerBytes;
    size_t                       gcMaxMallocBytes;
    double                       gcHeapGrowthFactor;

    bool                         hold;
    bool                         isSystemCompartment;

    int64_t                      lastCodeRelease;

    
    static const size_t LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 32 * 1024;
    js::LifoAlloc                analysisLifoAlloc;
    js::LifoAlloc                typeLifoAlloc;

    bool                         activeAnalysis;

    
    js::types::TypeCompartment   types;

    void                         *data;
    bool                         active;  

  private:
    js::WrapperMap               crossCompartmentWrappers;

  public:
    



    bool                         scheduledForDestruction;
    bool                         maybeAlive;

    
    int64_t                      lastAnimationTime;

    js::RegExpCompartment        regExps;

  private:
    void sizeOfTypeInferenceData(JS::TypeInferenceSizes *stats, JSMallocSizeOfFun mallocSizeOf);

  public:
    void sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *compartmentObject,
                             JS::TypeInferenceSizes *tiSizes,
                             size_t *shapesCompartmentTables, size_t *crossCompartmentWrappers,
                             size_t *regexpCompartment, size_t *debuggeesSet);

    


    js::PropertyTree             propertyTree;

    
    js::BaseShapeSet             baseShapes;
    void sweepBaseShapeTable();

    
    js::InitialShapeSet          initialShapes;
    void sweepInitialShapeTable();

    
    js::types::TypeObjectSet     newTypeObjects;
    js::types::TypeObjectSet     lazyTypeObjects;
    void sweepNewTypeObjectTable(js::types::TypeObjectSet &table);

    js::types::TypeObject *getNewType(JSContext *cx, js::Class *clasp, js::TaggedProto proto,
                                      JSFunction *fun = NULL);

    js::types::TypeObject *getLazyType(JSContext *cx, js::Class *clasp,
                                       js::Handle<js::TaggedProto> proto);

    




    js::CallsiteCloneTable callsiteClones;
    void sweepCallsiteClones();

    
    unsigned                     gcIndex;

    






    js::RawObject                gcIncomingGrayPointers;

    
    JSObject                     *gcLiveArrayBuffers;

    
    js::WeakMapBase              *gcWeakMapList;

    
    js::Vector<js::GrayRoot, 0, js::SystemAllocPolicy> gcGrayRoots;

  private:
    




    ptrdiff_t                    gcMallocBytes;

    enum { DebugFromC = 1, DebugFromJS = 2 };

    unsigned                     debugModeBits;  

  public:
    JSCompartment(JSRuntime *rt);
    ~JSCompartment();

    bool init(JSContext *cx);

    
    void markCrossCompartmentWrappers(JSTracer *trc);

    bool wrap(JSContext *cx, js::Value *vp, JSObject *existing = NULL);
    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, js::HeapPtrString *strp);
    bool wrap(JSContext *cx, JSObject **objp, JSObject *existing = NULL);
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, js::PropertyOp *op);
    bool wrap(JSContext *cx, js::StrictPropertyOp *op);
    bool wrap(JSContext *cx, js::PropertyDescriptor *desc);
    bool wrap(JSContext *cx, js::AutoIdVector &props);

    bool putWrapper(const js::CrossCompartmentKey& wrapped, const js::Value& wrapper);

    js::WrapperMap::Ptr lookupWrapper(const js::Value& wrapped) {
        return crossCompartmentWrappers.lookup(wrapped);
    }

    void removeWrapper(js::WrapperMap::Ptr p) {
        crossCompartmentWrappers.remove(p);
    }

    struct WrapperEnum : public js::WrapperMap::Enum {
        WrapperEnum(JSCompartment *c) : js::WrapperMap::Enum(c->crossCompartmentWrappers) {}
    };

    void mark(JSTracer *trc);
    void markTypes(JSTracer *trc);
    void discardJitCode(js::FreeOp *fop, bool discardConstraints);
    bool isDiscardingJitCode(JSTracer *trc);
    void sweep(js::FreeOp *fop, bool releaseTypes);
    void sweepCrossCompartmentWrappers();
    void purge();

    void findOutgoingEdges(js::gc::ComponentFinder<JSCompartment> &finder);

    void setGCLastBytes(size_t lastBytes, js::JSGCInvocationKind gckind);
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

    bool isTooMuchMalloc() const {
        return gcMallocBytes <= 0;
     }

    void onTooMuchMalloc();

    js::DtoaCache dtoaCache;

    
    uint64_t rngState;

  private:
    



    js::GlobalObjectSet              debuggees;

  private:
    JSCompartment *thisForCtor() { return this; }

  public:
    






    bool debugMode() const { return !!debugModeBits; }

    
    bool hasScriptsOnStack();

  private:
    
    void updateForDebugMode(js::FreeOp *fop, js::AutoDebugModeGC &dmgc);

  public:
    js::GlobalObjectSet &getDebuggees() { return debuggees; }
    bool addDebuggee(JSContext *cx, js::GlobalObject *global);
    bool addDebuggee(JSContext *cx, js::GlobalObject *global,
                     js::AutoDebugModeGC &dmgc);
    void removeDebuggee(js::FreeOp *fop, js::GlobalObject *global,
                        js::GlobalObjectSet::Enum *debuggeesEnum = NULL);
    void removeDebuggee(js::FreeOp *fop, js::GlobalObject *global,
                        js::AutoDebugModeGC &dmgc,
                        js::GlobalObjectSet::Enum *debuggeesEnum = NULL);
    bool setDebugModeFromC(JSContext *cx, bool b, js::AutoDebugModeGC &dmgc);

    void clearBreakpointsIn(js::FreeOp *fop, js::Debugger *dbg, JSObject *handler);
    void clearTraps(js::FreeOp *fop);

  private:
    void sweepBreakpoints(js::FreeOp *fop);

  public:
    js::WatchpointMap *watchpointMap;

    js::ScriptCountsMap *scriptCountsMap;

    js::DebugScriptMap *debugScriptMap;

    
    js::DebugScopes *debugScopes;

    



    js::NativeIterator *enumerators;

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

namespace JS {
typedef JSCompartment Zone;
} 





class js::AutoDebugModeGC
{
    JSRuntime *rt;
    bool needGC;
  public:
    explicit AutoDebugModeGC(JSRuntime *rt) : rt(rt), needGC(false) {}

    ~AutoDebugModeGC() {
        
        
        
        
        if (needGC)
            GC(rt, GC_NORMAL, gcreason::DEBUG_MODE_GC);
    }

    void scheduleGC(Zone *zone) {
        JS_ASSERT(!rt->isHeapBusy());
        PrepareZoneForGC(zone);
        needGC = true;
    }
};

inline bool
JSContext::typeInferenceEnabled() const
{
    return compartment->types.inferenceEnabled;
}

inline js::Handle<js::GlobalObject*>
JSContext::global() const
{
    





    return js::Handle<js::GlobalObject*>::fromMarkedLocation(compartment->global_.unsafeGet());
}

namespace js {

class AssertCompartmentUnchanged
{
  public:
    AssertCompartmentUnchanged(JSContext *cx
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), oldCompartment(cx->compartment)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AssertCompartmentUnchanged() {
        JS_ASSERT(cx->compartment == oldCompartment);
    }

  protected:
    JSContext * const cx;
    JSCompartment * const oldCompartment;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoCompartment
{
    JSContext * const cx_;
    JSCompartment * const origin_;

  public:
    inline AutoCompartment(JSContext *cx, JSObject *target);
    inline ~AutoCompartment();

    JSContext *context() const { return cx_; }
    JSCompartment *origin() const { return origin_; }

  private:
    AutoCompartment(const AutoCompartment &) MOZ_DELETE;
    AutoCompartment & operator=(const AutoCompartment &) MOZ_DELETE;
};









class AutoEnterAtomsCompartment
{
    JSContext *cx;
    JSCompartment *oldCompartment;
  public:
    AutoEnterAtomsCompartment(JSContext *cx)
      : cx(cx),
        oldCompartment(cx->compartment)
    {
        cx->setCompartment(cx->runtime->atomsCompartment);
    }

    ~AutoEnterAtomsCompartment()
    {
        cx->setCompartment(oldCompartment);
    }
};






class ErrorCopier
{
    mozilla::Maybe<AutoCompartment> &ac;
    RootedObject scope;

  public:
    ErrorCopier(mozilla::Maybe<AutoCompartment> &ac, JSObject *scope)
      : ac(ac), scope(ac.ref().context(), scope) {}
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





















struct WrapperValue
{
    





    explicit WrapperValue(const WrapperMap::Ptr &ptr)
      : value(*ptr->value.unsafeGet())
    {}

    explicit WrapperValue(const WrapperMap::Enum &e)
      : value(*e.front().value.unsafeGet())
    {}

    Value &get() { return value; }
    Value get() const { return value; }
    operator const Value &() const { return value; }
    JSObject &toObject() const { return value.toObject(); }

  private:
    Value value;
};

class AutoWrapperVector : public AutoVectorRooter<WrapperValue>
{
  public:
    explicit AutoWrapperVector(JSContext *cx
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<WrapperValue>(cx, WRAPVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoWrapperRooter : private AutoGCRooter {
  public:
    AutoWrapperRooter(JSContext *cx, WrapperValue v
                      MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, WRAPPER), value(v)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    operator JSObject *() const {
        return value.get().toObjectOrNull();
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    WrapperValue value;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#endif

