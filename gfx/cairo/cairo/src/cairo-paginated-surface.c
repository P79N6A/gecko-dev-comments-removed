









































#include "cairoint.h"

#include "cairo-paginated-surface-private.h"
#include "cairo-meta-surface-private.h"
#include "cairo-analysis-surface-private.h"

typedef struct _cairo_paginated_surface {
    cairo_surface_t base;

    
    cairo_surface_t *target;

    cairo_content_t content;

    




    int width;
    int height;

    
    const cairo_paginated_surface_backend_t *backend;

    


    cairo_surface_t *meta;

    int page_num;
    cairo_bool_t page_is_blank;

} cairo_paginated_surface_t;

const cairo_private cairo_surface_backend_t cairo_paginated_surface_backend;

static cairo_int_status_t
_cairo_paginated_surface_show_page (void *abstract_surface);

static cairo_surface_t *
_cairo_paginated_surface_create_similar (void			*abstract_surface,
					 cairo_content_t	 content,
					 int			 width,
					 int			 height)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    return cairo_surface_create_similar (surface->target, content,
					 width, height);
}

cairo_surface_t *
_cairo_paginated_surface_create (cairo_surface_t				*target,
				 cairo_content_t				 content,
				 int						 width,
				 int						 height,
				 const cairo_paginated_surface_backend_t	*backend)
{
    cairo_paginated_surface_t *surface;

    surface = malloc (sizeof (cairo_paginated_surface_t));
    if (surface == NULL)
	goto FAIL;

    _cairo_surface_init (&surface->base, &cairo_paginated_surface_backend,
			 content);

    

    surface->base.type = cairo_surface_get_type (target);

    surface->target = target;

    surface->content = content;
    surface->width = width;
    surface->height = height;

    surface->backend = backend;

    surface->meta = _cairo_meta_surface_create (content, width, height);
    if (cairo_surface_status (surface->meta))
	goto FAIL_CLEANUP_SURFACE;

    surface->page_num = 1;
    surface->page_is_blank = TRUE;

    return &surface->base;

  FAIL_CLEANUP_SURFACE:
    free (surface);
  FAIL:
    _cairo_error (CAIRO_STATUS_NO_MEMORY);
    return (cairo_surface_t*) &_cairo_surface_nil;
}

cairo_bool_t
_cairo_surface_is_paginated (cairo_surface_t *surface)
{
    return surface->backend == &cairo_paginated_surface_backend;
}

cairo_surface_t *
_cairo_paginated_surface_get_target (cairo_surface_t *surface)
{
    cairo_paginated_surface_t *paginated_surface;

    assert (_cairo_surface_is_paginated (surface));

    paginated_surface = (cairo_paginated_surface_t *) surface;

    return paginated_surface->target;
}

static cairo_status_t
_cairo_paginated_surface_finish (void *abstract_surface)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    if (surface->page_is_blank == FALSE || surface->page_num == 1)
	status = _cairo_paginated_surface_show_page (abstract_surface);

    if (status == CAIRO_STATUS_SUCCESS)
	cairo_surface_finish (surface->target);

    if (status == CAIRO_STATUS_SUCCESS)
	cairo_surface_finish (surface->meta);

    cairo_surface_destroy (surface->target);

    cairo_surface_destroy (surface->meta);

    return status;
}

static cairo_surface_t *
_cairo_paginated_surface_create_image_surface (void	       *abstract_surface,
					       int		width,
					       int		height)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_surface_t *image;
    cairo_font_options_t options;

    image = _cairo_image_surface_create_with_content (surface->content,
						      width,
						      height);

    cairo_surface_get_font_options (&surface->base, &options);
    _cairo_surface_set_font_options (image, &options);

    return image;
}

