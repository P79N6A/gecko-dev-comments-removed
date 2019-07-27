






#ifndef gc_Nursery_inl_h
#define gc_Nursery_inl_h

#ifdef JSGC_GENERATIONAL

#include "gc/Nursery.h"

#include "gc/Heap.h"
#include "js/TracingAPI.h"
#include "vm/Runtime.h"

template <typename T>
MOZ_ALWAYS_INLINE bool
js::Nursery::getForwardedPointer(T **ref)
{
    JS_ASSERT(ref);
    JS_ASSERT(isInside((void *)*ref));
    const gc::RelocationOverlay *overlay = reinterpret_cast<const gc::RelocationOverlay *>(*ref);
    if (!overlay->isForwarded())
        return false;
    
    *ref = static_cast<T *>(overlay->forwardingAddress());
    return true;
}

inline void
js::Nursery::forwardBufferPointer(JSTracer* trc, HeapSlot **pSlotElems)
{
    trc->runtime()->gc.nursery.forwardBufferPointer(pSlotElems);
}

#endif 

#endif 
