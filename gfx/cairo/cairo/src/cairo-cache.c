





































#include "cairoint.h"

static void
_cairo_cache_shrink_to_accommodate (cairo_cache_t *cache,
				    unsigned long  additional);

static cairo_bool_t
_cairo_cache_entry_is_non_zero (const void *entry)
{
    return ((const cairo_cache_entry_t *) entry)->size;
}

static cairo_status_t
_cairo_cache_init (cairo_cache_t		*cache,
		   cairo_cache_keys_equal_func_t keys_equal,
		   cairo_cache_predicate_func_t  predicate,
		   cairo_destroy_func_t		 entry_destroy,
		   unsigned long		 max_size)
{
    cache->hash_table = _cairo_hash_table_create (keys_equal);
    if (unlikely (cache->hash_table == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (predicate == NULL)
	predicate = _cairo_cache_entry_is_non_zero;
    cache->predicate = predicate;
    cache->entry_destroy = entry_destroy;

    cache->max_size = max_size;
    cache->size = 0;

    cache->freeze_count = 0;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_cache_pluck (void *entry, void *closure)
{
    _cairo_cache_remove (closure, entry);
}

static void
_cairo_cache_fini (cairo_cache_t *cache)
{
    _cairo_hash_table_foreach (cache->hash_table,
			       _cairo_cache_pluck,
			       cache);
    assert (cache->size == 0);
    _cairo_hash_table_destroy (cache->hash_table);
}



































cairo_cache_t *
_cairo_cache_create (cairo_cache_keys_equal_func_t keys_equal,
		     cairo_cache_predicate_func_t  predicate,
		     cairo_destroy_func_t	   entry_destroy,
		     unsigned long		   max_size)
{
    cairo_status_t status;
    cairo_cache_t *cache;

    cache = malloc (sizeof (cairo_cache_t));
    if (unlikely (cache == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    status = _cairo_cache_init (cache,
				keys_equal,
				predicate,
				entry_destroy,
				max_size);
    if (unlikely (status)) {
	free (cache);
	return NULL;
    }

    return cache;
}










void
_cairo_cache_destroy (cairo_cache_t *cache)
{
    _cairo_cache_fini (cache);

    free (cache);
}















void
_cairo_cache_freeze (cairo_cache_t *cache)
{
    assert (cache->freeze_count >= 0);

    cache->freeze_count++;
}















void
_cairo_cache_thaw (cairo_cache_t *cache)
{
    assert (cache->freeze_count > 0);

    cache->freeze_count--;

    if (cache->freeze_count == 0)
	_cairo_cache_shrink_to_accommodate (cache, 0);
}















void *
_cairo_cache_lookup (cairo_cache_t	  *cache,
		     cairo_cache_entry_t  *key)
{
    return _cairo_hash_table_lookup (cache->hash_table,
				     (cairo_hash_entry_t *) key);
}










static cairo_bool_t
_cairo_cache_remove_random (cairo_cache_t *cache)
{
    cairo_cache_entry_t *entry;

    entry = _cairo_hash_table_random_entry (cache->hash_table,
					    cache->predicate);
    if (unlikely (entry == NULL))
	return FALSE;

    _cairo_cache_remove (cache, entry);

    return TRUE;
}











static void
_cairo_cache_shrink_to_accommodate (cairo_cache_t *cache,
				    unsigned long  additional)
{
    if (cache->freeze_count)
	return;

    while (cache->size + additional > cache->max_size) {
	if (! _cairo_cache_remove_random (cache))
	    return;
    }
}













cairo_status_t
_cairo_cache_insert (cairo_cache_t	 *cache,
		     cairo_cache_entry_t *entry)
{
    cairo_status_t status;

    _cairo_cache_shrink_to_accommodate (cache, entry->size);

    status = _cairo_hash_table_insert (cache->hash_table,
				       (cairo_hash_entry_t *) entry);
    if (unlikely (status))
	return status;

    cache->size += entry->size;

    return CAIRO_STATUS_SUCCESS;
}








void
_cairo_cache_remove (cairo_cache_t	 *cache,
		     cairo_cache_entry_t *entry)
{
    cache->size -= entry->size;

    _cairo_hash_table_remove (cache->hash_table,
			      (cairo_hash_entry_t *) entry);

    if (cache->entry_destroy)
	cache->entry_destroy (entry);
}










void
_cairo_cache_foreach (cairo_cache_t		      *cache,
		      cairo_cache_callback_func_t      cache_callback,
		      void			      *closure)
{
    _cairo_hash_table_foreach (cache->hash_table,
			       cache_callback,
			       closure);
}

unsigned long
_cairo_hash_string (const char *c)
{
    
    unsigned long hash = _CAIRO_HASH_INIT_VALUE;
    while (c && *c)
	hash = ((hash << 5) + hash) + *c++;
    return hash;
}

unsigned long
_cairo_hash_bytes (unsigned long hash,
		   const void *ptr,
		   unsigned int length)
{
    const uint8_t *bytes = ptr;
    
    while (length--)
	hash = ((hash << 5) + hash) + *bytes++;
    return hash;
}