static cairo_status_t
_cairo_paginated_surface_acquire_source_image (void	       *abstract_surface,
					       cairo_image_surface_t **image_out,
					       void		   **image_extra)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_surface_t *image;
    cairo_rectangle_int16_t extents;

    _cairo_surface_get_extents (surface->target, &extents);

    image = _cairo_paginated_surface_create_image_surface (surface,
							   extents.width,
							   extents.height);

    _cairo_meta_surface_replay (surface->meta, image);

    *image_out = (cairo_image_surface_t*) image;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_paginated_surface_release_source_image (void	  *abstract_surface,
					       cairo_image_surface_t *image,
					       void	       *image_extra)
{
    cairo_surface_destroy (&image->base);
}

static cairo_int_status_t
_paint_page (cairo_paginated_surface_t *surface)
{
    cairo_surface_t *analysis;
    cairo_surface_t *image;
    cairo_pattern_t *pattern;
    cairo_status_t status;

    analysis = _cairo_analysis_surface_create (surface->target,
					       surface->width, surface->height);

    surface->backend->set_paginated_mode (surface->target, CAIRO_PAGINATED_MODE_ANALYZE);
    _cairo_meta_surface_replay (surface->meta, analysis);
    surface->backend->set_paginated_mode (surface->target, CAIRO_PAGINATED_MODE_RENDER);

    if (analysis->status) {
	status = analysis->status;
	cairo_surface_destroy (analysis);
	return status;
    }

    if (_cairo_analysis_surface_has_unsupported (analysis))
    {
	double x_scale = surface->base.x_fallback_resolution / 72.0;
	double y_scale = surface->base.y_fallback_resolution / 72.0;
	cairo_matrix_t matrix;

	image = _cairo_paginated_surface_create_image_surface (surface,
							       surface->width  * x_scale,
							       surface->height * y_scale);
	_cairo_surface_set_device_scale (image, x_scale, y_scale);

	_cairo_meta_surface_replay (surface->meta, image);

	pattern = cairo_pattern_create_for_surface (image);
	cairo_matrix_init_scale (&matrix, x_scale, y_scale);
	cairo_pattern_set_matrix (pattern, &matrix);

	_cairo_surface_paint (surface->target, CAIRO_OPERATOR_SOURCE, pattern);

	cairo_pattern_destroy (pattern);

	cairo_surface_destroy (image);
    }
    else
    {
	_cairo_meta_surface_replay (surface->meta, surface->target);
    }

    cairo_surface_destroy (analysis);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_start_page (cairo_paginated_surface_t *surface)
{
    if (! surface->backend->start_page)
	return CAIRO_STATUS_SUCCESS;

    return (surface->backend->start_page) (surface->target);
}

static cairo_int_status_t
_cairo_paginated_surface_copy_page (void *abstract_surface)
{
    cairo_status_t status;
    cairo_paginated_surface_t *surface = abstract_surface;

    status = _start_page (surface);
    if (status)
	return status;

    _paint_page (surface);

    surface->page_num++;

    







    return _cairo_surface_show_page (surface->target);
}

static cairo_int_status_t
_cairo_paginated_surface_show_page (void *abstract_surface)
{
    cairo_status_t status;
    cairo_paginated_surface_t *surface = abstract_surface;

    status = _start_page (surface);
    if (status)
	return status;

    _paint_page (surface);

    _cairo_surface_show_page (surface->target);

    cairo_surface_destroy (surface->meta);

    surface->meta = _cairo_meta_surface_create (surface->content,
						surface->width, surface->height);
    if (cairo_surface_status (surface->meta))
	return cairo_surface_status (surface->meta);

    surface->page_num++;
    surface->page_is_blank = TRUE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_paginated_surface_intersect_clip_path (void	  *abstract_surface,
					      cairo_path_fixed_t *path,
					      cairo_fill_rule_t	  fill_rule,
					      double		  tolerance,
					      cairo_antialias_t	  antialias)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_intersect_clip_path (surface->meta,
					       path, fill_rule,
					       tolerance, antialias);
}

static cairo_int_status_t
_cairo_paginated_surface_get_extents (void	              *abstract_surface,
				      cairo_rectangle_int16_t *rectangle)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_get_extents (surface->target, rectangle);
}

