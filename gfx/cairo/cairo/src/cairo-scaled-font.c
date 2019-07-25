







































#include "cairoint.h"
#include "cairo-error-private.h"
#include "cairo-scaled-font-private.h"

#if _XOPEN_SOURCE >= 600 || defined (_ISOC99_SOURCE)
#define ISFINITE(x) isfinite (x)
#else
#define ISFINITE(x) ((x) * (x) >= 0.) /* check for NaNs */
#endif

























#define MAX_GLYPH_PAGES_CACHED 256
static cairo_cache_t cairo_scaled_glyph_page_cache;

#define CAIRO_SCALED_GLYPH_PAGE_SIZE 32
struct _cairo_scaled_glyph_page {
    cairo_cache_entry_t cache_entry;

    cairo_list_t link;

    unsigned int num_glyphs;
    cairo_scaled_glyph_t glyphs[CAIRO_SCALED_GLYPH_PAGE_SIZE];
};










































































































static void
_cairo_scaled_font_fini_internal (cairo_scaled_font_t *scaled_font);

static void
_cairo_scaled_glyph_fini (cairo_scaled_font_t *scaled_font,
			  cairo_scaled_glyph_t *scaled_glyph)
{
    const cairo_surface_backend_t *surface_backend = scaled_font->surface_backend;

    if (surface_backend != NULL && surface_backend->scaled_glyph_fini != NULL)
	surface_backend->scaled_glyph_fini (scaled_glyph, scaled_font);

    if (scaled_glyph->surface != NULL)
	cairo_surface_destroy (&scaled_glyph->surface->base);

    if (scaled_glyph->path != NULL)
	_cairo_path_fixed_destroy (scaled_glyph->path);

    if (scaled_glyph->recording_surface != NULL) {
	cairo_surface_finish (scaled_glyph->recording_surface);
	cairo_surface_destroy (scaled_glyph->recording_surface);
    }
}

#define ZOMBIE 0
static const cairo_scaled_font_t _cairo_scaled_font_nil = {
    { ZOMBIE },			
    CAIRO_STATUS_NO_MEMORY,	
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },		
    NULL,			
    NULL,			
    { 1., 0., 0., 1., 0, 0},	
    { 1., 0., 0., 1., 0, 0},	
    { CAIRO_ANTIALIAS_DEFAULT,	
      CAIRO_SUBPIXEL_ORDER_DEFAULT,
      CAIRO_HINT_STYLE_DEFAULT,
      CAIRO_HINT_METRICS_DEFAULT} ,
    FALSE,			
    FALSE,			
    TRUE,			
    { 1., 0., 0., 1., 0, 0},	
    { 1., 0., 0., 1., 0, 0},	
    1.,				
    { 0., 0., 0., 0., 0. },	
    { 0., 0., 0., 0., 0. },	
    CAIRO_MUTEX_NIL_INITIALIZER,
    NULL,			
    { NULL, NULL },		
    FALSE,			
    FALSE,			
    NULL,			
    NULL,			
    NULL			
};




















cairo_status_t
_cairo_scaled_font_set_error (cairo_scaled_font_t *scaled_font,
			      cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS)
	return status;

    

    _cairo_status_set_error (&scaled_font->status, status);

    return _cairo_error (status);
}













cairo_font_type_t
cairo_scaled_font_get_type (cairo_scaled_font_t *scaled_font)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&scaled_font->ref_count))
	return CAIRO_FONT_TYPE_TOY;

    return scaled_font->backend->type;
}











cairo_status_t
cairo_scaled_font_status (cairo_scaled_font_t *scaled_font)
{
    return scaled_font->status;
}
slim_hidden_def (cairo_scaled_font_status);






















#define CAIRO_SCALED_FONT_MAX_HOLDOVERS 256

typedef struct _cairo_scaled_font_map {
    cairo_scaled_font_t *mru_scaled_font;
    cairo_hash_table_t *hash_table;
    cairo_scaled_font_t *holdovers[CAIRO_SCALED_FONT_MAX_HOLDOVERS];
    int num_holdovers;
} cairo_scaled_font_map_t;

static cairo_scaled_font_map_t *cairo_scaled_font_map;

static int
_cairo_scaled_font_keys_equal (const void *abstract_key_a, const void *abstract_key_b);

static cairo_scaled_font_map_t *
_cairo_scaled_font_map_lock (void)
{
    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);

    if (cairo_scaled_font_map == NULL) {
	cairo_scaled_font_map = malloc (sizeof (cairo_scaled_font_map_t));
	if (unlikely (cairo_scaled_font_map == NULL))
	    goto CLEANUP_MUTEX_LOCK;

	cairo_scaled_font_map->mru_scaled_font = NULL;
	cairo_scaled_font_map->hash_table =
	    _cairo_hash_table_create (_cairo_scaled_font_keys_equal);

	if (unlikely (cairo_scaled_font_map->hash_table == NULL))
	    goto CLEANUP_SCALED_FONT_MAP;

	cairo_scaled_font_map->num_holdovers = 0;
    }

    return cairo_scaled_font_map;

 CLEANUP_SCALED_FONT_MAP:
    free (cairo_scaled_font_map);
    cairo_scaled_font_map = NULL;
 CLEANUP_MUTEX_LOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
    _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
    return NULL;
}

static void
_cairo_scaled_font_map_unlock (void)
{
   CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
}

void
_cairo_scaled_font_map_destroy (void)
{
    cairo_scaled_font_map_t *font_map;
    cairo_scaled_font_t *scaled_font;

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);

    font_map = cairo_scaled_font_map;
    if (unlikely (font_map == NULL)) {
        goto CLEANUP_MUTEX_LOCK;
    }

    scaled_font = font_map->mru_scaled_font;
    if (scaled_font != NULL) {
	CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
	cairo_scaled_font_destroy (scaled_font);
	CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);
    }

    

    while (font_map->num_holdovers) {
	scaled_font = font_map->holdovers[font_map->num_holdovers-1];
	assert (! CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&scaled_font->ref_count));
	_cairo_hash_table_remove (font_map->hash_table,
				  &scaled_font->hash_entry);

	font_map->num_holdovers--;

	



	_cairo_scaled_font_fini (scaled_font);

	free (scaled_font);
    }

    _cairo_hash_table_destroy (font_map->hash_table);

    free (cairo_scaled_font_map);
    cairo_scaled_font_map = NULL;

 CLEANUP_MUTEX_LOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
}
static void
_cairo_scaled_glyph_page_destroy (void *closure)
{
    cairo_scaled_glyph_page_t *page = closure;
    cairo_scaled_font_t *scaled_font;
    unsigned int n;

    scaled_font = (cairo_scaled_font_t *) page->cache_entry.hash;
    for (n = 0; n < page->num_glyphs; n++) {
	_cairo_hash_table_remove (scaled_font->glyphs,
				  &page->glyphs[n].hash_entry);
	_cairo_scaled_glyph_fini (scaled_font, &page->glyphs[n]);
    }

    cairo_list_del (&page->link);

    free (page);
}

















cairo_status_t
_cairo_scaled_font_register_placeholder_and_unlock_font_map (cairo_scaled_font_t *scaled_font)
{
    cairo_status_t status;
    cairo_scaled_font_t *placeholder_scaled_font;

    assert (CAIRO_MUTEX_IS_LOCKED (_cairo_scaled_font_map_mutex));

    status = scaled_font->status;
    if (unlikely (status))
	return status;

    placeholder_scaled_font = malloc (sizeof (cairo_scaled_font_t));
    if (unlikely (placeholder_scaled_font == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    
    status = _cairo_scaled_font_init (placeholder_scaled_font,
				      scaled_font->font_face,
				      &scaled_font->font_matrix,
				      &scaled_font->ctm,
				      &scaled_font->options,
				      NULL);
    if (unlikely (status))
	goto FREE_PLACEHOLDER;

    placeholder_scaled_font->placeholder = TRUE;

    status = _cairo_hash_table_insert (cairo_scaled_font_map->hash_table,
				       &placeholder_scaled_font->hash_entry);
    if (unlikely (status))
	goto FINI_PLACEHOLDER;

    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
    CAIRO_MUTEX_LOCK (placeholder_scaled_font->mutex);

    return CAIRO_STATUS_SUCCESS;

  FINI_PLACEHOLDER:
    _cairo_scaled_font_fini_internal (placeholder_scaled_font);
  FREE_PLACEHOLDER:
    free (placeholder_scaled_font);

    return _cairo_scaled_font_set_error (scaled_font, status);
}

void
_cairo_scaled_font_unregister_placeholder_and_lock_font_map (cairo_scaled_font_t *scaled_font)
{
    cairo_scaled_font_t *placeholder_scaled_font;

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);

    placeholder_scaled_font =
	_cairo_hash_table_lookup (cairo_scaled_font_map->hash_table,
				  &scaled_font->hash_entry);
    assert (placeholder_scaled_font != NULL);
    assert (placeholder_scaled_font->placeholder);
    assert (CAIRO_MUTEX_IS_LOCKED (placeholder_scaled_font->mutex));

    _cairo_hash_table_remove (cairo_scaled_font_map->hash_table,
			      &placeholder_scaled_font->hash_entry);

    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);

    CAIRO_MUTEX_UNLOCK (placeholder_scaled_font->mutex);
    cairo_scaled_font_destroy (placeholder_scaled_font);

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);
}

