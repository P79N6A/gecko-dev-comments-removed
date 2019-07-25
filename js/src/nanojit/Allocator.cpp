






































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

    void* Allocator::allocSlow(size_t nbytes, bool fallible)
    {
        NanoAssert((nbytes & 7) == 0);
        if (fill(nbytes, fallible)) {
            NanoAssert(current_top + nbytes <= current_limit);
            void* p = current_top;
            current_top += nbytes;
            return p;
        }
        return NULL;
    }

    bool Allocator::fill(size_t nbytes, bool fallible)
    {
        if (nbytes < MIN_CHUNK_SZB)
            nbytes = MIN_CHUNK_SZB;
        size_t chunkbytes = sizeof(Chunk) + nbytes - sizeof(int64_t);
        void* mem = allocChunk(chunkbytes, fallible);
        if (mem) {
            Chunk* chunk = (Chunk*) mem;
            chunk->prev = current_chunk;
            current_chunk = chunk;
            current_top = (char*)chunk->data;
            current_limit = (char*)mem + chunkbytes;
            return true;
        } else {
            NanoAssert(fallible);
            return false;
        }
    }
}

#endif 
