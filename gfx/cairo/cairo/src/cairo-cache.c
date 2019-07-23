





































#include "cairoint.h"

static void
_cairo_cache_remove (cairo_cache_t	 *cache,
		     cairo_cache_entry_t *entry);

static void
_cairo_cache_shrink_to_accommodate (cairo_cache_t *cache,
				   unsigned long  additional);

static cairo_status_t
_cairo_cache_init (cairo_cache_t		*cache,
		   cairo_cache_keys_equal_func_t keys_equal,
		   cairo_destroy_func_t		 entry_destroy,
		   unsigned long		 max_size)
{
    cache->hash_table = _cairo_hash_table_create (keys_equal);
    if (cache->hash_table == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    cache->entry_destroy = entry_destroy;

    cache->max_size = max_size;
    cache->size = 0;

    cache->freeze_count = 0;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_cache_fini (cairo_cache_t *cache)
{
    cairo_cache_entry_t *entry;

    




    while (1) {
	entry = _cairo_hash_table_random_entry (cache->hash_table, NULL);
	if (entry == NULL)
	    break;
	_cairo_cache_remove (cache, entry);
    }

    _cairo_hash_table_destroy (cache->hash_table);
    cache->size = 0;
}




































cairo_cache_t *
_cairo_cache_create (cairo_cache_keys_equal_func_t keys_equal,
		     cairo_destroy_func_t	   entry_destroy,
		     unsigned long		   max_size)
{
    cairo_status_t status;
    cairo_cache_t *cache;

    cache = malloc (sizeof (cairo_cache_t));
    if (cache == NULL)
	return NULL;

    status = _cairo_cache_init (cache, keys_equal, entry_destroy, max_size);
    if (status) {
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















cairo_bool_t
_cairo_cache_lookup (cairo_cache_t	  *cache,
		     cairo_cache_entry_t  *key,
		     cairo_cache_entry_t **entry_return)
{
    return _cairo_hash_table_lookup (cache->hash_table,
				     (cairo_hash_entry_t *) key,
				     (cairo_hash_entry_t **) entry_return);
}











static cairo_int_status_t
_cairo_cache_remove_random (cairo_cache_t *cache)
{
    cairo_cache_entry_t *entry;

    entry = _cairo_hash_table_random_entry (cache->hash_table, NULL);
    if (entry == NULL)
	return CAIRO_INT_STATUS_CACHE_EMPTY;

    _cairo_cache_remove (cache, entry);

    return CAIRO_STATUS_SUCCESS;
}











static void
_cairo_cache_shrink_to_accommodate (cairo_cache_t *cache,
				   unsigned long  additional)
{
    cairo_int_status_t status;

    if (cache->freeze_count)
	return;

    while (cache->size + additional > cache->max_size) {
	status = _cairo_cache_remove_random (cache);
	if (status) {
	    if (status == CAIRO_INT_STATUS_CACHE_EMPTY)
		return;
	    ASSERT_NOT_REACHED;
	}
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
    if (status)
	return status;

    cache->size += entry->size;

    return CAIRO_STATUS_SUCCESS;
}













static void
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
_cairo_cache_foreach (cairo_cache_t	 	      *cache,
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
    
    unsigned long hash = 5381;
    while (c && *c)
	hash = ((hash << 5) + hash) + *c++;
    return hash;
}
