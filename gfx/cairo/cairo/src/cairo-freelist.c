





















#include "cairoint.h"

#include "cairo-freelist-private.h"

void
_cairo_freelist_init (cairo_freelist_t *freelist, unsigned nodesize)
{
    memset (freelist, 0, sizeof(cairo_freelist_t));
    freelist->nodesize = nodesize;
}

void
_cairo_freelist_fini (cairo_freelist_t *freelist)
{
    cairo_freelist_node_t *node = freelist->first_free_node;
    while (node) {
	cairo_freelist_node_t *next = node->next;
	free (node);
	node = next;
    }
}

void *
_cairo_freelist_alloc (cairo_freelist_t *freelist)
{
    if (freelist->first_free_node) {
	cairo_freelist_node_t *node = freelist->first_free_node;
	freelist->first_free_node = node->next;
	return (void*)node;
    }
    return malloc (freelist->nodesize);
}

void *
_cairo_freelist_calloc (cairo_freelist_t *freelist)
{
    void *node = _cairo_freelist_alloc (freelist);
    if (node)
	memset (node, 0, freelist->nodesize);
    return node;
}

void
_cairo_freelist_free (cairo_freelist_t *freelist, void *voidnode)
{
    cairo_freelist_node_t *node = voidnode;
    if (node) {
	node->next = freelist->first_free_node;
	freelist->first_free_node = node;
    }
}
