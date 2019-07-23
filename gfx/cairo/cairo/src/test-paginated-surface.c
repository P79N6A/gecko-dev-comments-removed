














































#include "cairoint.h"

#include "test-paginated-surface.h"

#include "cairo-paginated-private.h"

typedef struct _test_paginated_surface {
    cairo_surface_t base;
    cairo_surface_t *target;
    cairo_paginated_mode_t paginated_mode;
} test_paginated_surface_t;

static const cairo_surface_backend_t test_paginated_surface_backend;
static const cairo_paginated_surface_backend_t test_paginated_surface_paginated_backend;

cairo_surface_t *
_cairo_test_paginated_surface_create_for_data (unsigned char		*data,
					 cairo_content_t	 content,
					 int		 	 width,
					 int		 	 height,
					 int		 	 stride)
{
    cairo_status_t status;
    cairo_surface_t *target;
    test_paginated_surface_t *surface;

    target =  _cairo_image_surface_create_for_data_with_content (data, content,
								width, height,
								stride);
    status = cairo_surface_status (target);
    if (status) {
	_cairo_error (status);
	return (cairo_surface_t *) &_cairo_surface_nil;
    }

    surface = malloc (sizeof (test_paginated_surface_t));
    if (surface == NULL) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t *) &_cairo_surface_nil;
    }

    _cairo_surface_init (&surface->base, &test_paginated_surface_backend,
			 content);

    surface->target = target;

    return _cairo_paginated_surface_create (&surface->base, content, width, height,
					    &test_paginated_surface_paginated_backend);
}

static cairo_status_t
_test_paginated_surface_finish (void *abstract_surface)
{
    test_paginated_surface_t *surface = abstract_surface;

    cairo_surface_destroy (surface->target);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_test_paginated_surface_set_clip_region (void *abstract_surface,
					 cairo_region_t *region)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    



























    surface->target->clip = surface->base.clip;

    return _cairo_image_surface_set_clip_region (surface->target, region);
}

static cairo_int_status_t
_test_paginated_surface_get_extents (void			*abstract_surface,
				     cairo_rectangle_int_t	*rectangle)
{
    test_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_get_extents (surface->target, rectangle);
}

static cairo_int_status_t
_test_paginated_surface_paint (void		*abstract_surface,
			       cairo_operator_t	 op,
			       cairo_pattern_t	*source)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_paint (surface->target, op, source);
}

static cairo_int_status_t
_test_paginated_surface_mask (void		*abstract_surface,
			      cairo_operator_t	 op,
			      cairo_pattern_t	*source,
			      cairo_pattern_t	*mask)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_mask (surface->target, op, source, mask);
}

static cairo_int_status_t
_test_paginated_surface_stroke (void			*abstract_surface,
				cairo_operator_t	 op,
				cairo_pattern_t		*source,
				cairo_path_fixed_t	*path,
				cairo_stroke_style_t	*style,
				cairo_matrix_t		*ctm,
				cairo_matrix_t		*ctm_inverse,
				double			 tolerance,
				cairo_antialias_t	 antialias)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_stroke (surface->target, op, source,
				  path, style,
				  ctm, ctm_inverse,
				  tolerance, antialias);
}

static cairo_int_status_t
_test_paginated_surface_fill (void			*abstract_surface,
			      cairo_operator_t		 op,
			      cairo_pattern_t		*source,
			      cairo_path_fixed_t	*path,
			      cairo_fill_rule_t		 fill_rule,
			      double			 tolerance,
			      cairo_antialias_t		 antialias)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_fill (surface->target, op, source,
				path, fill_rule,
				tolerance, antialias);
}

static cairo_int_status_t
_test_paginated_surface_show_glyphs (void			*abstract_surface,
				     cairo_operator_t		 op,
				     cairo_pattern_t		*source,
				     cairo_glyph_t		*glyphs,
				     int			 num_glyphs,
				     cairo_scaled_font_t	*scaled_font)
{
    test_paginated_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    









    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);
    status = _cairo_surface_show_glyphs (surface->target, op, source,
					 glyphs, num_glyphs, scaled_font);
    CAIRO_MUTEX_LOCK (scaled_font->mutex);

    return status;
}

static void
_test_paginated_surface_set_paginated_mode (void			*abstract_surface,
					    cairo_paginated_mode_t	 mode)
{
    test_paginated_surface_t *surface = abstract_surface;

    surface->paginated_mode = mode;
}

static const cairo_surface_backend_t test_paginated_surface_backend = {
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_PAGINATED,

    


    NULL, 
    _test_paginated_surface_finish,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _test_paginated_surface_set_clip_region,
    NULL, 
    _test_paginated_surface_get_extents,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 

    


    _test_paginated_surface_paint,
    _test_paginated_surface_mask,
    _test_paginated_surface_stroke,
    _test_paginated_surface_fill,
    _test_paginated_surface_show_glyphs,
    NULL 
};

static const cairo_paginated_surface_backend_t test_paginated_surface_paginated_backend = {
    NULL, 
    _test_paginated_surface_set_paginated_mode
};
