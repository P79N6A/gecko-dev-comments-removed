







































#define _BSD_SOURCE
#include "cairoint.h"

static const cairo_font_face_backend_t _cairo_toy_font_face_backend;



const cairo_toy_font_face_t _cairo_font_face_nil = {
    {
    { 0 },				
    CAIRO_STATUS_NO_MEMORY,		
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    &_cairo_toy_font_face_backend
    },
    CAIRO_FONT_FAMILY_DEFAULT,		
    TRUE,				
    CAIRO_FONT_SLANT_DEFAULT,		
    CAIRO_FONT_WEIGHT_DEFAULT		
};

static const cairo_toy_font_face_t _cairo_font_face_null_pointer = {
    {
    { 0 },				
    CAIRO_STATUS_NULL_POINTER,		
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    &_cairo_toy_font_face_backend
    },
    CAIRO_FONT_FAMILY_DEFAULT,		
    TRUE,				
    CAIRO_FONT_SLANT_DEFAULT,		
    CAIRO_FONT_WEIGHT_DEFAULT		
};

static const cairo_toy_font_face_t _cairo_font_face_invalid_string = {
    {
    { 0 },				
    CAIRO_STATUS_INVALID_STRING,	
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    &_cairo_toy_font_face_backend
    },
    CAIRO_FONT_FAMILY_DEFAULT,		
    TRUE,				
    CAIRO_FONT_SLANT_DEFAULT,		
    CAIRO_FONT_WEIGHT_DEFAULT		
};

static const cairo_toy_font_face_t _cairo_font_face_invalid_slant = {
    {
    { 0 },				
    CAIRO_STATUS_INVALID_SLANT,		
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    &_cairo_toy_font_face_backend
    },
    CAIRO_FONT_FAMILY_DEFAULT,		
    TRUE,				
    CAIRO_FONT_SLANT_DEFAULT,		
    CAIRO_FONT_WEIGHT_DEFAULT		
};

static const cairo_toy_font_face_t _cairo_font_face_invalid_weight = {
    {
    { 0 },				
    CAIRO_STATUS_INVALID_WEIGHT,	
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    &_cairo_toy_font_face_backend
    },
    CAIRO_FONT_FAMILY_DEFAULT,		
    TRUE,				
    CAIRO_FONT_SLANT_DEFAULT,		
    CAIRO_FONT_WEIGHT_DEFAULT		
};

cairo_status_t
_cairo_font_face_set_error (cairo_font_face_t *font_face,
	                    cairo_status_t     status)
{
    if (status == CAIRO_STATUS_SUCCESS)
	return status;

    

    _cairo_status_set_error (&font_face->status, status);

    return _cairo_error (status);
}

void
_cairo_font_face_init (cairo_font_face_t               *font_face,
		       const cairo_font_face_backend_t *backend)
{
    CAIRO_MUTEX_INITIALIZE ();

    font_face->status = CAIRO_STATUS_SUCCESS;
    CAIRO_REFERENCE_COUNT_INIT (&font_face->ref_count, 1);
    font_face->backend = backend;

    _cairo_user_data_array_init (&font_face->user_data);
}















cairo_font_face_t *
cairo_font_face_reference (cairo_font_face_t *font_face)
{
    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return font_face;

    



    _cairo_reference_count_inc (&font_face->ref_count);

    return font_face;
}
slim_hidden_def (cairo_font_face_reference);









void
cairo_font_face_destroy (cairo_font_face_t *font_face)
{
    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&font_face->ref_count));

    if (! _cairo_reference_count_dec_and_test (&font_face->ref_count))
	return;

    if (font_face->backend->destroy)
	font_face->backend->destroy (font_face);

    



    if (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&font_face->ref_count))
	return;

    _cairo_user_data_array_fini (&font_face->user_data);

    free (font_face);
}
slim_hidden_def (cairo_font_face_destroy);












cairo_font_type_t
cairo_font_face_get_type (cairo_font_face_t *font_face)
{
    return font_face->backend->type;
}












unsigned int
cairo_font_face_get_reference_count (cairo_font_face_t *font_face)
{
    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&font_face->ref_count);
}











cairo_status_t
cairo_font_face_status (cairo_font_face_t *font_face)
{
    return font_face->status;
}













