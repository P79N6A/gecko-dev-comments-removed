





#ifndef vm_ObjectImpl_inl_h
#define vm_ObjectImpl_inl_h

#include "mozilla/Assertions.h"

#include "jscompartment.h"
#include "jsgc.h"
#include "jsproxy.h"

#include "gc/Heap.h"
#include "gc/Marking.h"
#include "js/TemplateLib.h"
#include "vm/ObjectImpl.h"

#include "gc/Barrier-inl.h"
#include "vm/Interpreter.h"
#include "vm/ObjectImpl.h"
#include "vm/ProxyObject.h"

inline JSCompartment *
js::ObjectImpl::compartment() const
{
    return lastProperty()->base()->compartment();
}

inline bool
js::ObjectImpl::nativeContains(ExclusiveContext *cx, Shape *shape)
{
    return nativeLookup(cx, shape->propid()) == shape;
}

inline bool
js::ObjectImpl::nativeContainsPure(Shape *shape)
{
    return nativeLookupPure(shape->propid()) == shape;
}

inline bool
js::ObjectImpl::nonProxyIsExtensible() const
{
    MOZ_ASSERT(!asObjectPtr()->is<ProxyObject>());

    
    return !lastProperty()->hasObjectFlag(BaseShape::NOT_EXTENSIBLE);
}

 inline bool
js::ObjectImpl::isExtensible(ExclusiveContext *cx, js::Handle<ObjectImpl*> obj, bool *extensible)
{
    if (obj->asObjectPtr()->is<ProxyObject>()) {
        if (!cx->shouldBeJSContext())
            return false;
        HandleObject h =
            HandleObject::fromMarkedLocation(reinterpret_cast<JSObject* const*>(obj.address()));
        return Proxy::isExtensible(cx->asJSContext(), h, extensible);
    }

    *extensible = obj->nonProxyIsExtensible();
    return true;
}

inline bool
js::ObjectImpl::isNative() const
{
    return lastProperty()->isNative();
}

#ifdef DEBUG
inline bool
IsObjectValueInCompartment(js::Value v, JSCompartment *comp)
{
    if (!v.isObject())
        return true;
    return v.toObject().compartment() == comp;
}
#endif

inline void
js::ObjectImpl::setSlot(uint32_t slot, const js::Value &value)
{
    MOZ_ASSERT(slotInRange(slot));
    MOZ_ASSERT(IsObjectValueInCompartment(value, asObjectPtr()->compartment()));
    getSlotRef(slot).set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
}

inline void
js::ObjectImpl::setCrossCompartmentSlot(uint32_t slot, const js::Value &value)
{
    MOZ_ASSERT(slotInRange(slot));
    getSlotRef(slot).set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
}

inline void
js::ObjectImpl::initSlot(uint32_t slot, const js::Value &value)
{
    MOZ_ASSERT(getSlot(slot).isUndefined());
    MOZ_ASSERT(slotInRange(slot));
    MOZ_ASSERT(IsObjectValueInCompartment(value, asObjectPtr()->compartment()));
    initSlotUnchecked(slot, value);
}

inline void
js::ObjectImpl::initCrossCompartmentSlot(uint32_t slot, const js::Value &value)
{
    MOZ_ASSERT(getSlot(slot).isUndefined());
    MOZ_ASSERT(slotInRange(slot));
    initSlotUnchecked(slot, value);
}

inline void
js::ObjectImpl::initSlotUnchecked(uint32_t slot, const js::Value &value)
{
    getSlotAddressUnchecked(slot)->init(this->asObjectPtr(), HeapSlot::Slot, slot, value);
}

inline void
js::ObjectImpl::setFixedSlot(uint32_t slot, const js::Value &value)
{
    MOZ_ASSERT(slot < numFixedSlots());
    fixedSlots()[slot].set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
}

inline void
js::ObjectImpl::initFixedSlot(uint32_t slot, const js::Value &value)
{
    MOZ_ASSERT(slot < numFixedSlots());
    fixedSlots()[slot].init(this->asObjectPtr(), HeapSlot::Slot, slot, value);
}

