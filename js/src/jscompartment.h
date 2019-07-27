





#ifndef jscompartment_h
#define jscompartment_h

#include "mozilla/MemoryReporting.h"

#include "builtin/RegExp.h"
#include "gc/Zone.h"
#include "vm/GlobalObject.h"
#include "vm/PIC.h"
#include "vm/SavedStacks.h"

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

    explicit CrossCompartmentKey(JSObject *wrapped)
      : kind(ObjectWrapper), debugger(nullptr), wrapped(wrapped)
    {
        MOZ_RELEASE_ASSERT(wrapped);
    }
    explicit CrossCompartmentKey(JSString *wrapped)
      : kind(StringWrapper), debugger(nullptr), wrapped(wrapped)
    {
        MOZ_RELEASE_ASSERT(wrapped);
    }
    explicit CrossCompartmentKey(Value wrappedArg)
      : kind(wrappedArg.isString() ? StringWrapper : ObjectWrapper),
        debugger(nullptr),
        wrapped((js::gc::Cell *)wrappedArg.toGCThing())
    {
        MOZ_RELEASE_ASSERT(wrappedArg.isString() || wrappedArg.isObject());
        MOZ_RELEASE_ASSERT(wrapped);
    }
    explicit CrossCompartmentKey(const RootedValue &wrappedArg)
      : kind(wrappedArg.get().isString() ? StringWrapper : ObjectWrapper),
        debugger(nullptr),
        wrapped((js::gc::Cell *)wrappedArg.get().toGCThing())
    {
        MOZ_RELEASE_ASSERT(wrappedArg.isString() || wrappedArg.isObject());
        MOZ_RELEASE_ASSERT(wrapped);
    }
    CrossCompartmentKey(Kind kind, JSObject *dbg, js::gc::Cell *wrapped)
      : kind(kind), debugger(dbg), wrapped(wrapped)
    {
        MOZ_RELEASE_ASSERT(dbg);
        MOZ_RELEASE_ASSERT(wrapped);
    }

  private:
    CrossCompartmentKey() MOZ_DELETE;
};

struct WrapperHasher : public DefaultHasher<CrossCompartmentKey>
{
    static HashNumber hash(const CrossCompartmentKey &key) {
        MOZ_ASSERT(!IsPoisonedPtr(key.wrapped));
        static_assert(sizeof(HashNumber) == sizeof(uint32_t),
                      "subsequent code assumes a four-byte hash");
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
class DebugScopes;
class LazyArrayBufferTable;
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
    bool                         isSelfHosting;
    bool                         marked;

    
    
    JSAddonId                    *addonId;

#ifdef DEBUG
    bool                         firedOnNewGlobalObject;
#endif

    void mark() { marked = true; }

  private:
    friend struct JSRuntime;
    friend struct JSContext;
    friend class js::ExclusiveContext;
    js::ReadBarrieredGlobalObject global_;

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
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
        return runtime_;
    }

    
    
    JSRuntime *runtimeFromAnyThread() const {
        return runtime_;
    }

    










    inline js::GlobalObject *maybeGlobal() const;

    
    inline js::GlobalObject *unsafeUnbarrieredMaybeGlobal() const;

    inline void initGlobal(js::GlobalObject &global);

  public:
    




    void adoptWorkerAllocator(js::Allocator *workerAllocator);

    
    js::types::TypeCompartment   types;

    void                         *data;

  private:
    js::ObjectMetadataCallback   objectMetadataCallback;

    js::SavedStacks              savedStacks_;

    js::WrapperMap               crossCompartmentWrappers;

  public:
    
    int64_t                      lastAnimationTime;

    js::RegExpCompartment        regExps;

    






    bool                         globalWriteBarriered;

    
    int32_t                      neuteredTypedObjects;