void *
cairo_font_face_get_user_data (cairo_font_face_t	   *font_face,
			       const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&font_face->user_data,
					    key);
}
slim_hidden_def (cairo_font_face_get_user_data);

















cairo_status_t
cairo_font_face_set_user_data (cairo_font_face_t	   *font_face,
			       const cairo_user_data_key_t *key,
			       void			   *user_data,
			       cairo_destroy_func_t	    destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return font_face->status;

    return _cairo_user_data_array_set_data (&font_face->user_data,
					    key, user_data, destroy);
}
slim_hidden_def (cairo_font_face_set_user_data);

static const cairo_font_face_backend_t _cairo_toy_font_face_backend;

static int
_cairo_toy_font_face_keys_equal (const void *key_a,
				 const void *key_b);











static cairo_hash_table_t *cairo_toy_font_face_hash_table = NULL;

static cairo_hash_table_t *
_cairo_toy_font_face_hash_table_lock (void)
{
    CAIRO_MUTEX_LOCK (_cairo_font_face_mutex);

    if (cairo_toy_font_face_hash_table == NULL)
    {
	cairo_toy_font_face_hash_table =
	    _cairo_hash_table_create (_cairo_toy_font_face_keys_equal);

	if (cairo_toy_font_face_hash_table == NULL) {
	    CAIRO_MUTEX_UNLOCK (_cairo_font_face_mutex);
	    return NULL;
	}
    }

    return cairo_toy_font_face_hash_table;
}

