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

void *
BumpChunk::tryAllocUnaligned(size_t n)
{
    char *oldBump = bump;
    char *newBump = bump + n;
    if (newBump > limit)
        return NULL;

    setBump(newBump);
    return oldBump;
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

    
    size_t freed = 0;
    for (BumpChunk *victim = latest->next(); victim; victim = victim->next()) {
        BumpChunk::delete_(victim);
        freed++;
    }
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
    size_t chunkSize = n > defaultChunkFreeSpace
                       ? RoundUpPow2(n + sizeof(BumpChunk))
                       : defaultChunkSize_;

    
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

void *
LifoAlloc::allocUnaligned(size_t n)
{
    void *result;
    if (latest && (result = latest->tryAllocUnaligned(n)))
        return result;

    return alloc(n);
}

void *
LifoAlloc::reallocUnaligned(void *origPtr, size_t origSize, size_t incr)
{
    JS_ASSERT(first && latest);

    





    if (latest
        && origPtr == (char *) latest->mark() - origSize
        && latest->canAllocUnaligned(incr)) {
        JS_ALWAYS_TRUE(allocUnaligned(incr));
        return origPtr;
    }

    
    size_t newSize = origSize + incr;
    void *newPtr = allocUnaligned(newSize);
    return newPtr ? memcpy(newPtr, origPtr, origSize) : NULL;
}
