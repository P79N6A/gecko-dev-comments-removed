





#ifndef gc_Barrier_inl_h
#define gc_Barrier_inl_h

#include "gc/Barrier.h"

#include "jscompartment.h"

#include "gc/Marking.h"
#include "gc/StoreBuffer.h"

#include "vm/String-inl.h"

namespace js {

#ifdef JSGC_GENERATIONAL
class DenseRangeRef : public gc::BufferableRef
{
    JSObject *owner;
    uint32_t start;
    uint32_t end;

  public:
    DenseRangeRef(JSObject *obj, uint32_t start, uint32_t end)
      : owner(obj), start(start), end(end)
    {
        JS_ASSERT(start < end);
    }

    void mark(JSTracer *trc) {
        
        IsObjectMarked(&owner);
        uint32_t initLen = owner->getDenseInitializedLength();
        uint32_t clampedStart = Min(start, initLen);
        gc::MarkArraySlots(trc, Min(end, initLen) - clampedStart,
                           owner->getDenseElements() + clampedStart, "element");
    }
};
#endif

inline void
DenseRangeWriteBarrierPost(JSRuntime *rt, JSObject *obj, uint32_t start, uint32_t count)
{
#ifdef JSGC_GENERATIONAL
    if (count > 0)
        rt->gcStoreBuffer.putGeneric(DenseRangeRef(obj, start, start + count));
#endif
}






template <class Map, class Key>
inline void
HashTableWriteBarrierPost(JSRuntime *rt, Map *map, const Key &key)
{
#ifdef JSGC_GENERATIONAL
    if (key && IsInsideNursery(rt, key))
        rt->gcStoreBuffer.putGeneric(gc::HashKeyRef<Map, Key>(map, key));
#endif
}

inline
EncapsulatedId::~EncapsulatedId()
{
    pre();
}

inline EncapsulatedId &
EncapsulatedId::operator=(const EncapsulatedId &v)
{
    if (v.value != value)
        pre();
    JS_ASSERT(!IsPoisonedId(v.value));
    value = v.value;
    return *this;
}

inline void
EncapsulatedId::pre()
{
#ifdef JSGC_INCREMENTAL
    if (JSID_IS_OBJECT(value)) {
        JSObject *obj = JSID_TO_OBJECT(value);
        Zone *zone = obj->zone();
        if (zone->needsBarrier()) {
            js::gc::MarkObjectUnbarriered(zone->barrierTracer(), &obj, "write barrier");
            JS_ASSERT(obj == JSID_TO_OBJECT(value));
        }
    } else if (JSID_IS_STRING(value)) {
        JSString *str = JSID_TO_STRING(value);
        Zone *zone = str->zone();
        if (zone->needsBarrier()) {
            js::gc::MarkStringUnbarriered(zone->barrierTracer(), &str, "write barrier");
            JS_ASSERT(str == JSID_TO_STRING(value));
        }
    }
#endif
}

inline
RelocatableId::~RelocatableId()
{
    pre();
}

inline RelocatableId &
RelocatableId::operator=(jsid id)
{
    if (id != value)
        pre();
    JS_ASSERT(!IsPoisonedId(id));
    value = id;
    return *this;
}

inline RelocatableId &
RelocatableId::operator=(const RelocatableId &v)
{
    if (v.value != value)
        pre();
    JS_ASSERT(!IsPoisonedId(v.value));
    value = v.value;
    return *this;
}

inline
HeapId::HeapId(jsid id)
    : EncapsulatedId(id)
{
    JS_ASSERT(!IsPoisonedId(id));
    post();
}

inline
HeapId::~HeapId()
{
    pre();
}

inline void
HeapId::init(jsid id)
{
    JS_ASSERT(!IsPoisonedId(id));
    value = id;
    post();
}

inline void
HeapId::post()
{
}

inline HeapId &
HeapId::operator=(jsid id)
{
    if (id != value)
        pre();
    JS_ASSERT(!IsPoisonedId(id));
    value = id;
    post();
    return *this;
}

inline HeapId &
HeapId::operator=(const HeapId &v)
{
    if (v.value != value)
        pre();
    JS_ASSERT(!IsPoisonedId(v.value));
    value = v.value;
    post();
    return *this;
}

inline const Value &
ReadBarrieredValue::get() const
{
    if (value.isObject())
        JSObject::readBarrier(&value.toObject());
    else if (value.isString())
        JSString::readBarrier(value.toString());
    else
        JS_ASSERT(!value.isMarkable());

    return value;
}

inline
ReadBarrieredValue::operator const Value &() const
{
    return get();
}

inline JSObject &
ReadBarrieredValue::toObject() const
{
    return get().toObject();
}

} 

#endif 
