






#ifndef gc_Nursery_inl_h
#define gc_Nursery_inl_h

#include "gc/Nursery.h"

#include "gc/Heap.h"
#include "js/TracingAPI.h"
#include "vm/Runtime.h"

MOZ_ALWAYS_INLINE bool
js::Nursery::getForwardedPointer(JSObject** ref) const
{
    MOZ_ASSERT(ref);
    MOZ_ASSERT(isInside((void*)*ref));
    const gc::RelocationOverlay* overlay = reinterpret_cast<const gc::RelocationOverlay*>(*ref);
    if (!overlay->isForwarded())
        return false;
    *ref = static_cast<JSObject*>(overlay->forwardingAddress());
    return true;
}

namespace js {





template <typename T>
static inline T*
AllocateObjectBuffer(ExclusiveContext* cx, uint32_t count)
{
    if (cx->isJSContext()) {
        Nursery& nursery = cx->asJSContext()->runtime()->gc.nursery;
        return static_cast<T*>(nursery.allocateBuffer(cx->zone(), count * sizeof(T)));
    }
    return cx->zone()->pod_malloc<T>(count);
}

template <typename T>
static inline T*
AllocateObjectBuffer(ExclusiveContext* cx, JSObject* obj, uint32_t count)
{
    if (cx->isJSContext()) {
        Nursery& nursery = cx->asJSContext()->runtime()->gc.nursery;
        return static_cast<T*>(nursery.allocateBuffer(obj, count * sizeof(T)));
    }
    return obj->zone()->pod_malloc<T>(count);
}


template <typename T>
static inline T*
ReallocateObjectBuffer(ExclusiveContext* cx, JSObject* obj, T* oldBuffer,
                       uint32_t oldCount, uint32_t newCount)
{
    if (cx->isJSContext()) {
        Nursery& nursery = cx->asJSContext()->runtime()->gc.nursery;
        return static_cast<T*>(nursery.reallocateBuffer(obj, oldBuffer,
                                                        oldCount * sizeof(T),
                                                        newCount * sizeof(T)));
    }
    return obj->zone()->pod_realloc<T>(oldBuffer, oldCount, newCount);
}

} 

#endif 
