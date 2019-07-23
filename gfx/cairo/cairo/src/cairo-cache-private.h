





































#ifndef CAIRO_CACHE_PRIVATE_H
#define CAIRO_CACHE_PRIVATE_H

#include "cairo-types-private.h"










































typedef struct _cairo_cache_entry {
    unsigned long hash;
    unsigned long size;
} cairo_cache_entry_t;

typedef cairo_bool_t
(*cairo_cache_keys_equal_func_t) (const void *key_a, const void *key_b);

typedef void
(*cairo_cache_callback_func_t) (void *entry,
				void *closure);

cairo_private cairo_cache_t *
_cairo_cache_create (cairo_cache_keys_equal_func_t keys_equal,
		     cairo_destroy_func_t	   entry_destroy,
		     unsigned long		   max_size);

cairo_private void
_cairo_cache_destroy (cairo_cache_t *cache);

cairo_private void
_cairo_cache_freeze (cairo_cache_t *cache);

cairo_private void
_cairo_cache_thaw (cairo_cache_t *cache);

cairo_private cairo_bool_t
_cairo_cache_lookup (cairo_cache_t	  *cache,
		     cairo_cache_entry_t  *key,
		     cairo_cache_entry_t **entry_return);

cairo_private cairo_status_t
_cairo_cache_insert (cairo_cache_t	 *cache,
		     cairo_cache_entry_t *entry);

cairo_private void
_cairo_cache_foreach (cairo_cache_t 	      	 *cache,
		      cairo_cache_callback_func_t cache_callback,
		      void			 *closure);

#endif
