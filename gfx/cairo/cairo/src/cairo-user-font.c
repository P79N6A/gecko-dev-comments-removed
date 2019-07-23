



































#include "cairoint.h"
#include "cairo-user-font-private.h"
#include "cairo-meta-surface-private.h"
#include "cairo-analysis-surface-private.h"

typedef struct _cairo_user_scaled_font_methods {
    cairo_user_scaled_font_init_func_t			init;
    cairo_user_scaled_font_render_glyph_func_t		render_glyph;
    cairo_user_scaled_font_unicode_to_glyph_func_t	unicode_to_glyph;
    cairo_user_scaled_font_text_to_glyphs_func_t	text_to_glyphs;
} cairo_user_scaled_font_methods_t;

typedef struct _cairo_user_font_face {
    cairo_font_face_t	             base;

    

    cairo_bool_t		     immutable;

    cairo_user_scaled_font_methods_t scaled_font_methods;
} cairo_user_font_face_t;

typedef struct _cairo_user_scaled_font {
    cairo_scaled_font_t  base;

    cairo_text_extents_t default_glyph_extents;

    
    cairo_matrix_t extent_scale;
    double extent_x_scale;
    double extent_y_scale;

    
    double snap_x_scale;
    double snap_y_scale;

} cairo_user_scaled_font_t;



static cairo_t *
_cairo_user_scaled_font_create_meta_context (cairo_user_scaled_font_t *scaled_font)
{
    cairo_content_t content;
    cairo_surface_t *meta_surface;
    cairo_t *cr;

    content = scaled_font->base.options.antialias == CAIRO_ANTIALIAS_SUBPIXEL ?
						     CAIRO_CONTENT_COLOR_ALPHA :
						     CAIRO_CONTENT_ALPHA;

    meta_surface = _cairo_meta_surface_create (content, -1, -1);
    cr = cairo_create (meta_surface);
    cairo_surface_destroy (meta_surface);

    cairo_set_matrix (cr, &scaled_font->base.scale);
    cairo_set_font_size (cr, 1.0);
    cairo_set_font_options (cr, &scaled_font->base.options);

    return cr;
}

