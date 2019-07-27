






#ifndef gc_ForkJoinNursery_inl_h
#define gc_ForkJoinNursery_inl_h

#ifdef JSGC_FJGENERATIONAL

#include "gc/ForkJoinNursery.h"

namespace js {
namespace gc {



























MOZ_ALWAYS_INLINE bool
ForkJoinNursery::isInsideNewspace(const void *addr)
{
    uintptr_t p = reinterpret_cast<uintptr_t>(addr);
    for (unsigned i = 0 ; i <= currentChunk_ ; i++) {
        if (p >= newspace[i]->start() && p < newspace[i]->end())
            return true;
    }
    return false;
}

MOZ_ALWAYS_INLINE bool
ForkJoinNursery::isInsideFromspace(const void *addr)
{
    uintptr_t p = reinterpret_cast<uintptr_t>(addr);
    for (unsigned i = 0 ; i < numFromspaceChunks_ ; i++) {
        if (p >= fromspace[i]->start() && p < fromspace[i]->end())
            return true;
    }
    return false;
}

MOZ_ALWAYS_INLINE bool
ForkJoinNursery::isForwarded(Cell *cell)
{
    JS_ASSERT(isInsideFromspace(cell));
    const RelocationOverlay *overlay = RelocationOverlay::fromCell(cell);
    return overlay->isForwarded();
}

template <typename T>
MOZ_ALWAYS_INLINE bool
ForkJoinNursery::getForwardedPointer(T **ref)
{
    JS_ASSERT(ref);
    JS_ASSERT(isInsideFromspace(*ref));
    const RelocationOverlay *overlay = RelocationOverlay::fromCell(*ref);
    if (!overlay->isForwarded())
        return false;
    
    *ref = static_cast<T *>(overlay->forwardingAddress());
    return true;
}

} 
} 

#endif 

#endif 
