





































#include "cairoint.h"















#define DEAD_ENTRY ((cairo_hash_entry_t *) 0x1)

#define ENTRY_IS_FREE(entry) ((entry) == NULL)
#define ENTRY_IS_DEAD(entry) ((entry) == DEAD_ENTRY)
#define ENTRY_IS_LIVE(entry) ((entry) >  DEAD_ENTRY)























typedef struct _cairo_hash_table_arrangement {
    unsigned long high_water_mark;
    unsigned long size;
    unsigned long rehash;
} cairo_hash_table_arrangement_t;

static const cairo_hash_table_arrangement_t hash_table_arrangements [] = {
    { 16,		43,		41		},
    { 32,		73,		71		},
    { 64,		151,		149		},
    { 128,		283,		281		},
    { 256,		571,		569		},
    { 512,		1153,		1151		},
    { 1024,		2269,		2267		},
    { 2048,		4519,		4517		},
    { 4096,		9013,		9011		},
    { 8192,		18043,		18041		},
    { 16384,		36109,		36107		},
    { 32768,		72091,		72089		},
    { 65536,		144409,		144407		},
    { 131072,		288361,		288359		},
    { 262144,		576883,		576881		},
    { 524288,		1153459,	1153457		},
    { 1048576,		2307163,	2307161		},
    { 2097152,		4613893,	4613891		},
    { 4194304,		9227641,	9227639		},
    { 8388608,		18455029,	18455027	},
    { 16777216,		36911011,	36911009	},
    { 33554432,		73819861,	73819859	},
    { 67108864,		147639589,	147639587	},
    { 134217728,	295279081,	295279079	},
    { 268435456,	590559793,	590559791	}
};

#define NUM_HASH_TABLE_ARRANGEMENTS ARRAY_LENGTH (hash_table_arrangements)

struct _cairo_hash_table {
    cairo_hash_keys_equal_func_t keys_equal;

    const cairo_hash_table_arrangement_t *arrangement;
    cairo_hash_entry_t **entries;

    unsigned long live_entries;
    unsigned long iterating;   
};

















