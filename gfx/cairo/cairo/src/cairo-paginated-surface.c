











































#include "cairoint.h"

#include "cairo-paginated-private.h"
#include "cairo-paginated-surface-private.h"
#include "cairo-meta-surface-private.h"
#include "cairo-analysis-surface-private.h"

static const cairo_surface_backend_t cairo_paginated_surface_backend;

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
    cairo_status_t status;

    surface = malloc (sizeof (cairo_paginated_surface_t));
    if (unlikely (surface == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FAIL;
    }

    _cairo_surface_init (&surface->base, &cairo_paginated_surface_backend,
			 content);

    

    surface->base.type = cairo_surface_get_type (target);

    surface->target = cairo_surface_reference (target);

    surface->content = content;
    surface->width = width;
    surface->height = height;

    surface->backend = backend;

    surface->meta = _cairo_meta_surface_create (content, width, height);
    status = cairo_surface_status (surface->meta);
    if (unlikely (status))
	goto FAIL_CLEANUP_SURFACE;

    surface->page_num = 1;
    surface->page_is_blank = TRUE;

    return &surface->base;

  FAIL_CLEANUP_SURFACE:
    cairo_surface_destroy (target);
    free (surface);
  FAIL:
    return _cairo_surface_create_in_error (status);
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

cairo_status_t
_cairo_paginated_surface_set_size (cairo_surface_t	*surface,
				   int			 width,
				   int			 height)
{
    cairo_paginated_surface_t *paginated_surface;
    cairo_status_t status;

    assert (_cairo_surface_is_paginated (surface));

    paginated_surface = (cairo_paginated_surface_t *) surface;

    paginated_surface->width = width;
    paginated_surface->height = height;

    cairo_surface_destroy (paginated_surface->meta);
    paginated_surface->meta = _cairo_meta_surface_create (paginated_surface->content,
							  width, height);
    status = cairo_surface_status (paginated_surface->meta);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_paginated_surface_finish (void *abstract_surface)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    if (surface->page_is_blank == FALSE || surface->page_num == 1) {
	cairo_surface_show_page (abstract_surface);
	status = cairo_surface_status (abstract_surface);
    }

    if (status == CAIRO_STATUS_SUCCESS) {
	cairo_surface_finish (surface->target);
	status = cairo_surface_status (surface->target);
    }

    if (status == CAIRO_STATUS_SUCCESS) {
	cairo_surface_finish (surface->meta);
	status = cairo_surface_status (surface->meta);
    }

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
    cairo_status_t status;
    cairo_rectangle_int_t extents;

    status = _cairo_surface_get_extents (surface->target, &extents);
    if (unlikely (status))
	return status;

    image = _cairo_paginated_surface_create_image_surface (surface,
							   extents.width,
							   extents.height);

    status = _cairo_meta_surface_replay (surface->meta, image);
    if (unlikely (status)) {
	cairo_surface_destroy (image);
	return status;
    }

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
_paint_fallback_image (cairo_paginated_surface_t *surface,
		       cairo_rectangle_int_t     *rect)
{
    double x_scale = surface->base.x_fallback_resolution / surface->target->x_resolution;
    double y_scale = surface->base.y_fallback_resolution / surface->target->y_resolution;
    cairo_matrix_t matrix;
    int x, y, width, height;
    cairo_status_t status;
    cairo_surface_t *image;
    cairo_surface_pattern_t pattern;

    x = rect->x;
    y = rect->y;
    width = rect->width;
    height = rect->height;
    image = _cairo_paginated_surface_create_image_surface (surface,
							   ceil (width  * x_scale),
							   ceil (height * y_scale));
    _cairo_surface_set_device_scale (image, x_scale, y_scale);
    

    cairo_surface_set_device_offset (image, -x*x_scale, -y*y_scale);

    status = _cairo_meta_surface_replay (surface->meta, image);
    if (unlikely (status))
	goto CLEANUP_IMAGE;

    _cairo_pattern_init_for_surface (&pattern, image);
    cairo_matrix_init (&matrix, x_scale, 0, 0, y_scale, -x*x_scale, -y*y_scale);
    cairo_pattern_set_matrix (&pattern.base, &matrix);
    

    pattern.base.filter = CAIRO_FILTER_NEAREST;

    status = _cairo_surface_paint (surface->target,
				   CAIRO_OPERATOR_SOURCE,
				   &pattern.base, NULL);

    _cairo_pattern_fini (&pattern.base);
CLEANUP_IMAGE:
    cairo_surface_destroy (image);

    return status;
}

static cairo_int_status_t
_paint_page (cairo_paginated_surface_t *surface)
{
    cairo_surface_t *analysis;
    cairo_status_t status;
    cairo_bool_t has_supported, has_page_fallback, has_finegrained_fallback;

    if (surface->target->status)
	return surface->target->status;

    analysis = _cairo_analysis_surface_create (surface->target,
					       surface->width, surface->height);
    if (analysis->status)
	return _cairo_surface_set_error (surface->target, analysis->status);

    surface->backend->set_paginated_mode (surface->target,
	                                  CAIRO_PAGINATED_MODE_ANALYZE);
    status = _cairo_meta_surface_replay_and_create_regions (surface->meta,
	                                                    analysis);
    if (status || analysis->status) {
	if (status == CAIRO_STATUS_SUCCESS)
	    status = analysis->status;
	goto FAIL;
    }

     if (surface->backend->set_bounding_box) {
	 cairo_box_t bbox;

	 _cairo_analysis_surface_get_bounding_box (analysis, &bbox);
	 status = surface->backend->set_bounding_box (surface->target, &bbox);
	 if (unlikely (status))
	     goto FAIL;
     }

    if (surface->backend->set_fallback_images_required) {
	cairo_bool_t has_fallbacks = _cairo_analysis_surface_has_unsupported (analysis);

	status = surface->backend->set_fallback_images_required (surface->target,
								 has_fallbacks);
	if (unlikely (status))
	    goto FAIL;
    }

    

    if (surface->backend->supports_fine_grained_fallbacks != NULL &&
	surface->backend->supports_fine_grained_fallbacks (surface->target))
    {
	has_supported = _cairo_analysis_surface_has_supported (analysis);
	has_page_fallback = FALSE;
	has_finegrained_fallback = _cairo_analysis_surface_has_unsupported (analysis);
    }
    else
    {
	if (_cairo_analysis_surface_has_unsupported (analysis)) {
	    has_supported = FALSE;
	    has_page_fallback = TRUE;
	} else {
	    has_supported = TRUE;
	    has_page_fallback = FALSE;
	}
	has_finegrained_fallback = FALSE;
    }

    if (has_supported) {
	surface->backend->set_paginated_mode (surface->target,
		                              CAIRO_PAGINATED_MODE_RENDER);

	status = _cairo_meta_surface_replay_region (surface->meta,
						    surface->target,
						    CAIRO_META_REGION_NATIVE);
	assert (status != CAIRO_INT_STATUS_UNSUPPORTED);
	if (unlikely (status))
	    goto FAIL;
    }

    if (has_page_fallback) {
	cairo_rectangle_int_t rect;

	surface->backend->set_paginated_mode (surface->target,
		                              CAIRO_PAGINATED_MODE_FALLBACK);

	rect.x = 0;
	rect.y = 0;
	rect.width = surface->width;
	rect.height = surface->height;
	status = _paint_fallback_image (surface, &rect);
	if (unlikely (status))
	    goto FAIL;
    }

    if (has_finegrained_fallback) {
        cairo_region_t *region;
        int num_rects, i;

	surface->backend->set_paginated_mode (surface->target,
		                              CAIRO_PAGINATED_MODE_FALLBACK);

    
	status = _cairo_surface_intersect_clip_path (surface->target,
						     NULL,
						     CAIRO_FILL_RULE_WINDING,
						     CAIRO_GSTATE_TOLERANCE_DEFAULT,
						     CAIRO_ANTIALIAS_DEFAULT);
	if (unlikely (status))
	    goto FAIL;

	region = _cairo_analysis_surface_get_unsupported (analysis);

	num_rects = cairo_region_num_rectangles (region);
	for (i = 0; i < num_rects; i++) {
	    cairo_rectangle_int_t rect;

	    cairo_region_get_rectangle (region, i, &rect);
	    
	    status = _paint_fallback_image (surface, &rect);
	    
	    if (unlikely (status))
		goto FAIL;
	}
    }

  FAIL:
    cairo_surface_destroy (analysis);

    return _cairo_surface_set_error (surface->target, status);
}

static cairo_status_t
_start_page (cairo_paginated_surface_t *surface)
{
    if (surface->target->status)
	return surface->target->status;

    if (! surface->backend->start_page)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_surface_set_error (surface->target,
	                        surface->backend->start_page (surface->target));
}

static cairo_int_status_t
_cairo_paginated_surface_copy_page (void *abstract_surface)
{
    cairo_status_t status;
    cairo_paginated_surface_t *surface = abstract_surface;

    status = _start_page (surface);
    if (unlikely (status))
	return status;

    status = _paint_page (surface);
    if (unlikely (status))
	return status;

    surface->page_num++;

    






    cairo_surface_show_page (surface->target);
    return cairo_surface_status (surface->target);
}

static cairo_int_status_t
_cairo_paginated_surface_show_page (void *abstract_surface)
{
    cairo_status_t status;
    cairo_paginated_surface_t *surface = abstract_surface;

    status = _start_page (surface);
    if (unlikely (status))
	return status;

    status = _paint_page (surface);
    if (unlikely (status))
	return status;

    cairo_surface_show_page (surface->target);
    status = cairo_surface_status (surface->target);
    if (unlikely (status))
	return status;

    status = cairo_surface_status (surface->meta);
    if (unlikely (status))
	return status;

    cairo_surface_destroy (surface->meta);

    surface->meta = _cairo_meta_surface_create (surface->content,
						surface->width,
						surface->height);
    status = cairo_surface_status (surface->meta);
    if (unlikely (status))
	return status;

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
				      cairo_rectangle_int_t   *rectangle)
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
				const cairo_pattern_t	*source,
				cairo_rectangle_int_t   *extents)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    return _cairo_surface_paint (surface->meta, op, source, NULL);
}

