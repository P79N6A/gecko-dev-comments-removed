






































#include "nanojit.h"

#ifdef FEATURE_NANOJIT

namespace nanojit
{
    Allocator::Allocator()
        : current_chunk(NULL)
        , current_top(NULL)
        , current_limit(NULL)
    { }

    Allocator::~Allocator()
    {
        reset();
    }

    void Allocator::reset()
    {
        Chunk *c = current_chunk;
        while (c) {
            Chunk *prev = c->prev;
            freeChunk(c);
            c = prev;
        }
        current_chunk = NULL;
        current_top = NULL;
        current_limit = NULL;
        postReset();
    }

    void* Allocator::allocSlow(size_t nbytes)
    {
        NanoAssert((nbytes & 7) == 0);
        fill(nbytes);
        NanoAssert(current_top + nbytes <= current_limit);
        void* p = current_top;
        current_top += nbytes;
        return p;
    }

    void Allocator::fill(size_t nbytes)
    {
        const size_t minChunk = 2000;
        if (nbytes < minChunk)
            nbytes = minChunk;
        size_t chunkbytes = sizeof(Chunk) + nbytes - sizeof(int64_t);
        void* mem = allocChunk(chunkbytes);
        Chunk* chunk = (Chunk*) mem;
        chunk->prev = current_chunk;
        current_chunk = chunk;
        current_top = (char*)chunk->data;
        current_limit = (char*)mem + chunkbytes;
    }
}

#endif 
