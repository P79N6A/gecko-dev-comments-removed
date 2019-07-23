





































#include "cairoint.h"
#include "cairo-scaled-font-test.h"

static cairo_bool_t
_cairo_scaled_glyph_keys_equal (const void *abstract_key_a, const void *abstract_key_b)
{
    const cairo_scaled_glyph_t *key_a = abstract_key_a;
    const cairo_scaled_glyph_t *key_b = abstract_key_b;

    return (_cairo_scaled_glyph_index (key_a) ==
	    _cairo_scaled_glyph_index (key_b));
}

static void
_cairo_scaled_glyph_fini (cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_scaled_font_t	*scaled_font = scaled_glyph->scaled_font;
    const cairo_surface_backend_t *surface_backend = scaled_font->surface_backend;

    if (surface_backend != NULL && surface_backend->scaled_glyph_fini != NULL)
	surface_backend->scaled_glyph_fini (scaled_glyph, scaled_font);
    if (scaled_glyph->surface != NULL)
	cairo_surface_destroy (&scaled_glyph->surface->base);
    if (scaled_glyph->path != NULL)
	_cairo_path_fixed_destroy (scaled_glyph->path);
}

static void
_cairo_scaled_glyph_destroy (void *abstract_glyph)
{
    cairo_scaled_glyph_t *scaled_glyph = abstract_glyph;
    _cairo_scaled_glyph_fini (scaled_glyph);
    free (scaled_glyph);
}

const cairo_scaled_font_t _cairo_scaled_font_nil = {
    { 0 },			
    CAIRO_STATUS_NO_MEMORY,	
    CAIRO_REF_COUNT_INVALID,	
    { 0, 0, 0, NULL },		
    NULL,			
    { 1., 0., 0., 1., 0, 0},	
    { 1., 0., 0., 1., 0, 0},	
    { CAIRO_ANTIALIAS_DEFAULT,	
      CAIRO_SUBPIXEL_ORDER_DEFAULT,
      CAIRO_HINT_STYLE_DEFAULT,
      CAIRO_HINT_METRICS_DEFAULT} ,
    { 1., 0., 0., 1., 0, 0},	
    { 0., 0., 0., 0., 0. },	
    CAIRO_MUTEX_NIL_INITIALIZER,
    NULL,			
    NULL,			
    NULL,			
    CAIRO_SCALED_FONT_BACKEND_DEFAULT,
};

















void
_cairo_scaled_font_set_error (cairo_scaled_font_t *scaled_font,
			      cairo_status_t status)
{
    


    if (scaled_font->status == CAIRO_STATUS_SUCCESS)
	scaled_font->status = status;

    _cairo_error (status);
}












cairo_font_type_t
cairo_scaled_font_get_type (cairo_scaled_font_t *scaled_font)
{
    return scaled_font->backend->type;
}











cairo_status_t
cairo_scaled_font_status (cairo_scaled_font_t *scaled_font)
{
    return scaled_font->status;
}






















#define CAIRO_SCALED_FONT_MAX_HOLDOVERS 256

typedef struct _cairo_scaled_font_map {
    cairo_hash_table_t *hash_table;
    cairo_scaled_font_t *holdovers[CAIRO_SCALED_FONT_MAX_HOLDOVERS];
    int num_holdovers;
} cairo_scaled_font_map_t;

static cairo_scaled_font_map_t *cairo_scaled_font_map = NULL;

CAIRO_MUTEX_DECLARE (_cairo_scaled_font_map_mutex);

static int
_cairo_scaled_font_keys_equal (const void *abstract_key_a, const void *abstract_key_b);

