




















#ifndef CAIRO_FREELIST_H
#define CAIRO_FREELIST_H

#include "cairoint.h"
#include <stddef.h>


typedef struct _cairo_freelist cairo_freelist_t;
typedef struct _cairo_freelist_node cairo_freelist_node_t;

struct _cairo_freelist_node {
    cairo_freelist_node_t *next;
};

struct _cairo_freelist {
    cairo_freelist_node_t *first_free_node;
    unsigned nodesize;
};




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

#endif 
