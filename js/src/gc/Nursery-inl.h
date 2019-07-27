






#ifndef gc_Nursery_inl_h
#define gc_Nursery_inl_h

#ifdef JSGC_GENERATIONAL

#include "gc/Nursery.h"

#include "js/TracingAPI.h"
#include "gc/Heap.h"
#include "vm/Runtime.h"

namespace js {
namespace gc {


inline RelocationOverlay *
RelocationOverlay::fromCell(Cell *cell)
{
    JS_ASSERT(!cell->isTenured());
    return reinterpret_cast<RelocationOverlay *>(cell);
}

inline bool
RelocationOverlay::isForwarded() const
{
    return magic_ == Relocated;
}

inline Cell *
RelocationOverlay::forwardingAddress() const
{
    JS_ASSERT(isForwarded());
    return newLocation_;
}

inline void
RelocationOverlay::forwardTo(Cell *cell)
{
    JS_ASSERT(!isForwarded());
    magic_ = Relocated;
    newLocation_ = cell;
    next_ = nullptr;
}

inline RelocationOverlay *
RelocationOverlay::next() const
{
    return next_;
}

} 
} 

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
