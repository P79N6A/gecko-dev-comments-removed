





#ifndef gc_Barrier_h
#define gc_Barrier_h

#include "NamespaceImports.h"

#include "gc/Heap.h"
#ifdef JSGC_GENERATIONAL
# include "gc/StoreBuffer.h"
#endif
#include "js/HashTable.h"
#include "js/Id.h"
#include "js/RootingAPI.h"






































































































































class JSAtom;
struct JSCompartment;
class JSFlatString;
class JSLinearString;

namespace JS {
class Symbol;
}

namespace js {

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
class NestedScopeObject;
class Nursery;
class ObjectImpl;
class PropertyName;
class SavedFrame;
class ScopeObject;
class ScriptSourceObject;
class Shape;
class UnownedBaseShape;

namespace types {
struct TypeObject;
}

namespace jit {
class JitCode;
}

#ifdef DEBUG


bool
CurrentThreadIsIonCompiling();
#endif

bool
StringIsPermanentAtom(JSString *str);

namespace gc {

template <typename T> struct MapTypeToTraceKind {};
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
template <> struct MapTypeToTraceKind<ObjectImpl>       { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<PropertyName>     { static const JSGCTraceKind kind = JSTRACE_STRING; };
template <> struct MapTypeToTraceKind<SavedFrame>       { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<ScopeObject>      { static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<Shape>            { static const JSGCTraceKind kind = JSTRACE_SHAPE; };
template <> struct MapTypeToTraceKind<SharedArrayBufferObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<SharedTypedArrayObject>{ static const JSGCTraceKind kind = JSTRACE_OBJECT; };
template <> struct MapTypeToTraceKind<UnownedBaseShape> { static const JSGCTraceKind kind = JSTRACE_BASE_SHAPE; };
template <> struct MapTypeToTraceKind<jit::JitCode>     { static const JSGCTraceKind kind = JSTRACE_JITCODE; };
template <> struct MapTypeToTraceKind<types::TypeObject>{ static const JSGCTraceKind kind = JSTRACE_TYPE_OBJECT; };


void
MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name);



void
MarkStringUnbarriered(JSTracer *trc, JSString **str, const char *name);
void
MarkSymbolUnbarriered(JSTracer *trc, JS::Symbol **sym, const char *name);

} 



static inline const gc::TenuredCell *AsTenuredCell(const JSString *str) {
    return reinterpret_cast<const gc::TenuredCell *>(str);
}
static inline const gc::TenuredCell *AsTenuredCell(const JS::Symbol *sym) {
    return reinterpret_cast<const gc::TenuredCell *>(sym);
}

JS::Zone *
ZoneOfObjectFromAnyThread(const JSObject &obj);

static inline JS::shadow::Zone *
ShadowZoneOfObjectFromAnyThread(JSObject *obj)
{
    return JS::shadow::Zone::asShadowZone(ZoneOfObjectFromAnyThread(*obj));
}

static inline JS::shadow::Zone *
ShadowZoneOfStringFromAnyThread(JSString *str)
{
    return JS::shadow::Zone::asShadowZone(AsTenuredCell(str)->zoneFromAnyThread());
}

static inline JS::shadow::Zone *
ShadowZoneOfSymbolFromAnyThread(JS::Symbol *sym)
{
    return JS::shadow::Zone::asShadowZone(AsTenuredCell(sym)->zoneFromAnyThread());
}

MOZ_ALWAYS_INLINE JS::Zone *
ZoneOfValueFromAnyThread(const JS::Value &value)
{
    JS_ASSERT(value.isMarkable());
    if (value.isObject())
        return ZoneOfObjectFromAnyThread(value.toObject());
    return js::gc::TenuredCell::fromPointer(value.toGCThing())->zoneFromAnyThread();
}

void
ValueReadBarrier(const Value &value);

template <typename T>
struct InternalGCMethods {};

template <typename T>
struct InternalGCMethods<T *>
{
    static bool isMarkable(T *v) { return v != nullptr; }

    static void preBarrier(T *v) { T::writeBarrierPre(v); }
    static void preBarrier(Zone *zone, T *v) { T::writeBarrierPre(zone, v); }

    static void postBarrier(T **vp) { T::writeBarrierPost(*vp, vp); }
    static void postBarrierRelocate(T **vp) { T::writeBarrierPostRelocate(*vp, vp); }
    static void postBarrierRemove(T **vp) { T::writeBarrierPostRemove(*vp, vp); }

    static void readBarrier(T *v) { T::readBarrier(v); }
};

template <>
struct InternalGCMethods<Value>
{
    static JSRuntime *runtimeFromAnyThread(const Value &v) {
        JS_ASSERT(v.isMarkable());
        return static_cast<js::gc::Cell *>(v.toGCThing())->runtimeFromAnyThread();
    }
    static JS::shadow::Runtime *shadowRuntimeFromAnyThread(const Value &v) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromAnyThread(v));
    }
    static JSRuntime *runtimeFromMainThread(const Value &v) {
        JS_ASSERT(v.isMarkable());
        return static_cast<js::gc::Cell *>(v.toGCThing())->runtimeFromMainThread();
    }
    static JS::shadow::Runtime *shadowRuntimeFromMainThread(const Value &v) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromMainThread(v));
    }

    static bool isMarkable(Value v) { return v.isMarkable(); }

    static void preBarrier(Value v) {
#ifdef JSGC_INCREMENTAL
        JS_ASSERT(!CurrentThreadIsIonCompiling());
        if (v.isMarkable() && shadowRuntimeFromAnyThread(v)->needsIncrementalBarrier())
            preBarrier(ZoneOfValueFromAnyThread(v), v);
#endif
    }

    static void preBarrier(Zone *zone, Value v) {
#ifdef JSGC_INCREMENTAL
        JS_ASSERT(!CurrentThreadIsIonCompiling());
        if (v.isString() && StringIsPermanentAtom(v.toString()))
            return;
        JS::shadow::Zone *shadowZone = JS::shadow::Zone::asShadowZone(zone);
        if (shadowZone->needsIncrementalBarrier()) {
            JS_ASSERT_IF(v.isMarkable(), shadowRuntimeFromMainThread(v)->needsIncrementalBarrier());
            Value tmp(v);
            js::gc::MarkValueUnbarriered(shadowZone->barrierTracer(), &tmp, "write barrier");
            JS_ASSERT(tmp == v);
        }
#endif
    }

    static void postBarrier(Value *vp) {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(!CurrentThreadIsIonCompiling());
        if (vp->isObject()) {
            gc::StoreBuffer *sb = reinterpret_cast<gc::Cell *>(&vp->toObject())->storeBuffer();
            if (sb)
                sb->putValueFromAnyThread(vp);
        }
#endif
    }

    static void postBarrierRelocate(Value *vp) {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(!CurrentThreadIsIonCompiling());
        if (vp->isObject()) {
            gc::StoreBuffer *sb = reinterpret_cast<gc::Cell *>(&vp->toObject())->storeBuffer();
            if (sb)
                sb->putRelocatableValueFromAnyThread(vp);
        }
#endif
    }

    static void postBarrierRemove(Value *vp) {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(vp);
        JS_ASSERT(vp->isMarkable());
        JS_ASSERT(!CurrentThreadIsIonCompiling());
        JSRuntime *rt = static_cast<js::gc::Cell *>(vp->toGCThing())->runtimeFromAnyThread();
        JS::shadow::Runtime *shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
        shadowRuntime->gcStoreBufferPtr()->removeRelocatableValueFromAnyThread(vp);
#endif
    }

    static void readBarrier(const Value &v) { ValueReadBarrier(v); }
};