static cairo_int_status_t
_cairo_user_scaled_glyph_init (void			 *abstract_font,
			       cairo_scaled_glyph_t	 *scaled_glyph,
			       cairo_scaled_glyph_info_t  info)
{
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_user_scaled_font_t *scaled_font = abstract_font;
    cairo_surface_t *meta_surface = scaled_glyph->meta_surface;

    if (!scaled_glyph->meta_surface) {
	cairo_user_font_face_t *face =
	    (cairo_user_font_face_t *) scaled_font->base.font_face;
	cairo_text_extents_t extents = scaled_font->default_glyph_extents;
	cairo_t *cr;

	cr = _cairo_user_scaled_font_create_meta_context (scaled_font);

	if (face->scaled_font_methods.render_glyph)
	    status = face->scaled_font_methods.render_glyph ((cairo_scaled_font_t *)scaled_font,
							     _cairo_scaled_glyph_index(scaled_glyph),
							     cr, &extents);
	else
	    status = CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED;

	if (status == CAIRO_STATUS_SUCCESS)
	    status = cairo_status (cr);

	meta_surface = cairo_surface_reference (cairo_get_target (cr));

	cairo_destroy (cr);

	if (unlikely (status)) {
	    cairo_surface_destroy (meta_surface);
	    return status;
	}

	_cairo_scaled_glyph_set_meta_surface (scaled_glyph,
					      &scaled_font->base,
					      meta_surface);


	

	if (extents.width == 0.) {
	    

	    cairo_box_t bbox;
	    double x1, y1, x2, y2;
	    double x_scale, y_scale;
	    cairo_surface_t *null_surface;
	    cairo_surface_t *analysis_surface;

	    null_surface = _cairo_null_surface_create (cairo_surface_get_content (meta_surface));
	    analysis_surface = _cairo_analysis_surface_create (null_surface, -1, -1);
	    cairo_surface_destroy (null_surface);
	    status = analysis_surface->status;
	    if (unlikely (status))
		return status;

	    _cairo_analysis_surface_set_ctm (analysis_surface,
					     &scaled_font->extent_scale);
	    status = _cairo_meta_surface_replay (meta_surface,
						 analysis_surface);
	    _cairo_analysis_surface_get_bounding_box (analysis_surface, &bbox);
	    cairo_surface_destroy (analysis_surface);

	    if (unlikely (status))
		return status;

	    _cairo_box_to_doubles (&bbox, &x1, &y1, &x2, &y2);

	    x_scale = scaled_font->extent_x_scale;
	    y_scale = scaled_font->extent_y_scale;
	    extents.x_bearing = x1 * x_scale;
	    extents.y_bearing = y1 * y_scale;
	    extents.width     = (x2 - x1) * x_scale;
	    extents.height    = (y2 - y1) * y_scale;
	}

	if (scaled_font->base.options.hint_metrics != CAIRO_HINT_METRICS_OFF) {
	    extents.x_advance = _cairo_lround (extents.x_advance / scaled_font->snap_x_scale) * scaled_font->snap_x_scale;
	    extents.y_advance = _cairo_lround (extents.y_advance / scaled_font->snap_y_scale) * scaled_font->snap_y_scale;
	}

	_cairo_scaled_glyph_set_metrics (scaled_glyph,
					 &scaled_font->base,
					 &extents);
    }

    if (info & CAIRO_SCALED_GLYPH_INFO_SURFACE) {
	cairo_surface_t	*surface;
	cairo_format_t format;
	int width, height;

	





	width = _cairo_fixed_integer_ceil (scaled_glyph->bbox.p2.x) -
	  _cairo_fixed_integer_floor (scaled_glyph->bbox.p1.x);
	height = _cairo_fixed_integer_ceil (scaled_glyph->bbox.p2.y) -
	  _cairo_fixed_integer_floor (scaled_glyph->bbox.p1.y);

	switch (scaled_font->base.options.antialias) {
	default:
	case CAIRO_ANTIALIAS_DEFAULT:
	case CAIRO_ANTIALIAS_GRAY:	format = CAIRO_FORMAT_A8;	break;
	case CAIRO_ANTIALIAS_NONE:	format = CAIRO_FORMAT_A1;	break;
	case CAIRO_ANTIALIAS_SUBPIXEL:	format = CAIRO_FORMAT_ARGB32;	break;
	}
	surface = cairo_image_surface_create (format, width, height);

	cairo_surface_set_device_offset (surface,
	                                 - _cairo_fixed_integer_floor (scaled_glyph->bbox.p1.x),
	                                 - _cairo_fixed_integer_floor (scaled_glyph->bbox.p1.y));
	status = _cairo_meta_surface_replay (meta_surface, surface);

	if (unlikely (status)) {
	    cairo_surface_destroy(surface);
	    return status;
	}

	_cairo_scaled_glyph_set_surface (scaled_glyph,
					 &scaled_font->base,
					 (cairo_image_surface_t *) surface);
    }

    if (info & CAIRO_SCALED_GLYPH_INFO_PATH) {
	cairo_path_fixed_t *path = _cairo_path_fixed_create ();
	if (!path)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	status = _cairo_meta_surface_get_path (meta_surface, path);

	if (unlikely (status)) {
	    _cairo_path_fixed_destroy (path);
	    return status;
	}

	_cairo_scaled_glyph_set_path (scaled_glyph,
				      &scaled_font->base,
				      path);
    }

    return status;
}

static unsigned long
_cairo_user_ucs4_to_index (void	    *abstract_font,
			   uint32_t  ucs4)
{
    cairo_user_scaled_font_t *scaled_font = abstract_font;
    cairo_user_font_face_t *face =
	(cairo_user_font_face_t *) scaled_font->base.font_face;
    unsigned long glyph = 0;

    if (face->scaled_font_methods.unicode_to_glyph) {
	cairo_status_t status;

	status = face->scaled_font_methods.unicode_to_glyph (&scaled_font->base,
							     ucs4, &glyph);

	if (status == CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED)
	    goto not_implemented;

	if (status != CAIRO_STATUS_SUCCESS) {
	    status = _cairo_scaled_font_set_error (&scaled_font->base, status);
	    glyph = 0;
	}

    } else {
not_implemented:
	glyph = ucs4;
    }

    return glyph;
}

