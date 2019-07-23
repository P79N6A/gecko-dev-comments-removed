




















#ifndef CAIRO_FREELIST_H
#define CAIRO_FREELIST_H

#include "cairo-types-private.h"
#include "cairo-compiler-private.h"


#ifndef VG
#define VG(x)
#endif

#ifndef NULL
#define NULL (void *) 0
#endif

typedef struct _cairo_freelist_node cairo_freelist_node_t;
struct _cairo_freelist_node {
    cairo_freelist_node_t *next;
};

typedef struct _cairo_freelist {
    cairo_freelist_node_t *first_free_node;
    unsigned nodesize;
} cairo_freelist_t;

typedef struct _cairo_freelist_pool cairo_freelist_pool_t;
struct _cairo_freelist_pool {
    cairo_freelist_pool_t *next;
    unsigned size, rem;
    uint8_t *data;
};

typedef struct _cairo_freepool {
    cairo_freelist_node_t *first_free_node;
    cairo_freelist_pool_t *pools;
    unsigned nodesize;
    cairo_freelist_pool_t embedded_pool;
    uint8_t embedded_data[1000];
} cairo_freepool_t;




cairo_private void
_cairo_freelist_init (cairo_freelist_t *freelist, unsigned nodesize);


cairo_private void
_cairo_freelist_fini (cairo_freelist_t *freelist);





cairo_private void *
_cairo_freelist_alloc (cairo_freelist_t *freelist);





cairo_private void *
_cairo_freelist_calloc (cairo_freelist_t *freelist);




cairo_private void
_cairo_freelist_free (cairo_freelist_t *freelist, void *node);


cairo_private void
_cairo_freepool_init (cairo_freepool_t *freepool, unsigned nodesize);

cairo_private void
_cairo_freepool_fini (cairo_freepool_t *freepool);

cairo_private void *
_cairo_freepool_alloc_from_new_pool (cairo_freepool_t *freepool);

static inline void *
_cairo_freepool_alloc_from_pool (cairo_freepool_t *freepool)
{
    cairo_freelist_pool_t *pool;
    uint8_t *ptr;

    pool = freepool->pools;
    if (unlikely (freepool->nodesize > pool->rem))
	return _cairo_freepool_alloc_from_new_pool (freepool);

    ptr = pool->data;
    pool->data += freepool->nodesize;
    pool->rem -= freepool->nodesize;
    VG (VALGRIND_MAKE_MEM_UNDEFINED (ptr, freepool->nodesize));
    return ptr;
}

static inline void *
_cairo_freepool_alloc (cairo_freepool_t *freepool)
{
    cairo_freelist_node_t *node;

    node = freepool->first_free_node;
    if (unlikely (node == NULL))
	return _cairo_freepool_alloc_from_pool (freepool);

    VG (VALGRIND_MAKE_MEM_DEFINED (node, sizeof (node->next)));
    freepool->first_free_node = node->next;
    VG (VALGRIND_MAKE_MEM_UNDEFINED (node, freepool->nodesize));

    return node;
}

cairo_private cairo_status_t
_cairo_freepool_alloc_array (cairo_freepool_t *freepool,
			     int count,
			     void **array);

static inline void
_cairo_freepool_free (cairo_freepool_t *freepool, void *ptr)
{
    cairo_freelist_node_t *node = ptr;

    node->next = freepool->first_free_node;
    freepool->first_free_node = node;
    VG (VALGRIND_MAKE_MEM_NOACCESS (node, freepool->nodesize));
}

#endif 