template <>
struct InternalGCMethods<jsid>
{
    static bool isMarkable(jsid id) { return JSID_IS_STRING(id) || JSID_IS_SYMBOL(id); }

    static void preBarrier(jsid id) {
#ifdef JSGC_INCREMENTAL
        if (JSID_IS_STRING(id)) {
            JSString *str = JSID_TO_STRING(id);
            JS::shadow::Zone *shadowZone = ShadowZoneOfStringFromAnyThread(str);
            if (shadowZone->needsIncrementalBarrier()) {
                js::gc::MarkStringUnbarriered(shadowZone->barrierTracer(), &str, "write barrier");
                JS_ASSERT(str == JSID_TO_STRING(id));
            }
        } else if (JSID_IS_SYMBOL(id)) {
            JS::Symbol *sym = JSID_TO_SYMBOL(id);
            JS::shadow::Zone *shadowZone = ShadowZoneOfSymbolFromAnyThread(sym);
            if (shadowZone->needsIncrementalBarrier()) {
                js::gc::MarkSymbolUnbarriered(shadowZone->barrierTracer(), &sym, "write barrier");
                JS_ASSERT(sym == JSID_TO_SYMBOL(id));
            }
        }
#endif
    }
    static void preBarrier(Zone *zone, jsid id) { preBarrier(id); }