static cairo_int_status_t
_cairo_user_text_to_glyphs (void		      *abstract_font,
			    double		       x,
			    double		       y,
			    const char		      *utf8,
			    int			       utf8_len,
			    cairo_glyph_t	     **glyphs,
			    int			       *num_glyphs,
			    cairo_text_cluster_t      **clusters,
			    int			       *num_clusters,
			    cairo_text_cluster_flags_t *cluster_flags)
{
    cairo_int_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;

    cairo_user_scaled_font_t *scaled_font = abstract_font;
    cairo_user_font_face_t *face =
	(cairo_user_font_face_t *) scaled_font->base.font_face;

    if (face->scaled_font_methods.text_to_glyphs) {
	int i;
	cairo_glyph_t *orig_glyphs = *glyphs;
	int orig_num_glyphs = *num_glyphs;

	status = face->scaled_font_methods.text_to_glyphs (&scaled_font->base,
							   utf8, utf8_len,
							   glyphs, num_glyphs,
							   clusters, num_clusters, cluster_flags);

	if (status != CAIRO_STATUS_SUCCESS &&
	    status != CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED)
	    return status;

	if (status == CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED || *num_glyphs < 0) {
	    if (orig_glyphs != *glyphs) {
		cairo_glyph_free (*glyphs);
		*glyphs = orig_glyphs;
	    }
	    *num_glyphs = orig_num_glyphs;
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}

	
	for (i = 0; i < *num_glyphs; i++) {
	    double gx = (*glyphs)[i].x;
	    double gy = (*glyphs)[i].y;

	    cairo_matrix_transform_point (&scaled_font->base.font_matrix,
					  &gx, &gy);

	    (*glyphs)[i].x = gx + x;
	    (*glyphs)[i].y = gy + y;
	}
    }

    return status;
}

static cairo_status_t
_cairo_user_font_face_scaled_font_create (void                        *abstract_face,
					  const cairo_matrix_t        *font_matrix,
					  const cairo_matrix_t        *ctm,
					  const cairo_font_options_t  *options,
					  cairo_scaled_font_t        **scaled_font);

static cairo_status_t
_cairo_user_font_face_create_for_toy (cairo_toy_font_face_t   *toy_face,
				      cairo_font_face_t      **font_face)
{
    return _cairo_font_face_twin_create_for_toy (toy_face, font_face);
}

static const cairo_scaled_font_backend_t _cairo_user_scaled_font_backend = {
    CAIRO_FONT_TYPE_USER,
    NULL,	
    _cairo_user_scaled_glyph_init,
    _cairo_user_text_to_glyphs,
    _cairo_user_ucs4_to_index,
    NULL,	
    NULL,	
    NULL	
};



static cairo_status_t
_cairo_user_font_face_scaled_font_create (void                        *abstract_face,
					  const cairo_matrix_t        *font_matrix,
					  const cairo_matrix_t        *ctm,
					  const cairo_font_options_t  *options,
					  cairo_scaled_font_t        **scaled_font)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_user_font_face_t *font_face = abstract_face;
    cairo_user_scaled_font_t *user_scaled_font = NULL;
    cairo_font_extents_t font_extents = {1., 0., 1., 1., 0.};

    font_face->immutable = TRUE;

    user_scaled_font = malloc (sizeof (cairo_user_scaled_font_t));
    if (unlikely (user_scaled_font == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_scaled_font_init (&user_scaled_font->base,
				      &font_face->base,
				      font_matrix, ctm, options,
				      &_cairo_user_scaled_font_backend);

    if (unlikely (status)) {
	free (user_scaled_font);
	return status;
    }

    

    


    {
	double fixed_scale, x_scale, y_scale;

	user_scaled_font->extent_scale = user_scaled_font->base.scale_inverse;
	status = _cairo_matrix_compute_basis_scale_factors (&user_scaled_font->extent_scale,
						      &x_scale, &y_scale,
						      1);
	if (status == CAIRO_STATUS_SUCCESS) {

	    if (x_scale == 0) x_scale = 1.;
	    if (y_scale == 0) y_scale = 1.;

	    user_scaled_font->snap_x_scale = x_scale;
	    user_scaled_font->snap_y_scale = y_scale;

	    

	    fixed_scale = 1024.;
	    x_scale /= fixed_scale;
	    y_scale /= fixed_scale;

	    cairo_matrix_scale (&user_scaled_font->extent_scale, 1. / x_scale, 1. / y_scale);

	    user_scaled_font->extent_x_scale = x_scale;
	    user_scaled_font->extent_y_scale = y_scale;
	}
    }

    if (status == CAIRO_STATUS_SUCCESS &&
	font_face->scaled_font_methods.init != NULL)
    {
	

	CAIRO_MUTEX_LOCK (user_scaled_font->base.mutex);

	
	status = _cairo_scaled_font_register_placeholder_and_unlock_font_map (&user_scaled_font->base);
	if (status == CAIRO_STATUS_SUCCESS) {
	    cairo_t *cr;

	    cr = _cairo_user_scaled_font_create_meta_context (user_scaled_font);

	    status = font_face->scaled_font_methods.init (&user_scaled_font->base,
							  cr,
							  &font_extents);

	    if (status == CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED)
		status = CAIRO_STATUS_SUCCESS;

	    if (status == CAIRO_STATUS_SUCCESS)
		status = cairo_status (cr);

	    cairo_destroy (cr);

	    _cairo_scaled_font_unregister_placeholder_and_lock_font_map (&user_scaled_font->base);
	}

	CAIRO_MUTEX_UNLOCK (user_scaled_font->base.mutex);
    }

    if (status == CAIRO_STATUS_SUCCESS)
	status = _cairo_scaled_font_set_metrics (&user_scaled_font->base, &font_extents);

    if (status != CAIRO_STATUS_SUCCESS) {
        _cairo_scaled_font_fini (&user_scaled_font->base);
	free (user_scaled_font);
    } else {
        user_scaled_font->default_glyph_extents.x_bearing = 0.;
        user_scaled_font->default_glyph_extents.y_bearing = -font_extents.ascent;
        user_scaled_font->default_glyph_extents.width = 0.;
        user_scaled_font->default_glyph_extents.height = font_extents.ascent + font_extents.descent;
        user_scaled_font->default_glyph_extents.x_advance = font_extents.max_x_advance;
        user_scaled_font->default_glyph_extents.y_advance = 0.;

	*scaled_font = &user_scaled_font->base;
    }

    return status;
}

const cairo_font_face_backend_t _cairo_user_font_face_backend = {
    CAIRO_FONT_TYPE_USER,
    _cairo_user_font_face_create_for_toy,
    NULL,	
    _cairo_user_font_face_scaled_font_create
};


cairo_bool_t
_cairo_font_face_is_user (cairo_font_face_t *font_face)
{
    return font_face->backend == &_cairo_user_font_face_backend;
}






















cairo_font_face_t *
cairo_user_font_face_create (void)
{
    cairo_user_font_face_t *font_face;

    font_face = malloc (sizeof (cairo_user_font_face_t));
    if (!font_face) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_font_face_t *)&_cairo_font_face_nil;
    }

    _cairo_font_face_init (&font_face->base, &_cairo_user_font_face_backend);

    font_face->immutable = FALSE;
    memset (&font_face->scaled_font_methods, 0, sizeof (font_face->scaled_font_methods));

    return &font_face->base;
}
slim_hidden_def(cairo_user_font_face_create);



















