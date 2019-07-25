



































#ifndef CAIRO_FREED_POOL_H
#define CAIRO_FREED_POOL_H

#include "cairoint.h"
#include "cairo-atomic-private.h"

#if HAS_ATOMIC_OPS



#define MAX_FREED_POOL_SIZE 4
typedef struct {
    void *pool[MAX_FREED_POOL_SIZE];
    int top;
} freed_pool_t;

static cairo_always_inline void *
_atomic_fetch (void **slot)
{
    void *ptr;

    do {
        ptr = _cairo_atomic_ptr_get (slot);
    } while (! _cairo_atomic_ptr_cmpxchg (slot, ptr, NULL));

    return ptr;
}

static cairo_always_inline cairo_bool_t
_atomic_store (void **slot, void *ptr)
{
    return _cairo_atomic_ptr_cmpxchg (slot, NULL, ptr);
}

cairo_private void *
_freed_pool_get_search (freed_pool_t *pool);

static inline void *
_freed_pool_get (freed_pool_t *pool)
{
    void *ptr;
    int i;

    i = pool->top - 1;
    if (i < 0)
	i = 0;

    ptr = _atomic_fetch (&pool->pool[i]);
    if (likely (ptr != NULL)) {
	pool->top = i;
	return ptr;
    }

    
    return _freed_pool_get_search (pool);
}

cairo_private void
_freed_pool_put_search (freed_pool_t *pool, void *ptr);

static inline void
_freed_pool_put (freed_pool_t *pool, void *ptr)
{
    int i;

    i = pool->top;
    if (likely (i < ARRAY_LENGTH (pool->pool) &&
		_atomic_store (&pool->pool[i], ptr)))
    {
	pool->top = i + 1;
	return;
    }

    
    _freed_pool_put_search (pool, ptr);
}

cairo_private void
_freed_pool_reset (freed_pool_t *pool);

#define HAS_FREED_POOL 1

#else

typedef int freed_pool_t;

#define _freed_pool_get(pool) NULL
#define _freed_pool_put(pool, ptr) free(ptr)
#define _freed_pool_reset(ptr)

#endif

#endif 