static void
_cairo_toy_font_face_hash_table_unlock (void)
{
    CAIRO_MUTEX_UNLOCK (_cairo_font_face_mutex);
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
_cairo_toy_font_face_init (cairo_toy_font_face_t *font_face,
			   const char	         *family,
			   cairo_font_slant_t	  slant,
			   cairo_font_weight_t	  weight)
{
    char *family_copy;

    family_copy = strdup (family);
    if (family_copy == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_toy_font_face_init_key (font_face, family_copy,
				      slant, weight);
    font_face->owns_family = TRUE;

    _cairo_font_face_init (&font_face->base, &_cairo_toy_font_face_backend);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_toy_font_face_fini (cairo_toy_font_face_t *font_face)
{
    

    assert (font_face->owns_family);
    free ((char*) font_face->family);
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
    if (status == CAIRO_STATUS_INVALID_STRING)
	return (cairo_font_face_t*) &_cairo_font_face_invalid_string;
    else if (status)
	return (cairo_font_face_t*) &_cairo_font_face_nil;

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
    if (hash_table == NULL)
	goto UNWIND;

    _cairo_toy_font_face_init_key (&key, family, slant, weight);

    
    if (_cairo_hash_table_lookup (hash_table,
				  &key.base.hash_entry,
				  (cairo_hash_entry_t **) &font_face))
    {
	if (! font_face->base.status)  {
	    

	    _cairo_reference_count_inc (&font_face->base.ref_count);
	    _cairo_toy_font_face_hash_table_unlock ();
	    return &font_face->base;
	}

	
	_cairo_hash_table_remove (hash_table, &key.base.hash_entry);
	font_face->base.hash_entry.hash = 0;
    }

    
    font_face = malloc (sizeof (cairo_toy_font_face_t));
    if (font_face == NULL) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto UNWIND_HASH_TABLE_LOCK;
    }

    status = _cairo_toy_font_face_init (font_face, family, slant, weight);
    if (status)
	goto UNWIND_FONT_FACE_MALLOC;

    status = _cairo_hash_table_insert (hash_table, &font_face->base.hash_entry);
    if (status)
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

    if (font_face->base.hash_entry.hash != 0)
	_cairo_hash_table_remove (hash_table, &font_face->base.hash_entry);

    _cairo_toy_font_face_hash_table_unlock ();

    _cairo_toy_font_face_fini (font_face);
}

static cairo_status_t
_cairo_toy_font_face_scaled_font_get_implementation (void                *abstract_font_face,
						     cairo_font_face_t **font_face_out)
{
    cairo_toy_font_face_t *font_face = abstract_font_face;
    cairo_status_t status;

    if (font_face->base.status)
	return font_face->base.status;

    if (CAIRO_SCALED_FONT_BACKEND_DEFAULT != &_cairo_user_scaled_font_backend)
    {
	const cairo_scaled_font_backend_t * backend = CAIRO_SCALED_FONT_BACKEND_DEFAULT;

	if (backend->get_implementation == NULL) {
	    *font_face_out = &font_face->base;
	    return CAIRO_STATUS_SUCCESS;
	}

	status = backend->get_implementation (font_face,
					      font_face_out);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_font_face_set_error (&font_face->base, status);
    }

    status = _cairo_user_scaled_font_backend.get_implementation (font_face,
								 font_face_out);

    return _cairo_font_face_set_error (&font_face->base, status);
}

static cairo_status_t
_cairo_toy_font_face_scaled_font_create (void                *abstract_font_face,
					 const cairo_matrix_t       *font_matrix,
					 const cairo_matrix_t       *ctm,
					 const cairo_font_options_t *options,
					 cairo_scaled_font_t	   **scaled_font)
{
    cairo_toy_font_face_t *font_face = abstract_font_face;
    cairo_status_t status;

    if (font_face->base.status)
	return font_face->base.status;

    status = cairo_font_options_status ((cairo_font_options_t *) options);
    if (status)
	return status;

    if (CAIRO_SCALED_FONT_BACKEND_DEFAULT != &_cairo_user_scaled_font_backend)
    {
	const cairo_scaled_font_backend_t * backend = CAIRO_SCALED_FONT_BACKEND_DEFAULT;

	*scaled_font = NULL;
	status =  backend->create_toy (font_face,
				       font_matrix,
				       ctm,
				       options,
				       scaled_font);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_font_face_set_error (&font_face->base, status);

	if (*scaled_font)
	    cairo_scaled_font_destroy (*scaled_font);
    }

    status = _cairo_user_scaled_font_backend.create_toy (font_face,
							 font_matrix,
							 ctm,
							 options,
							 scaled_font);

    return _cairo_font_face_set_error (&font_face->base, status);
}

static cairo_bool_t
_cairo_font_face_is_toy (cairo_font_face_t *font_face)
{
    return font_face->backend == &_cairo_toy_font_face_backend;
}












const char *
cairo_toy_font_face_get_family (cairo_font_face_t *font_face)
{
    cairo_toy_font_face_t *toy_font_face = (cairo_toy_font_face_t *) font_face;
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
    cairo_toy_font_face_t *toy_font_face = (cairo_toy_font_face_t *) font_face;
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
    cairo_toy_font_face_t *toy_font_face = (cairo_toy_font_face_t *) font_face;
    if (! _cairo_font_face_is_toy (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return CAIRO_FONT_WEIGHT_DEFAULT;
    }
    return toy_font_face->weight;
}
slim_hidden_def (cairo_toy_font_face_get_weight);

static const cairo_font_face_backend_t _cairo_toy_font_face_backend = {
    CAIRO_FONT_TYPE_TOY,
    _cairo_toy_font_face_destroy,
    _cairo_toy_font_face_scaled_font_get_implementation,
    _cairo_toy_font_face_scaled_font_create
};

void
_cairo_unscaled_font_init (cairo_unscaled_font_t               *unscaled_font,
			   const cairo_unscaled_font_backend_t *backend)
{
    CAIRO_REFERENCE_COUNT_INIT (&unscaled_font->ref_count, 1);
    unscaled_font->backend = backend;
}

cairo_unscaled_font_t *
_cairo_unscaled_font_reference (cairo_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return NULL;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&unscaled_font->ref_count));

    _cairo_reference_count_inc (&unscaled_font->ref_count);

    return unscaled_font;
}

void
_cairo_unscaled_font_destroy (cairo_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&unscaled_font->ref_count));

    if (! _cairo_reference_count_dec_and_test (&unscaled_font->ref_count))
	return;

    unscaled_font->backend->destroy (unscaled_font);

    free (unscaled_font);
}

void
_cairo_font_face_reset_static_data (void)
{
    _cairo_scaled_font_map_destroy ();

    


    CAIRO_MUTEX_LOCK (_cairo_font_face_mutex);
    _cairo_hash_table_destroy (cairo_toy_font_face_hash_table);
    cairo_toy_font_face_hash_table = NULL;
    CAIRO_MUTEX_UNLOCK (_cairo_font_face_mutex);
}
