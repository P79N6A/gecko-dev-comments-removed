






#include "LifoAlloc.h"

using namespace js;

namespace js {
namespace detail {

BumpChunk *
BumpChunk::new_(size_t chunkSize)
{
    JS_ASSERT(RoundUpPow2(chunkSize) == chunkSize);
    void *mem = js_malloc(chunkSize);
    if (!mem)
        return NULL;
    BumpChunk *result = new (mem) BumpChunk(chunkSize - sizeof(BumpChunk));

    




    JS_ASSERT(AlignPtr(result->bump) == result->bump);
    return result;
}

void
BumpChunk::delete_(BumpChunk *chunk)
{
#ifdef DEBUG
    
    
    size_t size = sizeof(*chunk) + chunk->bumpSpaceSize;
    MOZ_MAKE_MEM_UNDEFINED(chunk, size);
    memset(chunk, 0xcd, size);
#endif
    js_free(chunk);
}

bool
BumpChunk::canAlloc(size_t n)
{
    char *aligned = AlignPtr(bump);
    char *bumped = aligned + n;
    return bumped <= limit && bumped > headerBase();
}

} 
} 

void
LifoAlloc::freeAll()
{
    while (first) {
        BumpChunk *victim = first;
        first = first->next();
        decrementCurSize(victim->computedSizeOfIncludingThis());
        BumpChunk::delete_(victim);
    }
    first = latest = last = NULL;

    



    JS_ASSERT(curSize_ == 0);
}

LifoAlloc::BumpChunk *
LifoAlloc::getOrCreateChunk(size_t n)
{
    if (first) {
        
        while (latest->next()) {
            latest = latest->next();
            latest->resetBump(); 
            if (latest->canAlloc(n))
                return latest;
        }
    }

    size_t defaultChunkFreeSpace = defaultChunkSize_ - sizeof(BumpChunk);
    size_t chunkSize;
    if (n > defaultChunkFreeSpace) {
        size_t allocSizeWithHeader = n + sizeof(BumpChunk);

        
        if (allocSizeWithHeader < n ||
            (allocSizeWithHeader & (size_t(1) << (tl::BitSize<size_t>::result - 1)))) {
            return NULL;
        }

        chunkSize = RoundUpPow2(allocSizeWithHeader);
    } else {
        chunkSize = defaultChunkSize_;
    }

    
    BumpChunk *newChunk = BumpChunk::new_(chunkSize);
    if (!newChunk)
        return NULL;
    if (!first) {
        latest = first = last = newChunk;
    } else {
        JS_ASSERT(latest && !latest->next());
        latest->setNext(newChunk);
        latest = last = newChunk;
    }

    size_t computedChunkSize = newChunk->computedSizeOfIncludingThis();
    JS_ASSERT(computedChunkSize == chunkSize);
    incrementCurSize(computedChunkSize);

    return newChunk;
}

void
LifoAlloc::transferFrom(LifoAlloc *other)
{
    JS_ASSERT(!markCount);
    JS_ASSERT(latest == first);
    JS_ASSERT(!other->markCount);

    if (!other->first)
        return;

    incrementCurSize(other->curSize_);
    append(other->first, other->last);
    other->first = other->last = other->latest = NULL;
    other->curSize_ = 0;
}

void
LifoAlloc::transferUnusedFrom(LifoAlloc *other)
{
    JS_ASSERT(!markCount);
    JS_ASSERT(latest == first);

    if (other->markCount || !other->first)
        return;

    

    if (other->latest->next()) {
        if (other->latest == other->first) {
            
            size_t delta = other->curSize_ - other->first->computedSizeOfIncludingThis();
            other->decrementCurSize(delta);
            incrementCurSize(delta);
        } else {
            for (BumpChunk *chunk = other->latest->next(); chunk; chunk = chunk->next()) {
                size_t size = chunk->computedSizeOfIncludingThis();
                incrementCurSize(size);
                other->decrementCurSize(size);
            }
        }

        append(other->latest->next(), other->last);
        other->latest->setNext(NULL);
        other->last = other->latest;
    }
}
