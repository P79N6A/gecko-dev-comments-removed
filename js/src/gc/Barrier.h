





#ifndef gc_Barrier_h
#define gc_Barrier_h

#include "NamespaceImports.h"

#include "gc/Heap.h"
#include "gc/StoreBuffer.h"
#include "js/Id.h"
#include "js/RootingAPI.h"
#include "js/Value.h"






































































































































class JSAtom;
struct JSCompartment;
class JSFlatString;
class JSLinearString;

namespace JS {
class Symbol;
}

namespace js {

class AccessorShape;
class ArrayObject;
class ArgumentsObject;
class ArrayBufferObjectMaybeShared;
class ArrayBufferObject;
class ArrayBufferViewObject;
class SharedArrayBufferObject;
class SharedTypedArrayObject;
class BaseShape;
class DebugScopeObject;
class GlobalObject;
class LazyScript;
class NativeObject;
class NestedScopeObject;
class PlainObject;
class PropertyName;
class SavedFrame;
class ScopeObject;
class ScriptSourceObject;
class Shape;
class UnownedBaseShape;
class ObjectGroup;

namespace jit {
class JitCode;
}

#ifdef DEBUG


bool
CurrentThreadIsIonCompiling();

bool
CurrentThreadIsGCSweeping();
#endif

bool
StringIsPermanentAtom(JSString* str);

namespace gc {

template <typename T> struct MapTypeToTraceKind {};
template <> struct MapTypeToTraceKind<NativeObject>     { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArrayObject>      { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArgumentsObject>  { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArrayBufferObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArrayBufferObjectMaybeShared>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ArrayBufferViewObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<BaseShape>        { static const JSGCTraceKind kind = JSTRACE_BASE_SHAPE; };
template <> struct MapTypeToTraceKind<DebugScopeObject> { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<GlobalObject>     { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<JS::Symbol>       { static const JSGCTraceKind kind = JSTRACE_SYMBOL; };
template <> struct MapTypeToTraceKind<JSAtom>           { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<JSFlatString>     { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<JSFunction>       { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<JSLinearString>   { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<JSObject>         { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<JSScript>         { static const JSGCTraceKind kind = JSTRACE_SCRIPT; };
template <> struct MapTypeToTraceKind<JSString>         { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<LazyScript>       { static const JSGCTraceKind kind = JSTRACE_LAZY_SCRIPT; };
template <> struct MapTypeToTraceKind<NestedScopeObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<PlainObject>      { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<PropertyName>     { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<SavedFrame>       { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ScopeObject>      { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<Shape>            { static const JSGCTraceKind kind = JSTRACE_SHAPE; };
template <> struct MapTypeToTraceKind<AccessorShape>    { static const JSGCTraceKind kind = JSTRACE_SHAPE; };
template <> struct MapTypeToTraceKind<SharedArrayBufferObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<SharedTypedArrayObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<UnownedBaseShape> { static const JSGCTraceKind kind = JSTRACE_BASE_SHAPE; };
template <> struct MapTypeToTraceKind<jit::JitCode>     { static const JSGCTraceKind kind = JSTRACE_JITCODE; };
template <> struct MapTypeToTraceKind<ObjectGroup>      { static const JSGCTraceKind kind = JSTRACE_OBJECT_GROUP; };



void MarkValueForBarrier(JSTracer* trc, Value* v, const char* name);
void MarkIdForBarrier(JSTracer* trc, jsid* idp, const char* name);

} 



static inline const gc::TenuredCell* AsTenuredCell(const JSString* str) {
    return reinterpret_cast<const gc::TenuredCell*>(str);
}
static inline const gc::TenuredCell* AsTenuredCell(const JS::Symbol* sym) {
    return reinterpret_cast<const gc::TenuredCell*>(sym);
}

JS::Zone*
ZoneOfObjectFromAnyThread(const JSObject& obj);

static inline JS::shadow::Zone*
ShadowZoneOfObjectFromAnyThread(JSObject* obj)
{
    return JS::shadow::Zone::asShadowZone(ZoneOfObjectFromAnyThread(*obj));
}

static inline JS::shadow::Zone*
ShadowZoneOfStringFromAnyThread(JSString* str)
{
    return JS::shadow::Zone::asShadowZone(AsTenuredCell(str)->zoneFromAnyThread());
}

static inline JS::shadow::Zone*
ShadowZoneOfSymbolFromAnyThread(JS::Symbol* sym)
{
    return JS::shadow::Zone::asShadowZone(AsTenuredCell(sym)->zoneFromAnyThread());
}

MOZ_ALWAYS_INLINE JS::Zone*
ZoneOfValueFromAnyThread(const JS::Value& value)
{
    MOZ_ASSERT(value.isMarkable());
    if (value.isObject())
        return ZoneOfObjectFromAnyThread(value.toObject());
    return js::gc::TenuredCell::fromPointer(value.toGCThing())->zoneFromAnyThread();
}

MOZ_ALWAYS_INLINE JS::Zone*
ZoneOfIdFromAnyThread(const jsid& id)
{
    MOZ_ASSERT(JSID_IS_GCTHING(id));
    return js::gc::TenuredCell::fromPointer(JSID_TO_GCTHING(id).asCell())->zoneFromAnyThread();
}

void
ValueReadBarrier(const Value& value);

template <typename T>
struct InternalGCMethods {};

template <typename T>
struct InternalGCMethods<T*>
{
    static bool isMarkable(T* v) { return v != nullptr; }

    static void preBarrier(T* v) { T::writeBarrierPre(v); }

    static void postBarrier(T** vp) { T::writeBarrierPost(*vp, vp); }
    static void postBarrierRelocate(T** vp) { T::writeBarrierPostRelocate(*vp, vp); }
    static void postBarrierRemove(T** vp) { T::writeBarrierPostRemove(*vp, vp); }

    static void readBarrier(T* v) { T::readBarrier(v); }
};

template <>
struct InternalGCMethods<Value>
{
    static JSRuntime* runtimeFromAnyThread(const Value& v) {
        MOZ_ASSERT(v.isMarkable());
        return static_cast<js::gc::Cell*>(v.toGCThing())->runtimeFromAnyThread();
    }
    static JS::shadow::Runtime* shadowRuntimeFromAnyThread(const Value& v) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromAnyThread(v));
    }
    static JSRuntime* runtimeFromMainThread(const Value& v) {
        MOZ_ASSERT(v.isMarkable());
        return static_cast<js::gc::Cell*>(v.toGCThing())->runtimeFromMainThread();
    }
    static JS::shadow::Runtime* shadowRuntimeFromMainThread(const Value& v) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromMainThread(v));
    }

    static bool isMarkable(Value v) { return v.isMarkable(); }

    static void preBarrier(Value v) {
        MOZ_ASSERT(!CurrentThreadIsIonCompiling());
        if (v.isString() && StringIsPermanentAtom(v.toString()))
            return;
        if (v.isMarkable() && shadowRuntimeFromAnyThread(v)->needsIncrementalBarrier())
            preBarrierImpl(ZoneOfValueFromAnyThread(v), v);
    }

  private:
    static void preBarrierImpl(Zone* zone, Value v) {
        JS::shadow::Zone* shadowZone = JS::shadow::Zone::asShadowZone(zone);
        if (shadowZone->needsIncrementalBarrier()) {
            MOZ_ASSERT_IF(v.isMarkable(), shadowRuntimeFromMainThread(v)->needsIncrementalBarrier());
            Value tmp(v);
            js::gc::MarkValueForBarrier(shadowZone->barrierTracer(), &tmp, "write barrier");
            MOZ_ASSERT(tmp == v);
        }
    }

  public:
    static void postBarrier(Value* vp) {
        MOZ_ASSERT(!CurrentThreadIsIonCompiling());
        if (vp->isObject()) {
            gc::StoreBuffer* sb = reinterpret_cast<gc::Cell*>(&vp->toObject())->storeBuffer();
            if (sb)
                sb->putValueFromAnyThread(vp);
        }
    }

    static void postBarrierRelocate(Value* vp) {
        MOZ_ASSERT(!CurrentThreadIsIonCompiling());
        if (vp->isObject()) {
            gc::StoreBuffer* sb = reinterpret_cast<gc::Cell*>(&vp->toObject())->storeBuffer();
            if (sb)
                sb->putRelocatableValueFromAnyThread(vp);
        }
    }

    static void postBarrierRemove(Value* vp) {
        MOZ_ASSERT(vp);
        MOZ_ASSERT(vp->isMarkable());
        MOZ_ASSERT(!CurrentThreadIsIonCompiling());
        JSRuntime* rt = static_cast<js::gc::Cell*>(vp->toGCThing())->runtimeFromAnyThread();
        JS::shadow::Runtime* shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
        shadowRuntime->gcStoreBufferPtr()->removeRelocatableValueFromAnyThread(vp);
    }

    static void readBarrier(const Value& v) { ValueReadBarrier(v); }
};

template <>
struct InternalGCMethods<jsid>
{
    static bool isMarkable(jsid id) { return JSID_IS_STRING(id) || JSID_IS_SYMBOL(id); }

    static void preBarrier(jsid id) {
        MOZ_ASSERT(!CurrentThreadIsIonCompiling());
        if (JSID_IS_STRING(id) && StringIsPermanentAtom(JSID_TO_STRING(id)))
            return;
        if (JSID_IS_GCTHING(id) && shadowRuntimeFromAnyThread(id)->needsIncrementalBarrier())
            preBarrierImpl(ZoneOfIdFromAnyThread(id), id);
    }

  private:
    static JSRuntime* runtimeFromAnyThread(jsid id) {
        MOZ_ASSERT(JSID_IS_GCTHING(id));
        return JSID_TO_GCTHING(id).asCell()->runtimeFromAnyThread();
    }
    static JS::shadow::Runtime* shadowRuntimeFromAnyThread(jsid id) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromAnyThread(id));
    }
    static void preBarrierImpl(Zone* zone, jsid id) {
        JS::shadow::Zone* shadowZone = JS::shadow::Zone::asShadowZone(zone);
        if (shadowZone->needsIncrementalBarrier()) {
            jsid tmp(id);
            js::gc::MarkIdForBarrier(shadowZone->barrierTracer(), &tmp, "id write barrier");
            MOZ_ASSERT(tmp == id);
        }
    }

  public:
    static void postBarrier(jsid* idp) {}
    static void postBarrierRelocate(jsid* idp) {}
    static void postBarrierRemove(jsid* idp) {}
};

template <typename T>
class BarrieredBaseMixins {};




template <class T>
class BarrieredBase : public BarrieredBaseMixins<T>
{
  protected:
    T value;

    explicit BarrieredBase(T v) : value(v) {}
    ~BarrieredBase() { pre(); }

  public:
    void init(T v) {
        this->value = v;
    }

    DECLARE_POINTER_COMPARISON_OPS(T);
    DECLARE_POINTER_CONSTREF_OPS(T);

    
    const T& get() const { return value; }

    



    T* unsafeGet() { return &value; }
    const T* unsafeGet() const { return &value; }
    void unsafeSet(T v) { value = v; }

    
    static void writeBarrierPre(const T& v) { InternalGCMethods<T>::preBarrier(v); }
    static void writeBarrierPost(const T& v, T* vp) { InternalGCMethods<T>::postBarrier(vp); }

  protected:
    void pre() { InternalGCMethods<T>::preBarrier(value); }
};

template <>
class BarrieredBaseMixins<JS::Value> : public ValueOperations<BarrieredBase<JS::Value> >
{
    friend class ValueOperations<BarrieredBase<JS::Value> >;
    const JS::Value * extract() const {
        return static_cast<const BarrieredBase<JS::Value>*>(this)->unsafeGet();
    }
};







template <class T>
class PreBarriered : public BarrieredBase<T>
{
  public:
    PreBarriered() : BarrieredBase<T>(GCMethods<T>::initial()) {}
    


    MOZ_IMPLICIT PreBarriered(T v) : BarrieredBase<T>(v) {}
    explicit PreBarriered(const PreBarriered<T>& v)
      : BarrieredBase<T>(v.value) {}

    
    void clear() {
        this->pre();
        this->value = nullptr;
    }

    DECLARE_POINTER_ASSIGN_OPS(PreBarriered, T);

  private:
    void set(const T& v) {
        this->pre();
        this->value = v;
    }
};















template <class T>
class HeapPtr : public BarrieredBase<T>
{
  public:
    HeapPtr() : BarrieredBase<T>(GCMethods<T>::initial()) {}
    explicit HeapPtr(T v) : BarrieredBase<T>(v) { post(); }
    explicit HeapPtr(const HeapPtr<T>& v) : BarrieredBase<T>(v) { post(); }
#ifdef DEBUG
    ~HeapPtr() {
        MOZ_ASSERT(CurrentThreadIsGCSweeping());
    }
#endif

    void init(T v) {
        this->value = v;
        post();
    }

    DECLARE_POINTER_ASSIGN_OPS(HeapPtr, T);

  protected:
    void post() { InternalGCMethods<T>::postBarrier(&this->value); }

  private:
    void set(const T& v) {
        this->pre();
        this->value = v;
        post();
    }

    






    HeapPtr(HeapPtr<T>&&) = delete;
    HeapPtr<T>& operator=(HeapPtr<T>&&) = delete;
};













template <typename T>
class ImmutableTenuredPtr
{
    T value;

  public:
    operator T() const { return value; }
    T operator->() const { return value; }

    operator Handle<T>() const {
        return Handle<T>::fromMarkedLocation(&value);
    }

    void init(T ptr) {
        MOZ_ASSERT(ptr->isTenured());
        value = ptr;
    }

    T get() const { return value; }
    const T* address() { return &value; }
};








template <class T>
class RelocatablePtr : public BarrieredBase<T>
{
  public:
    RelocatablePtr() : BarrieredBase<T>(GCMethods<T>::initial()) {}
    explicit RelocatablePtr(T v) : BarrieredBase<T>(v) {
        if (GCMethods<T>::needsPostBarrier(v))
            post();
    }

    





    RelocatablePtr(const RelocatablePtr<T>& v) : BarrieredBase<T>(v) {
        if (GCMethods<T>::needsPostBarrier(this->value))
            post();
    }

    ~RelocatablePtr() {
        if (GCMethods<T>::needsPostBarrier(this->value))
            relocate();
    }

    DECLARE_POINTER_ASSIGN_OPS(RelocatablePtr, T);

    
    template <class T1, class T2>
    friend inline void
    BarrieredSetPair(Zone* zone,
                     RelocatablePtr<T1*>& v1, T1* val1,
                     RelocatablePtr<T2*>& v2, T2* val2);

  protected:
    void set(const T& v) {
        this->pre();
        postBarrieredSet(v);
    }

    void postBarrieredSet(const T& v) {
        if (GCMethods<T>::needsPostBarrier(v)) {
            this->value = v;
            post();
        } else if (GCMethods<T>::needsPostBarrier(this->value)) {
            relocate();
            this->value = v;
        } else {
            this->value = v;
        }
    }

    void post() {
        MOZ_ASSERT(GCMethods<T>::needsPostBarrier(this->value));
        InternalGCMethods<T>::postBarrierRelocate(&this->value);
    }

    void relocate() {
        MOZ_ASSERT(GCMethods<T>::needsPostBarrier(this->value));
        InternalGCMethods<T>::postBarrierRemove(&this->value);
    }
};





template <class T1, class T2>
static inline void
BarrieredSetPair(Zone* zone,
                 RelocatablePtr<T1*>& v1, T1* val1,
                 RelocatablePtr<T2*>& v2, T2* val2)
{
    if (T1::needWriteBarrierPre(zone)) {
        v1.pre();
        v2.pre();
    }
    v1.postBarrieredSet(val1);
    v2.postBarrieredSet(val2);
}


template <class T>
struct HeapPtrHasher
{
    typedef HeapPtr<T> Key;
    typedef T Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T>::hash(obj); }
    static bool match(const Key& k, Lookup l) { return k.get() == l; }
    static void rekey(Key& k, const Key& newKey) { k.unsafeSet(newKey); }
};


template <class T>
struct DefaultHasher<HeapPtr<T>> : HeapPtrHasher<T> { };

template <class T>
struct PreBarrieredHasher
{
    typedef PreBarriered<T> Key;
    typedef T Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T>::hash(obj); }
    static bool match(const Key& k, Lookup l) { return k.get() == l; }
    static void rekey(Key& k, const Key& newKey) { k.unsafeSet(newKey); }
};

template <class T>
struct DefaultHasher<PreBarriered<T>> : PreBarrieredHasher<T> { };











template <class T>
class ReadBarriered
{
    T value;

  public:
    ReadBarriered() : value(nullptr) {}
    explicit ReadBarriered(T value) : value(value) {}
    explicit ReadBarriered(const Rooted<T>& rooted) : value(rooted) {}

    T get() const {
        if (!InternalGCMethods<T>::isMarkable(value))
            return GCMethods<T>::initial();
        InternalGCMethods<T>::readBarrier(value);
        return value;
    }

    T unbarrieredGet() const {
        return value;
    }

    operator T() const { return get(); }

    T& operator*() const { return *get(); }
    T operator->() const { return get(); }

    T* unsafeGet() { return &value; }
    T const * unsafeGet() const { return &value; }

    void set(T v) { value = v; }
};


template <class T>
struct ReadBarrieredHasher
{
    typedef ReadBarriered<T> Key;
    typedef T Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T>::hash(obj); }
    static bool match(const Key& k, Lookup l) { return k.get() == l; }
    static void rekey(Key& k, const Key& newKey) { k.set(newKey); }
};


template <class T>
struct DefaultHasher<ReadBarriered<T>> : ReadBarrieredHasher<T> { };

class ArrayObject;
class ArrayBufferObject;
class NestedScopeObject;
class DebugScopeObject;
class GlobalObject;
class ScriptSourceObject;
class Shape;
class BaseShape;
class UnownedBaseShape;
namespace jit {
class JitCode;
}

typedef PreBarriered<JSObject*> PreBarrieredObject;
typedef PreBarriered<JSScript*> PreBarrieredScript;
typedef PreBarriered<jit::JitCode*> PreBarrieredJitCode;
typedef PreBarriered<JSString*> PreBarrieredString;
typedef PreBarriered<JSAtom*> PreBarrieredAtom;

typedef RelocatablePtr<JSObject*> RelocatablePtrObject;
typedef RelocatablePtr<JSFunction*> RelocatablePtrFunction;
typedef RelocatablePtr<PlainObject*> RelocatablePtrPlainObject;
typedef RelocatablePtr<JSScript*> RelocatablePtrScript;
typedef RelocatablePtr<NativeObject*> RelocatablePtrNativeObject;
typedef RelocatablePtr<NestedScopeObject*> RelocatablePtrNestedScopeObject;
typedef RelocatablePtr<Shape*> RelocatablePtrShape;
typedef RelocatablePtr<ObjectGroup*> RelocatablePtrObjectGroup;
typedef RelocatablePtr<jit::JitCode*> RelocatablePtrJitCode;
typedef RelocatablePtr<JSLinearString*> RelocatablePtrLinearString;
typedef RelocatablePtr<JSString*> RelocatablePtrString;
typedef RelocatablePtr<JSAtom*> RelocatablePtrAtom;
typedef RelocatablePtr<ArrayBufferObjectMaybeShared*> RelocatablePtrArrayBufferObjectMaybeShared;

typedef HeapPtr<NativeObject*> HeapPtrNativeObject;
typedef HeapPtr<ArrayObject*> HeapPtrArrayObject;
typedef HeapPtr<ArrayBufferObjectMaybeShared*> HeapPtrArrayBufferObjectMaybeShared;
typedef HeapPtr<ArrayBufferObject*> HeapPtrArrayBufferObject;
typedef HeapPtr<BaseShape*> HeapPtrBaseShape;
typedef HeapPtr<JSAtom*> HeapPtrAtom;
typedef HeapPtr<JSFlatString*> HeapPtrFlatString;
typedef HeapPtr<JSFunction*> HeapPtrFunction;
typedef HeapPtr<JSLinearString*> HeapPtrLinearString;
typedef HeapPtr<JSObject*> HeapPtrObject;
typedef HeapPtr<JSScript*> HeapPtrScript;
typedef HeapPtr<JSString*> HeapPtrString;
typedef HeapPtr<PlainObject*> HeapPtrPlainObject;
typedef HeapPtr<PropertyName*> HeapPtrPropertyName;
typedef HeapPtr<Shape*> HeapPtrShape;
typedef HeapPtr<UnownedBaseShape*> HeapPtrUnownedBaseShape;
typedef HeapPtr<jit::JitCode*> HeapPtrJitCode;
typedef HeapPtr<ObjectGroup*> HeapPtrObjectGroup;

typedef PreBarriered<Value> PreBarrieredValue;
typedef RelocatablePtr<Value> RelocatableValue;
typedef HeapPtr<Value> HeapValue;

typedef PreBarriered<jsid> PreBarrieredId;
typedef RelocatablePtr<jsid> RelocatableId;
typedef HeapPtr<jsid> HeapId;

typedef ImmutableTenuredPtr<PropertyName*> ImmutablePropertyNamePtr;
typedef ImmutableTenuredPtr<JS::Symbol*> ImmutableSymbolPtr;

typedef ReadBarriered<DebugScopeObject*> ReadBarrieredDebugScopeObject;
typedef ReadBarriered<GlobalObject*> ReadBarrieredGlobalObject;
typedef ReadBarriered<JSFunction*> ReadBarrieredFunction;
typedef ReadBarriered<JSObject*> ReadBarrieredObject;
typedef ReadBarriered<ScriptSourceObject*> ReadBarrieredScriptSourceObject;
typedef ReadBarriered<Shape*> ReadBarrieredShape;
typedef ReadBarriered<UnownedBaseShape*> ReadBarrieredUnownedBaseShape;
typedef ReadBarriered<jit::JitCode*> ReadBarrieredJitCode;
typedef ReadBarriered<ObjectGroup*> ReadBarrieredObjectGroup;
typedef ReadBarriered<JSAtom*> ReadBarrieredAtom;
typedef ReadBarriered<JS::Symbol*> ReadBarrieredSymbol;

typedef ReadBarriered<Value> ReadBarrieredValue;




class HeapSlot : public BarrieredBase<Value>
{
  public:
    enum Kind {
        Slot = 0,
        Element = 1
    };

    explicit HeapSlot() = delete;

    explicit HeapSlot(NativeObject* obj, Kind kind, uint32_t slot, const Value& v)
      : BarrieredBase<Value>(v)
    {
        post(obj, kind, slot, v);
    }

    explicit HeapSlot(NativeObject* obj, Kind kind, uint32_t slot, const HeapSlot& s)
      : BarrieredBase<Value>(s.value)
    {
        post(obj, kind, slot, s);
    }

    ~HeapSlot() {
        pre();
    }

    void init(NativeObject* owner, Kind kind, uint32_t slot, const Value& v) {
        value = v;
        post(owner, kind, slot, v);
    }

#ifdef DEBUG
    bool preconditionForSet(NativeObject* owner, Kind kind, uint32_t slot);
    bool preconditionForWriteBarrierPost(NativeObject* obj, Kind kind, uint32_t slot, Value target) const;
#endif

    void set(NativeObject* owner, Kind kind, uint32_t slot, const Value& v) {
        MOZ_ASSERT(preconditionForSet(owner, kind, slot));
        pre();
        value = v;
        post(owner, kind, slot, v);
    }

    
    static void writeBarrierPost(NativeObject* owner, Kind kind, uint32_t slot, const Value& target) {
        reinterpret_cast<HeapSlot*>(const_cast<Value*>(&target))->post(owner, kind, slot, target);
    }

  private:
    void post(NativeObject* owner, Kind kind, uint32_t slot, const Value& target) {
        MOZ_ASSERT(preconditionForWriteBarrierPost(owner, kind, slot, target));
        if (this->value.isObject()) {
            gc::Cell* cell = reinterpret_cast<gc::Cell*>(&this->value.toObject());
            if (cell->storeBuffer())
                cell->storeBuffer()->putSlotFromAnyThread(owner, kind, slot, 1);
        }
    }
};

static inline const Value*
Valueify(const BarrieredBase<Value>* array)
{
    JS_STATIC_ASSERT(sizeof(HeapValue) == sizeof(Value));
    JS_STATIC_ASSERT(sizeof(HeapSlot) == sizeof(Value));
    return (const Value*)array;
}

static inline HeapValue*
HeapValueify(Value* v)
{
    JS_STATIC_ASSERT(sizeof(HeapValue) == sizeof(Value));
    JS_STATIC_ASSERT(sizeof(HeapSlot) == sizeof(Value));
    return (HeapValue*)v;
}

class HeapSlotArray
{
    HeapSlot* array;

    
    
#ifdef DEBUG
    bool allowWrite_;
#endif

  public:
    explicit HeapSlotArray(HeapSlot* array, bool allowWrite)
      : array(array)
#ifdef DEBUG
      , allowWrite_(allowWrite)
#endif
    {}

    operator const Value*() const { return Valueify(array); }
    operator HeapSlot*() const { MOZ_ASSERT(allowWrite()); return array; }

    HeapSlotArray operator +(int offset) const { return HeapSlotArray(array + offset, allowWrite()); }
    HeapSlotArray operator +(uint32_t offset) const { return HeapSlotArray(array + offset, allowWrite()); }

  private:
    bool allowWrite() const {
#ifdef DEBUG
        return allowWrite_;
#else
        return true;
#endif
    }
};






template <typename T> struct Unbarriered {};
template <typename S> struct Unbarriered< PreBarriered<S> > { typedef S* type; };
template <typename S> struct Unbarriered< RelocatablePtr<S> > { typedef S* type; };
template <> struct Unbarriered<PreBarrieredValue> { typedef Value type; };
template <> struct Unbarriered<RelocatableValue> { typedef Value type; };
template <typename S> struct Unbarriered< DefaultHasher< PreBarriered<S> > > {
    typedef DefaultHasher<S*> type;
};

} 

#endif 