static cairo_int_status_t
_cairo_paginated_surface_mask (void		*abstract_surface,
			       cairo_operator_t	 op,
			       const cairo_pattern_t	*source,
			       const cairo_pattern_t	*mask,
			       cairo_rectangle_int_t    *extents)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_mask (surface->meta, op, source, mask, NULL);
}

static cairo_int_status_t
_cairo_paginated_surface_stroke (void			*abstract_surface,
				 cairo_operator_t	 op,
				 const cairo_pattern_t	*source,
				 cairo_path_fixed_t	*path,
				 cairo_stroke_style_t	*style,
				 cairo_matrix_t		*ctm,
				 cairo_matrix_t		*ctm_inverse,
				 double			 tolerance,
				 cairo_antialias_t	 antialias,
				 cairo_rectangle_int_t  *extents)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    return _cairo_surface_stroke (surface->meta, op, source,
				  path, style,
				  ctm, ctm_inverse,
				  tolerance, antialias, NULL);
}

static cairo_int_status_t
_cairo_paginated_surface_fill (void			*abstract_surface,
			       cairo_operator_t		 op,
			       const cairo_pattern_t	*source,
			       cairo_path_fixed_t	*path,
			       cairo_fill_rule_t	 fill_rule,
			       double			 tolerance,
			       cairo_antialias_t	 antialias,
			       cairo_rectangle_int_t    *extents)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    return _cairo_surface_fill (surface->meta, op, source,
				path, fill_rule,
				tolerance, antialias, NULL);
}

static cairo_bool_t
_cairo_paginated_surface_has_show_text_glyphs (void *abstract_surface)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    return cairo_surface_has_show_text_glyphs (surface->target);
}

static cairo_int_status_t
_cairo_paginated_surface_show_text_glyphs (void			    *abstract_surface,
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
					   cairo_rectangle_int_t    *extents)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    









    status = _cairo_surface_show_text_glyphs (surface->meta, op, source,
					      utf8, utf8_len,
					      glyphs, num_glyphs,
					      clusters, num_clusters,
					      cluster_flags,
					      scaled_font, NULL);

    return status;
}

static cairo_surface_t *
_cairo_paginated_surface_snapshot (void *abstract_other)
{
    cairo_paginated_surface_t *other = abstract_other;

    return _cairo_surface_snapshot (other->meta);
}

static const cairo_surface_backend_t cairo_paginated_surface_backend = {
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
    NULL, 
    _cairo_paginated_surface_snapshot,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_paginated_surface_has_show_text_glyphs,
    _cairo_paginated_surface_show_text_glyphs
};
