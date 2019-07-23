






















#ifndef SKIPLIST_H
#define SKIPLIST_H

#include "cairoint.h"










#define MAX_LEVEL   15


#define FREELIST_FOR_LEVEL(level) (((level) - 1) / 2)


#define FREELIST_MAX_LEVEL_FOR(level) (((level) + 1) & ~1)

#define MAX_FREELIST_LEVEL (FREELIST_FOR_LEVEL (MAX_LEVEL - 1) + 1)









typedef struct _skip_elt {
    int prev_index;
    struct _skip_elt *prev;
    struct _skip_elt *next[1];
} skip_elt_t;

#define SKIP_LIST_ELT_TO_DATA(type, elt) ((type *) ((char *) (elt) - (sizeof (type) - sizeof (skip_elt_t))))

typedef int
(*cairo_skip_list_compare_t) (void *list, void *a, void *b);

typedef struct _skip_list {
    cairo_skip_list_compare_t compare;
    size_t elt_size;
    size_t data_size;
    skip_elt_t *chains[MAX_LEVEL];
    skip_elt_t *freelists[MAX_FREELIST_LEVEL];
    int		max_level;
} cairo_skip_list_t;









cairo_private void
_cairo_skip_list_init (cairo_skip_list_t		*list,
		cairo_skip_list_compare_t	 compare,
		size_t			 elt_size);





cairo_private void
_cairo_skip_list_fini (cairo_skip_list_t		*list);






cairo_private void *
_cairo_skip_list_insert (cairo_skip_list_t *list, void *data, int unique);


cairo_private void *
_cairo_skip_list_find (cairo_skip_list_t *list, void *data);


cairo_private void
_cairo_skip_list_delete (cairo_skip_list_t *list, void *data);


cairo_private void
_cairo_skip_list_delete_given (cairo_skip_list_t *list, skip_elt_t *given);

#endif
