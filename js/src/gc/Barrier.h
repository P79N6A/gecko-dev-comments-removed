





#ifndef gc_Barrier_h
#define gc_Barrier_h

#include "NamespaceImports.h"

#include "gc/Heap.h"
#include "gc/StoreBuffer.h"
#include "js/HashTable.h"
#include "js/Id.h"
#include "js/RootingAPI.h"






































































































namespace js {

class PropertyName;

namespace gc {

void
MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name);
}

JS::Zone *
ZoneOfObject(const JSObject &obj);

JS_ALWAYS_INLINE JS::Zone *
ZoneOfValue(const JS::Value &value)
{
    JS_ASSERT(value.isMarkable());
    if (value.isObject())
        return ZoneOfObject(value.toObject());
    return static_cast<js::gc::Cell *>(value.toGCThing())->tenuredZone();
}

template<class T, typename Unioned = uintptr_t>
class EncapsulatedPtr
{
  protected:
    union {
        T *value;
        Unioned other;
    };

  public:
    EncapsulatedPtr() : value(NULL) {}
    EncapsulatedPtr(T *v) : value(v) {}
    explicit EncapsulatedPtr(const EncapsulatedPtr<T> &v) : value(v.value) {}

    ~EncapsulatedPtr() { pre(); }

    void init(T *v) {
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        this->value = v;
    }

    
    void clear() {
        pre();
        value = NULL;
    }

    EncapsulatedPtr<T, Unioned> &operator=(T *v) {
        pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        value = v;
        return *this;
    }

    EncapsulatedPtr<T, Unioned> &operator=(const EncapsulatedPtr<T> &v) {
        pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v.value));
        value = v.value;
        return *this;
    }

    
    T *get() const { return value; }

    



    T **unsafeGet() { return &value; }
    void unsafeSet(T *v) { value = v; }

    Unioned *unsafeGetUnioned() { return &other; }

    T &operator*() const { return *value; }
    T *operator->() const { return value; }

    operator T*() const { return value; }

  protected:
    void pre();
};

template <class T, class Unioned = uintptr_t>
class HeapPtr : public EncapsulatedPtr<T, Unioned>
{
  public:
    HeapPtr() : EncapsulatedPtr<T>(NULL) {}
    explicit HeapPtr(T *v) : EncapsulatedPtr<T>(v) { post(); }
    explicit HeapPtr(const HeapPtr<T> &v)
      : EncapsulatedPtr<T>(v) { post(); }

    void init(T *v) {
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        this->value = v;
        post();
    }

    HeapPtr<T, Unioned> &operator=(T *v) {
        this->pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        this->value = v;
        post();
        return *this;
    }

    HeapPtr<T, Unioned> &operator=(const HeapPtr<T> &v) {
        this->pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v.value));
        this->value = v.value;
        post();
        return *this;
    }

  protected:
    void post() { T::writeBarrierPost(this->value, (void *)&this->value); }

    
    template<class T1, class T2>
    friend inline void
    BarrieredSetPair(Zone *zone,
                     HeapPtr<T1> &v1, T1 *val1,
                     HeapPtr<T2> &v2, T2 *val2);
};













template <class T>
class FixedHeapPtr
{
    T *value;

  public:
    operator T*() const { return value; }
    T * operator->() const { return value; }

    operator Handle<T*>() const {
        return Handle<T*>::fromMarkedLocation(&value);
    }

    void init(T *ptr) {
        value = ptr;
    }
};

template <class T>
class RelocatablePtr : public EncapsulatedPtr<T>
{
  public:
    RelocatablePtr() : EncapsulatedPtr<T>(NULL) {}
    explicit RelocatablePtr(T *v) : EncapsulatedPtr<T>(v) {
        if (v)
            post();
    }
    RelocatablePtr(const RelocatablePtr<T> &v) : EncapsulatedPtr<T>(v) {
        if (this->value)
            post();
    }

    ~RelocatablePtr() {
        if (this->value)
            relocate(this->value->runtimeFromMainThread());
    }