static void
_cairo_scaled_font_placeholder_wait_for_creation_to_finish (cairo_scaled_font_t *placeholder_scaled_font)
{
    
    cairo_scaled_font_reference (placeholder_scaled_font);

    
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);

    
    CAIRO_MUTEX_LOCK (placeholder_scaled_font->mutex);

    
    CAIRO_MUTEX_UNLOCK (placeholder_scaled_font->mutex);
    cairo_scaled_font_destroy (placeholder_scaled_font);

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);
}







#define FNV_32_PRIME ((uint32_t)0x01000193)
#define FNV1_32_INIT ((uint32_t)0x811c9dc5)

static uint32_t
_hash_matrix_fnv (const cairo_matrix_t	*matrix,
		  uint32_t		 hval)
{
    const uint8_t *buffer = (const uint8_t *) matrix;
    int len = sizeof (cairo_matrix_t);
    do {
	hval *= FNV_32_PRIME;
	hval ^= *buffer++;
    } while (--len);

    return hval;
}

static uint32_t
_hash_mix_bits (uint32_t hash)
{
    hash += hash << 12;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return hash;
}

static void
_cairo_scaled_font_init_key (cairo_scaled_font_t        *scaled_font,
			     cairo_font_face_t	        *font_face,
			     const cairo_matrix_t       *font_matrix,
			     const cairo_matrix_t       *ctm,
			     const cairo_font_options_t *options)
{
    uint32_t hash = FNV1_32_INIT;

    scaled_font->status = CAIRO_STATUS_SUCCESS;
    scaled_font->placeholder = FALSE;
    scaled_font->font_face = font_face;
    scaled_font->font_matrix = *font_matrix;
    scaled_font->ctm = *ctm;
    
    scaled_font->ctm.x0 = 0.;
    scaled_font->ctm.y0 = 0.;
    _cairo_font_options_init_copy (&scaled_font->options, options);

    
    hash = _hash_matrix_fnv (&scaled_font->font_matrix, hash);
    hash = _hash_matrix_fnv (&scaled_font->ctm, hash);
    hash = _hash_mix_bits (hash);

    hash ^= (uintptr_t) scaled_font->font_face;
    hash ^= cairo_font_options_hash (&scaled_font->options);

    
    hash = _hash_mix_bits (hash);

    assert (hash != ZOMBIE);
    scaled_font->hash_entry.hash = hash;
}

static cairo_bool_t
_cairo_scaled_font_keys_equal (const void *abstract_key_a,
			       const void *abstract_key_b)
{
    const cairo_scaled_font_t *key_a = abstract_key_a;
    const cairo_scaled_font_t *key_b = abstract_key_b;

    if (key_a->hash_entry.hash != key_b->hash_entry.hash)
	return FALSE;

    return key_a->font_face == key_b->font_face &&
	    memcmp ((unsigned char *)(&key_a->font_matrix.xx),
		    (unsigned char *)(&key_b->font_matrix.xx),
		    sizeof(cairo_matrix_t)) == 0 &&
	    memcmp ((unsigned char *)(&key_a->ctm.xx),
		    (unsigned char *)(&key_b->ctm.xx),
		    sizeof(cairo_matrix_t)) == 0 &&
	    cairo_font_options_equal (&key_a->options, &key_b->options);
}

static cairo_bool_t
_cairo_scaled_font_matches (const cairo_scaled_font_t *scaled_font,
	                    const cairo_font_face_t *font_face,
			    const cairo_matrix_t *font_matrix,
			    const cairo_matrix_t *ctm,
			    const cairo_font_options_t *options)
{
    return scaled_font->original_font_face == font_face &&
	    memcmp ((unsigned char *)(&scaled_font->font_matrix.xx),
		    (unsigned char *)(&font_matrix->xx),
		    sizeof(cairo_matrix_t)) == 0 &&
	    memcmp ((unsigned char *)(&scaled_font->ctm.xx),
		    (unsigned char *)(&ctm->xx),
		    sizeof(cairo_matrix_t)) == 0 &&
	    cairo_font_options_equal (&scaled_font->options, options);
}

static cairo_bool_t
_cairo_scaled_glyphs_equal (const void *abstract_a, const void *abstract_b)
{
    const cairo_scaled_glyph_t *a = abstract_a;
    const cairo_scaled_glyph_t *b = abstract_b;

    return a->hash_entry.hash == b->hash_entry.hash;
}





cairo_status_t
_cairo_scaled_font_init (cairo_scaled_font_t               *scaled_font,
			 cairo_font_face_t		   *font_face,
			 const cairo_matrix_t              *font_matrix,
			 const cairo_matrix_t              *ctm,
			 const cairo_font_options_t	   *options,
			 const cairo_scaled_font_backend_t *backend)
{
    cairo_status_t status;

    status = cairo_font_options_status ((cairo_font_options_t *) options);
    if (unlikely (status))
	return status;

    _cairo_scaled_font_init_key (scaled_font, font_face,
				 font_matrix, ctm, options);

    cairo_matrix_multiply (&scaled_font->scale,
			   &scaled_font->font_matrix,
			   &scaled_font->ctm);

    scaled_font->max_scale = MAX (fabs (scaled_font->scale.xx) + fabs (scaled_font->scale.xy),
				  fabs (scaled_font->scale.yx) + fabs (scaled_font->scale.yy));
    scaled_font->scale_inverse = scaled_font->scale;
    status = cairo_matrix_invert (&scaled_font->scale_inverse);
    if (unlikely (status)) {
	








        if (_cairo_matrix_is_scale_0 (&scaled_font->scale)) {
	    cairo_matrix_init (&scaled_font->scale_inverse,
			       0, 0, 0, 0,
			       -scaled_font->scale.x0,
			       -scaled_font->scale.y0);
	} else
	    return status;
    }

    scaled_font->glyphs = _cairo_hash_table_create (_cairo_scaled_glyphs_equal);
    if (unlikely (scaled_font->glyphs == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    cairo_list_init (&scaled_font->glyph_pages);
    scaled_font->cache_frozen = FALSE;
    scaled_font->global_cache_frozen = FALSE;

    scaled_font->holdover = FALSE;
    scaled_font->finished = FALSE;

    CAIRO_REFERENCE_COUNT_INIT (&scaled_font->ref_count, 1);

    _cairo_user_data_array_init (&scaled_font->user_data);

    cairo_font_face_reference (font_face);
    scaled_font->original_font_face = NULL;

    CAIRO_MUTEX_INIT (scaled_font->mutex);

    scaled_font->surface_backend = NULL;
    scaled_font->surface_private = NULL;

    scaled_font->backend = backend;
    cairo_list_init (&scaled_font->link);

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_scaled_font_freeze_cache (cairo_scaled_font_t *scaled_font)
{
    
    assert (scaled_font->status == CAIRO_STATUS_SUCCESS);

    CAIRO_MUTEX_LOCK (scaled_font->mutex);
    scaled_font->cache_frozen = TRUE;
}

void
_cairo_scaled_font_thaw_cache (cairo_scaled_font_t *scaled_font)
{
    scaled_font->cache_frozen = FALSE;

    if (scaled_font->global_cache_frozen) {
	CAIRO_MUTEX_LOCK (_cairo_scaled_glyph_page_cache_mutex);
	_cairo_cache_thaw (&cairo_scaled_glyph_page_cache);
	CAIRO_MUTEX_UNLOCK (_cairo_scaled_glyph_page_cache_mutex);

	scaled_font->global_cache_frozen = FALSE;
    }

    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);
}

void
_cairo_scaled_font_reset_cache (cairo_scaled_font_t *scaled_font)
{
    assert (! scaled_font->cache_frozen);

    CAIRO_MUTEX_LOCK (_cairo_scaled_glyph_page_cache_mutex);
    while (! cairo_list_is_empty (&scaled_font->glyph_pages)) {
	_cairo_cache_remove (&cairo_scaled_glyph_page_cache,
                             &cairo_list_first_entry (&scaled_font->glyph_pages,
                                                      cairo_scaled_glyph_page_t,
                                                      link)->cache_entry);
    }
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_glyph_page_cache_mutex);
}

cairo_status_t
_cairo_scaled_font_set_metrics (cairo_scaled_font_t	    *scaled_font,
				cairo_font_extents_t	    *fs_metrics)
{
    cairo_status_t status;
    double  font_scale_x, font_scale_y;

    scaled_font->fs_extents = *fs_metrics;

    status = _cairo_matrix_compute_basis_scale_factors (&scaled_font->font_matrix,
						  &font_scale_x, &font_scale_y,
						  1);
    if (unlikely (status))
	return status;

    




    scaled_font->extents.ascent = fs_metrics->ascent * font_scale_y;
    scaled_font->extents.descent = fs_metrics->descent * font_scale_y;
    scaled_font->extents.height = fs_metrics->height * font_scale_y;
    scaled_font->extents.max_x_advance = fs_metrics->max_x_advance * font_scale_x;
    scaled_font->extents.max_y_advance = fs_metrics->max_y_advance * font_scale_y;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_scaled_font_fini_internal (cairo_scaled_font_t *scaled_font)
{
    scaled_font->finished = TRUE;

    _cairo_scaled_font_reset_cache (scaled_font);
    _cairo_hash_table_destroy (scaled_font->glyphs);

    cairo_font_face_destroy (scaled_font->font_face);
    cairo_font_face_destroy (scaled_font->original_font_face);

    CAIRO_MUTEX_FINI (scaled_font->mutex);

    if (scaled_font->surface_backend != NULL &&
	scaled_font->surface_backend->scaled_font_fini != NULL)
	scaled_font->surface_backend->scaled_font_fini (scaled_font);

    if (scaled_font->backend != NULL && scaled_font->backend->fini != NULL)
	scaled_font->backend->fini (scaled_font);

    _cairo_user_data_array_fini (&scaled_font->user_data);
}


void
_cairo_scaled_font_revoke_ownership (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font->surface_backend == NULL)
	return;

    _cairo_scaled_font_reset_cache (scaled_font);

    if (scaled_font->surface_backend->scaled_font_fini != NULL)
	scaled_font->surface_backend->scaled_font_fini (scaled_font);

    scaled_font->surface_backend = NULL;
    scaled_font->surface_private = NULL;
}

void
_cairo_scaled_font_fini (cairo_scaled_font_t *scaled_font)
{
    

    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
    _cairo_scaled_font_fini_internal (scaled_font);
    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);
}





