  public:
    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t *tiAllocationSiteTables,
                                size_t *tiArrayTypeTables,
                                size_t *tiObjectTypeTables,
                                size_t *compartmentObject,
                                size_t *compartmentTables,
                                size_t *innerViews,
                                size_t *lazyArrayBuffers,
                                size_t *crossCompartmentWrappers,
                                size_t *regexpCompartment,
                                size_t *savedStacksSet);

    


    js::PropertyTree             propertyTree;

    
    js::BaseShapeSet             baseShapes;
    void sweepBaseShapeTable();

    
    js::InitialShapeSet          initialShapes;
    void sweepInitialShapeTable();

    
    js::types::TypeObjectWithNewScriptSet newTypeObjects;
    js::types::TypeObjectWithNewScriptSet lazyTypeObjects;
    void sweepNewTypeObjectTable(js::types::TypeObjectWithNewScriptSet &table);

#ifdef JSGC_HASH_TABLE_CHECKS
    void checkTypeObjectTablesAfterMovingGC();
    void checkTypeObjectTableAfterMovingGC(js::types::TypeObjectWithNewScriptSet &table);
    void checkInitialShapesTableAfterMovingGC();
    void checkWrapperMapAfterMovingGC();
#endif

    




    js::CallsiteCloneTable callsiteClones;
    void sweepCallsiteClones();

    



    js::ReadBarrieredScriptSourceObject selfHostingScriptSource;

    
    js::InnerViewTable innerViews;

    
    js::LazyArrayBufferTable *lazyArrayBuffers;

    
    unsigned                     gcIndex;

    






    JSObject                     *gcIncomingGrayPointers;

    
    js::WeakMapBase              *gcWeakMapList;

  private:
    
    bool                         gcPreserveJitCode;

    enum {
        DebugMode = 1 << 0,
        DebugObservesAllExecution = 1 << 1,
        DebugNeedDelazification = 1 << 2
    };

    
    
    static const unsigned DebugExecutionMask = DebugMode | DebugObservesAllExecution;

    unsigned                     debugModeBits;

  public:
    JSCompartment(JS::Zone *zone, const JS::CompartmentOptions &options);
    ~JSCompartment();

    bool init(JSContext *cx);

    
    void markCrossCompartmentWrappers(JSTracer *trc);

    inline bool wrap(JSContext *cx, JS::MutableHandleValue vp,
                     JS::HandleObject existing = js::NullPtr());

    bool wrap(JSContext *cx, js::MutableHandleString strp);
    bool wrap(JSContext *cx, JS::MutableHandleObject obj,
              JS::HandleObject existingArg = js::NullPtr());
    bool wrap(JSContext *cx, JS::MutableHandle<js::PropertyDescriptor> desc);
    bool wrap(JSContext *cx, JS::MutableHandle<js::PropDesc> desc);

    template<typename T> bool wrap(JSContext *cx, JS::AutoVectorRooter<T> &vec) {
        for (size_t i = 0; i < vec.length(); ++i) {
            if (!wrap(cx, vec[i]))
                return false;
        }
        return true;
    };

    bool putWrapper(JSContext *cx, const js::CrossCompartmentKey& wrapped, const js::Value& wrapper);

    js::WrapperMap::Ptr lookupWrapper(const js::Value& wrapped) {
        return crossCompartmentWrappers.lookup(js::CrossCompartmentKey(wrapped));
    }

    void removeWrapper(js::WrapperMap::Ptr p) {
        crossCompartmentWrappers.remove(p);
    }

    struct WrapperEnum : public js::WrapperMap::Enum {
        explicit WrapperEnum(JSCompartment *c) : js::WrapperMap::Enum(c->crossCompartmentWrappers) {}
    };

    void trace(JSTracer *trc);
    void markRoots(JSTracer *trc);
    bool preserveJitCode() { return gcPreserveJitCode; }

    void sweepInnerViews();
    void sweepCrossCompartmentWrappers();
    void sweepTypeObjectTables();
    void sweepSavedStacks();
    void sweepGlobalObject(js::FreeOp *fop);
    void sweepSelfHostingScriptSource();
    void sweepJitCompartment(js::FreeOp *fop);
    void sweepRegExps();
    void sweepDebugScopes();
    void sweepWeakMaps();
    void sweepNativeIterators();

    void purge();
    void clearTables();