    static void postBarrier(jsid *idp) {}
    static void postBarrierRelocate(jsid *idp) {}
    static void postBarrierRemove(jsid *idp) {}
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
        JS_ASSERT(!GCMethods<T>::poisoned(v));
        this->value = v;
    }

    bool operator==(const T &other) const { return value == other; }
    bool operator!=(const T &other) const { return value != other; }

    
    const T &get() const { return value; }

    



    T *unsafeGet() { return &value; }
    const T *unsafeGet() const { return &value; }
    void unsafeSet(T v) { value = v; }

    T operator->() const { return value; }

    operator const T &() const { return value; }

    
    static void writeBarrierPre(const T &v) { InternalGCMethods<T>::preBarrier(v); }
    static void writeBarrierPost(const T &v, T *vp) { InternalGCMethods<T>::postBarrier(vp); }

  protected:
    void pre() { InternalGCMethods<T>::preBarrier(value); }
    void pre(Zone *zone) { InternalGCMethods<T>::preBarrier(zone, value); }
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
    explicit PreBarriered(const PreBarriered<T> &v)
      : BarrieredBase<T>(v.value) {}

    
    void clear() {
        this->pre();
        this->value = nullptr;
    }

    PreBarriered<T> &operator=(T v) {
        this->pre();
        JS_ASSERT(!GCMethods<T>::poisoned(v));
        this->value = v;
        return *this;
    }

    PreBarriered<T> &operator=(const PreBarriered<T> &v) {
        this->pre();
        JS_ASSERT(!GCMethods<T>::poisoned(v.value));
        this->value = v.value;
        return *this;
    }
};













template <class T>
class HeapPtr : public BarrieredBase<T>
{
  public:
    HeapPtr() : BarrieredBase<T>(GCMethods<T>::initial()) {}
    explicit HeapPtr(T v) : BarrieredBase<T>(v) { post(); }
    explicit HeapPtr(const HeapPtr<T> &v) : BarrieredBase<T>(v) { post(); }

    void init(T v) {
        JS_ASSERT(!GCMethods<T>::poisoned(v));
        this->value = v;
        post();
    }

    HeapPtr<T> &operator=(T v) {
        this->pre();
        JS_ASSERT(!GCMethods<T>::poisoned(v));
        this->value = v;
        post();
        return *this;
    }

    HeapPtr<T> &operator=(const HeapPtr<T> &v) {
        this->pre();
        JS_ASSERT(!GCMethods<T>::poisoned(v.value));
        this->value = v.value;
        post();
        return *this;
    }

  protected:
    void post() { InternalGCMethods<T>::postBarrier(&this->value); }

    
    template <class T1, class T2>
    friend inline void
    BarrieredSetPair(Zone *zone,
                     HeapPtr<T1*> &v1, T1 *val1,
                     HeapPtr<T2*> &v2, T2 *val2);

  private:
    