cairo_scaled_font_t *
cairo_scaled_font_create (cairo_font_face_t          *font_face,
			  const cairo_matrix_t       *font_matrix,
			  const cairo_matrix_t       *ctm,
			  const cairo_font_options_t *options)
{
    cairo_status_t status;
    cairo_scaled_font_map_t *font_map;
    cairo_font_face_t *original_font_face = font_face;
    cairo_scaled_font_t key, *old = NULL, *scaled_font = NULL, *dead = NULL;
    double det;

    status = font_face->status;
    if (unlikely (status))
	return _cairo_scaled_font_create_in_error (status);

    det = _cairo_matrix_compute_determinant (font_matrix);
    if (! ISFINITE (det))
	return _cairo_scaled_font_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_MATRIX));

    det = _cairo_matrix_compute_determinant (ctm);
    if (! ISFINITE (det))
	return _cairo_scaled_font_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_MATRIX));

    status = cairo_font_options_status ((cairo_font_options_t *) options);
    if (unlikely (status))
	return _cairo_scaled_font_create_in_error (status);

    


    font_map = _cairo_scaled_font_map_lock ();
    if (unlikely (font_map == NULL))
	return _cairo_scaled_font_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    scaled_font = font_map->mru_scaled_font;
    if (scaled_font != NULL &&
	_cairo_scaled_font_matches (scaled_font,
	                            font_face, font_matrix, ctm, options))
    {
	assert (scaled_font->hash_entry.hash != ZOMBIE);
	assert (! scaled_font->placeholder);

	if (likely (scaled_font->status == CAIRO_STATUS_SUCCESS)) {
	    



	    _cairo_reference_count_inc (&scaled_font->ref_count);
	    _cairo_scaled_font_map_unlock ();
	    return scaled_font;
	}

	
	_cairo_hash_table_remove (font_map->hash_table,
				  &scaled_font->hash_entry);
	scaled_font->hash_entry.hash = ZOMBIE;
	dead = scaled_font;
	font_map->mru_scaled_font = NULL;

	if (font_face->backend->get_implementation != NULL) {
	    font_face = font_face->backend->get_implementation (font_face,
								font_matrix,
								ctm,
								options);
	    if (unlikely (font_face->status)) {
		_cairo_scaled_font_map_unlock ();
		cairo_scaled_font_destroy (scaled_font);
		return _cairo_scaled_font_create_in_error (font_face->status);
	    }
	}

	_cairo_scaled_font_init_key (&key, font_face,
				     font_matrix, ctm, options);
    }
    else
    {
	if (font_face->backend->get_implementation != NULL) {
	    font_face = font_face->backend->get_implementation (font_face,
								font_matrix,
								ctm,
								options);
	    if (unlikely (font_face->status)) {
		_cairo_scaled_font_map_unlock ();
		return _cairo_scaled_font_create_in_error (font_face->status);
	    }
	}

	_cairo_scaled_font_init_key (&key, font_face,
				     font_matrix, ctm, options);

	while ((scaled_font = _cairo_hash_table_lookup (font_map->hash_table,
							&key.hash_entry)))
	{
	    if (! scaled_font->placeholder)
		break;

	    

	    _cairo_scaled_font_placeholder_wait_for_creation_to_finish (scaled_font);
	}

	
	if (scaled_font != NULL) {
	    




	    if (! CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&scaled_font->ref_count)) {
		if (scaled_font->holdover) {
		    int i;

		    for (i = 0; i < font_map->num_holdovers; i++) {
			if (font_map->holdovers[i] == scaled_font) {
			    font_map->num_holdovers--;
			    memmove (&font_map->holdovers[i],
				     &font_map->holdovers[i+1],
				     (font_map->num_holdovers - i) * sizeof (cairo_scaled_font_t*));
			    break;
			}
		    }

		    scaled_font->holdover = FALSE;
		}

		
		scaled_font->status = CAIRO_STATUS_SUCCESS;
	    }

	    if (likely (scaled_font->status == CAIRO_STATUS_SUCCESS)) {
		




		old = font_map->mru_scaled_font;
		font_map->mru_scaled_font = scaled_font;
		
		_cairo_reference_count_inc (&scaled_font->ref_count);
		
		_cairo_reference_count_inc (&scaled_font->ref_count);
		_cairo_scaled_font_map_unlock ();

		cairo_scaled_font_destroy (old);
		if (font_face != original_font_face)
		    cairo_font_face_destroy (font_face);

		return scaled_font;
	    }

	    
	    _cairo_hash_table_remove (font_map->hash_table,
				      &scaled_font->hash_entry);
	    scaled_font->hash_entry.hash = ZOMBIE;
	}
    }

    
    status = font_face->backend->scaled_font_create (font_face, font_matrix,
						     ctm, options, &scaled_font);
    
    if (unlikely (status)) {
	_cairo_scaled_font_map_unlock ();
	if (font_face != original_font_face)
	    cairo_font_face_destroy (font_face);

	if (dead != NULL)
	    cairo_scaled_font_destroy (dead);

	status = _cairo_font_face_set_error (font_face, status);
	return _cairo_scaled_font_create_in_error (status);
    }
    
    if (unlikely (scaled_font->status)) {
	_cairo_scaled_font_map_unlock ();
	if (font_face != original_font_face)
	    cairo_font_face_destroy (font_face);

	if (dead != NULL)
	    cairo_scaled_font_destroy (dead);

	return scaled_font;
    }

    



    assert (scaled_font->font_face == font_face);

    scaled_font->original_font_face =
	cairo_font_face_reference (original_font_face);

    status = _cairo_hash_table_insert (font_map->hash_table,
				       &scaled_font->hash_entry);
    if (likely (status == CAIRO_STATUS_SUCCESS)) {
	old = font_map->mru_scaled_font;
	font_map->mru_scaled_font = scaled_font;
	_cairo_reference_count_inc (&scaled_font->ref_count);
    }

    _cairo_scaled_font_map_unlock ();

    cairo_scaled_font_destroy (old);
    if (font_face != original_font_face)
	cairo_font_face_destroy (font_face);

    if (dead != NULL)
	cairo_scaled_font_destroy (dead);

    if (unlikely (status)) {
	


	_cairo_scaled_font_fini_internal (scaled_font);
	free (scaled_font);
	return _cairo_scaled_font_create_in_error (status);
    }

    return scaled_font;
}
slim_hidden_def (cairo_scaled_font_create);

static cairo_scaled_font_t *_cairo_scaled_font_nil_objects[CAIRO_STATUS_LAST_STATUS + 1];


cairo_scaled_font_t *
_cairo_scaled_font_create_in_error (cairo_status_t status)
{
    cairo_scaled_font_t *scaled_font;

    assert (status != CAIRO_STATUS_SUCCESS);

    if (status == CAIRO_STATUS_NO_MEMORY)
	return (cairo_scaled_font_t *) &_cairo_scaled_font_nil;

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_error_mutex);
    scaled_font = _cairo_scaled_font_nil_objects[status];
    if (unlikely (scaled_font == NULL)) {
	scaled_font = malloc (sizeof (cairo_scaled_font_t));
	if (unlikely (scaled_font == NULL)) {
	    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_error_mutex);
	    _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	    return (cairo_scaled_font_t *) &_cairo_scaled_font_nil;
	}

	*scaled_font = _cairo_scaled_font_nil;
	scaled_font->status = status;
	_cairo_scaled_font_nil_objects[status] = scaled_font;
    }
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_error_mutex);

    return scaled_font;
}

