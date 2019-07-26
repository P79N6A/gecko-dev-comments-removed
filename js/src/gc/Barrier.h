






































#ifndef jsgc_barrier_h___
#define jsgc_barrier_h___

#include "jsapi.h"

#include "gc/Heap.h"
#include "js/HashTable.h"






































































































struct JSXML;

namespace js {

template<class T, typename Unioned = uintptr_t>
class HeapPtr
{
    union {
        T *value;
        Unioned other;
    };

  public:
    HeapPtr() : value(NULL) {}
    explicit HeapPtr(T *v) : value(v) { post(); }
    explicit HeapPtr(const HeapPtr<T> &v) : value(v.value) { post(); }

    ~HeapPtr() { pre(); }

    
    void init(T *v) {
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        value = v;
        post();
    }

    
    void clear() {
	pre();
	value = NULL;
    }

    
    T *get() const { return value; }

    



    T **unsafeGet() { return &value; }
    void unsafeSet(T *v) { value = v; }

    Unioned *unsafeGetUnioned() { return &other; }

    HeapPtr<T, Unioned> &operator=(T *v) {
        pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v));
        value = v;
        post();
        return *this;
    }

    HeapPtr<T, Unioned> &operator=(const HeapPtr<T> &v) {
        pre();
        JS_ASSERT(!IsPoisonedPtr<T>(v.value));
        value = v.value;
        post();
        return *this;
    }

    T &operator*() const { return *value; }
    T *operator->() const { return value; }

    operator T*() const { return value; }

  private:
    void pre() { T::writeBarrierPre(value); }
    void post() { T::writeBarrierPost(value, (void *)&value); }

    
    template<class T1, class T2>
    friend inline void
    BarrieredSetPair(JSCompartment *comp,
                     HeapPtr<T1> &v1, T1 *val1,
                     HeapPtr<T2> &v2, T2 *val2);
};





template<class T1, class T2>
static inline void
BarrieredSetPair(JSCompartment *comp,
                 HeapPtr<T1> &v1, T1 *val1,
                 HeapPtr<T2> &v2, T2 *val2)
{
    if (T1::needWriteBarrierPre(comp)) {
        v1.pre();
        v2.pre();
    }
    v1.unsafeSet(val1);
    v2.unsafeSet(val2);
    v1.post();
    v2.post();
}

struct Shape;
class BaseShape;
namespace types { struct TypeObject; }

typedef HeapPtr<JSObject> HeapPtrObject;
typedef HeapPtr<JSFunction> HeapPtrFunction;
typedef HeapPtr<JSString> HeapPtrString;
typedef HeapPtr<JSScript> HeapPtrScript;
typedef HeapPtr<Shape> HeapPtrShape;
typedef HeapPtr<BaseShape> HeapPtrBaseShape;
typedef HeapPtr<types::TypeObject> HeapPtrTypeObject;
typedef HeapPtr<JSXML> HeapPtrXML;


template<class T>
struct HeapPtrHasher
{
    typedef HeapPtr<T> Key;
    typedef T *Lookup;

    static HashNumber hash(Lookup obj) { return DefaultHasher<T *>::hash(obj); }
    static bool match(const Key &k, Lookup l) { return k.get() == l; }
};


template <class T>
struct DefaultHasher< HeapPtr<T> >: HeapPtrHasher<T> { };

class EncapsulatedValue
{
  protected:
    Value value;

    



    EncapsulatedValue() MOZ_DELETE;
    EncapsulatedValue(const EncapsulatedValue &v) MOZ_DELETE;
    EncapsulatedValue &operator=(const Value &v) MOZ_DELETE;
    EncapsulatedValue &operator=(const EncapsulatedValue &v) MOZ_DELETE;

    EncapsulatedValue(const Value &v) : value(v) {}
    ~EncapsulatedValue() {}

  public:
    inline bool operator==(const EncapsulatedValue &v) const { return value == v.value; }
    inline bool operator!=(const EncapsulatedValue &v) const { return value != v.value; }

    const Value &get() const { return value; }
    Value *unsafeGet() { return &value; }
    operator const Value &() const { return value; }

    bool isUndefined() const { return value.isUndefined(); }
    bool isNull() const { return value.isNull(); }
    bool isBoolean() const { return value.isBoolean(); }
    bool isTrue() const { return value.isTrue(); }
    bool isFalse() const { return value.isFalse(); }
    bool isNumber() const { return value.isNumber(); }
    bool isInt32() const { return value.isInt32(); }
    bool isDouble() const { return value.isDouble(); }
    bool isString() const { return value.isString(); }
    bool isObject() const { return value.isObject(); }
    bool isMagic(JSWhyMagic why) const { return value.isMagic(why); }
    bool isGCThing() const { return value.isGCThing(); }
    bool isMarkable() const { return value.isMarkable(); }

