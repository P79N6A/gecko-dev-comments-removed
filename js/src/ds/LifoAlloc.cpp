







































#include "LifoAlloc.h"

#include <new>

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
        memset(chunk, 0xcd, sizeof(*chunk) + chunk->bumpSpaceSize);
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
        BumpChunk::delete_(victim);
    }
    first = latest = NULL;
}

void
LifoAlloc::freeUnused()
{
    
    if (markCount || !first)
        return; 

    JS_ASSERT(first && latest);

    
    if (!latest->used()) {
        BumpChunk *lastUsed = NULL;
        for (BumpChunk *it = first; it != latest; it = it->next()) {
            if (it->used())
                lastUsed = it;
        }
        if (!lastUsed) {
            freeAll();
            return;
        }
        latest = lastUsed;
    }

    
    BumpChunk *it = latest->next();
    while (it) {
        BumpChunk *victim = it;
        it = it->next();
        BumpChunk::delete_(victim);
    }

    latest->setNext(NULL);
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
        latest = first = newChunk;
    } else {
        JS_ASSERT(latest && !latest->next());
        latest->setNext(newChunk);
        latest = newChunk;
    }
    return newChunk;
}

bool
LifoAlloc::ensureUnusedApproximateSlow(size_t n)
{
    
    LifoAllocScope scope(this);
    return !!getOrCreateChunk(n);
}