    RelocatablePtr<T> &operator=(T *v) {
        this->pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        if (v) {
            this->value = v;
            post();
        } else if (this->value) {
            JSRuntime *rt = this->value->runtimeFromMainThread();
            this->value = v;
            relocate(rt);
        }
        return *this;
    }

    RelocatablePtr<T> &operator=(const RelocatablePtr<T> &v) {
        this->pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v.value));
        if (v.value) {
            this->value = v.value;
            post();
        } else if (this->value) {
            JSRuntime *rt = this->value->runtimeFromMainThread();
            this->value = v;
            relocate(rt);
        }
        return *this;
    }

  protected:
    inline void post();
    inline void relocate(JSRuntime *rt);
};





template<class T1, class T2>
static inline void
BarrieredSetPair(Zone *zone,
                 HeapPtr<T1> &v1, T1 *val1,
                 HeapPtr<T2> &v2, T2 *val2)
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

class Shape;
class BaseShape;
namespace types { struct TypeObject; }

typedef EncapsulatedPtr<JSObject> EncapsulatedPtrObject;
typedef EncapsulatedPtr<JSScript> EncapsulatedPtrScript;

typedef RelocatablePtr<JSObject> RelocatablePtrObject;
typedef RelocatablePtr<JSScript> RelocatablePtrScript;

typedef HeapPtr<JSObject> HeapPtrObject;
typedef HeapPtr<JSFunction> HeapPtrFunction;
typedef HeapPtr<JSString> HeapPtrString;
typedef HeapPtr<PropertyName> HeapPtrPropertyName;
typedef HeapPtr<JSScript> HeapPtrScript;
typedef HeapPtr<Shape> HeapPtrShape;
typedef HeapPtr<BaseShape> HeapPtrBaseShape;
typedef HeapPtr<types::TypeObject> HeapPtrTypeObject;


template<class T>
struct HeapPtrHasher
{
    typedef HeapPtr<T> Key;
    typedef T *Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T *>::hash(obj); }
    static bool match(const Key &k, Lookup l) { return k.get() == l; }
    static void rekey(Key &k, const Key& newKey) { k.unsafeSet(newKey); }
};


template <class T>
struct DefaultHasher< HeapPtr<T> > : HeapPtrHasher<T> { };

template<class T>
struct EncapsulatedPtrHasher
{
    typedef EncapsulatedPtr<T> Key;
    typedef T *Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T *>::hash(obj); }
    static bool match(const Key &k, Lookup l) { return k.get() == l; }
    static void rekey(Key &k, const Key& newKey) { k.unsafeSet(newKey); }
};

template <class T>
struct DefaultHasher< EncapsulatedPtr<T> > : EncapsulatedPtrHasher<T> { };

class EncapsulatedValue : public ValueOperations<EncapsulatedValue>
{
  protected:
    Value value;

    



    EncapsulatedValue() MOZ_DELETE;

  public:
    EncapsulatedValue(const Value &v) : value(v) {
        JS_ASSERT(!IsPoisonedValue(v));
    }
    EncapsulatedValue(const EncapsulatedValue &v) : value(v) {
        JS_ASSERT(!IsPoisonedValue(v));
    }

    ~EncapsulatedValue() {
        pre();
    }

    void init(const Value &v) {
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
    }
    void init(JSRuntime *rt, const Value &v) {
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
    }

    EncapsulatedValue &operator=(const Value &v) {
        pre();
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        return *this;
    }

    EncapsulatedValue &operator=(const EncapsulatedValue &v) {
        pre();
        JS_ASSERT(!IsPoisonedValue(v));
        value = v.get();
        return *this;
    }

    bool operator==(const EncapsulatedValue &v) const { return value == v.value; }
    bool operator!=(const EncapsulatedValue &v) const { return value != v.value; }

    const Value &get() const { return value; }
    Value *unsafeGet() { return &value; }
    operator const Value &() const { return value; }

    JSGCTraceKind gcKind() const { return value.gcKind(); }

    uint64_t asRawBits() const { return value.asRawBits(); }

    static void writeBarrierPre(const Value &v) {
#ifdef JSGC_INCREMENTAL
        if (v.isMarkable() && shadowRuntimeFromAnyThread(v)->needsBarrier())
            writeBarrierPre(ZoneOfValue(v), v);
#endif
    }

