





































#ifndef CAIRO_HASH_PRIVATE_H
#define CAIRO_HASH_PRIVATE_H






typedef struct _cairo_hash_table cairo_hash_table_t;



































typedef struct _cairo_hash_entry {
    unsigned long hash;
} cairo_hash_entry_t;

typedef cairo_bool_t
(*cairo_hash_keys_equal_func_t) (const void *key_a, const void *key_b);

typedef cairo_bool_t
(*cairo_hash_predicate_func_t) (void *entry);

typedef void
(*cairo_hash_callback_func_t) (void *entry,
			       void *closure);

cairo_private cairo_hash_table_t *
_cairo_hash_table_create (cairo_hash_keys_equal_func_t keys_equal);

cairo_private void
_cairo_hash_table_destroy (cairo_hash_table_t *hash_table);

cairo_private cairo_bool_t
_cairo_hash_table_lookup (cairo_hash_table_t  *hash_table,
			  cairo_hash_entry_t  *key,
			  cairo_hash_entry_t **entry_return);

cairo_private void *
_cairo_hash_table_random_entry (cairo_hash_table_t	   *hash_table,
				cairo_hash_predicate_func_t predicate);

cairo_private cairo_status_t
_cairo_hash_table_insert (cairo_hash_table_t *hash_table,
			  cairo_hash_entry_t *entry);

cairo_private void
_cairo_hash_table_remove (cairo_hash_table_t *hash_table,
			  cairo_hash_entry_t *key);

cairo_private void
_cairo_hash_table_foreach (cairo_hash_table_t 	      *hash_table,
			   cairo_hash_callback_func_t  hash_callback,
			   void			      *closure);

#endif