#ifdef JSGC_COMPACTING
    void fixupInitialShapeTable();
    void fixupNewTypeObjectTable(js::types::TypeObjectWithNewScriptSet &table);
    void fixupAfterMovingGC();
    void fixupGlobal();
#endif

    bool hasObjectMetadataCallback() const { return objectMetadataCallback; }
    void setObjectMetadataCallback(js::ObjectMetadataCallback callback);
    void forgetObjectMetadataCallback() {
        objectMetadataCallback = nullptr;
    }
    bool callObjectMetadataCallback(JSContext *cx, JSObject **obj) const {
        return objectMetadataCallback(cx, obj);
    }
    const void *addressOfMetadataCallback() const {
        return &objectMetadataCallback;
    }

    js::SavedStacks &savedStacks() { return savedStacks_; }

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone> &finder);

    js::DtoaCache dtoaCache;

    
    uint64_t rngState;

  private:
    JSCompartment *thisForCtor() { return this; }

  public:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    bool isDebuggee() const { return !!(debugModeBits & DebugMode); }
    void setIsDebuggee() { debugModeBits |= DebugMode; }
    void unsetIsDebuggee();

    
    
    
    bool debugObservesAllExecution() const {
        return (debugModeBits & DebugExecutionMask) == DebugExecutionMask;
    }
    void setDebugObservesAllExecution() {
        MOZ_ASSERT(isDebuggee());
        debugModeBits |= DebugObservesAllExecution;
    }
    void unsetDebugObservesAllExecution() {
        MOZ_ASSERT(isDebuggee());
        debugModeBits &= ~DebugObservesAllExecution;
    }

    



    void scheduleDelazificationForDebugMode() { debugModeBits |= DebugNeedDelazification; }

    



    bool ensureDelazifyScriptsForDebugMode(JSContext *cx);

    void clearBreakpointsIn(js::FreeOp *fop, js::Debugger *dbg, JS::HandleObject handler);

  private:
    void sweepBreakpoints(js::FreeOp *fop);

  public:
    js::WatchpointMap *watchpointMap;

    js::ScriptCountsMap *scriptCountsMap;

    js::DebugScriptMap *debugScriptMap;

    
    js::DebugScopes *debugScopes;

    



    js::NativeIterator *enumerators;

    
    void               *compartmentStats;

    
    
    bool scheduledForDestruction;
    bool maybeAlive;

  private:
    js::jit::JitCompartment *jitCompartment_;

  public:
    bool ensureJitCompartmentExists(JSContext *cx);
    js::jit::JitCompartment *jitCompartment() {
        return jitCompartment_;
    }
};

inline bool
JSRuntime::isAtomsZone(JS::Zone *zone)
{
    return zone == atomsCompartment_->zone();
}

namespace js {

inline js::Handle<js::GlobalObject*>
ExclusiveContext::global() const
{
    





    MOZ_ASSERT(compartment_, "Caller needs to enter a compartment first");
    return Handle<GlobalObject*>::fromMarkedLocation(compartment_->global_.unsafeGet());
}

class AssertCompartmentUnchanged
{
  public:
    explicit AssertCompartmentUnchanged(JSContext *cx
                                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), oldCompartment(cx->compartment())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AssertCompartmentUnchanged() {
        MOZ_ASSERT(cx->compartment() == oldCompartment);
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

  public:
    explicit ErrorCopier(mozilla::Maybe<AutoCompartment> &ac)
      : ac(ac) {}
    ~ErrorCopier();
};





















struct WrapperValue
{
    





    explicit WrapperValue(const WrapperMap::Ptr &ptr)
      : value(*ptr->value().unsafeGet())
    {}

    explicit WrapperValue(const WrapperMap::Enum &e)
      : value(*e.front().value().unsafeGet())
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

class AutoWrapperRooter : private JS::AutoGCRooter {
  public:
    AutoWrapperRooter(JSContext *cx, WrapperValue v
                      MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : JS::AutoGCRooter(cx, WRAPPER), value(v)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    operator JSObject *() const {
        return value.get().toObjectOrNull();
    }

    friend void JS::AutoGCRooter::trace(JSTracer *trc);

  private:
    WrapperValue value;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#endif