static cairo_scaled_font_map_t *
_cairo_scaled_font_map_lock (void)
{
    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);

    if (cairo_scaled_font_map == NULL) {
	cairo_scaled_font_map = malloc (sizeof (cairo_scaled_font_map_t));
	if (cairo_scaled_font_map == NULL)
	    goto CLEANUP_MUTEX_LOCK;

	cairo_scaled_font_map->hash_table =
	    _cairo_hash_table_create (_cairo_scaled_font_keys_equal);

	if (cairo_scaled_font_map->hash_table == NULL)
	    goto CLEANUP_SCALED_FONT_MAP;

	cairo_scaled_font_map->num_holdovers = 0;
    }

    return cairo_scaled_font_map;

 CLEANUP_SCALED_FONT_MAP:
    free (cairo_scaled_font_map);
 CLEANUP_MUTEX_LOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
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
    int i;
    cairo_scaled_font_map_t *font_map;
    cairo_scaled_font_t *scaled_font;

    CAIRO_MUTEX_LOCK (_cairo_scaled_font_map_mutex);

    font_map = cairo_scaled_font_map;
    if (font_map == NULL) {
        goto CLEANUP_MUTEX_LOCK;
    }

    for (i = 0; i < font_map->num_holdovers; i++) {
	scaled_font = font_map->holdovers[i];
	


	assert (scaled_font->ref_count == 0);
	_cairo_hash_table_remove (font_map->hash_table,
				  &scaled_font->hash_entry);
	_cairo_scaled_font_fini (scaled_font);
	free (scaled_font);
    }

    _cairo_hash_table_destroy (font_map->hash_table);

    free (cairo_scaled_font_map);
    cairo_scaled_font_map = NULL;

 CLEANUP_MUTEX_LOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_scaled_font_map_mutex);
}







#define FNV_32_PRIME ((uint32_t)0x01000193)
#define FNV1_32_INIT ((uint32_t)0x811c9dc5)

static uint32_t
_hash_bytes_fnv (unsigned char *buffer,
		 int            len,
		 uint32_t       hval)
{
    while (len--) {
	hval *= FNV_32_PRIME;
	hval ^= *buffer++;
    }

    return hval;
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
    scaled_font->font_face = font_face;
    scaled_font->font_matrix = *font_matrix;
    scaled_font->ctm = *ctm;
    scaled_font->options = *options;

    

    hash = _hash_bytes_fnv ((unsigned char *)(&scaled_font->font_matrix.xx),
			    sizeof(cairo_matrix_t), hash);
    hash = _hash_bytes_fnv ((unsigned char *)(&scaled_font->ctm.xx),
			    sizeof(double) * 4, hash);

    hash ^= (unsigned long) scaled_font->font_face;

    hash ^= cairo_font_options_hash (&scaled_font->options);

    scaled_font->hash_entry.hash = hash;
}

static cairo_bool_t
_cairo_scaled_font_keys_equal (const void *abstract_key_a, const void *abstract_key_b)
{
    const cairo_scaled_font_t *key_a = abstract_key_a;
    const cairo_scaled_font_t *key_b = abstract_key_b;

    return (key_a->font_face == key_b->font_face &&
	    memcmp ((unsigned char *)(&key_a->font_matrix.xx),
		    (unsigned char *)(&key_b->font_matrix.xx),
		    sizeof(cairo_matrix_t)) == 0 &&
	    memcmp ((unsigned char *)(&key_a->ctm.xx),
		    (unsigned char *)(&key_b->ctm.xx),
		    sizeof(double) * 4) == 0 &&
	    cairo_font_options_equal (&key_a->options, &key_b->options));
}






static int max_glyphs_cached_per_font = 256;


void
_cairo_scaled_font_test_set_max_glyphs_cached_per_font (int max)
{
    max_glyphs_cached_per_font = max;
}





