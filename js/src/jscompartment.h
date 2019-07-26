





#ifndef jscompartment_h
#define jscompartment_h

#include "mozilla/MemoryReporting.h"

#include "builtin/TypeRepresentation.h"
#include "gc/Zone.h"
#include "vm/GlobalObject.h"

namespace js {

namespace jit {
class JitCompartment;
}

namespace gc {
template<class Node> class ComponentFinder;
}

struct NativeIterator;








class DtoaCache {
    double       d;
    int          base;
    JSFlatString *s;      

  public:
    DtoaCache() : s(nullptr) {}
    void purge() { s = nullptr; }

    JSFlatString *lookup(int base, double d) {
        return this->s && base == this->base && d == this->d ? this->s : nullptr;
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
        DebuggerSource,
        DebuggerObject,
        DebuggerEnvironment
    };

    Kind kind;
    JSObject *debugger;
    js::gc::Cell *wrapped;

    CrossCompartmentKey()
      : kind(ObjectWrapper), debugger(nullptr), wrapped(nullptr) {}
    CrossCompartmentKey(JSObject *wrapped)
      : kind(ObjectWrapper), debugger(nullptr), wrapped(wrapped) {}
    CrossCompartmentKey(JSString *wrapped)
      : kind(StringWrapper), debugger(nullptr), wrapped(wrapped) {}
    CrossCompartmentKey(Value wrapped)
      : kind(wrapped.isString() ? StringWrapper : ObjectWrapper),
        debugger(nullptr),
        wrapped((js::gc::Cell *)wrapped.toGCThing()) {}
    CrossCompartmentKey(const RootedValue &wrapped)
      : kind(wrapped.get().isString() ? StringWrapper : ObjectWrapper),
        debugger(nullptr),
        wrapped((js::gc::Cell *)wrapped.get().toGCThing()) {}
    CrossCompartmentKey(Kind kind, JSObject *dbg, js::gc::Cell *wrapped)
      : kind(kind), debugger(dbg), wrapped(wrapped) {}
};

struct WrapperHasher : public DefaultHasher<CrossCompartmentKey>
{
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
class AutoDebugModeInvalidation;
class ArrayBufferObject;
class DebugScopes;
class WeakMapBase;
}

struct JSCompartment
{
    JS::CompartmentOptions       options_;

  private:
    JS::Zone                     *zone_;
    JSRuntime                    *runtime_;

  public:
    JSPrincipals                 *principals;
    bool                         isSystem;
    bool                         marked;

#ifdef DEBUG
    bool                         firedOnNewGlobalObject;
#endif

    void mark() { marked = true; }

  private:
    friend struct JSRuntime;
    friend struct JSContext;
    friend class js::ExclusiveContext;
    js::ReadBarriered<js::GlobalObject> global_;

    unsigned                     enterCompartmentDepth;

  public:
    void enter() { enterCompartmentDepth++; }
    void leave() { enterCompartmentDepth--; }
    bool hasBeenEntered() { return !!enterCompartmentDepth; }

    JS::Zone *zone() { return zone_; }
    const JS::Zone *zone() const { return zone_; }
    JS::CompartmentOptions &options() { return options_; }
    const JS::CompartmentOptions &options() const { return options_; }

    JSRuntime *runtimeFromMainThread() {
        JS_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
        return runtime_;
    }

    
    
    JSRuntime *runtimeFromAnyThread() const {
        return runtime_;
    }

    










    inline js::GlobalObject *maybeGlobal() const;

    inline void initGlobal(js::GlobalObject &global);

  public:
    




    void adoptWorkerAllocator(js::Allocator *workerAllocator);


    int64_t                      lastCodeRelease;
    bool                         activeAnalysis;

    
    js::types::TypeCompartment   types;

    void                         *data;

  private:
    js::ObjectMetadataCallback   objectMetadataCallback;

    js::WrapperMap               crossCompartmentWrappers;

  public:
    
    int64_t                      lastAnimationTime;

    js::RegExpCompartment        regExps;

    
    js::TypeRepresentationHash   typeReprs;

    






    bool                         globalWriteBarriered;

  public:
    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t *tiPendingArrays,
                                size_t *tiAllocationSiteTables,
                                size_t *tiArrayTypeTables,
                                size_t *tiObjectTypeTables,
                                size_t *compartmentObject,
                                size_t *shapesCompartmentTables,
                                size_t *crossCompartmentWrappers,
                                size_t *regexpCompartment,
                                size_t *debuggeesSet,
                                size_t *baselineStubsOptimized);

    


    js::PropertyTree             propertyTree;

    
    js::BaseShapeSet             baseShapes;
    void sweepBaseShapeTable();

    
    js::InitialShapeSet          initialShapes;
    void sweepInitialShapeTable();
    void markAllInitialShapeTableEntries(JSTracer *trc);

    
    js::types::TypeObjectSet     newTypeObjects;
    js::types::TypeObjectSet     lazyTypeObjects;
    void sweepNewTypeObjectTable(js::types::TypeObjectSet &table);

    




    js::CallsiteCloneTable callsiteClones;
    void sweepCallsiteClones();

    
    unsigned                     gcIndex;

    






    JSObject                     *gcIncomingGrayPointers;

    
    js::ArrayBufferObject        *gcLiveArrayBuffers;

    
    js::WeakMapBase              *gcWeakMapList;