    bool toBoolean() const { return value.toBoolean(); }
    double toNumber() const { return value.toNumber(); }
    int32_t toInt32() const { return value.toInt32(); }
    double toDouble() const { return value.toDouble(); }
    JSString *toString() const { return value.toString(); }
    JSObject &toObject() const { return value.toObject(); }
    JSObject *toObjectOrNull() const { return value.toObjectOrNull(); }
    void *toGCThing() const { return value.toGCThing(); }

    JSGCTraceKind gcKind() const { return value.gcKind(); }

    uint64_t asRawBits() const { return value.asRawBits(); }

#ifdef DEBUG
    JSWhyMagic whyMagic() const { return value.whyMagic(); }
#endif

    static inline void writeBarrierPre(const Value &v);
    static inline void writeBarrierPre(JSCompartment *comp, const Value &v);

  protected:
    inline void pre();
    inline void pre(JSCompartment *comp);
};

class HeapValue : public EncapsulatedValue
{
  public:
    explicit inline HeapValue();
    explicit inline HeapValue(const Value &v);
    explicit inline HeapValue(const HeapValue &v);
    inline ~HeapValue();

    inline void init(const Value &v);
    inline void init(JSCompartment *comp, const Value &v);

    inline HeapValue &operator=(const Value &v);
    inline HeapValue &operator=(const HeapValue &v);

    





    inline void set(JSCompartment *comp, const Value &v);

    static inline void writeBarrierPost(const Value &v, void *addr);
    static inline void writeBarrierPost(JSCompartment *comp, const Value &v, void *addr);

  private:
    inline void post();
    inline void post(JSCompartment *comp);
};

class RelocatableValue : public EncapsulatedValue
{
  public:
    explicit inline RelocatableValue();
    explicit inline RelocatableValue(const Value &v);
    explicit inline RelocatableValue(const RelocatableValue &v);
    inline ~RelocatableValue();

    inline RelocatableValue &operator=(const Value &v);
    inline RelocatableValue &operator=(const RelocatableValue &v);
};

class HeapSlot : public EncapsulatedValue
{
    



    inline HeapSlot &operator=(const Value &v) MOZ_DELETE;
    inline HeapSlot &operator=(const HeapValue &v) MOZ_DELETE;
    inline HeapSlot &operator=(const HeapSlot &v) MOZ_DELETE;

  public:
    explicit inline HeapSlot() MOZ_DELETE;
    explicit inline HeapSlot(JSObject *obj, uint32_t slot, const Value &v);
    explicit inline HeapSlot(JSObject *obj, uint32_t slot, const HeapSlot &v);
    inline ~HeapSlot();

    inline void init(JSObject *owner, uint32_t slot, const Value &v);
    inline void init(JSCompartment *comp, JSObject *owner, uint32_t slot, const Value &v);

    inline void set(JSObject *owner, uint32_t slot, const Value &v);
    inline void set(JSCompartment *comp, JSObject *owner, uint32_t slot, const Value &v);

    static inline void writeBarrierPost(JSObject *obj, uint32_t slot);
    static inline void writeBarrierPost(JSCompartment *comp, JSObject *obj, uint32_t slotno);

  private:
    inline void post(JSObject *owner, uint32_t slot);
    inline void post(JSCompartment *comp, JSObject *owner, uint32_t slot);
};







static inline void
SlotRangeWriteBarrierPost(JSCompartment *comp, JSObject *obj, uint32_t start, uint32_t count)
{
}

static inline const Value *
Valueify(const EncapsulatedValue *array)
{
    JS_STATIC_ASSERT(sizeof(HeapValue) == sizeof(Value));
    JS_STATIC_ASSERT(sizeof(HeapSlot) == sizeof(Value));
    return (const Value *)array;
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

    explicit EncapsulatedId() : value(JSID_VOID) {}
    explicit inline EncapsulatedId(jsid id) : value(id) {}
    ~EncapsulatedId() {}

  private:
    EncapsulatedId(const EncapsulatedId &v) MOZ_DELETE;
    EncapsulatedId &operator=(const EncapsulatedId &v) MOZ_DELETE;

  public:
    bool operator==(jsid id) const { return value == id; }
    bool operator!=(jsid id) const { return value != id; }

    jsid get() const { return value; }
    jsid *unsafeGet() { return &value; }
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

    T *get() const {
        if (!value)
            return NULL;
        T::readBarrier(value);
        return value;
    }

    operator T*() const { return get(); }

    T &operator*() const { return *get(); }
    T *operator->() const { return get(); }

    T *unsafeGet() { return value; }
    T **unsafeGetAddress() { return &value; }

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
    inline operator const Value &() const;

    inline JSObject &toObject() const;
};

namespace tl {

template <class T> struct IsPostBarrieredType<HeapPtr<T> > {
                                                    static const bool result = true; };
template <> struct IsPostBarrieredType<HeapSlot>  { static const bool result = true; };
template <> struct IsPostBarrieredType<HeapValue> { static const bool result = true; };
template <> struct IsPostBarrieredType<HeapId>    { static const bool result = true; };

} 
} 

#endif 