cairo_status_t
_cairo_scaled_font_init (cairo_scaled_font_t               *scaled_font,
			 cairo_font_face_t		   *font_face,
			 const cairo_matrix_t              *font_matrix,
			 const cairo_matrix_t              *ctm,
			 const cairo_font_options_t	   *options,
			 const cairo_scaled_font_backend_t *backend)
{
    scaled_font->ref_count = 1;

    _cairo_user_data_array_init (&scaled_font->user_data);

    _cairo_scaled_font_init_key (scaled_font, font_face,
				 font_matrix, ctm, options);

    cairo_font_face_reference (font_face);

    cairo_matrix_multiply (&scaled_font->scale,
			   &scaled_font->font_matrix,
			   &scaled_font->ctm);

    CAIRO_MUTEX_INIT (&scaled_font->mutex);
    scaled_font->glyphs = _cairo_cache_create (_cairo_scaled_glyph_keys_equal,
					       _cairo_scaled_glyph_destroy,
					       max_glyphs_cached_per_font);

    scaled_font->surface_backend = NULL;
    scaled_font->surface_private = NULL;

    scaled_font->backend = backend;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_scaled_font_freeze_cache (cairo_scaled_font_t *scaled_font)
{
    _cairo_cache_freeze (scaled_font->glyphs);
}

void
_cairo_scaled_font_thaw_cache (cairo_scaled_font_t *scaled_font)
{
    _cairo_cache_thaw (scaled_font->glyphs);
}

void
_cairo_scaled_font_reset_cache (cairo_scaled_font_t *scaled_font)
{
    _cairo_cache_destroy (scaled_font->glyphs);
    scaled_font->glyphs = _cairo_cache_create (_cairo_scaled_glyph_keys_equal,
					       _cairo_scaled_glyph_destroy,
					       max_glyphs_cached_per_font);
}

void
_cairo_scaled_font_set_metrics (cairo_scaled_font_t	    *scaled_font,
				cairo_font_extents_t	    *fs_metrics)
{
    double  font_scale_x, font_scale_y;

    _cairo_matrix_compute_scale_factors (&scaled_font->font_matrix,
					 &font_scale_x, &font_scale_y,
					  1);

    




    scaled_font->extents.ascent = fs_metrics->ascent * font_scale_y;
    scaled_font->extents.descent = fs_metrics->descent * font_scale_y;
    scaled_font->extents.height = fs_metrics->height * font_scale_y;
    scaled_font->extents.max_x_advance = fs_metrics->max_x_advance * font_scale_x;
    scaled_font->extents.max_y_advance = fs_metrics->max_y_advance * font_scale_y;
}

void
_cairo_scaled_font_fini (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font->font_face != NULL)
	cairo_font_face_destroy (scaled_font->font_face);

    if (scaled_font->glyphs != NULL)
	_cairo_cache_destroy (scaled_font->glyphs);

    CAIRO_MUTEX_FINI (&scaled_font->mutex);

    if (scaled_font->surface_backend != NULL &&
	scaled_font->surface_backend->scaled_font_fini != NULL)
	scaled_font->surface_backend->scaled_font_fini (scaled_font);

    scaled_font->backend->fini (scaled_font);

    _cairo_user_data_array_fini (&scaled_font->user_data);
}





















cairo_scaled_font_t *
cairo_scaled_font_create (cairo_font_face_t          *font_face,
			  const cairo_matrix_t       *font_matrix,
			  const cairo_matrix_t       *ctm,
			  const cairo_font_options_t *options)
{
    cairo_status_t status;
    cairo_scaled_font_map_t *font_map;
    cairo_scaled_font_t key, *scaled_font = NULL;

    if (font_face->status)
	return (cairo_scaled_font_t *)&_cairo_scaled_font_nil;

    font_map = _cairo_scaled_font_map_lock ();
    if (font_map == NULL)
	return NULL;

    _cairo_scaled_font_init_key (&key, font_face,
				 font_matrix, ctm, options);

    
    if (_cairo_hash_table_lookup (font_map->hash_table, &key.hash_entry,
				  (cairo_hash_entry_t**) &scaled_font))
    {
	



	if (scaled_font->ref_count == 0) {
	    int i;

	    for (i = 0; i < font_map->num_holdovers; i++)
		if (font_map->holdovers[i] == scaled_font)
		    break;
	    assert (i < font_map->num_holdovers);

	    font_map->num_holdovers--;
	    memmove (&font_map->holdovers[i],
		     &font_map->holdovers[i+1],
		     (font_map->num_holdovers - i) * sizeof (cairo_scaled_font_t*));
	}

	



	scaled_font->ref_count++;
	_cairo_scaled_font_map_unlock ();
    } else {
	
	status = font_face->backend->scaled_font_create (font_face, font_matrix,
							 ctm, options, &scaled_font);
	if (status) {
	    _cairo_scaled_font_map_unlock ();
	    return NULL;
	}

	status = _cairo_hash_table_insert (font_map->hash_table,
					   &scaled_font->hash_entry);
	_cairo_scaled_font_map_unlock ();

	if (status) {
	    


	    _cairo_scaled_font_fini (scaled_font);
	    free (scaled_font);
	    return NULL;
	}
    }

    return scaled_font;
}
slim_hidden_def (cairo_scaled_font_create);