static void
_cairo_paginated_surface_get_font_options (void                  *abstract_surface,
					   cairo_font_options_t  *options)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    cairo_surface_get_font_options (surface->target, options);
}

static cairo_int_status_t
_cairo_paginated_surface_paint (void			*abstract_surface,
				cairo_operator_t	 op,
				cairo_pattern_t		*source)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    return _cairo_surface_paint (surface->meta, op, source);
}

static cairo_int_status_t
_cairo_paginated_surface_mask (void		*abstract_surface,
			       cairo_operator_t	 op,
			       cairo_pattern_t	*source,
			       cairo_pattern_t	*mask)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_mask (surface->meta, op, source, mask);
}

static cairo_int_status_t
_cairo_paginated_surface_stroke (void			*abstract_surface,
				 cairo_operator_t	 op,
				 cairo_pattern_t	*source,
				 cairo_path_fixed_t	*path,
				 cairo_stroke_style_t	*style,
				 cairo_matrix_t		*ctm,
				 cairo_matrix_t		*ctm_inverse,
				 double			 tolerance,
				 cairo_antialias_t	 antialias)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    return _cairo_surface_stroke (surface->meta, op, source,
				  path, style,
				  ctm, ctm_inverse,
				  tolerance, antialias);
}

static cairo_int_status_t
_cairo_paginated_surface_fill (void			*abstract_surface,
			       cairo_operator_t		 op,
			       cairo_pattern_t		*source,
			       cairo_path_fixed_t	*path,
			       cairo_fill_rule_t	 fill_rule,
			       double			 tolerance,
			       cairo_antialias_t	 antialias)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    return _cairo_surface_fill (surface->meta, op, source,
				path, fill_rule,
				tolerance, antialias);
}

static cairo_int_status_t
_cairo_paginated_surface_show_glyphs (void			*abstract_surface,
				      cairo_operator_t		 op,
				      cairo_pattern_t		*source,
				      cairo_glyph_t		*glyphs,
				      int			 num_glyphs,
				      cairo_scaled_font_t	*scaled_font)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    









    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);
    status = _cairo_surface_show_glyphs (surface->meta, op, source,
					 glyphs, num_glyphs,
					 scaled_font);
    CAIRO_MUTEX_LOCK (scaled_font->mutex);

    return status;
}

static cairo_surface_t *
_cairo_paginated_surface_snapshot (void *abstract_other)
{
    cairo_paginated_surface_t *other = abstract_other;

    











#if 0
    return _cairo_surface_snapshot (other->meta);
#else
    cairo_rectangle_int16_t extents;
    cairo_surface_t *surface;

    _cairo_surface_get_extents (other->target, &extents);

    surface = _cairo_paginated_surface_create_image_surface (other,
							     extents.width,
							     extents.height);

    _cairo_meta_surface_replay (other->meta, surface);

    return surface;
#endif
}

const cairo_surface_backend_t cairo_paginated_surface_backend = {
    CAIRO_INTERNAL_SURFACE_TYPE_PAGINATED,
    _cairo_paginated_surface_create_similar,
    _cairo_paginated_surface_finish,
    _cairo_paginated_surface_acquire_source_image,
    _cairo_paginated_surface_release_source_image,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_paginated_surface_copy_page,
    _cairo_paginated_surface_show_page,
    NULL, 
    _cairo_paginated_surface_intersect_clip_path,
    _cairo_paginated_surface_get_extents,
    NULL, 
    _cairo_paginated_surface_get_font_options,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_paginated_surface_paint,
    _cairo_paginated_surface_mask,
    _cairo_paginated_surface_stroke,
    _cairo_paginated_surface_fill,
    _cairo_paginated_surface_show_glyphs,
    _cairo_paginated_surface_snapshot
};