cairo_hash_table_t *
_cairo_hash_table_create (cairo_hash_keys_equal_func_t keys_equal)
{
    cairo_hash_table_t *hash_table;

    hash_table = malloc (sizeof (cairo_hash_table_t));
    if (unlikely (hash_table == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    hash_table->keys_equal = keys_equal;

    hash_table->arrangement = &hash_table_arrangements[0];

    hash_table->entries = calloc (hash_table->arrangement->size,
				  sizeof(cairo_hash_entry_t *));
    if (unlikely (hash_table->entries == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	free (hash_table);
	return NULL;
    }

    hash_table->live_entries = 0;
    hash_table->iterating = 0;

    return hash_table;
}


















void
_cairo_hash_table_destroy (cairo_hash_table_t *hash_table)
{
    
    assert (hash_table->live_entries == 0);
    
    assert (hash_table->iterating == 0);

    free (hash_table->entries);
    hash_table->entries = NULL;

    free (hash_table);
}

static cairo_hash_entry_t **
_cairo_hash_table_lookup_unique_key (cairo_hash_table_t *hash_table,
				     cairo_hash_entry_t *key)
{
    unsigned long table_size, i, idx, step;
    cairo_hash_entry_t **entry;

    table_size = hash_table->arrangement->size;
    idx = key->hash % table_size;

    entry = &hash_table->entries[idx];
    if (! ENTRY_IS_LIVE (*entry))
	return entry;

    i = 1;
    step = key->hash % hash_table->arrangement->rehash;
    if (step == 0)
	step = 1;
    do {
	idx += step;
	if (idx >= table_size)
	    idx -= table_size;

	entry = &hash_table->entries[idx];
	if (! ENTRY_IS_LIVE (*entry))
	    return entry;
    } while (++i < table_size);

    ASSERT_NOT_REACHED;
    return NULL;
}












static cairo_status_t
_cairo_hash_table_resize (cairo_hash_table_t *hash_table)
{
    cairo_hash_table_t tmp;
    unsigned long new_size, i;

    
    unsigned long high = hash_table->arrangement->high_water_mark;
    unsigned long low = high >> 2;

    if (hash_table->live_entries >= low && hash_table->live_entries <= high)
	return CAIRO_STATUS_SUCCESS;

    tmp = *hash_table;

    if (hash_table->live_entries > high)
    {
	tmp.arrangement = hash_table->arrangement + 1;
	
	assert (tmp.arrangement - hash_table_arrangements <
		NUM_HASH_TABLE_ARRANGEMENTS);
    }
    else 
    {
	
	if (hash_table->arrangement == &hash_table_arrangements[0])
	    return CAIRO_STATUS_SUCCESS;
	tmp.arrangement = hash_table->arrangement - 1;
    }

    new_size = tmp.arrangement->size;
    tmp.entries = calloc (new_size, sizeof (cairo_hash_entry_t*));
    if (unlikely (tmp.entries == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    for (i = 0; i < hash_table->arrangement->size; ++i) {
	if (ENTRY_IS_LIVE (hash_table->entries[i])) {
	    *_cairo_hash_table_lookup_unique_key (&tmp, hash_table->entries[i])
		= hash_table->entries[i];
	}
    }

    free (hash_table->entries);
    hash_table->entries = tmp.entries;
    hash_table->arrangement = tmp.arrangement;

    return CAIRO_STATUS_SUCCESS;
}












void *
_cairo_hash_table_lookup (cairo_hash_table_t *hash_table,
			  cairo_hash_entry_t *key)
{
    cairo_hash_entry_t *entry;
    unsigned long table_size, i, idx, step;

    table_size = hash_table->arrangement->size;
    idx = key->hash % table_size;

    entry = hash_table->entries[idx];
    if (ENTRY_IS_LIVE (entry)) {
	if (hash_table->keys_equal (key, entry))
	    return entry;
    } else if (ENTRY_IS_FREE (entry))
	return NULL;

    i = 1;
    step = key->hash % hash_table->arrangement->rehash;
    if (step == 0)
	step = 1;
    do {
	idx += step;
	if (idx >= table_size)
	    idx -= table_size;

	entry = hash_table->entries[idx];
	if (ENTRY_IS_LIVE (entry)) {
	    if (hash_table->keys_equal (key, entry))
		return entry;
	} else if (ENTRY_IS_FREE (entry))
	    return NULL;
    } while (++i < table_size);

    return NULL;
}




















void *
_cairo_hash_table_random_entry (cairo_hash_table_t	   *hash_table,
				cairo_hash_predicate_func_t predicate)
{
    cairo_hash_entry_t *entry;
    unsigned long hash;
    unsigned long table_size, i, idx, step;

    assert (predicate != NULL);

    table_size = hash_table->arrangement->size;
    hash = rand ();
    idx = hash % table_size;

    entry = hash_table->entries[idx];
    if (ENTRY_IS_LIVE (entry) && predicate (entry))
	return entry;

    i = 1;
    step = hash % hash_table->arrangement->rehash;
    if (step == 0)
	step = 1;
    do {
	idx += step;
	if (idx >= table_size)
	    idx -= table_size;

	entry = hash_table->entries[idx];
	if (ENTRY_IS_LIVE (entry) && predicate (entry))
	    return entry;
    } while (++i < table_size);

    return NULL;
}





















cairo_status_t
_cairo_hash_table_insert (cairo_hash_table_t *hash_table,
			  cairo_hash_entry_t *key_and_value)
{
    cairo_status_t status;

    
    assert (hash_table->iterating == 0);

    hash_table->live_entries++;
    status = _cairo_hash_table_resize (hash_table);
    if (unlikely (status)) {
	
	hash_table->live_entries--;
	return status;
    }

    *_cairo_hash_table_lookup_unique_key (hash_table,
					  key_and_value) = key_and_value;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_hash_entry_t **
_cairo_hash_table_lookup_exact_key (cairo_hash_table_t *hash_table,
				    cairo_hash_entry_t *key)
{
    unsigned long table_size, i, idx, step;
    cairo_hash_entry_t **entry;

    table_size = hash_table->arrangement->size;
    idx = key->hash % table_size;

    entry = &hash_table->entries[idx];
    if (*entry == key)
	return entry;

    i = 1;
    step = key->hash % hash_table->arrangement->rehash;
    if (step == 0)
	step = 1;
    do {
	idx += step;
	if (idx >= table_size)
	    idx -= table_size;

	entry = &hash_table->entries[idx];
	if (*entry == key)
	    return entry;
    } while (++i < table_size);

    ASSERT_NOT_REACHED;
    return NULL;
}










void
_cairo_hash_table_remove (cairo_hash_table_t *hash_table,
			  cairo_hash_entry_t *key)
{
    *_cairo_hash_table_lookup_exact_key (hash_table, key) = DEAD_ENTRY;
    hash_table->live_entries--;

    


    if (hash_table->iterating == 0) {
	



	_cairo_hash_table_resize (hash_table);
    }
}
















void
_cairo_hash_table_foreach (cairo_hash_table_t	      *hash_table,
			   cairo_hash_callback_func_t  hash_callback,
			   void			      *closure)
{
    unsigned long i;
    cairo_hash_entry_t *entry;

    
    ++hash_table->iterating;
    for (i = 0; i < hash_table->arrangement->size; i++) {
	entry = hash_table->entries[i];
	if (ENTRY_IS_LIVE(entry))
	    hash_callback (entry, closure);
    }
    



    if (--hash_table->iterating == 0) {
	

	_cairo_hash_table_resize (hash_table);
    }
}