    static void writeBarrierPre(Zone *zone, const Value &v) {
#ifdef JSGC_INCREMENTAL
        JS::shadow::Zone *shadowZone = JS::shadow::Zone::asShadowZone(zone);
        if (shadowZone->needsBarrier()) {
            JS_ASSERT_IF(v.isMarkable(), shadowRuntimeFromMainThread(v)->needsBarrier());
            Value tmp(v);
            js::gc::MarkValueUnbarriered(shadowZone->barrierTracer(), &tmp, "write barrier");
            JS_ASSERT(tmp == v);
        }
#endif
    }

  protected:
    void pre() { writeBarrierPre(value); }
    void pre(Zone *zone) { writeBarrierPre(zone, value); }

    static JSRuntime *runtimeFromMainThread(const Value &v) {
        JS_ASSERT(v.isMarkable());
        return static_cast<js::gc::Cell *>(v.toGCThing())->runtimeFromMainThread();
    }
    static JSRuntime *runtimeFromAnyThread(const Value &v) {
        JS_ASSERT(v.isMarkable());
        return static_cast<js::gc::Cell *>(v.toGCThing())->runtimeFromAnyThread();
    }
    static JS::shadow::Runtime *shadowRuntimeFromMainThread(const Value &v) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromMainThread(v));
    }
    static JS::shadow::Runtime *shadowRuntimeFromAnyThread(const Value &v) {
        return reinterpret_cast<JS::shadow::Runtime*>(runtimeFromAnyThread(v));
    }

  private:
    friend class ValueOperations<EncapsulatedValue>;
    const Value * extract() const { return &value; }
};

class HeapValue : public EncapsulatedValue
{
  public:
    explicit HeapValue()
      : EncapsulatedValue(UndefinedValue())
    {
        post();
    }

    explicit HeapValue(const Value &v)
      : EncapsulatedValue(v)
    {
        JS_ASSERT(!IsPoisonedValue(v));
        post();
    }

    explicit HeapValue(const HeapValue &v)
      : EncapsulatedValue(v.value)
    {
        JS_ASSERT(!IsPoisonedValue(v.value));
        post();
    }

    ~HeapValue() {
        pre();
    }

    void init(const Value &v) {
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        post();
    }

    void init(JSRuntime *rt, const Value &v) {
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        post(rt);
    }

    HeapValue &operator=(const Value &v) {
        pre();
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        post();
        return *this;
    }

    HeapValue &operator=(const HeapValue &v) {
        pre();
        JS_ASSERT(!IsPoisonedValue(v.value));
        value = v.value;
        post();
        return *this;
    }

#ifdef DEBUG
    bool preconditionForSet(Zone *zone);
#endif

    





    void set(Zone *zone, const Value &v) {
        JS::shadow::Zone *shadowZone = JS::shadow::Zone::asShadowZone(zone);
        JS_ASSERT(preconditionForSet(zone));
        pre(zone);
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        post(shadowZone->runtimeFromAnyThread());
    }

    static void writeBarrierPost(const Value &value, Value *addr) {
#ifdef JSGC_GENERATIONAL
        if (value.isMarkable())
            shadowRuntimeFromMainThread(value)->gcStoreBufferPtr()->putValue(addr);
#endif
    }

    static void writeBarrierPost(JSRuntime *rt, const Value &value, Value *addr) {
#ifdef JSGC_GENERATIONAL
        if (value.isMarkable()) {
            JS::shadow::Runtime *shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
            shadowRuntime->gcStoreBufferPtr()->putValue(addr);
        }
#endif
    }

  private:
    void post() {
        writeBarrierPost(value, &value);
    }

    void post(JSRuntime *rt) {
        writeBarrierPost(rt, value, &value);
    }
};

class RelocatableValue : public EncapsulatedValue
{
  public:
    explicit RelocatableValue()
      : EncapsulatedValue(UndefinedValue())
    {}