  private:
    enum { DebugFromC = 1, DebugFromJS = 2 };

    unsigned                     debugModeBits;  

  public:
    JSCompartment(JS::Zone *zone, const JS::CompartmentOptions &options);
    ~JSCompartment();

    bool init(JSContext *cx);

    
    void markCrossCompartmentWrappers(JSTracer *trc);
    void markAllCrossCompartmentWrappers(JSTracer *trc);

    inline bool wrap(JSContext *cx, JS::MutableHandleValue vp,
                     JS::HandleObject existing = js::NullPtr());

    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, js::HeapPtrString *strp);
    bool wrap(JSContext *cx, JS::MutableHandleObject obj,
              JS::HandleObject existingArg = js::NullPtr());
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, js::PropertyOp *op);
    bool wrap(JSContext *cx, js::StrictPropertyOp *op);
    bool wrap(JSContext *cx, JS::MutableHandle<js::PropertyDescriptor> desc);
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
    bool isDiscardingJitCode(JSTracer *trc);
    void sweep(js::FreeOp *fop, bool releaseTypes);
    void sweepCrossCompartmentWrappers();
    void purge();
    void clearTables();

    bool hasObjectMetadataCallback() const { return objectMetadataCallback; }
    void setObjectMetadataCallback(js::ObjectMetadataCallback callback);
    bool callObjectMetadataCallback(JSContext *cx, JSObject **obj) const {
        return objectMetadataCallback(cx, obj);
    }

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone> &finder);

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
    
    void updateForDebugMode(js::FreeOp *fop, js::AutoDebugModeInvalidation &invalidate);

  public:
    js::GlobalObjectSet &getDebuggees() { return debuggees; }
    bool addDebuggee(JSContext *cx, js::GlobalObject *global);
    bool addDebuggee(JSContext *cx, js::GlobalObject *global,
                     js::AutoDebugModeInvalidation &invalidate);
    void removeDebuggee(js::FreeOp *fop, js::GlobalObject *global,
                        js::GlobalObjectSet::Enum *debuggeesEnum = nullptr);
    void removeDebuggee(js::FreeOp *fop, js::GlobalObject *global,
                        js::AutoDebugModeInvalidation &invalidate,
                        js::GlobalObjectSet::Enum *debuggeesEnum = nullptr);
    bool setDebugModeFromC(JSContext *cx, bool b,
                           js::AutoDebugModeInvalidation &invalidate);

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

    
    void               *compartmentStats;

#ifdef JS_ION
  private:
    js::jit::JitCompartment *jitCompartment_;

  public:
    bool ensureJitCompartmentExists(JSContext *cx);
    js::jit::JitCompartment *jitCompartment() {
        return jitCompartment_;
    }
#endif
};

inline bool
JSRuntime::isAtomsZone(JS::Zone *zone)
{
    return zone == atomsCompartment_->zone();
}

















class js::AutoDebugModeInvalidation
{
    JSCompartment *comp_;
    JS::Zone *zone_;

    enum {
        NoNeed = 0,
        ToggledOn = 1,
        ToggledOff = 2
    } needInvalidation_;

  public:
    explicit AutoDebugModeInvalidation(JSCompartment *comp)
      : comp_(comp), zone_(nullptr), needInvalidation_(NoNeed)
    { }

    explicit AutoDebugModeInvalidation(JS::Zone *zone)
      : comp_(nullptr), zone_(zone), needInvalidation_(NoNeed)
    { }

    ~AutoDebugModeInvalidation();

    bool isFor(JSCompartment *comp) {
        if (comp_)
            return comp == comp_;
        return comp->zone() == zone_;
    }

    void scheduleInvalidation(bool debugMode) {
        
        
        
        MOZ_ASSERT_IF(needInvalidation_ != NoNeed,
                      needInvalidation_ == debugMode ? ToggledOn : ToggledOff);
        needInvalidation_ = debugMode ? ToggledOn : ToggledOff;
    }
};

namespace js {

inline bool
ExclusiveContext::typeInferenceEnabled() const
{
    return compartment_->options().typeInference(this);
}

inline js::Handle<js::GlobalObject*>
ExclusiveContext::global() const
{
    





    return Handle<GlobalObject*>::fromMarkedLocation(compartment_->global_.unsafeGet());
}

class AssertCompartmentUnchanged
{
  public:
    AssertCompartmentUnchanged(JSContext *cx
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), oldCompartment(cx->compartment())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AssertCompartmentUnchanged() {
        JS_ASSERT(cx->compartment() == oldCompartment);
    }

  protected:
    JSContext * const cx;
    JSCompartment * const oldCompartment;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoCompartment
{
    ExclusiveContext * const cx_;
    JSCompartment * const origin_;

  public:
    inline AutoCompartment(ExclusiveContext *cx, JSObject *target);
    inline AutoCompartment(ExclusiveContext *cx, JSCompartment *target);
    inline ~AutoCompartment();

    ExclusiveContext *context() const { return cx_; }
    JSCompartment *origin() const { return origin_; }

  private:
    AutoCompartment(const AutoCompartment &) MOZ_DELETE;
    AutoCompartment & operator=(const AutoCompartment &) MOZ_DELETE;
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