cairo_scaled_font_t *
cairo_scaled_font_reference (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font == NULL || scaled_font->ref_count == CAIRO_REF_COUNT_INVALID)
	return scaled_font;

    _cairo_scaled_font_map_lock ();
    {
	assert (scaled_font->ref_count > 0);

	scaled_font->ref_count++;
    }
    _cairo_scaled_font_map_unlock ();

    return scaled_font;
}
slim_hidden_def (cairo_scaled_font_reference);









void
cairo_scaled_font_destroy (cairo_scaled_font_t *scaled_font)
{
    cairo_scaled_font_map_t *font_map;
    cairo_scaled_font_t *lru = NULL;

    if (scaled_font == NULL || scaled_font->ref_count == CAIRO_REF_COUNT_INVALID)
	return;

    font_map = _cairo_scaled_font_map_lock ();
    {
	assert (font_map != NULL);

	assert (scaled_font->ref_count > 0);

	if (--(scaled_font->ref_count) == 0)
	{
	    




	    if (font_map->num_holdovers == CAIRO_SCALED_FONT_MAX_HOLDOVERS)
	    {
		lru = font_map->holdovers[0];
		assert (lru->ref_count == 0);

		_cairo_hash_table_remove (font_map->hash_table, &lru->hash_entry);

		font_map->num_holdovers--;
		memmove (&font_map->holdovers[0],
			 &font_map->holdovers[1],
			 font_map->num_holdovers * sizeof (cairo_scaled_font_t*));
	    }

	    font_map->holdovers[font_map->num_holdovers] = scaled_font;
	    font_map->num_holdovers++;
	}
    }
    _cairo_scaled_font_map_unlock ();

    





    if (lru) {
	_cairo_scaled_font_fini (lru);
	free (lru);
    }
}
slim_hidden_def (cairo_scaled_font_destroy);












unsigned int
cairo_scaled_font_get_reference_count (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font == NULL || scaled_font->ref_count == CAIRO_REF_COUNT_INVALID)
	return 0;

    return scaled_font->ref_count;
}















void *
cairo_scaled_font_get_user_data (cairo_scaled_font_t	     *scaled_font,
				 const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&scaled_font->user_data,
					    key);
}



















cairo_status_t
cairo_scaled_font_set_user_data (cairo_scaled_font_t	     *scaled_font,
				 const cairo_user_data_key_t *key,
				 void			     *user_data,
				 cairo_destroy_func_t	      destroy)
{
    if (scaled_font->ref_count == CAIRO_REF_COUNT_INVALID)
	return CAIRO_STATUS_NO_MEMORY;

    return _cairo_user_data_array_set_data (&scaled_font->user_data,
					    key, user_data, destroy);
}










void
cairo_scaled_font_extents (cairo_scaled_font_t  *scaled_font,
			   cairo_font_extents_t *extents)
{
    *extents = scaled_font->extents;
}
slim_hidden_def (cairo_scaled_font_extents);
























void
cairo_scaled_font_text_extents (cairo_scaled_font_t   *scaled_font,
				const char            *utf8,
				cairo_text_extents_t  *extents)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_glyph_t *glyphs;
    int num_glyphs;

    status = _cairo_scaled_font_text_to_glyphs (scaled_font, 0., 0., utf8, &glyphs, &num_glyphs);
    if (status) {
        _cairo_scaled_font_set_error (scaled_font, status);
        return;
    }
    cairo_scaled_font_glyph_extents (scaled_font, glyphs, num_glyphs, extents);
    free (glyphs);
}



















void
cairo_scaled_font_glyph_extents (cairo_scaled_font_t   *scaled_font,
				 const cairo_glyph_t   *glyphs,
				 int                    num_glyphs,
				 cairo_text_extents_t  *extents)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    int i;
    double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
    cairo_bool_t visible = FALSE;
    cairo_scaled_glyph_t *scaled_glyph = NULL;

    if (scaled_font->status)
	return;

    CAIRO_MUTEX_LOCK (scaled_font->mutex);

    for (i = 0; i < num_glyphs; i++) {
	double			left, top, right, bottom;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_METRICS,
					     &scaled_glyph);
	if (status) {
	    _cairo_scaled_font_set_error (scaled_font, status);
	    goto UNLOCK;
	}

	
	if (scaled_glyph->metrics.width == 0 && scaled_glyph->metrics.height == 0)
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
    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);
}
slim_hidden_def (cairo_scaled_font_glyph_extents);

