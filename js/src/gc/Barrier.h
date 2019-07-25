






































#ifndef jsgc_barrier_h___
#define jsgc_barrier_h___

#include "jsapi.h"
#include "jscell.h"

#include "js/HashTable.h"








































































































namespace js {











template<class T>
class MarkablePtr
{
  public:
    T *value;

    explicit MarkablePtr(T *value) : value(value) {}
};

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
        value = v;
        post();
        return *this;
    }

    HeapPtr<T, Unioned> &operator=(const HeapPtr<T> &v) {
        pre();
        value = v.value;
        post();
        return *this;
    }

    T &operator*() const { return *value; }
    T *operator->() const { return value; }

    operator T*() const { return value; }

    



    template<class U>
    operator MarkablePtr<U>() const { return MarkablePtr<U>(value); }

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

typedef HeapPtr<JSObject> HeapPtrObject;
typedef HeapPtr<JSFunction> HeapPtrFunction;
typedef HeapPtr<JSString> HeapPtrString;
typedef HeapPtr<JSScript> HeapPtrScript;
typedef HeapPtr<Shape> HeapPtrShape;
typedef HeapPtr<BaseShape> HeapPtrBaseShape;
typedef HeapPtr<const Shape> HeapPtrConstShape;
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

class HeapValue
{
    Value value;

  public:
    explicit HeapValue() : value(UndefinedValue()) {}
    explicit inline HeapValue(const Value &v);
    explicit inline HeapValue(const HeapValue &v);

    inline ~HeapValue();

    inline void init(const Value &v);

    inline HeapValue &operator=(const Value &v);
    inline HeapValue &operator=(const HeapValue &v);

    





    inline void set(JSCompartment *comp, const Value &v);

    const Value &get() const { return value; }
    operator const Value &() const { return value; }

    bool isMarkable() const { return value.isMarkable(); }
    bool isMagic(JSWhyMagic why) const { return value.isMagic(why); }
    bool isUndefined() const { return value.isUndefined(); }
    bool isObject() const { return value.isObject(); }
    bool isGCThing() const { return value.isGCThing(); }
    bool isTrue() const { return value.isTrue(); }
    bool isFalse() const { return value.isFalse(); }
    bool isInt32() const { return value.isInt32(); }
    bool isNull() const { return value.isNull(); }

    JSObject &toObject() const { return value.toObject(); }
    JSObject *toObjectOrNull() const { return value.toObjectOrNull(); }
    void *toGCThing() const { return value.toGCThing(); }
    double toDouble() const { return value.toDouble(); }
    int32 toInt32() const { return value.toInt32(); }
    JSString *toString() const { return value.toString(); }
    bool toBoolean() const { return value.toBoolean(); }
    double toNumber() const { return value.toNumber(); }

    JSGCTraceKind gcKind() const { return value.gcKind(); }

    inline void boxNonDoubleFrom(JSValueType type, uint64 *out);

    uint64 asRawBits() const { return value.asRawBits(); }

#ifdef DEBUG
    JSWhyMagic whyMagic() const { return value.whyMagic(); }
#endif

    static inline void writeBarrierPre(const Value &v);
    static inline void writeBarrierPost(const Value &v, void *addr);

    static inline void writeBarrierPre(JSCompartment *comp, const Value &v);
    static inline void writeBarrierPost(JSCompartment *comp, const Value &v, void *addr);

  private:
    inline void pre();
    inline void post();

    inline void pre(JSCompartment *comp);
    inline void post(JSCompartment *comp);
};

static inline const Value *
Valueify(const HeapValue *array)
{
    JS_ASSERT(sizeof(HeapValue) == sizeof(Value));
    return (const Value *)array;
}

class HeapValueArray
{
    HeapValue *array;

  public:
    HeapValueArray(HeapValue *array) : array(array) {}

    operator const Value *() const { return Valueify(array); }
    operator HeapValue *() const { return array; }

    HeapValueArray operator +(int offset) const { return HeapValueArray(array + offset); }
    HeapValueArray operator +(uint32 offset) const { return HeapValueArray(array + offset); }
};

class HeapId
{
    jsid value;

  public:
    explicit HeapId() : value(JSID_VOID) {}
    explicit inline HeapId(jsid id);

    inline ~HeapId();

    inline void init(jsid id);

    inline HeapId &operator=(jsid id);
    inline HeapId &operator=(const HeapId &v);

    bool operator==(jsid id) const { return value == id; }
    bool operator!=(jsid id) const { return value != id; }

    jsid get() const { return value; }
    operator jsid() const { return value; }

  private:
    inline void pre();
    inline void post();

    HeapId(const HeapId &v);
};











template<class T>
class ReadBarriered
{
    T *value;

  public:
    ReadBarriered(T *value) : value(value) {}

    T *get() const {
        if (!value)
            return NULL;
        T::readBarrier(value);
        return value;
    }

    operator T*() const { return get(); }

    T *unsafeGet() { return value; }

    void set(T *v) { value = v; }

    operator bool() { return !!value; }

    template<class U>
    operator MarkablePtr<U>() const { return MarkablePtr<U>(value); }
};

}

#endif 