void
cairo_user_font_face_set_init_func (cairo_font_face_t                  *font_face,
				    cairo_user_scaled_font_init_func_t  init_func)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.init = init_func;
}
slim_hidden_def(cairo_user_font_face_set_init_func);




















void
cairo_user_font_face_set_render_glyph_func (cairo_font_face_t                          *font_face,
					    cairo_user_scaled_font_render_glyph_func_t  render_glyph_func)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.render_glyph = render_glyph_func;
}
slim_hidden_def(cairo_user_font_face_set_render_glyph_func);
















void
cairo_user_font_face_set_text_to_glyphs_func (cairo_font_face_t                            *font_face,
					      cairo_user_scaled_font_text_to_glyphs_func_t  text_to_glyphs_func)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.text_to_glyphs = text_to_glyphs_func;
}
















void
cairo_user_font_face_set_unicode_to_glyph_func (cairo_font_face_t                              *font_face,
						cairo_user_scaled_font_unicode_to_glyph_func_t  unicode_to_glyph_func)
{
    cairo_user_font_face_t *user_font_face;
    if (font_face->status)
	return;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.unicode_to_glyph = unicode_to_glyph_func;
}
slim_hidden_def(cairo_user_font_face_set_unicode_to_glyph_func);














cairo_user_scaled_font_init_func_t
cairo_user_font_face_get_init_func (cairo_font_face_t *font_face)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.init;
}












cairo_user_scaled_font_render_glyph_func_t
cairo_user_font_face_get_render_glyph_func (cairo_font_face_t *font_face)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.render_glyph;
}












cairo_user_scaled_font_text_to_glyphs_func_t
cairo_user_font_face_get_text_to_glyphs_func (cairo_font_face_t *font_face)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.text_to_glyphs;
}












cairo_user_scaled_font_unicode_to_glyph_func_t
cairo_user_font_face_get_unicode_to_glyph_func (cairo_font_face_t *font_face)
{
    cairo_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _cairo_font_face_is_user (font_face)) {
	if (_cairo_font_face_set_error (font_face, CAIRO_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (cairo_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.unicode_to_glyph;
}
