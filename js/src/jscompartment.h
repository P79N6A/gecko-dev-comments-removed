





#ifndef jscompartment_h
#define jscompartment_h

#include "mozilla/MemoryReporting.h"

#include "prmjtime.h"
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
    JSFlatString* s;      

  public:
    DtoaCache() : s(nullptr) {}
    void purge() { s = nullptr; }

    JSFlatString* lookup(int base, double d) {
        return this->s && base == this->base && d == this->d ? this->s : nullptr;
    }

    void cache(int base, double d, JSFlatString* s) {
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
    JSObject* debugger;
    js::gc::Cell* wrapped;

    explicit CrossCompartmentKey(JSObject* wrapped)
      : kind(ObjectWrapper), debugger(nullptr), wrapped(wrapped)
    {
        MOZ_RELEASE_ASSERT(wrapped);
    }
    explicit CrossCompartmentKey(JSString* wrapped)
      : kind(StringWrapper), debugger(nullptr), wrapped(wrapped)
    {
        MOZ_RELEASE_ASSERT(wrapped);
    }
    explicit CrossCompartmentKey(Value wrappedArg)
      : kind(wrappedArg.isString() ? StringWrapper : ObjectWrapper),
        debugger(nullptr),
        wrapped((js::gc::Cell*)wrappedArg.toGCThing())
    {
        MOZ_RELEASE_ASSERT(wrappedArg.isString() || wrappedArg.isObject());
        MOZ_RELEASE_ASSERT(wrapped);
    }
    explicit CrossCompartmentKey(const RootedValue& wrappedArg)
      : kind(wrappedArg.get().isString() ? StringWrapper : ObjectWrapper),
        debugger(nullptr),
        wrapped((js::gc::Cell*)wrappedArg.get().toGCThing())
    {
        MOZ_RELEASE_ASSERT(wrappedArg.isString() || wrappedArg.isObject());
        MOZ_RELEASE_ASSERT(wrapped);
    }
    CrossCompartmentKey(Kind kind, JSObject* dbg, js::gc::Cell* wrapped)
      : kind(kind), debugger(dbg), wrapped(wrapped)
    {
        MOZ_RELEASE_ASSERT(dbg);
        MOZ_RELEASE_ASSERT(wrapped);
    }

  private:
    CrossCompartmentKey() = delete;
};

struct WrapperHasher : public DefaultHasher<CrossCompartmentKey>
{
    static HashNumber hash(const CrossCompartmentKey& key) {
        static_assert(sizeof(HashNumber) == sizeof(uint32_t),
                      "subsequent code assumes a four-byte hash");
        return uint32_t(uintptr_t(key.wrapped)) | uint32_t(key.kind);
    }

    static bool match(const CrossCompartmentKey& l, const CrossCompartmentKey& k) {
        return l.kind == k.kind && l.debugger == k.debugger && l.wrapped == k.wrapped;
    }
};

typedef HashMap<CrossCompartmentKey, ReadBarrieredValue,
                WrapperHasher, SystemAllocPolicy> WrapperMap;

} 

namespace js {
class DebugScopes;
class ObjectWeakMap;
class WatchpointMap;
class WeakMapBase;
}

struct JSCompartment
{
    JS::CompartmentOptions       options_;

  private:
    JS::Zone*                    zone_;
    JSRuntime*                   runtime_;

  public:
    





    inline JSPrincipals* principals() {
        return principals_;
    }
    inline void setPrincipals(JSPrincipals* principals) {
        if (principals_ == principals)
            return;

        
        
        
        
        
        
        
        performanceMonitoring.unlink();
        principals_ = principals;
    }
    inline bool isSystem() const {
        return isSystem_;
    }
    inline void setIsSystem(bool isSystem) {
        if (isSystem_ == isSystem)
            return;

        
        
        
        
        
        
        
        performanceMonitoring.unlink();
        isSystem_ = isSystem;
    }
  private:
    JSPrincipals*                principals_;
    bool                         isSystem_;
  public:
    bool                         isSelfHosting;
    bool                         marked;
    bool                         warnedAboutNoSuchMethod;
    bool                         warnedAboutFlagsArgument;

    
    