    explicit RelocatableValue(const Value &v)
      : EncapsulatedValue(v)
    {
        JS_ASSERT(!IsPoisonedValue(v));
        if (v.isMarkable())
            post();
    }

    RelocatableValue(const RelocatableValue &v)
      : EncapsulatedValue(v.value)
    {
        JS_ASSERT(!IsPoisonedValue(v.value));
        if (v.value.isMarkable())
            post();
    }

    ~RelocatableValue()
    {
        if (value.isMarkable())
            relocate(runtimeFromMainThread(value));
    }

    RelocatableValue &operator=(const Value &v) {
        pre();
        JS_ASSERT(!IsPoisonedValue(v));
        if (v.isMarkable()) {
            value = v;
            post();
        } else if (value.isMarkable()) {
            JSRuntime *rt = runtimeFromMainThread(value);
            relocate(rt);
            value = v;
        } else {
            value = v;
        }
        return *this;
    }

    RelocatableValue &operator=(const RelocatableValue &v) {
        pre();
        JS_ASSERT(!IsPoisonedValue(v.value));
        if (v.value.isMarkable()) {
            value = v.value;
            post();
        } else if (value.isMarkable()) {
            JSRuntime *rt = runtimeFromMainThread(value);
            relocate(rt);
            value = v.value;
        } else {
            value = v.value;
        }
        return *this;
    }

  private:
    void post() {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(value.isMarkable());
        shadowRuntimeFromMainThread(value)->gcStoreBufferPtr()->putRelocatableValue(&value);
#endif
    }

    void relocate(JSRuntime *rt) {
#ifdef JSGC_GENERATIONAL
        JS::shadow::Runtime *shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
        shadowRuntime->gcStoreBufferPtr()->removeRelocatableValue(&value);
#endif
    }
};

class HeapSlot : public EncapsulatedValue
{
    



    inline HeapSlot &operator=(const Value &v) MOZ_DELETE;
    inline HeapSlot &operator=(const HeapValue &v) MOZ_DELETE;
    inline HeapSlot &operator=(const HeapSlot &v) MOZ_DELETE;

  public:
    enum Kind {
        Slot,
        Element
    };

    explicit HeapSlot() MOZ_DELETE;

    explicit HeapSlot(JSObject *obj, Kind kind, uint32_t slot, const Value &v)
      : EncapsulatedValue(v)
    {
        JS_ASSERT(!IsPoisonedValue(v));
        post(obj, kind, slot, v);
    }

    explicit HeapSlot(JSObject *obj, Kind kind, uint32_t slot, const HeapSlot &s)
      : EncapsulatedValue(s.value)
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

    void init(JSRuntime *rt, JSObject *owner, Kind kind, uint32_t slot, const Value &v) {
        value = v;
        post(rt, owner, kind, slot, v);
    }

#ifdef DEBUG
    bool preconditionForSet(JSObject *owner, Kind kind, uint32_t slot);
    bool preconditionForSet(Zone *zone, JSObject *owner, Kind kind, uint32_t slot);
    static void preconditionForWriteBarrierPost(JSObject *obj, Kind kind, uint32_t slot,
                                                Value target);
#endif

    void set(JSObject *owner, Kind kind, uint32_t slot, const Value &v) {
        JS_ASSERT(preconditionForSet(owner, kind, slot));
        pre();
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        post(owner, kind, slot, v);
    }

    void set(Zone *zone, JSObject *owner, Kind kind, uint32_t slot, const Value &v) {
        JS_ASSERT(preconditionForSet(zone, owner, kind, slot));
        JS::shadow::Zone *shadowZone = JS::shadow::Zone::asShadowZone(zone);
        pre(zone);
        JS_ASSERT(!IsPoisonedValue(v));
        value = v;
        post(shadowZone->runtimeFromAnyThread(), owner, kind, slot, v);
    }

    static void writeBarrierPost(JSObject *obj, Kind kind, uint32_t slot, Value target)
    {
#ifdef JSGC_GENERATIONAL
        js::gc::Cell *cell = reinterpret_cast<js::gc::Cell*>(obj);
        writeBarrierPost(cell->runtimeFromAnyThread(), obj, kind, slot, target);
#endif
    }