void
_cairo_scaled_font_reset_static_data (void)
{
    int status;

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_error_mutex);
    for (status = CAIRO_STATUS_SUCCESS;
	 status <= CAIRO_STATUS_LAST_STATUS;
	 status++)
    {
	if (_cairo_scaled_font_nil_objects[status] != NULL) {
	    free (_cairo_scaled_font_nil_objects[status]);
	    _cairo_scaled_font_nil_objects[status] = NULL;
	}
    }
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_error_mutex);

    CAIRO_MUTEX_LOCK (_cairo_scaled_glyph_page_cache_mutex);
    if (cairo_scaled_glyph_page_cache.hash_table != NULL) {
	_cairo_cache_fini (&cairo_scaled_glyph_page_cache);
	cairo_scaled_glyph_page_cache.hash_table = NULL;
    }
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_glyph_page_cache_mutex);
}















cairo_scaled_font_t *
cairo_scaled_font_reference (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&scaled_font->ref_count))
	return scaled_font;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&scaled_font->ref_count));

    _cairo_reference_count_inc (&scaled_font->ref_count);

    return scaled_font;
}
slim_hidden_def (cairo_scaled_font_reference);









void
cairo_scaled_font_destroy (cairo_scaled_font_t *scaled_font)
{
    cairo_scaled_font_t *lru = NULL;
    cairo_scaled_font_map_t *font_map;

    assert (CAIRO_MUTEX_IS_UNLOCKED (_cairo_scaled_font_map_mutex));

    if (scaled_font == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&scaled_font->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&scaled_font->ref_count));

    if (! _cairo_reference_count_dec_and_test (&scaled_font->ref_count))
	return;

    font_map = _cairo_scaled_font_map_lock ();
    assert (font_map != NULL);

    
    if (! CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&scaled_font->ref_count)) {
	if (! scaled_font->placeholder &&
	    scaled_font->hash_entry.hash != ZOMBIE)
	{
	    
	    if (scaled_font->holdover)
		goto unlock;

	    






	    if (font_map->num_holdovers == CAIRO_SCALED_FONT_MAX_HOLDOVERS) {
		lru = font_map->holdovers[0];
		assert (! CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&lru->ref_count));

		_cairo_hash_table_remove (font_map->hash_table,
					  &lru->hash_entry);

		font_map->num_holdovers--;
		memmove (&font_map->holdovers[0],
			 &font_map->holdovers[1],
			 font_map->num_holdovers * sizeof (cairo_scaled_font_t*));
	    }

	    font_map->holdovers[font_map->num_holdovers++] = scaled_font;
	    scaled_font->holdover = TRUE;
	} else
	    lru = scaled_font;
    }

  unlock:
    _cairo_scaled_font_map_unlock ();

    





    if (lru != NULL) {
	_cairo_scaled_font_fini_internal (lru);
	free (lru);
    }
}
slim_hidden_def (cairo_scaled_font_destroy);












unsigned int
cairo_scaled_font_get_reference_count (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&scaled_font->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&scaled_font->ref_count);
}















void *
cairo_scaled_font_get_user_data (cairo_scaled_font_t	     *scaled_font,
				 const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&scaled_font->user_data,
					    key);
}
slim_hidden_def (cairo_scaled_font_get_user_data);



















cairo_status_t
cairo_scaled_font_set_user_data (cairo_scaled_font_t	     *scaled_font,
				 const cairo_user_data_key_t *key,
				 void			     *user_data,
				 cairo_destroy_func_t	      destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&scaled_font->ref_count))
	return scaled_font->status;

    return _cairo_user_data_array_set_data (&scaled_font->user_data,
					    key, user_data, destroy);
}
slim_hidden_def (cairo_scaled_font_set_user_data);










void
cairo_scaled_font_extents (cairo_scaled_font_t  *scaled_font,
			   cairo_font_extents_t *extents)
{
    if (scaled_font->status) {
	extents->ascent  = 0.0;
	extents->descent = 0.0;
	extents->height  = 0.0;
	extents->max_x_advance = 0.0;
	extents->max_y_advance = 0.0;
	return;
    }

    *extents = scaled_font->extents;
}
slim_hidden_def (cairo_scaled_font_extents);
























void
cairo_scaled_font_text_extents (cairo_scaled_font_t   *scaled_font,
				const char            *utf8,
				cairo_text_extents_t  *extents)
{
    cairo_status_t status;
    cairo_glyph_t *glyphs = NULL;
    int num_glyphs;

    if (scaled_font->status)
	goto ZERO_EXTENTS;

    if (utf8 == NULL)
	goto ZERO_EXTENTS;

    status = cairo_scaled_font_text_to_glyphs (scaled_font, 0., 0.,
					       utf8, -1,
					       &glyphs, &num_glyphs,
					       NULL, NULL,
					       NULL);
    if (unlikely (status)) {
	status = _cairo_scaled_font_set_error (scaled_font, status);
	goto ZERO_EXTENTS;
    }

    cairo_scaled_font_glyph_extents (scaled_font, glyphs, num_glyphs, extents);
    free (glyphs);

    return;

ZERO_EXTENTS:
    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width  = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;
}



















void
cairo_scaled_font_glyph_extents (cairo_scaled_font_t   *scaled_font,
				 const cairo_glyph_t   *glyphs,
				 int                    num_glyphs,
				 cairo_text_extents_t  *extents)
{
    cairo_status_t status;
    int i;
    double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
    cairo_bool_t visible = FALSE;
    cairo_scaled_glyph_t *scaled_glyph = NULL;

    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width  = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;

    if (unlikely (scaled_font->status))
	goto ZERO_EXTENTS;

    if (num_glyphs == 0)
	goto ZERO_EXTENTS;

    if (unlikely (num_glyphs < 0)) {
	_cairo_error_throw (CAIRO_STATUS_NEGATIVE_COUNT);
	
	goto ZERO_EXTENTS;
    }

    if (unlikely (glyphs == NULL)) {
	_cairo_error_throw (CAIRO_STATUS_NULL_POINTER);
	
	goto ZERO_EXTENTS;
    }

    _cairo_scaled_font_freeze_cache (scaled_font);

    for (i = 0; i < num_glyphs; i++) {
	double			left, top, right, bottom;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_METRICS,
					     &scaled_glyph);
	if (unlikely (status)) {
	    status = _cairo_scaled_font_set_error (scaled_font, status);
	    goto UNLOCK;
	}

	
	if (scaled_glyph->metrics.width == 0 || scaled_glyph->metrics.height == 0)
	    continue;

	left = scaled_glyph->metrics.x_bearing + glyphs[i].x;
	right = left + scaled_glyph->metrics.width;
	top = scaled_glyph->metrics.y_bearing + glyphs[i].y;
	bottom = top + scaled_glyph->metrics.height;

	if (!visible) {
	    visible = TRUE;
	    min_x = left;
	    max_x = right;
	    min_y = top;
	    max_y = bottom;
	} else {
	    if (left < min_x) min_x = left;
	    if (right > max_x) max_x = right;
	    if (top < min_y) min_y = top;
	    if (bottom > max_y) max_y = bottom;
	}
    }

    if (visible) {
	extents->x_bearing = min_x - glyphs[0].x;
	extents->y_bearing = min_y - glyphs[0].y;
	extents->width = max_x - min_x;
	extents->height = max_y - min_y;
    } else {
	extents->x_bearing = 0.0;
	extents->y_bearing = 0.0;
	extents->width = 0.0;
	extents->height = 0.0;
    }

    if (num_glyphs) {
        double x0, y0, x1, y1;

	x0 = glyphs[0].x;
	y0 = glyphs[0].y;

	
	x1 = glyphs[num_glyphs - 1].x + scaled_glyph->metrics.x_advance;
	y1 = glyphs[num_glyphs - 1].y + scaled_glyph->metrics.y_advance;

	extents->x_advance = x1 - x0;
	extents->y_advance = y1 - y0;
    } else {
	extents->x_advance = 0.0;
	extents->y_advance = 0.0;
    }

 UNLOCK:
    _cairo_scaled_font_thaw_cache (scaled_font);
    return;

ZERO_EXTENTS:
    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width  = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;
}
slim_hidden_def (cairo_scaled_font_glyph_extents);

