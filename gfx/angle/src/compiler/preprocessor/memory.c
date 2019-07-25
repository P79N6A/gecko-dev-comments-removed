











































#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <stdint.h>
#endif

#include "compiler/preprocessor/memory.h"


#define CHUNKSIZE       (64*1024)
#define ALIGN           8


#undef malloc
#undef free

struct chunk {
    struct chunk        *next;
};

struct cleanup {
    struct cleanup      *next;
    void                (*fn)(void *);
    void                *arg;
};

struct MemoryPool_rec {
    struct chunk        *next;
    uintptr_t           free, end;
    size_t              chunksize;
    uintptr_t           alignmask;
    struct cleanup      *cleanup;
};

MemoryPool *mem_CreatePool(size_t chunksize, unsigned int align)
{
    MemoryPool  *pool;

    if (align == 0) align = ALIGN;
    if (chunksize == 0) chunksize = CHUNKSIZE;
    if (align & (align-1)) return 0;
    if (chunksize < sizeof(MemoryPool)) return 0;
    if (chunksize & (align-1)) return 0;
    if (!(pool = malloc(chunksize))) return 0;
    pool->next = 0;
    pool->chunksize = chunksize;
    pool->alignmask = (uintptr_t)(align)-1;  
    pool->free = ((uintptr_t)(pool + 1) + pool->alignmask) & ~pool->alignmask;
    pool->end = (uintptr_t)pool + chunksize;
    pool->cleanup = 0;
    return pool;
}

void mem_FreePool(MemoryPool *pool)
{
    struct cleanup      *cleanup;
    struct chunk        *p, *next;

    for (cleanup = pool->cleanup; cleanup; cleanup = cleanup->next) {
        cleanup->fn(cleanup->arg);
    }
    for (p = (struct chunk *)pool; p; p = next) {
        next = p->next;
        free(p);
    }
}

void *mem_Alloc(MemoryPool *pool, size_t size)
{
    struct chunk *ch;
    void *rv = (void *)pool->free;
    size = (size + pool->alignmask) & ~pool->alignmask;
    if (size <= 0) size = pool->alignmask;
    pool->free += size;
    if (pool->free > pool->end || pool->free < (uintptr_t)rv) {
        size_t minreq = (size + sizeof(struct chunk) + pool->alignmask)
                      & ~pool->alignmask;
        pool->free = (uintptr_t)rv;
        if (minreq >= pool->chunksize) {
            
            
            ch = malloc(minreq);
            if (!ch) return 0;
        } else {
            ch = malloc(pool->chunksize);
            if (!ch) return 0;
            pool->free = (uintptr_t)ch + minreq;
            pool->end = (uintptr_t)ch + pool->chunksize;
        }
        ch->next = pool->next;
        pool->next = ch;
        rv = (void *)(((uintptr_t)(ch+1) + pool->alignmask) & ~pool->alignmask);
    }
    return rv;
}

int mem_AddCleanup(MemoryPool *pool, void (*fn)(void *), void *arg) {
    struct cleanup *cleanup;

    pool->free = (pool->free + sizeof(void *) - 1) & ~(sizeof(void *)-1);
    cleanup = mem_Alloc(pool, sizeof(struct cleanup));
    if (!cleanup) return -1;
    cleanup->next = pool->cleanup;
    cleanup->fn = fn;
    cleanup->arg = arg;
    pool->cleanup = cleanup;
    return 0;
}