    HeapPtr(HeapPtr<T> &&) MOZ_DELETE;
    HeapPtr<T> &operator=(HeapPtr<T> &&) MOZ_DELETE;
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
        JS_ASSERT(ptr->isTenured());
        value = ptr;
    }

    const T * address() { return &value; }
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

    





    RelocatablePtr(const RelocatablePtr<T> &v) : BarrieredBase<T>(v) {
        if (GCMethods<T>::needsPostBarrier(this->value))
            post();
    }

    ~RelocatablePtr() {
        if (GCMethods<T>::needsPostBarrier(this->value))
            relocate();
    }

    RelocatablePtr<T> &operator=(T v) {
        this->pre();
        JS_ASSERT(!GCMethods<T>::poisoned(v));
        if (GCMethods<T>::needsPostBarrier(v)) {
            this->value = v;
            post();
        } else if (GCMethods<T>::needsPostBarrier(this->value)) {
            relocate();
            this->value = v;
        } else {
            this->value = v;
        }
        return *this;
    }

    RelocatablePtr<T> &operator=(const RelocatablePtr<T> &v) {
        this->pre();
        JS_ASSERT(!GCMethods<T>::poisoned(v.value));
        if (GCMethods<T>::needsPostBarrier(v.value)) {
            this->value = v.value;
            post();
        } else if (GCMethods<T>::needsPostBarrier(this->value)) {
            relocate();
            this->value = v;
        } else {
            this->value = v;
        }

        return *this;
    }

  protected:
    void post() {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(GCMethods<T>::needsPostBarrier(this->value));
        InternalGCMethods<T>::postBarrierRelocate(&this->value);
#endif
    }

    void relocate() {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(GCMethods<T>::needsPostBarrier(this->value));
        InternalGCMethods<T>::postBarrierRemove(&this->value);
#endif
    }
};





template <class T1, class T2>
static inline void
BarrieredSetPair(Zone *zone,
                 HeapPtr<T1*> &v1, T1 *val1,
                 HeapPtr<T2*> &v2, T2 *val2)
{
    if (T1::needWriteBarrierPre(zone)) {
        v1.pre();
        v2.pre();
    }
    v1.unsafeSet(val1);
    v2.unsafeSet(val2);
    v1.post();
    v2.post();
}


template <class T>
struct HeapPtrHasher
{
    typedef HeapPtr<T> Key;
    typedef T Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T>::hash(obj); }
    static bool match(const Key &k, Lookup l) { return k.get() == l; }
    static void rekey(Key &k, const Key& newKey) { k.unsafeSet(newKey); }
};


template <class T>
struct DefaultHasher< HeapPtr<T> > : HeapPtrHasher<T> { };

template <class T>
struct PreBarrieredHasher
{
    typedef PreBarriered<T> Key;
    typedef T Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T>::hash(obj); }
    static bool match(const Key &k, Lookup l) { return k.get() == l; }
    static void rekey(Key &k, const Key& newKey) { k.unsafeSet(newKey); }
};

template <class T>
struct DefaultHasher< PreBarriered<T> > : PreBarrieredHasher<T> { };











template <class T>
class ReadBarriered
{
    T value;

  public:
    ReadBarriered() : value(nullptr) {}
    explicit ReadBarriered(T value) : value(value) {}
    explicit ReadBarriered(const Rooted<T> &rooted) : value(rooted) {}

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

    T &operator*() const { return *get(); }
    T operator->() const { return get(); }

    T *unsafeGet() { return &value; }
    T const * unsafeGet() const { return &value; }

    void set(T v) { value = v; }
};

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
namespace types {
struct TypeObject;
class TypeNewScript;
}

typedef PreBarriered<JSObject*> PreBarrieredObject;
typedef PreBarriered<JSScript*> PreBarrieredScript;
typedef PreBarriered<jit::JitCode*> PreBarrieredJitCode;
typedef PreBarriered<JSAtom*> PreBarrieredAtom;