#define GLYPH_LUT_SIZE 64
static cairo_status_t
cairo_scaled_font_text_to_glyphs_internal_cached (cairo_scaled_font_t		 *scaled_font,
						    double			  x,
						    double			  y,
						    const char			 *utf8,
						    cairo_glyph_t		 *glyphs,
						    cairo_text_cluster_t	**clusters,
						    int				  num_chars)
{
    struct glyph_lut_elt {
	unsigned long index;
	double x_advance;
	double y_advance;
    } glyph_lut[GLYPH_LUT_SIZE];
    uint32_t glyph_lut_unicode[GLYPH_LUT_SIZE];
    cairo_status_t status;
    const char *p;
    int i;

    for (i = 0; i < GLYPH_LUT_SIZE; i++)
	glyph_lut_unicode[i] = ~0U;

    p = utf8;
    for (i = 0; i < num_chars; i++) {
	int idx, num_bytes;
	uint32_t unicode;
	cairo_scaled_glyph_t *scaled_glyph;
	struct glyph_lut_elt *glyph_slot;

	num_bytes = _cairo_utf8_get_char_validated (p, &unicode);
	p += num_bytes;

	glyphs[i].x = x;
	glyphs[i].y = y;

	idx = unicode % ARRAY_LENGTH (glyph_lut);
	glyph_slot = &glyph_lut[idx];
	if (glyph_lut_unicode[idx] == unicode) {
	    glyphs[i].index = glyph_slot->index;
	    x += glyph_slot->x_advance;
	    y += glyph_slot->y_advance;
	} else {
	    unsigned long g;

	    g = scaled_font->backend->ucs4_to_index (scaled_font, unicode);
	    status = _cairo_scaled_glyph_lookup (scaled_font,
						 g,
						 CAIRO_SCALED_GLYPH_INFO_METRICS,
						 &scaled_glyph);
	    if (unlikely (status))
		return status;

	    x += scaled_glyph->metrics.x_advance;
	    y += scaled_glyph->metrics.y_advance;

	    glyph_lut_unicode[idx] = unicode;
	    glyph_slot->index = g;
	    glyph_slot->x_advance = scaled_glyph->metrics.x_advance;
	    glyph_slot->y_advance = scaled_glyph->metrics.y_advance;

	    glyphs[i].index = g;
	}

	if (clusters) {
	    (*clusters)[i].num_bytes  = num_bytes;
	    (*clusters)[i].num_glyphs = 1;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
cairo_scaled_font_text_to_glyphs_internal_uncached (cairo_scaled_font_t	 *scaled_font,
						  double		  x,
						  double		  y,
						  const char		 *utf8,
						  cairo_glyph_t		 *glyphs,
						  cairo_text_cluster_t	**clusters,
						  int			  num_chars)
{
    const char *p;
    int i;

    p = utf8;
    for (i = 0; i < num_chars; i++) {
	unsigned long g;
	int num_bytes;
	uint32_t unicode;
	cairo_scaled_glyph_t *scaled_glyph;
	cairo_status_t status;

	num_bytes = _cairo_utf8_get_char_validated (p, &unicode);
	p += num_bytes;

	glyphs[i].x = x;
	glyphs[i].y = y;

	g = scaled_font->backend->ucs4_to_index (scaled_font, unicode);

	



	if (num_chars > 1) {
	    status = _cairo_scaled_glyph_lookup (scaled_font,
					     g,
					     CAIRO_SCALED_GLYPH_INFO_METRICS,
					     &scaled_glyph);
	    if (unlikely (status))
		return status;

	    x += scaled_glyph->metrics.x_advance;
	    y += scaled_glyph->metrics.y_advance;
	}

	glyphs[i].index = g;

	if (clusters) {
	    (*clusters)[i].num_bytes  = num_bytes;
	    (*clusters)[i].num_glyphs = 1;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}








































































































































#define CACHING_THRESHOLD 16
cairo_status_t
cairo_scaled_font_text_to_glyphs (cairo_scaled_font_t   *scaled_font,
				  double		 x,
				  double		 y,
				  const char	        *utf8,
				  int		         utf8_len,
				  cairo_glyph_t	       **glyphs,
				  int		        *num_glyphs,
				  cairo_text_cluster_t **clusters,
				  int		        *num_clusters,
				  cairo_text_cluster_flags_t *cluster_flags)
{
    int num_chars = 0;
    cairo_status_t status;
    cairo_glyph_t *orig_glyphs;
    cairo_text_cluster_t *orig_clusters;

    status = scaled_font->status;
    if (unlikely (status))
	return status;

    

    
    if (glyphs     == NULL ||
	num_glyphs == NULL) {
	status = _cairo_error (CAIRO_STATUS_NULL_POINTER);
	goto BAIL;
    }

    
    if (utf8 == NULL && utf8_len == -1)
	utf8_len = 0;

    
    if ((utf8_len && utf8          == NULL) ||
	(clusters && num_clusters  == NULL) ||
	(clusters && cluster_flags == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NULL_POINTER);
	goto BAIL;
    }

    
    if (utf8_len == -1)
	utf8_len = strlen (utf8);

    
    if (glyphs && *glyphs == NULL)
	*num_glyphs = 0;

    
    if (clusters && *clusters == NULL)
	*num_clusters = 0;

    if (!clusters && num_clusters) {
	num_clusters = NULL;
    }

    if (cluster_flags) {
	*cluster_flags = FALSE;
    }

    if (!clusters && cluster_flags) {
	cluster_flags = NULL;
    }

    
    if (utf8_len < 0 ||
	*num_glyphs < 0 ||
	(num_clusters && *num_clusters < 0)) {
	status = _cairo_error (CAIRO_STATUS_NEGATIVE_COUNT);
	goto BAIL;
    }

    if (utf8_len == 0) {
	status = CAIRO_STATUS_SUCCESS;
	goto BAIL;
    }

    
    status = _cairo_utf8_to_ucs4 (utf8, utf8_len, NULL, &num_chars);
    if (unlikely (status))
	goto BAIL;

    _cairo_scaled_font_freeze_cache (scaled_font);

    orig_glyphs = *glyphs;
    orig_clusters = clusters ? *clusters : NULL;

    if (scaled_font->backend->text_to_glyphs) {
	status = scaled_font->backend->text_to_glyphs (scaled_font, x, y,
						       utf8, utf8_len,
						       glyphs, num_glyphs,
						       clusters, num_clusters,
						       cluster_flags);
        if (status != CAIRO_INT_STATUS_UNSUPPORTED) {
	    if (status == CAIRO_STATUS_SUCCESS) {
	        



	        if (*num_glyphs < 0) {
		    status = _cairo_error (CAIRO_STATUS_NEGATIVE_COUNT);
		    goto DONE;
		}
		if (num_glyphs && *glyphs == NULL) {
		    status = _cairo_error (CAIRO_STATUS_NULL_POINTER);
		    goto DONE;
		}

		if (clusters) {
		    if (*num_clusters < 0) {
			status = _cairo_error (CAIRO_STATUS_NEGATIVE_COUNT);
			goto DONE;
		    }
		    if (num_clusters && *clusters == NULL) {
			status = _cairo_error (CAIRO_STATUS_NULL_POINTER);
			goto DONE;
		    }

		    
		    status =
			_cairo_validate_text_clusters (utf8, utf8_len,
						       *glyphs, *num_glyphs,
						       *clusters, *num_clusters,
						       *cluster_flags);
		}
	    }

            goto DONE;
	}
    }

    if (*num_glyphs < num_chars) {
	*glyphs = cairo_glyph_allocate (num_chars);
	if (unlikely (*glyphs == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto DONE;
	}
    }
    *num_glyphs = num_chars;

    if (clusters) {
	if (*num_clusters < num_chars) {
	    *clusters = cairo_text_cluster_allocate (num_chars);
	    if (unlikely (*clusters == NULL)) {
		status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		goto DONE;
	    }
	}
	*num_clusters = num_chars;
    }

    if (num_chars > CACHING_THRESHOLD)
	status = cairo_scaled_font_text_to_glyphs_internal_cached (scaled_font,
								     x, y,
								     utf8,
								     *glyphs,
								     clusters,
								     num_chars);
    else
	status = cairo_scaled_font_text_to_glyphs_internal_uncached (scaled_font,
								   x, y,
								   utf8,
								   *glyphs,
								   clusters,
								   num_chars);

 DONE: 
    _cairo_scaled_font_thaw_cache (scaled_font);

    if (unlikely (status)) {
	*num_glyphs = 0;
	if (*glyphs != orig_glyphs) {
	    cairo_glyph_free (*glyphs);
	    *glyphs = orig_glyphs;
	}

	if (clusters) {
	    *num_clusters = 0;
	    if (*clusters != orig_clusters) {
		cairo_text_cluster_free (*clusters);
		*clusters = orig_clusters;
	    }
	}
    }

    return _cairo_scaled_font_set_error (scaled_font, status);

 BAIL: 

    if (num_glyphs)
	*num_glyphs = 0;

    if (num_clusters)
	*num_clusters = 0;

    return status;
}
slim_hidden_def (cairo_scaled_font_text_to_glyphs);

static inline cairo_bool_t
_range_contains_glyph (const cairo_box_t *extents,
		       cairo_fixed_t left,
		       cairo_fixed_t top,
		       cairo_fixed_t right,
		       cairo_fixed_t bottom)
{
    return right > extents->p1.x &&
	   left < extents->p2.x &&
	   bottom > extents->p1.y &&
	   top < extents->p2.y;
}




cairo_status_t
_cairo_scaled_font_glyph_device_extents (cairo_scaled_font_t	 *scaled_font,
					 const cairo_glyph_t	 *glyphs,
					 int                      num_glyphs,
					 cairo_rectangle_int_t   *extents,
					 cairo_bool_t *overlap_out)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_box_t box = { { INT_MAX, INT_MAX }, { INT_MIN, INT_MIN }};
    cairo_scaled_glyph_t *glyph_cache[64];
    cairo_bool_t overlap = overlap_out ? FALSE : TRUE;
    cairo_round_glyph_positions_t round_glyph_positions = _cairo_font_options_get_round_glyph_positions (&scaled_font->options);
    int i;

    if (unlikely (scaled_font->status))
	return scaled_font->status;

    _cairo_scaled_font_freeze_cache (scaled_font);

    memset (glyph_cache, 0, sizeof (glyph_cache));

    for (i = 0; i < num_glyphs; i++) {
	cairo_scaled_glyph_t	*scaled_glyph;
	cairo_fixed_t x, y, x1, y1, x2, y2;
	int cache_index = glyphs[i].index % ARRAY_LENGTH (glyph_cache);

	scaled_glyph = glyph_cache[cache_index];
	if (scaled_glyph == NULL ||
	    _cairo_scaled_glyph_index (scaled_glyph) != glyphs[i].index)
	{
	    status = _cairo_scaled_glyph_lookup (scaled_font,
						 glyphs[i].index,
						 CAIRO_SCALED_GLYPH_INFO_METRICS,
						 &scaled_glyph);
	    if (unlikely (status))
		break;

	    glyph_cache[cache_index] = scaled_glyph;
	}

	if (round_glyph_positions == CAIRO_ROUND_GLYPH_POS_ON)
	    x = _cairo_fixed_from_int (_cairo_lround (glyphs[i].x));
	else
	    x = _cairo_fixed_from_double (glyphs[i].x);
	x1 = x + scaled_glyph->bbox.p1.x;
	x2 = x + scaled_glyph->bbox.p2.x;

	if (round_glyph_positions == CAIRO_ROUND_GLYPH_POS_ON)
	    y = _cairo_fixed_from_int (_cairo_lround (glyphs[i].y));
	else
	    y = _cairo_fixed_from_double (glyphs[i].y);
	y1 = y + scaled_glyph->bbox.p1.y;
	y2 = y + scaled_glyph->bbox.p2.y;

	if (overlap == FALSE)
	    overlap = _range_contains_glyph (&box, x1, y1, x2, y2);

	if (x1 < box.p1.x) box.p1.x = x1;
	if (x2 > box.p2.x) box.p2.x = x2;
	if (y1 < box.p1.y) box.p1.y = y1;
	if (y2 > box.p2.y) box.p2.y = y2;
    }

    _cairo_scaled_font_thaw_cache (scaled_font);
    if (unlikely (status))
	return _cairo_scaled_font_set_error (scaled_font, status);

    if (box.p1.x < box.p2.x) {
	_cairo_box_round_to_rectangle (&box, extents);
    } else {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
    }

    if (overlap_out != NULL)
	*overlap_out = overlap;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_scaled_font_glyph_approximate_extents (cairo_scaled_font_t	 *scaled_font,
					      const cairo_glyph_t	 *glyphs,
					      int                      num_glyphs,
					      cairo_rectangle_int_t   *extents)
{
    double x0 = HUGE_VAL, x1 = -HUGE_VAL;
    double y0 = HUGE_VAL, y1 = -HUGE_VAL;
    int i;

    for (i = 0; i < num_glyphs; i++) {
	double g;

	g = glyphs[i].x;
	if (g < x0) x0 = g;
	if (g > x1) x1 = g;

	g = glyphs[i].y;
	if (g < y0) y0 = g;
	if (g > y1) y1 = g;
    }

    if (x0 <= x1 && y0 <= y1) {
	extents->x = floor (x0 - scaled_font->extents.max_x_advance);
	extents->width = ceil (x1 + scaled_font->extents.max_x_advance);
	extents->width -= extents->x;

	extents->y = floor (y0 - scaled_font->extents.ascent);
	extents->height = ceil (y1 + scaled_font->extents.descent);
	extents->height -= extents->y;
    } else {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
    }
}

cairo_status_t
_cairo_scaled_font_show_glyphs (cairo_scaled_font_t	*scaled_font,
				cairo_operator_t	 op,
				const cairo_pattern_t	*pattern,
				cairo_surface_t		*surface,
				int			 source_x,
				int			 source_y,
				int			 dest_x,
				int			 dest_y,
				unsigned int		 width,
				unsigned int		 height,
				cairo_glyph_t		*glyphs,
				int			 num_glyphs,
				cairo_region_t		*clip_region)
{
    cairo_status_t status;
    cairo_surface_t *mask = NULL;
    cairo_format_t mask_format = CAIRO_FORMAT_A1; 
    cairo_surface_pattern_t mask_pattern;
    int i;

    


    assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);

    if (scaled_font->status)
	return scaled_font->status;

    if (!num_glyphs)
	return CAIRO_STATUS_SUCCESS;

    if (scaled_font->backend->show_glyphs != NULL) {
	int remaining_glyphs = num_glyphs;
	status = scaled_font->backend->show_glyphs (scaled_font,
						    op, pattern,
						    surface,
						    source_x, source_y,
						    dest_x, dest_y,
						    width, height,
						    glyphs, num_glyphs,
						    clip_region,
						    &remaining_glyphs);
	glyphs += num_glyphs - remaining_glyphs;
	num_glyphs = remaining_glyphs;
	if (remaining_glyphs == 0)
	    status = CAIRO_STATUS_SUCCESS;
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_scaled_font_set_error (scaled_font, status);
    }

    

    _cairo_scaled_font_freeze_cache (scaled_font);

    for (i = 0; i < num_glyphs; i++) {
	int x, y;
	cairo_image_surface_t *glyph_surface;
	cairo_scaled_glyph_t *scaled_glyph;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_SURFACE,
					     &scaled_glyph);

	if (unlikely (status))
	    goto CLEANUP_MASK;

	glyph_surface = scaled_glyph->surface;

	

	if (mask == NULL) {
	    mask_format = glyph_surface->format;
	    mask = cairo_image_surface_create (mask_format, width, height);
	    status = mask->status;
	    if (unlikely (status))
		goto CLEANUP_MASK;
	}

	

	if (glyph_surface->format != mask_format &&
	    _cairo_format_bits_per_pixel (mask_format) <
	    _cairo_format_bits_per_pixel (glyph_surface->format) )
	{
	    cairo_surface_t *new_mask;

	    switch (glyph_surface->format) {
	    case CAIRO_FORMAT_ARGB32:
	    case CAIRO_FORMAT_A8:
	    case CAIRO_FORMAT_A1:
		mask_format = glyph_surface->format;
		break;
	    case CAIRO_FORMAT_RGB16_565:
	    case CAIRO_FORMAT_RGB24:
	    case CAIRO_FORMAT_INVALID:
	    default:
		ASSERT_NOT_REACHED;
		mask_format = CAIRO_FORMAT_ARGB32;
		break;
	    }

	    new_mask = cairo_image_surface_create (mask_format, width, height);
	    status = new_mask->status;
	    if (unlikely (status)) {
		cairo_surface_destroy (new_mask);
		goto CLEANUP_MASK;
	    }

	    _cairo_pattern_init_for_surface (&mask_pattern, mask);
	    


	    status = _cairo_surface_composite (CAIRO_OPERATOR_ADD,
					       &_cairo_pattern_white.base,
					       &mask_pattern.base,
					       new_mask,
					       0, 0,
					       0, 0,
					       0, 0,
					       width, height,
					       NULL);

	    _cairo_pattern_fini (&mask_pattern.base);

	    if (unlikely (status)) {
		cairo_surface_destroy (new_mask);
		goto CLEANUP_MASK;
	    }

	    cairo_surface_destroy (mask);
	    mask = new_mask;
	}

	if (glyph_surface->width && glyph_surface->height) {
	    cairo_surface_pattern_t glyph_pattern;

	    
	    
	    x = _cairo_lround (glyphs[i].x -
			       glyph_surface->base.device_transform.x0);
	    y = _cairo_lround (glyphs[i].y -
			       glyph_surface->base.device_transform.y0);

	    _cairo_pattern_init_for_surface (&glyph_pattern,
					     &glyph_surface->base);
	    if (mask_format == CAIRO_FORMAT_ARGB32)
		glyph_pattern.base.has_component_alpha = TRUE;

	    status = _cairo_surface_composite (CAIRO_OPERATOR_ADD,
					       &_cairo_pattern_white.base,
					       &glyph_pattern.base,
					       mask,
					       0, 0,
					       0, 0,
					       x - dest_x, y - dest_y,
					       glyph_surface->width,
					       glyph_surface->height,
					       NULL);

	    _cairo_pattern_fini (&glyph_pattern.base);

	    if (unlikely (status))
		goto CLEANUP_MASK;
	}
    }

    _cairo_pattern_init_for_surface (&mask_pattern, mask);
    if (mask_format == CAIRO_FORMAT_ARGB32)
	mask_pattern.base.has_component_alpha = TRUE;

    status = _cairo_surface_composite (op, pattern, &mask_pattern.base,
				       surface,
				       source_x, source_y,
				       0,        0,
				       dest_x,   dest_y,
				       width,    height,
				       clip_region);

    _cairo_pattern_fini (&mask_pattern.base);

CLEANUP_MASK:
    _cairo_scaled_font_thaw_cache (scaled_font);

    if (mask != NULL)
	cairo_surface_destroy (mask);
    return _cairo_scaled_font_set_error (scaled_font, status);
}


static cairo_status_t
_add_unit_rectangle_to_path (cairo_path_fixed_t *path,
			     cairo_fixed_t x,
			     cairo_fixed_t y)
{
    cairo_status_t status;

    status = _cairo_path_fixed_move_to (path, x, y);
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_rel_line_to (path,
					    _cairo_fixed_from_int (1),
					    _cairo_fixed_from_int (0));
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_rel_line_to (path,
					    _cairo_fixed_from_int (0),
					    _cairo_fixed_from_int (1));
    if (unlikely (status))
	return status;

    status = _cairo_path_fixed_rel_line_to (path,
					    _cairo_fixed_from_int (-1),
					    _cairo_fixed_from_int (0));
    if (unlikely (status))
	return status;

    return _cairo_path_fixed_close_path (path);
}


















static cairo_status_t
_trace_mask_to_path (cairo_image_surface_t *mask,
		     cairo_path_fixed_t *path,
		     double tx, double ty)
{
    const uint8_t *row;
    int rows, cols, bytes_per_row;
    int x, y, bit;
    double xoff, yoff;
    cairo_fixed_t x0, y0;
    cairo_fixed_t px, py;
    cairo_status_t status;

    mask = _cairo_image_surface_coerce_to_format (mask, CAIRO_FORMAT_A1);
    status = mask->base.status;
    if (unlikely (status))
	return status;

    cairo_surface_get_device_offset (&mask->base, &xoff, &yoff);
    x0 = _cairo_fixed_from_double (tx - xoff);
    y0 = _cairo_fixed_from_double (ty - yoff);

    bytes_per_row = (mask->width + 7) / 8;
    row = mask->data;
    for (y = 0, rows = mask->height; rows--; row += mask->stride, y++) {
	const uint8_t *byte_ptr = row;
	x = 0;
	py = _cairo_fixed_from_int (y);
	for (cols = bytes_per_row; cols--; ) {
	    uint8_t byte = *byte_ptr++;
	    if (byte == 0) {
		x += 8;
		continue;
	    }

	    byte = CAIRO_BITSWAP8_IF_LITTLE_ENDIAN (byte);
	    for (bit = 1 << 7; bit && x < mask->width; bit >>= 1, x++) {
		if (byte & bit) {
		    px = _cairo_fixed_from_int (x);
		    status = _add_unit_rectangle_to_path (path,
							  px + x0,
							  py + y0);
		    if (unlikely (status))
			goto BAIL;
		}
	    }
	}
    }

BAIL:
    cairo_surface_destroy (&mask->base);

    return status;
}

cairo_status_t
_cairo_scaled_font_glyph_path (cairo_scaled_font_t *scaled_font,
			       const cairo_glyph_t *glyphs,
			       int		    num_glyphs,
			       cairo_path_fixed_t  *path)
{
    cairo_status_t status;
    int	i;

    status = scaled_font->status;
    if (unlikely (status))
	return status;

    _cairo_scaled_font_freeze_cache (scaled_font);
    for (i = 0; i < num_glyphs; i++) {
	cairo_scaled_glyph_t *scaled_glyph;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_PATH,
					     &scaled_glyph);
	if (status == CAIRO_STATUS_SUCCESS) {
	    status = _cairo_path_fixed_append (path,
					       scaled_glyph->path, CAIRO_DIRECTION_FORWARD,
					       _cairo_fixed_from_double (glyphs[i].x),
					       _cairo_fixed_from_double (glyphs[i].y));

	} else if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	    


	    status = _cairo_scaled_glyph_lookup (scaled_font,
						 glyphs[i].index,
						 CAIRO_SCALED_GLYPH_INFO_SURFACE,
						 &scaled_glyph);
	    if (unlikely (status))
		goto BAIL;

	    status = _trace_mask_to_path (scaled_glyph->surface, path,
					  glyphs[i].x, glyphs[i].y);
	}

	if (unlikely (status))
	    goto BAIL;
    }
  BAIL:
    _cairo_scaled_font_thaw_cache (scaled_font);

    return _cairo_scaled_font_set_error (scaled_font, status);
}












void
_cairo_scaled_glyph_set_metrics (cairo_scaled_glyph_t *scaled_glyph,
				 cairo_scaled_font_t *scaled_font,
				 cairo_text_extents_t *fs_metrics)
{
    cairo_bool_t first = TRUE;
    double hm, wm;
    double min_user_x = 0.0, max_user_x = 0.0, min_user_y = 0.0, max_user_y = 0.0;
    double min_device_x = 0.0, max_device_x = 0.0, min_device_y = 0.0, max_device_y = 0.0;
    double device_x_advance, device_y_advance;

    scaled_glyph->fs_metrics = *fs_metrics;

    for (hm = 0.0; hm <= 1.0; hm += 1.0)
	for (wm = 0.0; wm <= 1.0; wm += 1.0) {
	    double x, y;

	    
	    x = fs_metrics->x_bearing + fs_metrics->width * wm;
	    y = fs_metrics->y_bearing + fs_metrics->height * hm;
	    cairo_matrix_transform_point (&scaled_font->font_matrix,
					  &x, &y);
	    if (first) {
		min_user_x = max_user_x = x;
		min_user_y = max_user_y = y;
	    } else {
		if (x < min_user_x) min_user_x = x;
		if (x > max_user_x) max_user_x = x;
		if (y < min_user_y) min_user_y = y;
		if (y > max_user_y) max_user_y = y;
	    }

	    
	    x = fs_metrics->x_bearing + fs_metrics->width * wm;
	    y = fs_metrics->y_bearing + fs_metrics->height * hm;
	    cairo_matrix_transform_distance (&scaled_font->scale,
					     &x, &y);

	    if (first) {
		min_device_x = max_device_x = x;
		min_device_y = max_device_y = y;
	    } else {
		if (x < min_device_x) min_device_x = x;
		if (x > max_device_x) max_device_x = x;
		if (y < min_device_y) min_device_y = y;
		if (y > max_device_y) max_device_y = y;
	    }
	    first = FALSE;
	}
    scaled_glyph->metrics.x_bearing = min_user_x;
    scaled_glyph->metrics.y_bearing = min_user_y;
    scaled_glyph->metrics.width = max_user_x - min_user_x;
    scaled_glyph->metrics.height = max_user_y - min_user_y;

    scaled_glyph->metrics.x_advance = fs_metrics->x_advance;
    scaled_glyph->metrics.y_advance = fs_metrics->y_advance;
    cairo_matrix_transform_distance (&scaled_font->font_matrix,
				     &scaled_glyph->metrics.x_advance,
				     &scaled_glyph->metrics.y_advance);

    device_x_advance = fs_metrics->x_advance;
    device_y_advance = fs_metrics->y_advance;
    cairo_matrix_transform_distance (&scaled_font->scale,
				     &device_x_advance,
				     &device_y_advance);

    scaled_glyph->bbox.p1.x = _cairo_fixed_from_double (min_device_x);
    scaled_glyph->bbox.p1.y = _cairo_fixed_from_double (min_device_y);
    scaled_glyph->bbox.p2.x = _cairo_fixed_from_double (max_device_x);
    scaled_glyph->bbox.p2.y = _cairo_fixed_from_double (max_device_y);

    scaled_glyph->x_advance = _cairo_lround (device_x_advance);
    scaled_glyph->y_advance = _cairo_lround (device_y_advance);

    scaled_glyph->has_info |= CAIRO_SCALED_GLYPH_INFO_METRICS;
}

void
_cairo_scaled_glyph_set_surface (cairo_scaled_glyph_t *scaled_glyph,
				 cairo_scaled_font_t *scaled_font,
				 cairo_image_surface_t *surface)
{
    if (scaled_glyph->surface != NULL)
	cairo_surface_destroy (&scaled_glyph->surface->base);

    
    _cairo_debug_check_image_surface_is_defined (&surface->base);
    scaled_glyph->surface = surface;

    if (surface != NULL)
	scaled_glyph->has_info |= CAIRO_SCALED_GLYPH_INFO_SURFACE;
    else
	scaled_glyph->has_info &= ~CAIRO_SCALED_GLYPH_INFO_SURFACE;
}

void
_cairo_scaled_glyph_set_path (cairo_scaled_glyph_t *scaled_glyph,
			      cairo_scaled_font_t *scaled_font,
			      cairo_path_fixed_t *path)
{
    if (scaled_glyph->path != NULL)
	_cairo_path_fixed_destroy (scaled_glyph->path);

    scaled_glyph->path = path;

    if (path != NULL)
	scaled_glyph->has_info |= CAIRO_SCALED_GLYPH_INFO_PATH;
    else
	scaled_glyph->has_info &= ~CAIRO_SCALED_GLYPH_INFO_PATH;
}

void
_cairo_scaled_glyph_set_recording_surface (cairo_scaled_glyph_t *scaled_glyph,
					   cairo_scaled_font_t *scaled_font,
					   cairo_surface_t *recording_surface)
{
    if (scaled_glyph->recording_surface != NULL) {
	cairo_surface_finish (scaled_glyph->recording_surface);
	cairo_surface_destroy (scaled_glyph->recording_surface);
    }

    scaled_glyph->recording_surface = recording_surface;

    if (recording_surface != NULL)
	scaled_glyph->has_info |= CAIRO_SCALED_GLYPH_INFO_RECORDING_SURFACE;
    else
	scaled_glyph->has_info &= ~CAIRO_SCALED_GLYPH_INFO_RECORDING_SURFACE;
}

static cairo_bool_t
_cairo_scaled_glyph_page_can_remove (const void *closure)
{
    const cairo_scaled_glyph_page_t *page = closure;
    const cairo_scaled_font_t *scaled_font;

    scaled_font = (cairo_scaled_font_t *) page->cache_entry.hash;
    return scaled_font->cache_frozen == 0;
}

static cairo_status_t
_cairo_scaled_font_allocate_glyph (cairo_scaled_font_t *scaled_font,
				   cairo_scaled_glyph_t **scaled_glyph)
{
    cairo_scaled_glyph_page_t *page;
    cairo_status_t status;

    
    if (! cairo_list_is_empty (&scaled_font->glyph_pages)) {
        page = cairo_list_last_entry (&scaled_font->glyph_pages,
                                      cairo_scaled_glyph_page_t,
                                      link);
        if (page->num_glyphs < CAIRO_SCALED_GLYPH_PAGE_SIZE) {
            *scaled_glyph = &page->glyphs[page->num_glyphs++];
            return CAIRO_STATUS_SUCCESS;
        }
    }

    page = malloc (sizeof (cairo_scaled_glyph_page_t));
    if (unlikely (page == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    page->cache_entry.hash = (uintptr_t) scaled_font;
    page->cache_entry.size = 1; 
    page->num_glyphs = 0;

    CAIRO_MUTEX_LOCK (_cairo_scaled_glyph_page_cache_mutex);
    if (scaled_font->global_cache_frozen == FALSE) {
	if (unlikely (cairo_scaled_glyph_page_cache.hash_table == NULL)) {
	    status = _cairo_cache_init (&cairo_scaled_glyph_page_cache,
					NULL,
					_cairo_scaled_glyph_page_can_remove,
					_cairo_scaled_glyph_page_destroy,
					MAX_GLYPH_PAGES_CACHED);
	    if (unlikely (status)) {
		CAIRO_MUTEX_UNLOCK (_cairo_scaled_glyph_page_cache_mutex);
		free (page);
		return status;
	    }
	}

	_cairo_cache_freeze (&cairo_scaled_glyph_page_cache);
	scaled_font->global_cache_frozen = TRUE;
    }

    status = _cairo_cache_insert (&cairo_scaled_glyph_page_cache,
				  &page->cache_entry);
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_glyph_page_cache_mutex);
    if (unlikely (status)) {
	free (page);
	return status;
    }

    cairo_list_add_tail (&page->link, &scaled_font->glyph_pages);

    *scaled_glyph = &page->glyphs[page->num_glyphs++];
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_scaled_font_free_last_glyph (cairo_scaled_font_t *scaled_font,
			           cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_scaled_glyph_page_t *page;

    assert (! cairo_list_is_empty (&scaled_font->glyph_pages));
    page = cairo_list_last_entry (&scaled_font->glyph_pages,
                                  cairo_scaled_glyph_page_t,
                                  link);
    assert (scaled_glyph == &page->glyphs[page->num_glyphs-1]);

    _cairo_scaled_glyph_fini (scaled_font, scaled_glyph);

    if (--page->num_glyphs == 0) {
	_cairo_cache_remove (&cairo_scaled_glyph_page_cache,
		             &page->cache_entry);
    }
}




























cairo_int_status_t
_cairo_scaled_glyph_lookup (cairo_scaled_font_t *scaled_font,
			    unsigned long index,
			    cairo_scaled_glyph_info_t info,
			    cairo_scaled_glyph_t **scaled_glyph_ret)
{
    cairo_status_t		 status = CAIRO_STATUS_SUCCESS;
    cairo_scaled_glyph_t	*scaled_glyph;
    cairo_scaled_glyph_info_t	 need_info;

    *scaled_glyph_ret = NULL;

    if (unlikely (scaled_font->status))
	return scaled_font->status;

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    


    scaled_glyph = _cairo_hash_table_lookup (scaled_font->glyphs,
					     (cairo_hash_entry_t *) &index);
    if (scaled_glyph == NULL) {
	status = _cairo_scaled_font_allocate_glyph (scaled_font, &scaled_glyph);
	if (unlikely (status))
	    goto err;

	memset (scaled_glyph, 0, sizeof (cairo_scaled_glyph_t));
	_cairo_scaled_glyph_set_index (scaled_glyph, index);

	
	status =
	    scaled_font->backend->scaled_glyph_init (scaled_font,
						     scaled_glyph,
						     info | CAIRO_SCALED_GLYPH_INFO_METRICS);
	if (unlikely (status)) {
	    _cairo_scaled_font_free_last_glyph (scaled_font, scaled_glyph);
	    goto err;
	}

	status = _cairo_hash_table_insert (scaled_font->glyphs,
					   &scaled_glyph->hash_entry);
	if (unlikely (status)) {
	    _cairo_scaled_font_free_last_glyph (scaled_font, scaled_glyph);
	    goto err;
	}
    }

    



    need_info = info & ~scaled_glyph->has_info;
    if (need_info) {
	status = scaled_font->backend->scaled_glyph_init (scaled_font,
							  scaled_glyph,
							  need_info);
	if (unlikely (status))
	    goto err;

	



	if (info & ~scaled_glyph->has_info)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    *scaled_glyph_ret = scaled_glyph;
    return CAIRO_STATUS_SUCCESS;

err:
    
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	status = _cairo_scaled_font_set_error (scaled_font, status);
    return status;
}

double
_cairo_scaled_font_get_max_scale (cairo_scaled_font_t *scaled_font)
{
    return scaled_font->max_scale;
}














cairo_font_face_t *
cairo_scaled_font_get_font_face (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font->status)
	return (cairo_font_face_t*) &_cairo_font_face_nil;

    if (scaled_font->original_font_face != NULL)
	return scaled_font->original_font_face;

    return scaled_font->font_face;
}
slim_hidden_def (cairo_scaled_font_get_font_face);











void
cairo_scaled_font_get_font_matrix (cairo_scaled_font_t	*scaled_font,
				   cairo_matrix_t	*font_matrix)
{
    if (scaled_font->status) {
	cairo_matrix_init_identity (font_matrix);
	return;
    }

    *font_matrix = scaled_font->font_matrix;
}
slim_hidden_def (cairo_scaled_font_get_font_matrix);













void
cairo_scaled_font_get_ctm (cairo_scaled_font_t	*scaled_font,
			   cairo_matrix_t	*ctm)
{
    if (scaled_font->status) {
	cairo_matrix_init_identity (ctm);
	return;
    }

    *ctm = scaled_font->ctm;
}
slim_hidden_def (cairo_scaled_font_get_ctm);













void
cairo_scaled_font_get_scale_matrix (cairo_scaled_font_t	*scaled_font,
				    cairo_matrix_t	*scale_matrix)
{
    if (scaled_font->status) {
	cairo_matrix_init_identity (scale_matrix);
	return;
    }

    *scale_matrix = scaled_font->scale;
}











void
cairo_scaled_font_get_font_options (cairo_scaled_font_t		*scaled_font,
				    cairo_font_options_t	*options)
{
    if (cairo_font_options_status (options))
	return;

    if (scaled_font->status) {
	_cairo_font_options_init_default (options);
	return;
    }

    _cairo_font_options_init_copy (options, &scaled_font->options);
}
slim_hidden_def (cairo_scaled_font_get_font_options);
