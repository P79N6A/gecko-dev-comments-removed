














































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
_cairo_test_paginated_surface_create (cairo_surface_t *target)
{
    cairo_status_t status;
    cairo_surface_t *paginated;
    test_paginated_surface_t *surface;

    status = cairo_surface_status (target);
    if (unlikely (status))
	return _cairo_surface_create_in_error (status);

    surface = malloc (sizeof (test_paginated_surface_t));
    if (unlikely (surface == NULL))
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    _cairo_surface_init (&surface->base, &test_paginated_surface_backend,
			 target->content);

    surface->target = cairo_surface_reference (target);

    paginated =  _cairo_paginated_surface_create (&surface->base,
						  target->content,
						  &test_paginated_surface_paginated_backend);
    status = paginated->status;
    if (status == CAIRO_STATUS_SUCCESS) {
	
	cairo_surface_destroy (&surface->base);
	return paginated;
    }

    cairo_surface_destroy (target);
    free (surface);
    return _cairo_surface_create_in_error (status);
}

static cairo_status_t
_test_paginated_surface_finish (void *abstract_surface)
{
    test_paginated_surface_t *surface = abstract_surface;

    cairo_surface_destroy (surface->target);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_bool_t
_test_paginated_surface_get_extents (void			*abstract_surface,
				     cairo_rectangle_int_t	*rectangle)
{
    test_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_get_extents (surface->target, rectangle);
}

static cairo_int_status_t
_test_paginated_surface_paint (void		*abstract_surface,
			       cairo_operator_t	 op,
			       const cairo_pattern_t	*source,
			       cairo_clip_t		*clip)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_paint (surface->target, op, source, clip);
}

static cairo_int_status_t
_test_paginated_surface_mask (void		*abstract_surface,
			      cairo_operator_t	 op,
			      const cairo_pattern_t	*source,
			      const cairo_pattern_t	*mask,
			      cairo_clip_t		*clip)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_mask (surface->target,
				op, source, mask, clip);
}

static cairo_int_status_t
_test_paginated_surface_stroke (void				*abstract_surface,
				cairo_operator_t		 op,
				const cairo_pattern_t		*source,
				cairo_path_fixed_t		*path,
				cairo_stroke_style_t		*style,
				cairo_matrix_t			*ctm,
				cairo_matrix_t			*ctm_inverse,
				double				 tolerance,
				cairo_antialias_t		 antialias,
				cairo_clip_t			*clip)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_stroke (surface->target, op, source,
				  path, style,
				  ctm, ctm_inverse,
				  tolerance, antialias,
				  clip);
}

static cairo_int_status_t
_test_paginated_surface_fill (void				*abstract_surface,
			      cairo_operator_t			 op,
			      const cairo_pattern_t		*source,
			      cairo_path_fixed_t		*path,
			      cairo_fill_rule_t			 fill_rule,
			      double				 tolerance,
			      cairo_antialias_t			 antialias,
			      cairo_clip_t			*clip)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_fill (surface->target, op, source,
				path, fill_rule,
				tolerance, antialias,
				clip);
}

static cairo_bool_t
_test_paginated_surface_has_show_text_glyphs (void *abstract_surface)
{
    test_paginated_surface_t *surface = abstract_surface;

    return cairo_surface_has_show_text_glyphs (surface->target);
}

static cairo_int_status_t
_test_paginated_surface_show_text_glyphs (void			    *abstract_surface,
					  cairo_operator_t	     op,
					  const cairo_pattern_t	    *source,
					  const char		    *utf8,
					  int			     utf8_len,
					  cairo_glyph_t		    *glyphs,
					  int			     num_glyphs,
					  const cairo_text_cluster_t *clusters,
					  int			     num_clusters,
					  cairo_text_cluster_flags_t cluster_flags,
					  cairo_scaled_font_t	    *scaled_font,
					  cairo_clip_t		    *clip)
{
    test_paginated_surface_t *surface = abstract_surface;

    if (surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_show_text_glyphs (surface->target, op, source,
					    utf8, utf8_len,
					    glyphs, num_glyphs,
					    clusters, num_clusters,
					    cluster_flags,
					    scaled_font,
					    clip);
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
    NULL, 
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
    NULL, 

    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 

    _test_paginated_surface_has_show_text_glyphs,
    _test_paginated_surface_show_text_glyphs
};

static const cairo_paginated_surface_backend_t test_paginated_surface_paginated_backend = {
    NULL, 
    _test_paginated_surface_set_paginated_mode
};