    JSAddonId*                   const addonId;

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
    int64_t                      startInterval;

  public:
    js::PerformanceGroupHolder performanceMonitoring;

    void enter() {
        enterCompartmentDepth++;
    }
    void leave() {
        enterCompartmentDepth--;
    }
    bool hasBeenEntered() { return !!enterCompartmentDepth; }

    JS::Zone* zone() { return zone_; }
    const JS::Zone* zone() const { return zone_; }
    JS::CompartmentOptions& options() { return options_; }
    const JS::CompartmentOptions& options() const { return options_; }

    JSRuntime* runtimeFromMainThread() {
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(runtime_));
        return runtime_;
    }

    
    
    JSRuntime* runtimeFromAnyThread() const {
        return runtime_;
    }

    










    inline js::GlobalObject* maybeGlobal() const;

    
    inline js::GlobalObject* unsafeUnbarrieredMaybeGlobal() const;

    inline void initGlobal(js::GlobalObject& global);

  public:
    void*                        data;

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
                                size_t* tiAllocationSiteTables,
                                size_t* tiArrayTypeTables,
                                size_t* tiObjectTypeTables,
                                size_t* compartmentObject,
                                size_t* compartmentTables,
                                size_t* innerViews,
                                size_t* lazyArrayBuffers,
                                size_t* objectMetadataTables,
                                size_t* crossCompartmentWrappers,
                                size_t* regexpCompartment,
                                size_t* savedStacksSet);

    


    js::PropertyTree             propertyTree;

    
    js::BaseShapeSet             baseShapes;
    void sweepBaseShapeTable();

    
    js::InitialShapeSet          initialShapes;
    void sweepInitialShapeTable();

    
    js::ObjectGroupCompartment   objectGroups;

#ifdef JSGC_HASH_TABLE_CHECKS
    void checkInitialShapesTableAfterMovingGC();
    void checkWrapperMapAfterMovingGC();
    void checkBaseShapeTableAfterMovingGC();