inline uint32_t
js::ObjectImpl::slotSpan() const
{
    if (inDictionaryMode())
        return lastProperty()->base()->slotSpan();
    return lastProperty()->slotSpan();
}

inline uint32_t
js::ObjectImpl::numDynamicSlots() const
{
    return dynamicSlotsCount(numFixedSlots(), slotSpan());
}

inline bool
js::ObjectImpl::isDelegate() const
{
    return lastProperty()->hasObjectFlag(BaseShape::DELEGATE);
}

inline bool
js::ObjectImpl::inDictionaryMode() const
{
    return lastProperty()->inDictionary();
}

JS_ALWAYS_INLINE JS::Zone *
js::ObjectImpl::zone() const
{
    JS_ASSERT(InSequentialOrExclusiveParallelSection());
    return shape_->zone();
}

 inline void
js::ObjectImpl::readBarrier(ObjectImpl *obj)
{
#ifdef JSGC_INCREMENTAL
    Zone *zone = obj->zone();
    if (zone->needsBarrier()) {
        MOZ_ASSERT(!zone->rt->isHeapMajorCollecting());
        JSObject *tmp = obj->asObjectPtr();
        MarkObjectUnbarriered(zone->barrierTracer(), &tmp, "read barrier");
        MOZ_ASSERT(tmp == obj->asObjectPtr());
    }
#endif
}

inline void
js::ObjectImpl::privateWriteBarrierPre(void **old)
{
#ifdef JSGC_INCREMENTAL
    Zone *zone = this->zone();
    if (zone->needsBarrier()) {
        if (*old && getClass()->trace)
            getClass()->trace(zone->barrierTracer(), this->asObjectPtr());
    }
#endif
}

inline void
js::ObjectImpl::privateWriteBarrierPost(void **pprivate)
{
#ifdef JSGC_GENERATIONAL
    runtime()->gcStoreBuffer.putCell(reinterpret_cast<js::gc::Cell **>(pprivate));
#endif
}

 inline void
js::ObjectImpl::writeBarrierPre(ObjectImpl *obj)
{
#ifdef JSGC_INCREMENTAL
    



    if (IsNullTaggedPointer(obj) || !obj->runtime()->needsBarrier())
        return;

    Zone *zone = obj->zone();
    if (zone->needsBarrier()) {
        MOZ_ASSERT(!zone->rt->isHeapMajorCollecting());
        JSObject *tmp = obj->asObjectPtr();
        MarkObjectUnbarriered(zone->barrierTracer(), &tmp, "write barrier");
        MOZ_ASSERT(tmp == obj->asObjectPtr());
    }
#endif
}

 inline void
js::ObjectImpl::writeBarrierPost(ObjectImpl *obj, void *addr)
{
#ifdef JSGC_GENERATIONAL
    if (IsNullTaggedPointer(obj))
        return;
    obj->runtime()->gcStoreBuffer.putCell((Cell **)addr);
#endif
}

 inline void
js::ObjectImpl::writeBarrierPostRelocate(ObjectImpl *obj, void *addr)
{
#ifdef JSGC_GENERATIONAL
    obj->runtime()->gcStoreBuffer.putRelocatableCell((Cell **)addr);
#endif
}

 inline void
js::ObjectImpl::writeBarrierPostRemove(ObjectImpl *obj, void *addr)
{
#ifdef JSGC_GENERATIONAL
    obj->runtime()->gcStoreBuffer.removeRelocatableCell((Cell **)addr);
#endif
}

inline void
js::ObjectImpl::setPrivate(void *data)
{
    void **pprivate = &privateRef(numFixedSlots());
    privateWriteBarrierPre(pprivate);
    *pprivate = data;
}

inline void
js::ObjectImpl::setPrivateGCThing(js::gc::Cell *cell)
{
    void **pprivate = &privateRef(numFixedSlots());
    privateWriteBarrierPre(pprivate);
    *pprivate = reinterpret_cast<void *>(cell);
    privateWriteBarrierPost(pprivate);
}

#endif 