    static void writeBarrierPost(JSRuntime *rt, JSObject *obj, Kind kind, uint32_t slot,
                                 Value target)
    {
#ifdef DEBUG
        preconditionForWriteBarrierPost(obj, kind, slot, target);
#endif
#ifdef JSGC_GENERATIONAL
        if (target.isObject()) {
            JS::shadow::Runtime *shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
            shadowRuntime->gcStoreBufferPtr()->putSlot(obj, kind, slot, &target.toObject());
        }
#endif
    }

  private:
    void post(JSObject *owner, Kind kind, uint32_t slot, Value target) {
        HeapSlot::writeBarrierPost(owner, kind, slot, target);
    }

    void post(JSRuntime *rt, JSObject *owner, Kind kind, uint32_t slot, Value target) {
        HeapSlot::writeBarrierPost(rt, owner, kind, slot, target);
    }
};







inline void
DenseRangeWriteBarrierPost(JSRuntime *rt, JSObject *obj, uint32_t start, uint32_t count);

static inline const Value *
Valueify(const EncapsulatedValue *array)
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

  public:
    HeapSlotArray(HeapSlot *array) : array(array) {}

    operator const Value *() const { return Valueify(array); }
    operator HeapSlot *() const { return array; }

    HeapSlotArray operator +(int offset) const { return HeapSlotArray(array + offset); }
    HeapSlotArray operator +(uint32_t offset) const { return HeapSlotArray(array + offset); }
};

class EncapsulatedId
{
  protected:
    jsid value;

  private:
    EncapsulatedId(const EncapsulatedId &v) MOZ_DELETE;

  public:
    explicit EncapsulatedId() : value(JSID_VOID) {}
    explicit EncapsulatedId(jsid id) : value(id) {}
    ~EncapsulatedId();

    inline EncapsulatedId &operator=(const EncapsulatedId &v);

    bool operator==(jsid id) const { return value == id; }
    bool operator!=(jsid id) const { return value != id; }

    jsid get() const { return value; }
    jsid *unsafeGet() { return &value; }
    void unsafeSet(jsid newId) { value = newId; }
    operator jsid() const { return value; }

  protected:
    inline void pre();
};

class RelocatableId : public EncapsulatedId
{
  public:
    explicit RelocatableId() : EncapsulatedId() {}
    explicit inline RelocatableId(jsid id) : EncapsulatedId(id) {}
    inline ~RelocatableId();

    inline RelocatableId &operator=(jsid id);
    inline RelocatableId &operator=(const RelocatableId &v);
};

class HeapId : public EncapsulatedId
{
  public:
    explicit HeapId() : EncapsulatedId() {}
    explicit inline HeapId(jsid id);
    inline ~HeapId();

    inline void init(jsid id);

    inline HeapId &operator=(jsid id);
    inline HeapId &operator=(const HeapId &v);

  private:
    inline void post();

    HeapId(const HeapId &v) MOZ_DELETE;
};











template<class T>
class ReadBarriered
{
    T *value;

  public:
    ReadBarriered() : value(NULL) {}
    ReadBarriered(T *value) : value(value) {}
    ReadBarriered(const Rooted<T*> &rooted) : value(rooted) {}

    T *get() const {
        if (!value)
            return NULL;
        T::readBarrier(value);
        return value;
    }

    operator T*() const { return get(); }

    T &operator*() const { return *get(); }
    T *operator->() const { return get(); }

    T **unsafeGet() { return &value; }
    T * const * unsafeGet() const { return &value; }

    void set(T *v) { value = v; }

    operator bool() { return !!value; }
};

class ReadBarrieredValue
{
    Value value;

  public:
    ReadBarrieredValue() : value(UndefinedValue()) {}
    ReadBarrieredValue(const Value &value) : value(value) {}

    inline const Value &get() const;
    Value *unsafeGet() { return &value; }
    inline operator const Value &() const;

    inline JSObject &toObject() const;
};

#ifdef DEBUG
bool
RuntimeFromMainThreadIsHeapMajorCollecting(JS::shadow::Zone *shadowZone);
#endif

} 

#endif 
