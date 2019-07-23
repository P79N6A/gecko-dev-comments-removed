











































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
    if (surface == NULL) {
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
    if (status)
	goto FAIL_CLEANUP_SURFACE;

    surface->page_num = 1;
    surface->page_is_blank = TRUE;

    return &surface->base;

  FAIL_CLEANUP_SURFACE:
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
    if (status)
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
    if (status)
	return status;

    image = _cairo_paginated_surface_create_image_surface (surface,
							   extents.width,
							   extents.height);

    status = _cairo_meta_surface_replay (surface->meta, image);
    if (status) {
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
		       cairo_box_int_t           *box)
{
    double x_scale = surface->base.x_fallback_resolution / surface->target->x_resolution;
    double y_scale = surface->base.y_fallback_resolution / surface->target->y_resolution;
    cairo_matrix_t matrix;
    int x, y, width, height;
    cairo_status_t status;
    cairo_surface_t *image;
    cairo_pattern_t *pattern;

    x = box->p1.x;
    y = box->p1.y;
    width = box->p2.x - x;
    height = box->p2.y - y;
    image = _cairo_paginated_surface_create_image_surface (surface,
							   ceil (width  * x_scale),
							   ceil (height * y_scale));
    _cairo_surface_set_device_scale (image, x_scale, y_scale);
    

    cairo_surface_set_device_offset (image, -x*x_scale, -y*y_scale);

    status = _cairo_meta_surface_replay (surface->meta, image);
    if (status)
	goto CLEANUP_IMAGE;

    pattern = cairo_pattern_create_for_surface (image);
    cairo_matrix_init (&matrix, x_scale, 0, 0, y_scale, -x*x_scale, -y*y_scale);
    cairo_pattern_set_matrix (pattern, &matrix);

    status = _cairo_surface_paint (surface->target,
				   CAIRO_OPERATOR_SOURCE,
				   pattern);

    cairo_pattern_destroy (pattern);
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

    surface->backend->set_paginated_mode (surface->target, CAIRO_PAGINATED_MODE_ANALYZE);
    status = _cairo_meta_surface_replay_and_create_regions (surface->meta, analysis);
    if (status || analysis->status) {
	if (status == CAIRO_STATUS_SUCCESS)
	    status = analysis->status;
	goto FAIL;
    }

     if (surface->backend->set_bounding_box) {
	 cairo_box_t bbox;

	 _cairo_analysis_surface_get_bounding_box (analysis, &bbox);
	 status = surface->backend->set_bounding_box (surface->target, &bbox);
	 if (status)
	     goto FAIL;
     }

    if (surface->backend->set_fallback_images_required) {
	cairo_bool_t has_fallbacks = _cairo_analysis_surface_has_unsupported (analysis);

	status = surface->backend->set_fallback_images_required (surface->target,
								 has_fallbacks);
	if (status)
	    goto FAIL;
    }

    surface->backend->set_paginated_mode (surface->target, CAIRO_PAGINATED_MODE_RENDER);

    

    switch (surface->target->type) {
        case CAIRO_SURFACE_TYPE_PDF:
        case CAIRO_SURFACE_TYPE_PS:
        case CAIRO_SURFACE_TYPE_WIN32_PRINTING:
            has_supported = _cairo_analysis_surface_has_supported (analysis);
            has_page_fallback = FALSE;
            has_finegrained_fallback = _cairo_analysis_surface_has_unsupported (analysis);
            break;

	case CAIRO_SURFACE_TYPE_IMAGE:
	case CAIRO_SURFACE_TYPE_XLIB:
	case CAIRO_SURFACE_TYPE_XCB:
	case CAIRO_SURFACE_TYPE_GLITZ:
	case CAIRO_SURFACE_TYPE_QUARTZ:
	case CAIRO_SURFACE_TYPE_QUARTZ_IMAGE:
	case CAIRO_SURFACE_TYPE_WIN32:
	case CAIRO_SURFACE_TYPE_BEOS:
	case CAIRO_SURFACE_TYPE_DIRECTFB:
	case CAIRO_SURFACE_TYPE_SVG:
	case CAIRO_SURFACE_TYPE_OS2:
        default:
            if (_cairo_analysis_surface_has_unsupported (analysis)) {
                has_supported = FALSE;
                has_page_fallback = TRUE;
            } else {
                has_supported = TRUE;
                has_page_fallback = FALSE;
            }
            has_finegrained_fallback = FALSE;
            break;
    }

    if (has_supported) {
	status = _cairo_meta_surface_replay_region (surface->meta,
						    surface->target,
						    CAIRO_META_REGION_NATIVE);
	assert (status != CAIRO_INT_STATUS_UNSUPPORTED);
	if (status)
	    goto FAIL;
    }

    if (has_page_fallback)
    {
	cairo_box_int_t box;

	box.p1.x = 0;
	box.p1.y = 0;
	box.p2.x = surface->width;
	box.p2.y = surface->height;
	status = _paint_fallback_image (surface, &box);
	if (status)
	    goto FAIL;
    }

    if (has_finegrained_fallback)
    {
        cairo_region_t *region;
        cairo_box_int_t *boxes;
        int num_boxes, i;

	surface->backend->set_paginated_mode (surface->target, CAIRO_PAGINATED_MODE_FALLBACK);

    
	status = _cairo_surface_intersect_clip_path (surface->target,
						     NULL,
						     CAIRO_FILL_RULE_WINDING,
						     CAIRO_GSTATE_TOLERANCE_DEFAULT,
						     CAIRO_ANTIALIAS_DEFAULT);
	if (status)
	    goto FAIL;

	region = _cairo_analysis_surface_get_unsupported (analysis);
	status = _cairo_region_get_boxes (region, &num_boxes, &boxes);
	if (status)
	    goto FAIL;
	for (i = 0; i < num_boxes; i++) {
	    status = _paint_fallback_image (surface, &boxes[i]);
	    if (status) {
                _cairo_region_boxes_fini (region, boxes);
		goto FAIL;
            }
	}
        _cairo_region_boxes_fini (region, boxes);
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
    if (status)
	return status;

    status = _paint_page (surface);
    if (status)
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
    if (status)
	return status;

    status = _paint_page (surface);
    if (status)
	return status;

    cairo_surface_show_page (surface->target);
    status = cairo_surface_status (surface->target);
    if (status)
	return status;

    status = cairo_surface_status (surface->meta);
    if (status)
	return status;

    cairo_surface_destroy (surface->meta);

    surface->meta = _cairo_meta_surface_create (surface->content,
						surface->width,
						surface->height);
    status = cairo_surface_status (surface->meta);
    if (status)
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

static cairo_bool_t
_cairo_paginated_surface_has_show_text_glyphs (void *abstract_surface)
{
    cairo_paginated_surface_t *surface = abstract_surface;

    return _cairo_surface_has_show_text_glyphs (surface->target);
}

static cairo_int_status_t
_cairo_paginated_surface_show_text_glyphs (void			    *abstract_surface,
					  cairo_operator_t	     op,
					  cairo_pattern_t	    *source,
					  const char		    *utf8,
					  int			     utf8_len,
					  cairo_glyph_t		    *glyphs,
					  int			     num_glyphs,
					  const cairo_text_cluster_t *clusters,
					  int			     num_clusters,
					  cairo_bool_t		     backward,
					  cairo_scaled_font_t	    *scaled_font)
{
    cairo_paginated_surface_t *surface = abstract_surface;
    cairo_int_status_t status;

    
    if (surface->page_is_blank && op == CAIRO_OPERATOR_CLEAR)
	return CAIRO_STATUS_SUCCESS;

    surface->page_is_blank = FALSE;

    









    CAIRO_MUTEX_UNLOCK (scaled_font->mutex);
    status = _cairo_surface_show_text_glyphs (surface->meta, op, source,
					      utf8, utf8_len,
					      glyphs, num_glyphs,
					      clusters, num_clusters,
					      backward,
					      scaled_font);
    CAIRO_MUTEX_LOCK (scaled_font->mutex);

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
    _cairo_paginated_surface_has_show_text_glyphs,
    _cairo_paginated_surface_show_text_glyphs
};