cairo_status_t
_cairo_scaled_font_text_to_glyphs (cairo_scaled_font_t *scaled_font,
				   double		x,
				   double		y,
				   const char          *utf8,
				   cairo_glyph_t      **glyphs,
				   int 		       *num_glyphs)
{
    int i;
    uint32_t *ucs4 = NULL;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_scaled_glyph_t *scaled_glyph;

    if (utf8[0] == '\0') {
	*num_glyphs = 0;
	*glyphs = NULL;
	return CAIRO_STATUS_SUCCESS;
    }

    CAIRO_MUTEX_LOCK (scaled_font->mutex);

    if (scaled_font->backend->text_to_glyphs) {
	status = scaled_font->backend->text_to_glyphs (scaled_font,
						       x, y, utf8,
						       glyphs, num_glyphs);

        if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto DONE;
    }

    status = _cairo_utf8_to_ucs4 ((unsigned char*)utf8, -1, &ucs4, num_glyphs);
    if (status)
	goto DONE;

    *glyphs = (cairo_glyph_t *) malloc ((*num_glyphs) * (sizeof (cairo_glyph_t)));

    if (*glyphs == NULL) {
	status = CAIRO_STATUS_NO_MEMORY;
	goto DONE;
    }

    for (i = 0; i < *num_glyphs; i++) {
        (*glyphs)[i].index = (*scaled_font->backend->
			      ucs4_to_index) (scaled_font, ucs4[i]);
	(*glyphs)[i].x = x;
	(*glyphs)[i].y = y;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     (*glyphs)[i].index,
					     CAIRO_SCALED_GLYPH_INFO_METRICS,
					     &scaled_glyph);
	if (status) {
	    free (*glyphs);
	    *glyphs = NULL;
	    goto DONE;
	}

        x += scaled_glyph->metrics.x_advance;
        y += scaled_glyph->metrics.y_advance;
    }

 DONE:
    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);

    if (ucs4)
	free (ucs4);

    return status;
}




