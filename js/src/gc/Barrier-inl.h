






































#include "jsgcmark.h"

#include "gc/Barrier.h"

#ifndef jsgc_barrier_inl_h___
#define jsgc_barrier_inl_h___

namespace js {

static JS_ALWAYS_INLINE void
ClearValueRange(JSCompartment *comp, HeapValue *vec, uintN len, bool useHoles)
{
    if (useHoles) {
        for (uintN i = 0; i < len; i++)
            vec[i].set(comp, MagicValue(JS_ARRAY_HOLE));
    } else {
        for (uintN i = 0; i < len; i++)
            vec[i].set(comp, UndefinedValue());
    }
}

static JS_ALWAYS_INLINE void
InitValueRange(HeapValue *vec, uintN len, bool useHoles)
{
    if (useHoles) {
        for (uintN i = 0; i < len; i++)
            vec[i].init(MagicValue(JS_ARRAY_HOLE));
    } else {
        for (uintN i = 0; i < len; i++)
            vec[i].init(UndefinedValue());
    }
}

static JS_ALWAYS_INLINE void
DestroyValueRange(HeapValue *vec, uintN len)
{
    for (uintN i = 0; i < len; i++)
        vec[i].~HeapValue();
}

inline
HeapValue::HeapValue(const Value &v)
    : value(v)
{
    post();
}

inline
HeapValue::HeapValue(const HeapValue &v)
    : value(v.value)
{
    post();
}

inline
HeapValue::~HeapValue()
{
    pre();
}

inline void
HeapValue::init(const Value &v)
{
    value = v;
    post();
}

inline void
HeapValue::writeBarrierPre(const Value &value)
{
#ifdef JSGC_INCREMENTAL
    if (value.isMarkable()) {
        js::gc::Cell *cell = (js::gc::Cell *)value.toGCThing();
        writeBarrierPre(cell->compartment(), value);
    }
#endif
}

inline void
HeapValue::writeBarrierPost(const Value &value, void *addr)
{
}

inline void
HeapValue::writeBarrierPre(JSCompartment *comp, const Value &value)
{
#ifdef JSGC_INCREMENTAL
    if (comp->needsBarrier())
        js::gc::MarkValueUnbarriered(comp->barrierTracer(), value, "write barrier");
#endif
}

inline void
HeapValue::writeBarrierPost(JSCompartment *comp, const Value &value, void *addr)
{
}

inline void
HeapValue::pre()
{
    writeBarrierPre(value);
}

inline void
HeapValue::post()
{
}

inline void
HeapValue::pre(JSCompartment *comp)
{
    writeBarrierPre(comp, value);
}

inline void
HeapValue::post(JSCompartment *comp)
{
}

inline HeapValue &
HeapValue::operator=(const Value &v)
{
    pre();
    value = v;
    post();
    return *this;
}

inline HeapValue &
HeapValue::operator=(const HeapValue &v)
{
    pre();
    value = v.value;
    post();
    return *this;
}

inline void
HeapValue::set(JSCompartment *comp, const Value &v)
{
#ifdef DEBUG
    if (value.isMarkable()) {
        js::gc::Cell *cell = (js::gc::Cell *)value.toGCThing();
        JS_ASSERT(cell->compartment() == comp ||
                  cell->compartment() == comp->rt->atomsCompartment);
    }
#endif

    pre(comp);
    value = v;
    post(comp);
}

inline void
HeapValue::boxNonDoubleFrom(JSValueType type, uint64 *out)
{
    pre();
    value.boxNonDoubleFrom(type, out);
    post();
}

inline
HeapId::HeapId(jsid id)
    : value(id)
{
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
    value = id;
    post();
}

inline void
HeapId::pre()
{
#ifdef JSGC_INCREMENTAL
    if (JS_UNLIKELY(JSID_IS_OBJECT(value))) {
        JSObject *obj = JSID_TO_OBJECT(value);
        JSCompartment *comp = obj->compartment();
        if (comp->needsBarrier())
            js::gc::MarkObjectUnbarriered(comp->barrierTracer(), obj, "write barrier");
    }
#endif
}

inline void
HeapId::post()
{
}

inline HeapId &
HeapId::operator=(jsid id)
{
    pre();
    value = id;
    post();
    return *this;
}

inline HeapId &
HeapId::operator=(const HeapId &v)
{
    pre();
    value = v.value;
    post();
    return *this;
}

} 

#endif 
