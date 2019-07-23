














































#include "cairoint.h"

#include "test-meta-surface.h"

#include "cairo-meta-surface-private.h"

typedef struct _test_meta_surface {
    cairo_surface_t base;

    
    cairo_surface_t *meta;

    
    cairo_surface_t *image;

    cairo_bool_t image_reflects_meta;
} test_meta_surface_t;

const cairo_private cairo_surface_backend_t test_meta_surface_backend;

static cairo_int_status_t
_test_meta_surface_show_page (void *abstract_surface);

cairo_surface_t *
_cairo_test_meta_surface_create (cairo_content_t	content,
			   int		 	width,
			   int		 	height)
{
    test_meta_surface_t *surface;

    surface = malloc (sizeof (test_meta_surface_t));
    if (surface == NULL)
	goto FAIL;

    _cairo_surface_init (&surface->base, &test_meta_surface_backend,
			 content);

    surface->meta = _cairo_meta_surface_create (content, width, height);
    if (cairo_surface_status (surface->meta))
	goto FAIL_CLEANUP_SURFACE;

    surface->image = _cairo_image_surface_create_with_content (content,
							       width, height);
    if (cairo_surface_status (surface->image))
	goto FAIL_CLEANUP_META;

    surface->image_reflects_meta = FALSE;

    return &surface->base;

  FAIL_CLEANUP_META:
    cairo_surface_destroy (surface->meta);
  FAIL_CLEANUP_SURFACE:
    free (surface);
  FAIL:
    _cairo_error (CAIRO_STATUS_NO_MEMORY);
    return (cairo_surface_t*) &_cairo_surface_nil;
}

static cairo_status_t
_test_meta_surface_finish (void *abstract_surface)
{
    test_meta_surface_t *surface = abstract_surface;

    cairo_surface_destroy (surface->meta);
    cairo_surface_destroy (surface->image);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_test_meta_surface_acquire_source_image (void		  *abstract_surface,
					 cairo_image_surface_t	**image_out,
					 void			**image_extra)
{
    test_meta_surface_t *surface = abstract_surface;

    if (! surface->image_reflects_meta)
	_test_meta_surface_show_page (abstract_surface);

    return _cairo_surface_acquire_source_image (surface->image,
						image_out, image_extra);
}

static void
_test_meta_surface_release_source_image (void			*abstract_surface,
					 cairo_image_surface_t	*image,
					 void		  	*image_extra)
{
    test_meta_surface_t *surface = abstract_surface;

    _cairo_surface_release_source_image (surface->image,
					 image, image_extra);
}

static cairo_int_status_t
_test_meta_surface_show_page (void *abstract_surface)
{
    test_meta_surface_t *surface = abstract_surface;
    cairo_status_t status;

    if (surface->image_reflects_meta)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_meta_surface_replay (surface->meta, surface->image);
    if (status)
	return status;

    surface->image_reflects_meta = TRUE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_test_meta_surface_intersect_clip_path (void			*abstract_surface,
					cairo_path_fixed_t	*path,
					cairo_fill_rule_t	 fill_rule,
					double			 tolerance,
					cairo_antialias_t	 antialias)
{
    test_meta_surface_t *surface = abstract_surface;

    return _cairo_surface_intersect_clip_path (surface->meta,
					       path, fill_rule,
					       tolerance, antialias);
}

static cairo_int_status_t
_test_meta_surface_get_extents (void			*abstract_surface,
				cairo_rectangle_int16_t	*rectangle)
{
    test_meta_surface_t *surface = abstract_surface;

    surface->image_reflects_meta = FALSE;

    return _cairo_surface_get_extents (surface->image, rectangle);
}

static cairo_int_status_t
_test_meta_surface_paint (void			*abstract_surface,
			  cairo_operator_t	 op,
			  cairo_pattern_t	*source)
{
    test_meta_surface_t *surface = abstract_surface;

    surface->image_reflects_meta = FALSE;

    return _cairo_surface_paint (surface->meta, op, source);
}

static cairo_int_status_t
_test_meta_surface_mask (void			*abstract_surface,
			 cairo_operator_t	 op,
			 cairo_pattern_t	*source,
			 cairo_pattern_t	*mask)
{
    test_meta_surface_t *surface = abstract_surface;

    surface->image_reflects_meta = FALSE;

    return _cairo_surface_mask (surface->meta, op, source, mask);
}

static cairo_int_status_t
_test_meta_surface_stroke (void			*abstract_surface,
			   cairo_operator_t	 op,
			   cairo_pattern_t	*source,
			   cairo_path_fixed_t	*path,
			   cairo_stroke_style_t	*style,
			   cairo_matrix_t	*ctm,
			   cairo_matrix_t	*ctm_inverse,
			   double		 tolerance,
			   cairo_antialias_t	 antialias)
{
    test_meta_surface_t *surface = abstract_surface;

    surface->image_reflects_meta = FALSE;

    return _cairo_surface_stroke (surface->meta, op, source,
				  path, style,
				  ctm, ctm_inverse,
				  tolerance, antialias);
}

static cairo_int_status_t
_test_meta_surface_fill (void			*abstract_surface,
			 cairo_operator_t	 op,
			 cairo_pattern_t	*source,
			 cairo_path_fixed_t	*path,
			 cairo_fill_rule_t	 fill_rule,
			 double			 tolerance,
			 cairo_antialias_t	 antialias)
{
    test_meta_surface_t *surface = abstract_surface;

    surface->image_reflects_meta = FALSE;

    return _cairo_surface_fill (surface->meta, op, source,
				path, fill_rule,
				tolerance, antialias);
}

static cairo_int_status_t
_test_meta_surface_show_glyphs (void			*abstract_surface,
				cairo_operator_t	 op,
				cairo_pattern_t		*source,
				cairo_glyph_t		*glyphs,
				int			 num_glyphs,
				cairo_scaled_font_t	*scaled_font)
{
    test_meta_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    surface->image_reflects_meta = FALSE;

    









    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);
    status = _cairo_surface_show_glyphs (surface->meta, op, source,
					 glyphs, num_glyphs,
					 scaled_font);
    CAIRO_MUTEX_LOCK (scaled_font->mutex);

    return status;
}

static cairo_surface_t *
_test_meta_surface_snapshot (void *abstract_other)
{
    test_meta_surface_t *other = abstract_other;
    cairo_status_t status;

    











#if 0
    return _cairo_surface_snapshot (other->meta);
#else
    cairo_rectangle_int16_t extents;
    cairo_surface_t *surface;

    status = _cairo_surface_get_extents (other->image, &extents);
    if (status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    surface = cairo_surface_create_similar (other->image,
					    CAIRO_CONTENT_COLOR_ALPHA,
					    extents.width,
					    extents.height);

    status = _cairo_meta_surface_replay (other->meta, surface);
    if (status) {
	cairo_surface_destroy (surface);
	surface = (cairo_surface_t*) &_cairo_surface_nil;
    }

    return surface;
#endif
}

const cairo_surface_backend_t test_meta_surface_backend = {
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_META,
    NULL, 
    _test_meta_surface_finish,
    _test_meta_surface_acquire_source_image,
    _test_meta_surface_release_source_image,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _test_meta_surface_show_page,
    NULL, 
    _test_meta_surface_intersect_clip_path,
    _test_meta_surface_get_extents,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _test_meta_surface_paint,
    _test_meta_surface_mask,
    _test_meta_surface_stroke,
    _test_meta_surface_fill,
    _test_meta_surface_show_glyphs,
    _test_meta_surface_snapshot
};