cairo_status_t
_cairo_scaled_font_glyph_device_extents (cairo_scaled_font_t	 *scaled_font,
					 const cairo_glyph_t	 *glyphs,
					 int                      num_glyphs,
					 cairo_rectangle_int16_t *extents)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    int i;
    int min_x = INT16_MAX, max_x = INT16_MIN;
    int	min_y = INT16_MAX, max_y = INT16_MIN;

    if (scaled_font->status)
	return scaled_font->status;

    for (i = 0; i < num_glyphs; i++) {
	cairo_scaled_glyph_t	*scaled_glyph;
	int			left, top;
	int			right, bottom;
	int			x, y;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_METRICS,
					     &scaled_glyph);
	if (status) {
	    _cairo_scaled_font_set_error (scaled_font, status);
	    return status;
	}

	
	x = _cairo_lround (glyphs[i].x);
	y = _cairo_lround (glyphs[i].y);

	left   = x + _cairo_fixed_integer_floor(scaled_glyph->bbox.p1.x);
	top    = y + _cairo_fixed_integer_floor (scaled_glyph->bbox.p1.y);
	right  = x + _cairo_fixed_integer_ceil(scaled_glyph->bbox.p2.x);
	bottom = y + _cairo_fixed_integer_ceil (scaled_glyph->bbox.p2.y);

	if (left < min_x) min_x = left;
	if (right > max_x) max_x = right;
	if (top < min_y) min_y = top;
	if (bottom > max_y) max_y = bottom;
    }
    if (min_x < max_x && min_y < max_y) {
	extents->x = min_x;
	extents->width = max_x - min_x;
	extents->y = min_y;
	extents->height = max_y - min_y;
    } else {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
    }
    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_scaled_font_show_glyphs (cairo_scaled_font_t    *scaled_font,
				cairo_operator_t        op,
				cairo_pattern_t        *pattern,
				cairo_surface_t        *surface,
				int                     source_x,
				int                     source_y,
				int			dest_x,
				int			dest_y,
				unsigned int		width,
				unsigned int		height,
				cairo_glyph_t	       *glyphs,
				int                     num_glyphs)
{
    cairo_status_t status;
    cairo_surface_t *mask = NULL;
    int i;

    


    assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);

    if (scaled_font->status)
	return scaled_font->status;

    if (!num_glyphs)
	return CAIRO_STATUS_SUCCESS;

    if (scaled_font->backend->show_glyphs != NULL) {
	status = scaled_font->backend->show_glyphs (scaled_font,
						    op, pattern,
						    surface,
						    source_x, source_y,
						    dest_x, dest_y,
						    width, height,
						    glyphs, num_glyphs);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    

    status = CAIRO_STATUS_SUCCESS;

    _cairo_cache_freeze (scaled_font->glyphs);

    for (i = 0; i < num_glyphs; i++) {
	int x, y;
	cairo_surface_pattern_t glyph_pattern;
	cairo_image_surface_t *glyph_surface;
	cairo_scaled_glyph_t *scaled_glyph;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_SURFACE,
					     &scaled_glyph);

	if (status)
	    goto CLEANUP_MASK;

	glyph_surface = scaled_glyph->surface;

	
	if (mask == NULL) {
	    mask = cairo_image_surface_create (glyph_surface->format,
					       width, height);
	    if (mask->status) {
		status = mask->status;
		goto CLEANUP_MASK;
	    }

	    status = _cairo_surface_fill_rectangle (mask,
						    CAIRO_OPERATOR_CLEAR,
						    CAIRO_COLOR_TRANSPARENT,
						    0, 0,
						    width, height);
	    if (status)
		goto CLEANUP_MASK;
	    if (glyph_surface->format == CAIRO_FORMAT_ARGB32)
		pixman_image_set_component_alpha (((cairo_image_surface_t*) mask)->
						  pixman_image, TRUE);

	}

	
	
	x = _cairo_lround (glyphs[i].x +
                           glyph_surface->base.device_transform.x0);
	y = _cairo_lround (glyphs[i].y +
                           glyph_surface->base.device_transform.y0);

	_cairo_pattern_init_for_surface (&glyph_pattern, &glyph_surface->base);

	status = _cairo_surface_composite (CAIRO_OPERATOR_ADD,
					   &glyph_pattern.base,
					   NULL,
					   mask,
					   0, 0,
					   0, 0,
					   x - dest_x,
					   y - dest_y,
					   glyph_surface->width,
					   glyph_surface->height);

	_cairo_pattern_fini (&glyph_pattern.base);
	if (status)
	    break;
    }

    if (mask != NULL) {
	cairo_surface_pattern_t mask_pattern;

	_cairo_pattern_init_for_surface (&mask_pattern, mask);

	status = _cairo_surface_composite (op, pattern, &mask_pattern.base,
					   surface,
					   source_x, source_y,
					   0,        0,
					   dest_x,   dest_y,
					   width,    height);

	_cairo_pattern_fini (&mask_pattern.base);
    }

CLEANUP_MASK:
    _cairo_cache_thaw (scaled_font->glyphs);

    if (mask != NULL)
	cairo_surface_destroy (mask);
    return status;
}

typedef struct _cairo_scaled_glyph_path_closure {
    cairo_point_t	    offset;
    cairo_path_fixed_t	    *path;
} cairo_scaled_glyph_path_closure_t;

static cairo_status_t
_scaled_glyph_path_move_to (void *abstract_closure, cairo_point_t *point)
{
    cairo_scaled_glyph_path_closure_t	*closure = abstract_closure;

    return _cairo_path_fixed_move_to (closure->path,
				      point->x + closure->offset.x,
				      point->y + closure->offset.y);
}

static cairo_status_t
_scaled_glyph_path_line_to (void *abstract_closure, cairo_point_t *point)
{
    cairo_scaled_glyph_path_closure_t	*closure = abstract_closure;

    return _cairo_path_fixed_line_to (closure->path,
				      point->x + closure->offset.x,
				      point->y + closure->offset.y);
}

static cairo_status_t
_scaled_glyph_path_curve_to (void *abstract_closure,
			     cairo_point_t *p0,
			     cairo_point_t *p1,
			     cairo_point_t *p2)
{
    cairo_scaled_glyph_path_closure_t	*closure = abstract_closure;

    return _cairo_path_fixed_curve_to (closure->path,
				       p0->x + closure->offset.x,
				       p0->y + closure->offset.y,
				       p1->x + closure->offset.x,
				       p1->y + closure->offset.y,
				       p2->x + closure->offset.x,
				       p2->y + closure->offset.y);
}