typedef RelocatablePtr<JSObject*> RelocatablePtrObject;
typedef RelocatablePtr<JSScript*> RelocatablePtrScript;
typedef RelocatablePtr<NestedScopeObject*> RelocatablePtrNestedScopeObject;

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
typedef HeapPtr<PropertyName*> HeapPtrPropertyName;
typedef HeapPtr<Shape*> HeapPtrShape;
typedef HeapPtr<UnownedBaseShape*> HeapPtrUnownedBaseShape;
typedef HeapPtr<jit::JitCode*> HeapPtrJitCode;
typedef HeapPtr<types::TypeObject*> HeapPtrTypeObject;
typedef HeapPtr<types::TypeNewScript*> HeapPtrTypeNewScript;

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
typedef ReadBarriered<types::TypeObject*> ReadBarrieredTypeObject;
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

    explicit HeapSlot() MOZ_DELETE;

    explicit HeapSlot(JSObject *obj, Kind kind, uint32_t slot, const Value &v)
      : BarrieredBase<Value>(v)
    {
        JS_ASSERT(!IsPoisonedValue(v));
        post(obj, kind, slot, v);
    }

    explicit HeapSlot(JSObject *obj, Kind kind, uint32_t slot, const HeapSlot &s)
      : BarrieredBase<Value>(s.value)
    {
        JS_ASSERT(!IsPoisonedValue(s.value));
        post(obj, kind, slot, s);
    }

    ~HeapSlot() {
        pre();
    }

    void init(JSObject *owner, Kind kind, uint32_t slot, const Value &v) {
        value = v;
        post(owner, kind, slot, v);
    }

#ifdef DEBUG
    bool preconditionForSet(JSObject *owner, Kind kind, uint32_t slot);
    bool preconditionForSet(Zone *zone, JSObject *owner, Kind kind, uint32_t slot);
    bool preconditionForWriteBarrierPost(JSObject *obj, Kind kind, uint32_t slot, Value target) const;
#endif

    void set(JSObject *owner, Kind kind, uint32_t slot, const Value &v) {
        JS_ASSERT(preconditionForSet(owner, kind, slot));
        JS_ASSERT(!IsPoisonedValue(v));
        pre();
        value = v;
        post(owner, kind, slot, v);
    }

    void set(Zone *zone, JSObject *owner, Kind kind, uint32_t slot, const Value &v) {
        JS_ASSERT(preconditionForSet(zone, owner, kind, slot));
        JS_ASSERT(!IsPoisonedValue(v));
        pre(zone);
        value = v;
        post(owner, kind, slot, v);
    }

    
    static void writeBarrierPost(JSObject *owner, Kind kind, uint32_t slot, const Value &target) {
        reinterpret_cast<HeapSlot *>(const_cast<Value *>(&target))->post(owner, kind, slot, target);
    }

  private:
    void post(JSObject *owner, Kind kind, uint32_t slot, const Value &target) {
        JS_ASSERT(preconditionForWriteBarrierPost(owner, kind, slot, target));
#ifdef JSGC_GENERATIONAL
        if (this->value.isObject()) {
            gc::Cell *cell = reinterpret_cast<gc::Cell *>(&this->value.toObject());
            if (cell->storeBuffer())
                cell->storeBuffer()->putSlotFromAnyThread(owner, kind, slot, 1);
        }
#endif
    }
};

static inline const Value *
Valueify(const BarrieredBase<Value> *array)
{
    JS_STATIC_ASSERT(sizeof(HeapValue) == sizeof(Value));
    JS_STATIC_ASSERT(sizeof(HeapSlot) == sizeof(Value));
    return (const Value *)array;
}

static inline HeapValue *
HeapValueify(Value *v)
{
    JS_STATIC_ASSERT(sizeof(HeapValue) == sizeof(Value));
    JS_STATIC_ASSERT(sizeof(HeapSlot) == sizeof(Value));
    return (HeapValue *)v;
}

class HeapSlotArray
{
    HeapSlot *array;

    
    
#ifdef DEBUG
    bool allowWrite_;
#endif

  public:
    explicit HeapSlotArray(HeapSlot *array, bool allowWrite)
      : array(array)
#ifdef DEBUG
      , allowWrite_(allowWrite)
#endif
    {}

    operator const Value *() const { return Valueify(array); }
    operator HeapSlot *() const { JS_ASSERT(allowWrite()); return array; }

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
template <typename S> struct Unbarriered< PreBarriered<S> > { typedef S *type; };
template <typename S> struct Unbarriered< RelocatablePtr<S> > { typedef S *type; };
template <> struct Unbarriered<PreBarrieredValue> { typedef Value type; };
template <> struct Unbarriered<RelocatableValue> { typedef Value type; };
template <typename S> struct Unbarriered< DefaultHasher< PreBarriered<S> > > {
    typedef DefaultHasher<S *> type;
};

} 

#endif 