#endif

    



    js::ReadBarrieredScriptSourceObject selfHostingScriptSource;

    
    
    js::ObjectWeakMap* objectMetadataTable;

    
    js::InnerViewTable innerViews;

    
    
    
    js::ObjectWeakMap* lazyArrayBuffers;

    
    mozilla::LinkedList<js::UnboxedLayout> unboxedLayouts;

    
    unsigned                     gcIndex;

    






    JSObject*                    gcIncomingGrayPointers;

    
    js::WeakMapBase*             gcWeakMapList;

  private:
    
    bool                         gcPreserveJitCode;

    enum {
        IsDebuggee = 1 << 0,
        DebuggerObservesAllExecution = 1 << 1,
        DebuggerObservesAsmJS = 1 << 2,
        DebuggerNeedsDelazification = 1 << 3
    };

    unsigned                     debugModeBits;

    static const unsigned DebuggerObservesMask = IsDebuggee |
                                                 DebuggerObservesAllExecution |
                                                 DebuggerObservesAsmJS;

    void updateDebuggerObservesFlag(unsigned flag);

  public:
    JSCompartment(JS::Zone* zone, const JS::CompartmentOptions& options);
    ~JSCompartment();

    bool init(JSContext* cx);

    
    void markCrossCompartmentWrappers(JSTracer* trc);

    inline bool wrap(JSContext* cx, JS::MutableHandleValue vp,
                     JS::HandleObject existing = js::NullPtr());

    bool wrap(JSContext* cx, js::MutableHandleString strp);
    bool wrap(JSContext* cx, JS::MutableHandleObject obj,
              JS::HandleObject existingArg = js::NullPtr());
    bool wrap(JSContext* cx, JS::MutableHandle<js::PropertyDescriptor> desc);

    template<typename T> bool wrap(JSContext* cx, JS::AutoVectorRooter<T>& vec) {
        for (size_t i = 0; i < vec.length(); ++i) {
            if (!wrap(cx, vec[i]))
                return false;
        }
        return true;
    };

    bool putWrapper(JSContext* cx, const js::CrossCompartmentKey& wrapped, const js::Value& wrapper);

    js::WrapperMap::Ptr lookupWrapper(const js::Value& wrapped) const {
        return crossCompartmentWrappers.lookup(js::CrossCompartmentKey(wrapped));
    }

    void removeWrapper(js::WrapperMap::Ptr p) {
        crossCompartmentWrappers.remove(p);
    }

    struct WrapperEnum : public js::WrapperMap::Enum {
        explicit WrapperEnum(JSCompartment* c) : js::WrapperMap::Enum(c->crossCompartmentWrappers) {}
    };

    void trace(JSTracer* trc);
    void markRoots(JSTracer* trc);
    bool preserveJitCode() { return gcPreserveJitCode; }

    void sweepInnerViews();
    void sweepCrossCompartmentWrappers();
    void sweepSavedStacks();
    void sweepGlobalObject(js::FreeOp* fop);
    void sweepSelfHostingScriptSource();
    void sweepJitCompartment(js::FreeOp* fop);
    void sweepRegExps();
    void sweepDebugScopes();
    void sweepWeakMaps();
    void sweepNativeIterators();

    void purge();
    void clearTables();

    void fixupInitialShapeTable();
    void fixupAfterMovingGC();
    void fixupGlobal();

    bool hasObjectMetadataCallback() const { return objectMetadataCallback; }
    void setObjectMetadataCallback(js::ObjectMetadataCallback callback);
    void forgetObjectMetadataCallback() {
        objectMetadataCallback = nullptr;
    }
    void setNewObjectMetadata(JSContext* cx, JSObject* obj);
    void clearObjectMetadata();
    const void* addressOfMetadataCallback() const {
        return &objectMetadataCallback;
    }

    js::SavedStacks& savedStacks() { return savedStacks_; }

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone>& finder);

    js::DtoaCache dtoaCache;

    
    uint64_t rngState;

  private:
    JSCompartment* thisForCtor() { return this; }

  public:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    bool isDebuggee() const { return !!(debugModeBits & IsDebuggee); }
    void setIsDebuggee() { debugModeBits |= IsDebuggee; }
    void unsetIsDebuggee();

    
    
    
    bool debuggerObservesAllExecution() const {
        static const unsigned Mask = IsDebuggee | DebuggerObservesAllExecution;
        return (debugModeBits & Mask) == Mask;
    }
    void updateDebuggerObservesAllExecution() {
        updateDebuggerObservesFlag(DebuggerObservesAllExecution);
    }

    
    
    
    
    
    
    bool debuggerObservesAsmJS() const {
        static const unsigned Mask = IsDebuggee | DebuggerObservesAsmJS;
        return (debugModeBits & Mask) == Mask;
    }
    void updateDebuggerObservesAsmJS() {
        updateDebuggerObservesFlag(DebuggerObservesAsmJS);
    }

    bool needsDelazificationForDebugger() const {
        return debugModeBits & DebuggerNeedsDelazification;
    }

    



    void scheduleDelazificationForDebugger() { debugModeBits |= DebuggerNeedsDelazification; }

    



    bool ensureDelazifyScriptsForDebugger(JSContext* cx);

    void clearBreakpointsIn(js::FreeOp* fop, js::Debugger* dbg, JS::HandleObject handler);

  private:
    void sweepBreakpoints(js::FreeOp* fop);

  public:
    js::WatchpointMap* watchpointMap;

    js::ScriptCountsMap* scriptCountsMap;

    js::DebugScriptMap* debugScriptMap;

    
    js::DebugScopes* debugScopes;

    



    js::NativeIterator* enumerators;

    
    void*              compartmentStats;

    
    
    bool scheduledForDestruction;
    bool maybeAlive;

  private:
    js::jit::JitCompartment* jitCompartment_;

  public:
    bool ensureJitCompartmentExists(JSContext* cx);
    js::jit::JitCompartment* jitCompartment() {
        return jitCompartment_;
    }

    enum DeprecatedLanguageExtension {
        DeprecatedForEach = 0,              
        
        DeprecatedLegacyGenerator = 2,      
        DeprecatedExpressionClosure = 3,    
        DeprecatedLetBlock = 4,             
        DeprecatedLetExpression = 5,        
        DeprecatedNoSuchMethod = 6,         
        DeprecatedFlagsArgument = 7,        
        RegExpSourceProperty = 8,           
        DeprecatedLanguageExtensionCount
    };

  private:
    
    bool sawDeprecatedLanguageExtension[DeprecatedLanguageExtensionCount];

    void reportTelemetry();

  public:
    void addTelemetry(const char* filename, DeprecatedLanguageExtension e);
};

