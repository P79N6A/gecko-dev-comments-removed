






#ifdef JSGC_GENERATIONAL
#ifndef gc_Nursery_inl_h__
#define gc_Nursery_inl_h__

#include "gc/Heap.h"
#include "gc/Nursery.h"

namespace js {
namespace gc {





class RelocationOverlay
{
    friend struct MinorCollectionTracer;

    
    const static uintptr_t Relocated = uintptr_t(0xbad0bad1);

    
    uintptr_t magic_;

    
    Cell *newLocation_;

    
    RelocationOverlay *next_;

  public:
    static RelocationOverlay *fromCell(Cell *cell) {
        JS_ASSERT(!cell->isTenured());
        return reinterpret_cast<RelocationOverlay *>(cell);
    }

    bool isForwarded() const {
        return magic_ == Relocated;
    }

    Cell *forwardingAddress() const {
        JS_ASSERT(isForwarded());
        return newLocation_;
    }

    void forwardTo(Cell *cell) {
        JS_ASSERT(!isForwarded());
        magic_ = Relocated;
        newLocation_ = cell;
        next_ = NULL;
    }

    RelocationOverlay *next() const {
        return next_;
    }
};

} 
} 

template <typename T>
JS_ALWAYS_INLINE bool
js::Nursery::getForwardedPointer(T **ref)
{
    JS_ASSERT(ref);
    JS_ASSERT(isInside(*ref));
    const gc::RelocationOverlay *overlay = reinterpret_cast<const gc::RelocationOverlay *>(*ref);
    if (!overlay->isForwarded())
        return false;
    
    *ref = static_cast<T *>(overlay->forwardingAddress());
    return true;
}

#endif 
#endif 
