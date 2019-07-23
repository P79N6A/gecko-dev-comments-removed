






































#include "nanojit.h"

#ifdef FEATURE_NANOJIT

namespace nanojit
{
    Allocator::Allocator(size_t minChunk)
        : minChunk(minChunk)
        , current_chunk(NULL)
        , current_top(NULL)
        , current_limit(NULL)
    { }

    Allocator::~Allocator()
    {
        reset();
    }

    void Allocator::reset(bool keepFirst)
    {
        Chunk *c = current_chunk;
        while (c) {
            Chunk *prev = c->prev;
            if (keepFirst && !prev)
                break;
            freeChunk(c);
            c = prev;
        }
        setChunk(c);
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

    void Allocator::setChunk(Chunk* chunk)
    {
        if (chunk) {
            current_chunk = chunk;
            current_top = (char*)chunk->data;
            current_limit = (char*)chunk + chunk->size;
        } else {
            current_chunk = NULL;
            current_top = current_limit = NULL;
        }
    }

    void Allocator::fill(size_t nbytes)
    {
        if (nbytes < minChunk)
            nbytes = minChunk;
        size_t chunkbytes = sizeof(Chunk) + nbytes - sizeof(int64_t);
        void* mem = allocChunk(chunkbytes);
        Chunk* chunk = (Chunk*) mem;
        chunk->prev = current_chunk;
        chunk->size = chunkbytes;
        setChunk(chunk);
    }
}

#endif 