inline bool
JSRuntime::isAtomsZone(JS::Zone* zone)
{
    return zone == atomsCompartment_->zone();
}

namespace js {






template<typename T> inline void SetMaybeAliveFlag(T* thing) {}
template<> inline void SetMaybeAliveFlag(JSObject* thing) {thing->compartment()->maybeAlive = true;}
template<> inline void SetMaybeAliveFlag(JSScript* thing) {thing->compartment()->maybeAlive = true;}

inline js::Handle<js::GlobalObject*>
ExclusiveContext::global() const
{
    





    MOZ_ASSERT(compartment_, "Caller needs to enter a compartment first");
    return Handle<GlobalObject*>::fromMarkedLocation(compartment_->global_.unsafeGet());
}

class AssertCompartmentUnchanged
{
  public:
    explicit AssertCompartmentUnchanged(JSContext* cx
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
    inline AutoCompartment(ExclusiveContext* cx, JSObject* target);
    inline AutoCompartment(ExclusiveContext* cx, JSCompartment* target);
    inline ~AutoCompartment();

    ExclusiveContext* context() const { return cx_; }
    JSCompartment* origin() const { return origin_; }

  private:
    AutoCompartment(const AutoCompartment&) = delete;
    AutoCompartment & operator=(const AutoCompartment&) = delete;
};






class ErrorCopier
{
    mozilla::Maybe<AutoCompartment>& ac;

  public:
    explicit ErrorCopier(mozilla::Maybe<AutoCompartment>& ac)
      : ac(ac) {}
    ~ErrorCopier();
};





















struct WrapperValue
{
    





    explicit WrapperValue(const WrapperMap::Ptr& ptr)
      : value(*ptr->value().unsafeGet())
    {}

    explicit WrapperValue(const WrapperMap::Enum& e)
      : value(*e.front().value().unsafeGet())
    {}

    Value& get() { return value; }
    Value get() const { return value; }
    operator const Value&() const { return value; }
    JSObject& toObject() const { return value.toObject(); }

  private:
    Value value;
};

class AutoWrapperVector : public AutoVectorRooter<WrapperValue>
{
  public:
    explicit AutoWrapperVector(JSContext* cx
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<WrapperValue>(cx, WRAPVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoWrapperRooter : private JS::AutoGCRooter {
  public:
    AutoWrapperRooter(JSContext* cx, WrapperValue v
                      MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : JS::AutoGCRooter(cx, WRAPPER), value(v)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    operator JSObject*() const {
        return value.get().toObjectOrNull();
    }

    friend void JS::AutoGCRooter::trace(JSTracer* trc);

  private:
    WrapperValue value;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#endif
