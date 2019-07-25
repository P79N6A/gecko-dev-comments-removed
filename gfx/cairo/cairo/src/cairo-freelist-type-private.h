




















#ifndef CAIRO_FREELIST_TYPE_H
#define CAIRO_FREELIST_TYPE_H

#include "cairo-types-private.h"
#include "cairo-compiler-private.h"

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
    cairo_freelist_pool_t *freepools;
    unsigned nodesize;
    cairo_freelist_pool_t embedded_pool;
    uint8_t embedded_data[1000];
} cairo_freepool_t;

#endif 