static cairo_status_t
_scaled_glyph_path_close_path (void *abstract_closure)
{
    cairo_scaled_glyph_path_closure_t	*closure = abstract_closure;

    return _cairo_path_fixed_close_path (closure->path);
}



















static cairo_status_t
_trace_mask_to_path (cairo_image_surface_t *mask,
		     cairo_path_fixed_t *path)
{
    cairo_image_surface_t *a1_mask;
    unsigned char *row, *byte_ptr, byte;
    int rows, cols, bytes_per_row;
    int x, y, bit;
    double xoff, yoff;

    if (mask->format == CAIRO_FORMAT_A1)
	a1_mask = mask;
    else
	a1_mask = _cairo_image_surface_clone (mask, CAIRO_FORMAT_A1);

    if (cairo_surface_status (&a1_mask->base))
	return cairo_surface_status (&a1_mask->base);

    cairo_surface_get_device_offset (&mask->base, &xoff, &yoff);

    bytes_per_row = (a1_mask->width + 7) / 8;
    for (y = 0, row = a1_mask->data, rows = a1_mask->height; rows; row += a1_mask->stride, rows--, y++) {
	for (x = 0, byte_ptr = row, cols = (a1_mask->width + 7) / 8; cols; byte_ptr++, cols--) {
	    byte = CAIRO_BITSWAP8_IF_LITTLE_ENDIAN (*byte_ptr);
	    for (bit = 7; bit >= 0 && x < a1_mask->width; bit--, x++) {
		if (byte & (1 << bit)) {
		    _cairo_path_fixed_move_to (path,
					       _cairo_fixed_from_int (x + xoff),
					       _cairo_fixed_from_int (y + yoff));
		    _cairo_path_fixed_rel_line_to (path,
						   _cairo_fixed_from_int (1),
						   _cairo_fixed_from_int (0));
		    _cairo_path_fixed_rel_line_to (path,
						   _cairo_fixed_from_int (0),
						   _cairo_fixed_from_int (1));
		    _cairo_path_fixed_rel_line_to (path,
						   _cairo_fixed_from_int (-1),
						   _cairo_fixed_from_int (0));
		    _cairo_path_fixed_close_path (path);
		}
	    }
	}
    }

    if (a1_mask != mask)
	cairo_surface_destroy (&a1_mask->base);

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_scaled_font_glyph_path (cairo_scaled_font_t *scaled_font,
			       const cairo_glyph_t *glyphs,
			       int		    num_glyphs,
			       cairo_path_fixed_t  *path)
{
    cairo_status_t status;
    int	i;
    cairo_scaled_glyph_path_closure_t closure;
    cairo_path_fixed_t *glyph_path;

    if (scaled_font->status)
	return scaled_font->status;

    closure.path = path;
    for (i = 0; i < num_glyphs; i++) {
	cairo_scaled_glyph_t *scaled_glyph;

	status = _cairo_scaled_glyph_lookup (scaled_font,
					     glyphs[i].index,
					     CAIRO_SCALED_GLYPH_INFO_PATH,
					     &scaled_glyph);
	if (status == CAIRO_STATUS_SUCCESS)
	    glyph_path = scaled_glyph->path;
	else if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;

	

	if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	    status = _cairo_scaled_glyph_lookup (scaled_font,
						 glyphs[i].index,
						 CAIRO_SCALED_GLYPH_INFO_SURFACE,
						 &scaled_glyph);

	    glyph_path = _cairo_path_fixed_create ();
	    if (glyph_path == NULL)
		return CAIRO_STATUS_NO_MEMORY;

	    status = _trace_mask_to_path (scaled_glyph->surface, glyph_path);
	    if (status) {
		_cairo_path_fixed_destroy (glyph_path);
		return status;
	    }
	}

	closure.offset.x = _cairo_fixed_from_double (glyphs[i].x);
	closure.offset.y = _cairo_fixed_from_double (glyphs[i].y);

	status = _cairo_path_fixed_interpret (glyph_path,
					      CAIRO_DIRECTION_FORWARD,
					      _scaled_glyph_path_move_to,
					      _scaled_glyph_path_line_to,
					      _scaled_glyph_path_curve_to,
					      _scaled_glyph_path_close_path,
					      &closure);
	if (glyph_path != scaled_glyph->path)
	    _cairo_path_fixed_destroy (glyph_path);
    }

    return CAIRO_STATUS_SUCCESS;
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
}

