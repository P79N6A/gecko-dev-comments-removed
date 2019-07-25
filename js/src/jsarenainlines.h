






































#ifndef jsarenainlines_h___
#define jsarenainlines_h___

#include "jsarena.h"

const jsuword JSArenaPool::POINTER_MASK = jsuword(JS_ALIGN_OF_POINTER - 1);

jsuword
JSArenaPool::headerBaseMask() const
{
    return mask | POINTER_MASK;
}

jsuword
JSArenaPool::headerSize() const
{
    return sizeof(JSArena **) + ((mask < POINTER_MASK) ? POINTER_MASK - mask : 0);
}

void *
JSArenaPool::grow(jsuword p, size_t size, size_t incr)
{
    countGrowth(size, incr);
    if (current->avail == p + align(size)) {
        size_t nb = align(size + incr);
        if (current->limit >= nb && p <= current->limit - nb) {
            current->avail = p + nb;
            countInplaceGrowth(size, incr);
        } else if (p == current->base) {
            p = jsuword(reallocInternal((void *) p, size, incr));
        } else {
            p = jsuword(growInternal((void *) p, size, incr));
        }
    } else {
        p = jsuword(growInternal((void *) p, size, incr));
    }
    return (void *) p;
}

#endif
