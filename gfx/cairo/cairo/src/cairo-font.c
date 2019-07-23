






































#include "cairoint.h"




static const cairo_font_face_backend_t _cairo_toy_font_face_backend;



const cairo_font_face_t _cairo_font_face_nil = {
    { 0 },			
    CAIRO_STATUS_NO_MEMORY,	
    CAIRO_REF_COUNT_INVALID,	
    { 0, 0, 0, NULL },		
    &_cairo_toy_font_face_backend
};

void
_cairo_font_face_init (cairo_font_face_t               *font_face,
		       const cairo_font_face_backend_t *backend)
{
    font_face->status = CAIRO_STATUS_SUCCESS;
    font_face->ref_count = 1;
    font_face->backend = backend;

    _cairo_user_data_array_init (&font_face->user_data);
}












cairo_font_face_t *
cairo_font_face_reference (cairo_font_face_t *font_face)
{
    if (font_face == NULL)
	return NULL;

    if (font_face->ref_count == CAIRO_REF_COUNT_INVALID)
	return font_face;

    



    font_face->ref_count++;

    return font_face;
}
slim_hidden_def (cairo_font_face_reference);









void
cairo_font_face_destroy (cairo_font_face_t *font_face)
{
    if (font_face == NULL)
	return;

    if (font_face->ref_count == CAIRO_REF_COUNT_INVALID)
	return;

    assert (font_face->ref_count > 0);

    if (--(font_face->ref_count) > 0)
	return;

    font_face->backend->destroy (font_face);

    



    if (font_face->ref_count > 0)
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

















cairo_status_t
cairo_font_face_set_user_data (cairo_font_face_t	   *font_face,
			       const cairo_user_data_key_t *key,
			       void			   *user_data,
			       cairo_destroy_func_t	    destroy)
{
    if (font_face->ref_count == CAIRO_REF_COUNT_INVALID)
	return CAIRO_STATUS_NO_MEMORY;

    return _cairo_user_data_array_set_data (&font_face->user_data,
					    key, user_data, destroy);
}

static const cairo_font_face_backend_t _cairo_toy_font_face_backend;

static int
_cairo_toy_font_face_keys_equal (const void *key_a,
				 const void *key_b);









static cairo_hash_table_t *cairo_toy_font_face_hash_table = NULL;

CAIRO_MUTEX_DECLARE (cairo_toy_font_face_hash_table_mutex);

static cairo_hash_table_t *
_cairo_toy_font_face_hash_table_lock (void)
{
    CAIRO_MUTEX_LOCK (cairo_toy_font_face_hash_table_mutex);

    if (cairo_toy_font_face_hash_table == NULL)
    {
	cairo_toy_font_face_hash_table =
	    _cairo_hash_table_create (_cairo_toy_font_face_keys_equal);

	if (cairo_toy_font_face_hash_table == NULL) {
	    CAIRO_MUTEX_UNLOCK (cairo_toy_font_face_hash_table_mutex);
	    return NULL;
	}
    }

    return cairo_toy_font_face_hash_table;
}

static void
_cairo_toy_font_face_hash_table_unlock (void)
{
    CAIRO_MUTEX_UNLOCK (cairo_toy_font_face_hash_table_mutex);
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
	return CAIRO_STATUS_NO_MEMORY;

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
_cairo_toy_font_face_create (const char          *family,
			     cairo_font_slant_t   slant,
			     cairo_font_weight_t  weight)
{
    cairo_status_t status;
    cairo_toy_font_face_t key, *font_face;
    cairo_hash_table_t *hash_table;

    hash_table = _cairo_toy_font_face_hash_table_lock ();
    if (hash_table == NULL)
	goto UNWIND;

    _cairo_toy_font_face_init_key (&key, family, slant, weight);

    
    if (_cairo_hash_table_lookup (hash_table,
				  &key.base.hash_entry,
				  (cairo_hash_entry_t **) &font_face))
    {
	_cairo_toy_font_face_hash_table_unlock ();
	return cairo_font_face_reference (&font_face->base);
    }

    
    font_face = malloc (sizeof (cairo_toy_font_face_t));
    if (font_face == NULL)
	goto UNWIND_HASH_TABLE_LOCK;

    status = _cairo_toy_font_face_init (font_face, family, slant, weight);
    if (status)
	goto UNWIND_FONT_FACE_MALLOC;

    status = _cairo_hash_table_insert (hash_table, &font_face->base.hash_entry);
    if (status)
	goto UNWIND_FONT_FACE_INIT;

    _cairo_toy_font_face_hash_table_unlock ();

    return &font_face->base;

 UNWIND_FONT_FACE_INIT:
 UNWIND_FONT_FACE_MALLOC:
    free (font_face);
 UNWIND_HASH_TABLE_LOCK:
    _cairo_toy_font_face_hash_table_unlock ();
 UNWIND:
    return (cairo_font_face_t*) &_cairo_font_face_nil;
}

static void
_cairo_toy_font_face_destroy (void *abstract_face)
{
    cairo_toy_font_face_t *font_face = abstract_face;
    cairo_hash_table_t *hash_table;

    if (font_face == NULL)
	return;

    hash_table = _cairo_toy_font_face_hash_table_lock ();
    
    assert (hash_table != NULL);

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
    cairo_toy_font_face_t *font_face = abstract_font_face;
    const cairo_scaled_font_backend_t * backend = CAIRO_SCALED_FONT_BACKEND_DEFAULT;

    return backend->create_toy (font_face,
				font_matrix, ctm, options, scaled_font);
}

static const cairo_font_face_backend_t _cairo_toy_font_face_backend = {
    CAIRO_FONT_TYPE_TOY,
    _cairo_toy_font_face_destroy,
    _cairo_toy_font_face_scaled_font_create
};

void
_cairo_unscaled_font_init (cairo_unscaled_font_t               *unscaled_font,
			   const cairo_unscaled_font_backend_t *backend)
{
    unscaled_font->ref_count = 1;
    unscaled_font->backend = backend;
}

cairo_unscaled_font_t *
_cairo_unscaled_font_reference (cairo_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return NULL;

    unscaled_font->ref_count++;

    return unscaled_font;
}

void
_cairo_unscaled_font_destroy (cairo_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return;

    if (--(unscaled_font->ref_count) > 0)
	return;

    unscaled_font->backend->destroy (unscaled_font);

    free (unscaled_font);
}

void
_cairo_font_reset_static_data (void)
{
    _cairo_scaled_font_map_destroy ();

    CAIRO_MUTEX_LOCK (cairo_toy_font_face_hash_table_mutex);
    _cairo_hash_table_destroy (cairo_toy_font_face_hash_table);
    cairo_toy_font_face_hash_table = NULL;
    CAIRO_MUTEX_UNLOCK (cairo_toy_font_face_hash_table_mutex);
}