void
_cairo_scaled_glyph_set_surface (cairo_scaled_glyph_t *scaled_glyph,
				 cairo_scaled_font_t *scaled_font,
				 cairo_image_surface_t *surface)
{
    if (scaled_glyph->surface != NULL)
	cairo_surface_destroy (&scaled_glyph->surface->base);
    scaled_glyph->surface = surface;
}

void
_cairo_scaled_glyph_set_path (cairo_scaled_glyph_t *scaled_glyph,
			      cairo_scaled_font_t *scaled_font,
			      cairo_path_fixed_t *path)
{
    if (scaled_glyph->path != NULL)
	_cairo_path_fixed_destroy (scaled_glyph->path);
    scaled_glyph->path = path;
}
























cairo_int_status_t
_cairo_scaled_glyph_lookup (cairo_scaled_font_t *scaled_font,
			    unsigned long index,
			    cairo_scaled_glyph_info_t info,
			    cairo_scaled_glyph_t **scaled_glyph_ret)
{
    cairo_status_t		status = CAIRO_STATUS_SUCCESS;
    cairo_cache_entry_t		key;
    cairo_scaled_glyph_t	*scaled_glyph;
    cairo_scaled_glyph_info_t	need_info;

    if (scaled_font->status)
	return scaled_font->status;

    key.hash = index;
    


    info |= CAIRO_SCALED_GLYPH_INFO_METRICS;
    if (!_cairo_cache_lookup (scaled_font->glyphs, &key,
			      (cairo_cache_entry_t **) &scaled_glyph))
    {
	


	scaled_glyph = malloc (sizeof (cairo_scaled_glyph_t));
	if (scaled_glyph == NULL) {
	    status = CAIRO_STATUS_NO_MEMORY;
	    goto CLEANUP;
	}

	_cairo_scaled_glyph_set_index(scaled_glyph, index);
	scaled_glyph->cache_entry.size = 1;	
	scaled_glyph->scaled_font = scaled_font;
	scaled_glyph->surface = NULL;
	scaled_glyph->path = NULL;
	scaled_glyph->surface_private = NULL;

	
	status = (*scaled_font->backend->
		  scaled_glyph_init) (scaled_font, scaled_glyph, info);
	if (status) {
	    _cairo_scaled_glyph_destroy (scaled_glyph);
	    goto CLEANUP;
	}

	
	status = _cairo_cache_insert (scaled_font->glyphs,
				      &scaled_glyph->cache_entry);
	if (status) {
	    _cairo_scaled_glyph_destroy (scaled_glyph);
	    goto CLEANUP;
	}
    }
    



    need_info = 0;
    if ((info & CAIRO_SCALED_GLYPH_INFO_SURFACE) != 0 &&
	scaled_glyph->surface == NULL)
	need_info |= CAIRO_SCALED_GLYPH_INFO_SURFACE;

    if (((info & CAIRO_SCALED_GLYPH_INFO_PATH) != 0 &&
	 scaled_glyph->path == NULL))
	need_info |= CAIRO_SCALED_GLYPH_INFO_PATH;

    if (need_info) {
	status = (*scaled_font->backend->
		  scaled_glyph_init) (scaled_font, scaled_glyph, need_info);
	if (status)
	    goto CLEANUP;
    }

  CLEANUP:
    if (status) {
	
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    _cairo_scaled_font_set_error (scaled_font, status);
	*scaled_glyph_ret = NULL;
    } else {
	*scaled_glyph_ret = scaled_glyph;
    }

    return status;
}












cairo_font_face_t *
cairo_scaled_font_get_font_face (cairo_scaled_font_t *scaled_font)
{
    if (scaled_font->status)
	return (cairo_font_face_t*) &_cairo_font_face_nil;

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
cairo_scaled_font_get_font_options (cairo_scaled_font_t		*scaled_font,
				    cairo_font_options_t	*options)
{
    if (scaled_font->status) {
	_cairo_font_options_init_default (options);
	return;
    }

    _cairo_font_options_init_copy (options, &scaled_font->options);
}
slim_hidden_def (cairo_scaled_font_get_font_options);
