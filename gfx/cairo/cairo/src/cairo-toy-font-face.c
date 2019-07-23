








































#define _BSD_SOURCE
#include "cairoint.h"


static const cairo_font_face_t _cairo_font_face_null_pointer = {
    { 0 },				
    CAIRO_STATUS_NULL_POINTER,		
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    NULL
};

static const cairo_font_face_t _cairo_font_face_invalid_string = {
    { 0 },				
    CAIRO_STATUS_INVALID_STRING,	
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    NULL
};

static const cairo_font_face_t _cairo_font_face_invalid_slant = {
    { 0 },				
    CAIRO_STATUS_INVALID_SLANT,		
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    NULL
};

static const cairo_font_face_t _cairo_font_face_invalid_weight = {
    { 0 },				
    CAIRO_STATUS_INVALID_WEIGHT,	
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    NULL
};


static const cairo_font_face_backend_t _cairo_toy_font_face_backend;

static int
_cairo_toy_font_face_keys_equal (const void *key_a,
				 const void *key_b);











static cairo_hash_table_t *cairo_toy_font_face_hash_table = NULL;

static cairo_hash_table_t *
_cairo_toy_font_face_hash_table_lock (void)
{
    CAIRO_MUTEX_LOCK (_cairo_toy_font_face_mutex);

    if (cairo_toy_font_face_hash_table == NULL)
    {
	cairo_toy_font_face_hash_table =
	    _cairo_hash_table_create (_cairo_toy_font_face_keys_equal);

	if (cairo_toy_font_face_hash_table == NULL) {
	    CAIRO_MUTEX_UNLOCK (_cairo_toy_font_face_mutex);
	    return NULL;
	}
    }

    return cairo_toy_font_face_hash_table;
}

static void
_cairo_toy_font_face_hash_table_unlock (void)
{
    CAIRO_MUTEX_UNLOCK (_cairo_toy_font_face_mutex);
}










static void
_cairo_toy_font_face_init_key (cairo_toy_font_face_t *key,
			       const char	     *family,
			       cairo_font_slant_t     slant,
			       cairo_font_weight_t    weight)
{
    unsigned long hash;

    key->family = family;
    key->owns_family = FALSE;

    key->slant = slant;
    key->weight = weight;

    
    hash = _cairo_hash_string (family);
    hash += ((unsigned long) slant) * 1607;
    hash += ((unsigned long) weight) * 1451;

    assert (hash != 0);
    key->base.hash_entry.hash = hash;
}

static cairo_status_t
_cairo_toy_font_face_create_impl_face (cairo_toy_font_face_t *font_face,
				       cairo_font_face_t **impl_font_face)
{
    const cairo_font_face_backend_t * backend = CAIRO_FONT_FACE_BACKEND_DEFAULT;
    cairo_int_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;

    if (unlikely (font_face->base.status))
	return font_face->base.status;

    if (backend->create_for_toy != NULL &&
	0 != strncmp (font_face->family, CAIRO_USER_FONT_FAMILY_DEFAULT,
		      strlen (CAIRO_USER_FONT_FAMILY_DEFAULT)))
    {
	status = backend->create_for_toy (font_face, impl_font_face);
    }

    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	backend = &_cairo_user_font_face_backend;
	status = backend->create_for_toy (font_face, impl_font_face);
    }

    return status;
}

