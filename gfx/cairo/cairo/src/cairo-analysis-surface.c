

































#include "cairoint.h"

#include "cairo-analysis-surface-private.h"
#include "cairo-paginated-private.h"

typedef struct {
    cairo_surface_t base;
    int width;
    int height;

    cairo_surface_t	*target;

    cairo_bool_t fallback;
} cairo_analysis_surface_t;

static cairo_int_status_t
_cairo_analysis_surface_get_extents (void	 		*abstract_surface,
				     cairo_rectangle_int_t	*rectangle)
{
    cairo_analysis_surface_t *surface = abstract_surface;

    return _cairo_surface_get_extents (surface->target, rectangle);
}

static cairo_int_status_t
_cairo_analysis_surface_paint (void			*abstract_surface,
			      cairo_operator_t		op,
			      cairo_pattern_t		*source)
{
    cairo_analysis_surface_t *surface = abstract_surface;
    cairo_status_t	     status;

    if (!surface->target->backend->paint)
	status = CAIRO_INT_STATUS_UNSUPPORTED;
    else
	status = (*surface->target->backend->paint) (surface->target, op,
						     source);
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	surface->fallback = TRUE;
	status = CAIRO_STATUS_SUCCESS;
    }
    return status;
}

static cairo_int_status_t
_cairo_analysis_surface_mask (void		*abstract_surface,
			      cairo_operator_t	 op,
			      cairo_pattern_t	*source,
			      cairo_pattern_t	*mask)
{
    cairo_analysis_surface_t *surface = abstract_surface;
    cairo_status_t	     status;

    if (!surface->target->backend->mask)
	status = CAIRO_INT_STATUS_UNSUPPORTED;
    else
	status = (*surface->target->backend->mask) (surface->target, op,
						    source, mask);
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	surface->fallback = TRUE;
	status = CAIRO_STATUS_SUCCESS;
    }
    return status;
}

static cairo_int_status_t
_cairo_analysis_surface_stroke (void			*abstract_surface,
				cairo_operator_t	 op,
				cairo_pattern_t		*source,
				cairo_path_fixed_t	*path,
				cairo_stroke_style_t	*style,
				cairo_matrix_t		*ctm,
				cairo_matrix_t		*ctm_inverse,
				double			 tolerance,
				cairo_antialias_t	 antialias)
{
    cairo_analysis_surface_t *surface = abstract_surface;
    cairo_status_t	     status;

    if (!surface->target->backend->stroke)
	status = CAIRO_INT_STATUS_UNSUPPORTED;
    else
	status = (*surface->target->backend->stroke) (surface->target, op,
						      source, path, style,
						      ctm, ctm_inverse,
						      tolerance, antialias);
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	surface->fallback = TRUE;
	status = CAIRO_STATUS_SUCCESS;
    }
    return status;
}

static cairo_int_status_t
_cairo_analysis_surface_fill (void			*abstract_surface,
			      cairo_operator_t		 op,
			      cairo_pattern_t		*source,
			      cairo_path_fixed_t	*path,
			      cairo_fill_rule_t	 	 fill_rule,
			      double			 tolerance,
			      cairo_antialias_t	 	 antialias)
{
    cairo_analysis_surface_t *surface = abstract_surface;
    cairo_status_t	     status;

    if (!surface->target->backend->fill)
	status = CAIRO_INT_STATUS_UNSUPPORTED;
    else
	status = (*surface->target->backend->fill) (surface->target, op,
						    source, path, fill_rule,
						    tolerance, antialias);
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	surface->fallback = TRUE;
	status = CAIRO_STATUS_SUCCESS;
    }
    return status;
}

static cairo_int_status_t
_cairo_analysis_surface_show_glyphs (void		  *abstract_surface,
				     cairo_operator_t	   op,
				     cairo_pattern_t	  *source,
				     cairo_glyph_t	  *glyphs,
				     int		   num_glyphs,
				     cairo_scaled_font_t  *scaled_font)
{
    cairo_analysis_surface_t *surface = abstract_surface;
    cairo_status_t	     status;

    if (!surface->target->backend->show_glyphs)
	status = CAIRO_INT_STATUS_UNSUPPORTED;
    else
	status = (*surface->target->backend->show_glyphs) (surface->target, op,
							   source,
							   glyphs, num_glyphs,
							   scaled_font);
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	surface->fallback = TRUE;
	status = CAIRO_STATUS_SUCCESS;
    }
    return status;
}

static const cairo_surface_backend_t cairo_analysis_surface_backend = {
    CAIRO_INTERNAL_SURFACE_TYPE_ANALYSIS,
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
    NULL, 
    NULL, 
    _cairo_analysis_surface_get_extents,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_analysis_surface_paint,
    _cairo_analysis_surface_mask,
    _cairo_analysis_surface_stroke,
    _cairo_analysis_surface_fill,
    _cairo_analysis_surface_show_glyphs,
    NULL, 
};

cairo_private cairo_surface_t *
_cairo_analysis_surface_create (cairo_surface_t		*target,
				int			 width,
				int			 height)
{
    cairo_analysis_surface_t *surface;

    surface = malloc (sizeof (cairo_analysis_surface_t));
    if (surface == NULL)
	goto FAIL;

    

    _cairo_surface_init (&surface->base, &cairo_analysis_surface_backend,
			 CAIRO_CONTENT_COLOR_ALPHA);

    surface->width = width;
    surface->height = height;

    surface->target = target;
    surface->fallback = FALSE;

    return &surface->base;
FAIL:
    _cairo_error (CAIRO_STATUS_NO_MEMORY);
    return NULL;
}

cairo_private cairo_region_t *
_cairo_analysis_surface_get_supported (cairo_surface_t *abstract_surface)
{
    
    return NULL;
}

cairo_private cairo_region_t *
_cairo_analysis_surface_get_unsupported (cairo_surface_t *abstract_surface)
{
    
    return NULL;
}

cairo_private cairo_bool_t
_cairo_analysis_surface_has_unsupported (cairo_surface_t *abstract_surface)
{
    cairo_analysis_surface_t	*surface = (cairo_analysis_surface_t *) abstract_surface;

    return surface->fallback;
}
