






























































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sphinxbase/glist.h"
#include "sphinxbase/ckd_alloc.h"


glist_t
glist_add_ptr(glist_t g, void *ptr)
{
    gnode_t *gn;

    gn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    gn->data.ptr = ptr;
    gn->next = g;
    return ((glist_t) gn);      
}


glist_t
glist_add_int32(glist_t g, int32 val)
{
    gnode_t *gn;

    gn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    gn->data.i = (long)val;
    gn->next = g;
    return ((glist_t) gn);      
}


glist_t
glist_add_uint32(glist_t g, uint32 val)
{
    gnode_t *gn;

    gn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    gn->data.ui = (unsigned long)val;
    gn->next = g;
    return ((glist_t) gn);      
}


glist_t
glist_add_float32(glist_t g, float32 val)
{
    gnode_t *gn;

    gn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    gn->data.fl = (double)val;
    gn->next = g;
    return ((glist_t) gn);      
}


glist_t
glist_add_float64(glist_t g, float64 val)
{
    gnode_t *gn;

    gn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    gn->data.fl = (double)val;
    gn->next = g;
    return ((glist_t) gn);      
}

void
glist_free(glist_t g)
{
    gnode_t *gn;

    while (g) {
        gn = g;
        g = gn->next;
        ckd_free((void *) gn);
    }
}

int32
glist_count(glist_t g)
{
    gnode_t *gn;
    int32 n;

    for (gn = g, n = 0; gn; gn = gn->next, n++);
    return n;
}


gnode_t *
glist_tail(glist_t g)
{
    gnode_t *gn;

    if (!g)
        return NULL;

    for (gn = g; gn->next; gn = gn->next);
    return gn;
}


glist_t
glist_reverse(glist_t g)
{
    gnode_t *gn, *nextgn;
    gnode_t *rev;

    rev = NULL;
    for (gn = g; gn; gn = nextgn) {
        nextgn = gn->next;

        gn->next = rev;
        rev = gn;
    }

    return rev;
}


gnode_t *
glist_insert_ptr(gnode_t * gn, void *ptr)
{
    gnode_t *newgn;

    newgn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    newgn->data.ptr = ptr;
    newgn->next = gn->next;
    gn->next = newgn;

    return newgn;
}


gnode_t *
glist_insert_int32(gnode_t * gn, int32 val)
{
    gnode_t *newgn;

    newgn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    newgn->data.i = val;
    newgn->next = gn->next;
    gn->next = newgn;

    return newgn;
}


gnode_t *
glist_insert_uint32(gnode_t * gn, uint32 val)
{
    gnode_t *newgn;

    newgn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    newgn->data.ui = val;
    newgn->next = gn->next;

    gn->next = newgn;

    return newgn;
}


gnode_t *
glist_insert_float32(gnode_t * gn, float32 val)
{
    gnode_t *newgn;

    newgn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    newgn->data.fl = (double)val;
    newgn->next = gn->next;
    gn->next = newgn;

    return newgn;
}


gnode_t *
glist_insert_float64(gnode_t * gn, float64 val)
{
    gnode_t *newgn;

    newgn = (gnode_t *) ckd_calloc(1, sizeof(gnode_t));
    newgn->data.fl = (double)val;
    newgn->next = gn->next;
    gn->next = newgn;

    return newgn;
}

gnode_t *
gnode_free(gnode_t * gn, gnode_t * pred)
{
    gnode_t *next;

    next = gn->next;
    if (pred) {
        assert(pred->next == gn);

        pred->next = next;
    }

    ckd_free((char *) gn);

    return next;
}