static cairo_status_t
_cairo_toy_font_face_init (cairo_toy_font_face_t *font_face,
			   const char	         *family,
			   cairo_font_slant_t	  slant,
			   cairo_font_weight_t	  weight)
{
    char *family_copy;
    cairo_status_t status;

    family_copy = strdup (family);
    if (unlikely (family_copy == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_toy_font_face_init_key (font_face, family_copy, slant, weight);
    font_face->owns_family = TRUE;

    _cairo_font_face_init (&font_face->base, &_cairo_toy_font_face_backend);

    status = _cairo_toy_font_face_create_impl_face (font_face,
						    &font_face->impl_face);
    if (unlikely (status)) {
	free (family_copy);
	return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_toy_font_face_fini (cairo_toy_font_face_t *font_face)
{
    

    assert (font_face->owns_family);
    free ((char*) font_face->family);

    if (font_face->impl_face)
	cairo_font_face_destroy (font_face->impl_face);
}

static int
_cairo_toy_font_face_keys_equal (const void *key_a,
				 const void *key_b)
{
    const cairo_toy_font_face_t *face_a = key_a;
    const cairo_toy_font_face_t *face_b = key_b;

    return (strcmp (face_a->family, face_b->family) == 0 &&
	    face_a->slant == face_b->slant &&
	    face_a->weight == face_b->weight);
}























cairo_font_face_t *
cairo_toy_font_face_create (const char          *family,
			    cairo_font_slant_t   slant,
			    cairo_font_weight_t  weight)
{
    cairo_status_t status;
    cairo_toy_font_face_t key, *font_face;
    cairo_hash_table_t *hash_table;

    if (family == NULL)
	return (cairo_font_face_t*) &_cairo_font_face_null_pointer;

    
    status = _cairo_utf8_to_ucs4 (family, -1, NULL, NULL);
    if (unlikely (status)) {
	if (status == CAIRO_STATUS_INVALID_STRING)
	    return (cairo_font_face_t*) &_cairo_font_face_invalid_string;

	return (cairo_font_face_t*) &_cairo_font_face_nil;
    }

    switch (slant) {
	case CAIRO_FONT_SLANT_NORMAL:
	case CAIRO_FONT_SLANT_ITALIC:
	case CAIRO_FONT_SLANT_OBLIQUE:
	    break;
	default:
	    return (cairo_font_face_t*) &_cairo_font_face_invalid_slant;
    }

    switch (weight) {
	case CAIRO_FONT_WEIGHT_NORMAL:
	case CAIRO_FONT_WEIGHT_BOLD:
	    break;
	default:
	    return (cairo_font_face_t*) &_cairo_font_face_invalid_weight;
    }

    if (*family == '\0')
	family = CAIRO_FONT_FAMILY_DEFAULT;

    hash_table = _cairo_toy_font_face_hash_table_lock ();
    if (unlikely (hash_table == NULL))
	goto UNWIND;

    _cairo_toy_font_face_init_key (&key, family, slant, weight);

    
    font_face = _cairo_hash_table_lookup (hash_table,
					  &key.base.hash_entry);
    if (font_face != NULL) {
	if (font_face->base.status == CAIRO_STATUS_SUCCESS) {
	    

	    _cairo_reference_count_inc (&font_face->base.ref_count);
	    _cairo_toy_font_face_hash_table_unlock ();
	    return &font_face->base;
	}

	
	_cairo_hash_table_remove (hash_table, &font_face->base.hash_entry);
	font_face->base.hash_entry.hash = 0;
    }

    
    font_face = malloc (sizeof (cairo_toy_font_face_t));
    if (unlikely (font_face == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto UNWIND_HASH_TABLE_LOCK;
    }

    status = _cairo_toy_font_face_init (font_face, family, slant, weight);
    if (unlikely (status))
	goto UNWIND_FONT_FACE_MALLOC;

    assert (font_face->base.hash_entry.hash == key.base.hash_entry.hash);
    status = _cairo_hash_table_insert (hash_table, &font_face->base.hash_entry);
    if (unlikely (status))
	goto UNWIND_FONT_FACE_INIT;

    _cairo_toy_font_face_hash_table_unlock ();

    return &font_face->base;

 UNWIND_FONT_FACE_INIT:
    _cairo_toy_font_face_fini (font_face);
 UNWIND_FONT_FACE_MALLOC:
    free (font_face);
 UNWIND_HASH_TABLE_LOCK:
    _cairo_toy_font_face_hash_table_unlock ();
 UNWIND:
    return (cairo_font_face_t*) &_cairo_font_face_nil;
}
slim_hidden_def (cairo_toy_font_face_create);

static void
_cairo_toy_font_face_destroy (void *abstract_face)
{
    cairo_toy_font_face_t *font_face = abstract_face;
    cairo_hash_table_t *hash_table;

    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->base.ref_count))
	return;

    hash_table = _cairo_toy_font_face_hash_table_lock ();
    
    assert (hash_table != NULL);

    if (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&font_face->base.ref_count)) {
	
	_cairo_toy_font_face_hash_table_unlock ();
	return;
    }

    if (font_face->base.hash_entry.hash != 0)
	_cairo_hash_table_remove (hash_table, &font_face->base.hash_entry);

    _cairo_toy_font_face_hash_table_unlock ();

    _cairo_toy_font_face_fini (font_face);
}

static cairo_status_t
_cairo_toy_font_face_scaled_font_create (void                *abstract_font_face,
					 const cairo_matrix_t       *font_matrix,
					 const cairo_matrix_t       *ctm,
					 const cairo_font_options_t *options,
					 cairo_scaled_font_t	   **scaled_font)
{
    cairo_toy_font_face_t *font_face = (cairo_toy_font_face_t *) abstract_font_face;

    ASSERT_NOT_REACHED;

    return _cairo_font_face_set_error (&font_face->base, CAIRO_STATUS_FONT_TYPE_MISMATCH);
}

static cairo_font_face_t *
_cairo_toy_font_face_get_implementation (void                *abstract_font_face,
					 const cairo_matrix_t       *font_matrix,
					 const cairo_matrix_t       *ctm,
					 const cairo_font_options_t *options)
{
    cairo_toy_font_face_t *font_face = abstract_font_face;

    if (font_face->impl_face) {
	cairo_font_face_t *impl = font_face->impl_face;

	if (impl->backend->get_implementation != NULL) {
	    return impl->backend->get_implementation (impl,
						      font_matrix,
						      ctm,
						      options);
	}

	return cairo_font_face_reference (impl);
    }

    return abstract_font_face;
}

static cairo_bool_t
_cairo_font_face_is_toy (cairo_font_face_t *font_face)
{
    return font_face->backend == &_cairo_toy_font_face_backend;
}












const char *
cairo_toy_font_face_get_family (cairo_font_face_t *font_face)
{
    cairo_toy_font_face_t *toy_font_face;

    if (font_face->status)
	return CAIRO_FONT_FAMILY_DEFAULT;

    toy_font_face = (cairo_toy_font_face_t *) font_face;
    if (! _cairo_font_face_is_toy (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return CAIRO_FONT_FAMILY_DEFAULT;
    }
    assert (toy_font_face->owns_family);
    return toy_font_face->family;
}











cairo_font_slant_t
cairo_toy_font_face_get_slant (cairo_font_face_t *font_face)
{
    cairo_toy_font_face_t *toy_font_face;

    if (font_face->status)
	return CAIRO_FONT_SLANT_DEFAULT;

    toy_font_face = (cairo_toy_font_face_t *) font_face;
    if (! _cairo_font_face_is_toy (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return CAIRO_FONT_SLANT_DEFAULT;
    }
    return toy_font_face->slant;
}
slim_hidden_def (cairo_toy_font_face_get_slant);











cairo_font_weight_t
cairo_toy_font_face_get_weight (cairo_font_face_t *font_face)
{
    cairo_toy_font_face_t *toy_font_face;

    if (font_face->status)
	return CAIRO_FONT_WEIGHT_DEFAULT;

    toy_font_face = (cairo_toy_font_face_t *) font_face;
    if (! _cairo_font_face_is_toy (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return CAIRO_FONT_WEIGHT_DEFAULT;
    }
    return toy_font_face->weight;
}
slim_hidden_def (cairo_toy_font_face_get_weight);

static const cairo_font_face_backend_t _cairo_toy_font_face_backend = {
    CAIRO_FONT_TYPE_TOY,
    NULL,					
    _cairo_toy_font_face_destroy,
    _cairo_toy_font_face_scaled_font_create,
    _cairo_toy_font_face_get_implementation
};

void
_cairo_toy_font_face_reset_static_data (void)
{
    cairo_hash_table_t *hash_table;

    


    CAIRO_MUTEX_LOCK (_cairo_toy_font_face_mutex);
    hash_table = cairo_toy_font_face_hash_table;
    cairo_toy_font_face_hash_table = NULL;
    CAIRO_MUTEX_UNLOCK (_cairo_toy_font_face_mutex);

    _cairo_hash_table_destroy (hash_table);
}
