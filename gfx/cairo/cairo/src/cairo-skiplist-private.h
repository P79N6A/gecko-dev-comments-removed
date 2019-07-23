






















#ifndef SKIPLIST_H
#define SKIPLIST_H

#include "cairoint.h"

#define MAX_LEVEL   31









typedef struct _skip_elt {
    int prev_index;
    struct _skip_elt *prev;
    struct _skip_elt *next[1];
} skip_elt_t;

#define SKIP_LIST_ELT_TO_DATA(type, elt) ((type *) ((char *) (elt) - (sizeof (type) - sizeof (skip_elt_t))))

typedef int
(*skip_list_compare_t) (void *list, void *a, void *b);

typedef struct _skip_list {
    skip_list_compare_t compare;
    size_t elt_size;
    size_t data_size;
    skip_elt_t *chains[MAX_LEVEL];
    skip_elt_t *freelists[MAX_LEVEL];
    int		max_level;
} skip_list_t;









cairo_private void
skip_list_init (skip_list_t		*list,
		skip_list_compare_t	 compare,
		size_t			 elt_size);





cairo_private void
skip_list_fini (skip_list_t		*list);






cairo_private void *
skip_list_insert (skip_list_t *list, void *data, int unique);


cairo_private void *
skip_list_find (skip_list_t *list, void *data);


cairo_private void
skip_list_delete (skip_list_t *list, void *data);


cairo_private void
skip_list_delete_given (skip_list_t *list, skip_elt_t *given);

#endif
