





































#include "cairoint.h"

#include "cairo-surface-fallback-private.h"
#include "cairo-clip-private.h"

#define DEFINE_NIL_SURFACE(status, name)			\
const cairo_surface_t name = {					\
    &cairo_image_surface_backend,	/* backend */		\
    CAIRO_SURFACE_TYPE_IMAGE,					\
    CAIRO_CONTENT_COLOR,					\
    CAIRO_REF_COUNT_INVALID,		/* ref_count */		\
    status,				/* status */		\
    FALSE,				/* finished */		\
    { 0,	/* size */					\
      0,	/* num_elements */				\
      0,	/* element_size */				\
      NULL,	/* elements */					\
    },					/* user_data */		\
    { 1.0, 0.0,							\
      0.0, 1.0,							\
      0.0, 0.0							\
    },					/* device_transform */	\
    { 1.0, 0.0,							\
      0.0, 1.0,							\
      0.0, 0.0							\
    },					/* device_transform_inverse */	\
    0.0,				/* x_fallback_resolution */	\
    0.0,				/* y_fallback_resolution */	\
    NULL,				/* clip */		\
    0,					/* next_clip_serial */	\
    0,					/* current_clip_serial */	\
    FALSE,				/* is_snapshot */	\
    FALSE,				/* has_font_options */	\
    { CAIRO_ANTIALIAS_DEFAULT,					\
      CAIRO_SUBPIXEL_ORDER_DEFAULT,				\
      CAIRO_HINT_STYLE_DEFAULT,					\
      CAIRO_HINT_METRICS_DEFAULT				\
    }					/* font_options */	\
}

DEFINE_NIL_SURFACE(CAIRO_STATUS_NO_MEMORY, _cairo_surface_nil);
DEFINE_NIL_SURFACE(CAIRO_STATUS_FILE_NOT_FOUND, _cairo_surface_nil_file_not_found);
DEFINE_NIL_SURFACE(CAIRO_STATUS_READ_ERROR, _cairo_surface_nil_read_error);
DEFINE_NIL_SURFACE(CAIRO_STATUS_WRITE_ERROR, _cairo_surface_nil_write_error);

static cairo_status_t
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t *pattern,
					     cairo_surface_t *destination,
					     cairo_pattern_t *pattern_out);

















void
_cairo_surface_set_error (cairo_surface_t *surface,
			  cairo_status_t status)
{
    


    if (surface->status == CAIRO_STATUS_SUCCESS)
	surface->status = status;

    _cairo_error (status);
}












cairo_surface_type_t
cairo_surface_get_type (cairo_surface_t *surface)
{
    



    return surface->type;
}
slim_hidden_def (cairo_surface_get_type);













cairo_content_t
cairo_surface_get_content (cairo_surface_t *surface)
{
    return surface->content;
}
slim_hidden_def(cairo_surface_get_content);













cairo_status_t
cairo_surface_status (cairo_surface_t *surface)
{
    return surface->status;
}
slim_hidden_def (cairo_surface_status);

void
_cairo_surface_init (cairo_surface_t			*surface,
		     const cairo_surface_backend_t	*backend,
		     cairo_content_t			 content)
{
    CAIRO_MUTEX_INITIALIZE ();

    surface->backend = backend;
    surface->content = content;
    surface->type = backend->type;

    surface->ref_count = 1;
    surface->status = CAIRO_STATUS_SUCCESS;
    surface->finished = FALSE;

    _cairo_user_data_array_init (&surface->user_data);

    cairo_matrix_init_identity (&surface->device_transform);
    cairo_matrix_init_identity (&surface->device_transform_inverse);

    surface->x_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;
    surface->y_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;

    surface->clip = NULL;
    surface->next_clip_serial = 0;
    surface->current_clip_serial = 0;

    surface->is_snapshot = FALSE;

    surface->has_font_options = FALSE;
}

cairo_surface_t *
_cairo_surface_create_similar_scratch (cairo_surface_t *other,
				       cairo_content_t	content,
				       int		width,
				       int		height)
{
    cairo_surface_t *surface = NULL;
    cairo_font_options_t options;

    cairo_format_t format = _cairo_format_from_content (content);

    if (other->status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    if (other->backend->create_similar) {
	surface = other->backend->create_similar (other, content, width, height);
	

	if (surface && surface->status) {
	    cairo_surface_destroy (surface);
	    surface = NULL;
	}
    }

    if (surface == NULL)
	surface = cairo_image_surface_create (format, width, height);

    
    if (surface->status)
	return surface;

    cairo_surface_get_font_options (other, &options);
    _cairo_surface_set_font_options (surface, &options);

    cairo_surface_set_fallback_resolution (surface,
					   other->x_fallback_resolution,
					   other->y_fallback_resolution);

    return surface;
}


























cairo_surface_t *
cairo_surface_create_similar (cairo_surface_t  *other,
			      cairo_content_t	content,
			      int		width,
			      int		height)
{
    if (other->status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    if (! CAIRO_CONTENT_VALID (content)) {
	_cairo_error (CAIRO_STATUS_INVALID_CONTENT);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    return _cairo_surface_create_similar_solid (other, content,
						width, height,
						CAIRO_COLOR_TRANSPARENT,
						NULL);
}
slim_hidden_def (cairo_surface_create_similar);

cairo_surface_t *
_cairo_surface_create_similar_solid (cairo_surface_t	 *other,
				     cairo_content_t	  content,
				     int		  width,
				     int		  height,
				     const cairo_color_t *color,
				     cairo_pattern_t	 *pattern)
{
    cairo_status_t status;
    cairo_surface_t *surface;
    cairo_pattern_t *source;

    surface = _cairo_surface_create_similar_scratch (other, content,
						     width, height);
    if (surface->status) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    if (pattern == NULL) {
	source = _cairo_pattern_create_solid (color, content);
	if (source->status) {
	    cairo_surface_destroy (surface);
	    _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    return (cairo_surface_t*) &_cairo_surface_nil;
	}
    } else
	source = pattern;

    status = _cairo_surface_paint (surface,
				   color == CAIRO_COLOR_TRANSPARENT ?
				   CAIRO_OPERATOR_CLEAR :
				   CAIRO_OPERATOR_SOURCE, source);

    if (source != pattern)
	cairo_pattern_destroy (source);

    if (status) {
	cairo_surface_destroy (surface);
	_cairo_error (status);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    return surface;
}

cairo_clip_mode_t
_cairo_surface_get_clip_mode (cairo_surface_t *surface)
{
    if (surface->backend->intersect_clip_path != NULL)
	return CAIRO_CLIP_MODE_PATH;
    else if (surface->backend->set_clip_region != NULL)
	return CAIRO_CLIP_MODE_REGION;
    else
	return CAIRO_CLIP_MODE_MASK;
}














cairo_surface_t *
cairo_surface_reference (cairo_surface_t *surface)
{
    if (surface == NULL || surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return surface;

    assert (surface->ref_count > 0);

    surface->ref_count++;

    return surface;
}
slim_hidden_def (cairo_surface_reference);









void
cairo_surface_destroy (cairo_surface_t *surface)
{
    if (surface == NULL || surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return;

    assert (surface->ref_count > 0);

    surface->ref_count--;
    if (surface->ref_count)
	return;

    if (! surface->finished)
	cairo_surface_finish (surface);

    _cairo_user_data_array_fini (&surface->user_data);

    free (surface);
}
slim_hidden_def(cairo_surface_destroy);








cairo_status_t
_cairo_surface_reset (cairo_surface_t *surface)
{
    if (surface == NULL || surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return CAIRO_STATUS_SUCCESS;

    assert (surface->ref_count == 1);

    _cairo_user_data_array_fini (&surface->user_data);

    if (surface->backend->reset != NULL) {
	cairo_status_t status = surface->backend->reset (surface);
	if (status)
	    return status;
    }

    _cairo_surface_init (surface, surface->backend, surface->content);

    return CAIRO_STATUS_SUCCESS;
}












unsigned int
cairo_surface_get_reference_count (cairo_surface_t *surface)
{
    if (surface == NULL || surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return 0;

    return surface->ref_count;
}



















void
cairo_surface_finish (cairo_surface_t *surface)
{
    cairo_status_t status;

    if (surface == NULL)
	return;

    if (surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    if (surface->backend->finish == NULL) {
	surface->finished = TRUE;
	return;
    }

    if (!surface->status && surface->backend->flush) {
	status = surface->backend->flush (surface);
	if (status) {
	    _cairo_surface_set_error (surface, status);
	    return;
	}
    }

    status = surface->backend->finish (surface);
    if (status)
	_cairo_surface_set_error (surface, status);

    surface->finished = TRUE;
}
slim_hidden_def (cairo_surface_finish);













void *
cairo_surface_get_user_data (cairo_surface_t		 *surface,
			     const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&surface->user_data,
					    key);
}

















cairo_status_t
cairo_surface_set_user_data (cairo_surface_t		 *surface,
			     const cairo_user_data_key_t *key,
			     void			 *user_data,
			     cairo_destroy_func_t	 destroy)
{
    if (surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return CAIRO_STATUS_NO_MEMORY;

    return _cairo_user_data_array_set_data (&surface->user_data,
					    key, user_data, destroy);
}
















void
_cairo_surface_set_font_options (cairo_surface_t       *surface,
				 cairo_font_options_t  *options)
{
    if (options) {
	surface->has_font_options = TRUE;
	_cairo_font_options_init_copy (&surface->font_options, options);
    } else {
	surface->has_font_options = FALSE;
    }
}













void
cairo_surface_get_font_options (cairo_surface_t       *surface,
				cairo_font_options_t  *options)
{
    if (cairo_font_options_status (options))
	return;

    if (!surface->has_font_options) {
	surface->has_font_options = TRUE;

	_cairo_font_options_init_default (&surface->font_options);

	if (!surface->finished && surface->backend->get_font_options) {
	    surface->backend->get_font_options (surface, &surface->font_options);
	}
    }

    _cairo_font_options_init_copy (options, &surface->font_options);
}
slim_hidden_def (cairo_surface_get_font_options);












void
cairo_surface_flush (cairo_surface_t *surface)
{
    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    if (surface->backend->flush) {
	cairo_status_t status;

	status = surface->backend->flush (surface);

	if (status)
	    _cairo_surface_set_error (surface, status);
    }
}









void
cairo_surface_mark_dirty (cairo_surface_t *surface)
{
    assert (! surface->is_snapshot);

    cairo_surface_mark_dirty_rectangle (surface, 0, 0, -1, -1);
}

















void
cairo_surface_mark_dirty_rectangle (cairo_surface_t *surface,
				    int              x,
				    int              y,
				    int              width,
				    int              height)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    




    surface->current_clip_serial = -1;

    if (surface->backend->mark_dirty_rectangle) {
	cairo_status_t status;

	




	status = surface->backend->mark_dirty_rectangle (surface,
                                                         x + surface->device_transform.x0,
                                                         y + surface->device_transform.y0,
							 width, height);

	if (status)
	    _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_mark_dirty_rectangle);





















void
_cairo_surface_set_device_scale (cairo_surface_t *surface,
				 double		  sx,
				 double		  sy)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    surface->device_transform.xx = sx;
    surface->device_transform.yy = sy;

    surface->device_transform_inverse.xx = 1.0 / sx;
    surface->device_transform_inverse.yy = 1.0 / sy;
}



















void
cairo_surface_set_device_offset (cairo_surface_t *surface,
				 double           x_offset,
				 double           y_offset)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    surface->device_transform.x0 = x_offset;
    surface->device_transform.y0 = y_offset;

    surface->device_transform_inverse.x0 = - x_offset;
    surface->device_transform_inverse.y0 = - y_offset;
}
slim_hidden_def (cairo_surface_set_device_offset);












void
cairo_surface_get_device_offset (cairo_surface_t *surface,
				 double          *x_offset,
				 double          *y_offset)
{
    if (x_offset)
	*x_offset = surface->device_transform.x0;
    if (y_offset)
	*y_offset = surface->device_transform.y0;
}
slim_hidden_def (cairo_surface_get_device_offset);































void
cairo_surface_set_fallback_resolution (cairo_surface_t	*surface,
				       double		 x_pixels_per_inch,
				       double		 y_pixels_per_inch)
{
    surface->x_fallback_resolution = x_pixels_per_inch;
    surface->y_fallback_resolution = y_pixels_per_inch;
}
slim_hidden_def (cairo_surface_set_fallback_resolution);

cairo_bool_t
_cairo_surface_has_device_transform (cairo_surface_t *surface)
{
    return ! _cairo_matrix_is_identity (&surface->device_transform);
}


















cairo_status_t
_cairo_surface_acquire_source_image (cairo_surface_t         *surface,
				     cairo_image_surface_t  **image_out,
				     void                   **image_extra)
{
    assert (!surface->finished);

    return surface->backend->acquire_source_image (surface,
						   image_out, image_extra);
}








void
_cairo_surface_release_source_image (cairo_surface_t        *surface,
				     cairo_image_surface_t  *image,
				     void                   *image_extra)
{
    assert (!surface->finished);

    if (surface->backend->release_source_image)
	surface->backend->release_source_image (surface, image, image_extra);
}
































cairo_status_t
_cairo_surface_acquire_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int_t   *interest_rect,
				   cairo_image_surface_t  **image_out,
				   cairo_rectangle_int_t   *image_rect,
				   void                   **image_extra)
{
    assert (!surface->finished);

    return surface->backend->acquire_dest_image (surface,
						 interest_rect,
						 image_out, image_rect, image_extra);
}













void
_cairo_surface_release_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int_t   *interest_rect,
				   cairo_image_surface_t   *image,
				   cairo_rectangle_int_t   *image_rect,
				   void                    *image_extra)
{
    assert (!surface->finished);

    if (surface->backend->release_dest_image)
	surface->backend->release_dest_image (surface, interest_rect,
					      image, image_rect, image_extra);
}





















cairo_status_t
_cairo_surface_clone_similar (cairo_surface_t  *surface,
			      cairo_surface_t  *src,
			      int               src_x,
			      int               src_y,
			      int               width,
			      int               height,
			      cairo_surface_t **clone_out)
{
    cairo_status_t status;
    cairo_image_surface_t *image;
    void *image_extra;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (surface->backend->clone_similar == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = surface->backend->clone_similar (surface, src, src_x, src_y,
					      width, height, clone_out);
    if (status == CAIRO_STATUS_SUCCESS && *clone_out != src)
        (*clone_out)->device_transform = src->device_transform;

    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_surface_acquire_source_image (src, &image, &image_extra);
    if (status != CAIRO_STATUS_SUCCESS)
	return status;

    status = surface->backend->clone_similar (surface, &image->base, src_x,
					      src_y, width, height, clone_out);
    if (status == CAIRO_STATUS_SUCCESS && *clone_out != src) {
        (*clone_out)->device_transform = src->device_transform;
        (*clone_out)->device_transform_inverse = src->device_transform_inverse;
    }

    





    _cairo_surface_release_source_image (src, image, image_extra);
    return status;
}


#include "cairo-meta-surface-private.h"















cairo_surface_t *
_cairo_surface_snapshot (cairo_surface_t *surface)
{
    if (surface->finished)
	return (cairo_surface_t *) &_cairo_surface_nil;

    if (surface->backend->snapshot)
	return surface->backend->snapshot (surface);

    return _cairo_surface_fallback_snapshot (surface);
}
















cairo_bool_t
_cairo_surface_is_similar (cairo_surface_t *surface_a,
	                   cairo_surface_t *surface_b,
			   cairo_content_t content)
{
    if (surface_a->backend != surface_b->backend)
	return FALSE;

    if (surface_a->backend->is_similar != NULL)
	return surface_a->backend->is_similar (surface_a, surface_b, content);

    return TRUE;
}

cairo_status_t
_cairo_surface_composite (cairo_operator_t	op,
			  cairo_pattern_t	*src,
			  cairo_pattern_t	*mask,
			  cairo_surface_t	*dst,
			  int			src_x,
			  int			src_y,
			  int			mask_x,
			  int			mask_y,
			  int			dst_x,
			  int			dst_y,
			  unsigned int		width,
			  unsigned int		height)
{
    cairo_int_status_t status;

    assert (! dst->is_snapshot);

    if (mask) {
	


	assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);
    }

    if (dst->status)
	return dst->status;

    if (dst->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (dst->backend->composite) {
	status = dst->backend->composite (op,
					  src, mask, dst,
                                          src_x, src_y,
                                          mask_x, mask_y,
                                          dst_x, dst_y,
					  width, height);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return _cairo_surface_fallback_composite (op,
					      src, mask, dst,
					      src_x, src_y,
					      mask_x, mask_y,
					      dst_x, dst_y,
					      width, height);
}
















cairo_status_t
_cairo_surface_fill_rectangle (cairo_surface_t	   *surface,
			       cairo_operator_t	    op,
			       const cairo_color_t *color,
			       int		    x,
			       int		    y,
			       int		    width,
			       int		    height)
{
    cairo_rectangle_int_t rect;

    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    return _cairo_surface_fill_rectangles (surface, op, color, &rect, 1);
}














cairo_status_t
_cairo_surface_fill_region (cairo_surface_t	   *surface,
			    cairo_operator_t	    op,
			    const cairo_color_t    *color,
			    cairo_region_t         *region)
{
    int num_boxes;
    cairo_box_int_t *boxes;
    cairo_rectangle_int_t stack_rects[CAIRO_STACK_BUFFER_SIZE / sizeof (cairo_rectangle_int_t)];
    cairo_rectangle_int_t *rects;
    cairo_status_t status;
    int i;

    assert (! surface->is_snapshot);

    status = _cairo_region_get_boxes (region, &num_boxes, &boxes);
    if (status)
	return status;

    if (num_boxes == 0)
	return CAIRO_STATUS_SUCCESS;

    rects = stack_rects;
    if (num_boxes > ARRAY_LENGTH (stack_rects)) {
	rects = _cairo_malloc_ab (num_boxes, sizeof (cairo_rectangle_int_t));
	if (!rects) {
	    _cairo_region_boxes_fini (region, boxes);
	    return CAIRO_STATUS_NO_MEMORY;
        }
    }

    for (i = 0; i < num_boxes; i++) {
	rects[i].x = boxes[i].p1.x;
	rects[i].y = boxes[i].p1.y;
	rects[i].width = boxes[i].p2.x - boxes[i].p1.x;
	rects[i].height = boxes[i].p2.y - boxes[i].p1.y;
    }

    status =  _cairo_surface_fill_rectangles (surface, op,
					      color, rects, num_boxes);

    _cairo_region_boxes_fini (region, boxes);

    if (rects != stack_rects)
	free (rects);

    return status;
}

















cairo_status_t
_cairo_surface_fill_rectangles (cairo_surface_t		*surface,
				cairo_operator_t         op,
				const cairo_color_t	*color,
				cairo_rectangle_int_t	*rects,
				int			 num_rects)
{
    cairo_int_status_t status;

    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (num_rects == 0)
	return CAIRO_STATUS_SUCCESS;

    if (surface->backend->fill_rectangles) {
	status = surface->backend->fill_rectangles (surface, op, color,
						    rects, num_rects);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return _cairo_surface_fallback_fill_rectangles (surface, op, color,
						    rects, num_rects);
}

cairo_status_t
_cairo_surface_paint (cairo_surface_t	*surface,
		      cairo_operator_t	 op,
		      cairo_pattern_t	*source)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    assert (! surface->is_snapshot);

    status = _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);
    if (status)
	return status;

    if (surface->backend->paint) {
	status = surface->backend->paint (surface, op, &dev_source.base);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_paint (surface, op, &dev_source.base);

 FINISH:
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_mask (cairo_surface_t	*surface,
		     cairo_operator_t	 op,
		     cairo_pattern_t	*source,
		     cairo_pattern_t	*mask)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_pattern_union_t dev_mask;

    assert (! surface->is_snapshot);

    status = _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);
    if (status)
	goto FINISH;
    status = _cairo_surface_copy_pattern_for_destination (mask, surface, &dev_mask.base);
    if (status)
	goto CLEANUP_SOURCE;

    if (surface->backend->mask) {
	status = surface->backend->mask (surface, op, &dev_source.base, &dev_mask.base);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto CLEANUP_MASK;
    }

    status = _cairo_surface_fallback_mask (surface, op, &dev_source.base, &dev_mask.base);

 CLEANUP_MASK:
    _cairo_pattern_fini (&dev_mask.base);
 CLEANUP_SOURCE:
    _cairo_pattern_fini (&dev_source.base);
 FINISH:

    return status;
}

cairo_status_t
_cairo_surface_stroke (cairo_surface_t		*surface,
		       cairo_operator_t		 op,
		       cairo_pattern_t		*source,
		       cairo_path_fixed_t	*path,
		       cairo_stroke_style_t	*stroke_style,
		       cairo_matrix_t		*ctm,
		       cairo_matrix_t		*ctm_inverse,
		       double			 tolerance,
		       cairo_antialias_t	 antialias)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_path_fixed_t *dev_path = path;
    cairo_path_fixed_t real_dev_path;
    cairo_matrix_t dev_ctm = *ctm;
    cairo_matrix_t dev_ctm_inverse = *ctm_inverse;

    assert (! surface->is_snapshot);

    status = _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);
    if (status)
	return status;

    if (surface->backend->stroke) {
	status = surface->backend->stroke (surface, op, &dev_source.base,
					   path, stroke_style,
					   &dev_ctm, &dev_ctm_inverse,
					   tolerance, antialias);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_stroke (surface, op, &dev_source.base,
                                             path, stroke_style,
                                             &dev_ctm, &dev_ctm_inverse,
                                             tolerance, antialias);

 FINISH:
    if (dev_path == &real_dev_path)
        _cairo_path_fixed_fini (&real_dev_path);
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_fill (cairo_surface_t	*surface,
		     cairo_operator_t	 op,
		     cairo_pattern_t	*source,
		     cairo_path_fixed_t	*path,
		     cairo_fill_rule_t	 fill_rule,
		     double		 tolerance,
		     cairo_antialias_t	 antialias)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    assert (! surface->is_snapshot);

    status = _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);
    if (status)
	return status;

    if (surface->backend->fill) {
	status = surface->backend->fill (surface, op, &dev_source.base,
					 path, fill_rule,
					 tolerance, antialias);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_fill (surface, op, &dev_source.base,
                                           path, fill_rule,
                                           tolerance, antialias);

 FINISH:
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_composite_trapezoids (cairo_operator_t		op,
				     cairo_pattern_t		*pattern,
				     cairo_surface_t		*dst,
				     cairo_antialias_t		antialias,
				     int			src_x,
				     int			src_y,
				     int			dst_x,
				     int			dst_y,
				     unsigned int		width,
				     unsigned int		height,
				     cairo_trapezoid_t		*traps,
				     int			num_traps)
{
    cairo_int_status_t status;

    assert (! dst->is_snapshot);

    


    assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);

    if (dst->status)
	return dst->status;

    if (dst->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (dst->backend->composite_trapezoids) {
	status = dst->backend->composite_trapezoids (op,
						     pattern, dst,
						     antialias,
						     src_x, src_y,
                                                     dst_x, dst_y,
						     width, height,
						     traps, num_traps);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return  _cairo_surface_fallback_composite_trapezoids (op, pattern, dst,
							  antialias,
							  src_x, src_y,
							  dst_x, dst_y,
							  width, height,
							  traps, num_traps);
}

cairo_status_t
_cairo_surface_copy_page (cairo_surface_t *surface)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    
    if (surface->backend->copy_page == NULL)
	return CAIRO_STATUS_SUCCESS;

    return surface->backend->copy_page (surface);
}

cairo_status_t
_cairo_surface_show_page (cairo_surface_t *surface)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    
    if (surface->backend->show_page == NULL)
	return CAIRO_STATUS_SUCCESS;

    return surface->backend->show_page (surface);
}










unsigned int
_cairo_surface_get_current_clip_serial (cairo_surface_t *surface)
{
    return surface->current_clip_serial;
}











unsigned int
_cairo_surface_allocate_clip_serial (cairo_surface_t *surface)
{
    unsigned int    serial;

    if (surface->status)
	return 0;

    if ((serial = ++(surface->next_clip_serial)) == 0)
	serial = ++(surface->next_clip_serial);
    return serial;
}










cairo_status_t
_cairo_surface_reset_clip (cairo_surface_t *surface)
{
    cairo_status_t  status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    surface->current_clip_serial = 0;

    if (surface->backend->intersect_clip_path) {
	status = surface->backend->intersect_clip_path (surface,
							NULL,
							CAIRO_FILL_RULE_WINDING,
							0,
							CAIRO_ANTIALIAS_DEFAULT);
	if (status)
	    return status;
    }

    if (surface->backend->set_clip_region != NULL) {
	status = surface->backend->set_clip_region (surface, NULL);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}











cairo_status_t
_cairo_surface_set_clip_region (cairo_surface_t	    *surface,
				cairo_region_t	    *region,
				unsigned int	    serial)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    assert (surface->backend->set_clip_region != NULL);

    surface->current_clip_serial = serial;

    status = surface->backend->set_clip_region (surface, region);

    return status;
}

cairo_int_status_t
_cairo_surface_intersect_clip_path (cairo_surface_t    *surface,
				    cairo_path_fixed_t *path,
				    cairo_fill_rule_t   fill_rule,
				    double		tolerance,
				    cairo_antialias_t	antialias)
{
    cairo_path_fixed_t *dev_path = path;
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    assert (surface->backend->intersect_clip_path != NULL);

    status = surface->backend->intersect_clip_path (surface,
						    dev_path,
						    fill_rule,
						    tolerance,
						    antialias);

    return status;
}

static cairo_status_t
_cairo_surface_set_clip_path_recursive (cairo_surface_t *surface,
					cairo_clip_path_t *clip_path)
{
    cairo_status_t status;

    if (clip_path == NULL)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_surface_set_clip_path_recursive (surface, clip_path->prev);
    if (status)
	return status;

    return _cairo_surface_intersect_clip_path (surface,
					       &clip_path->path,
					       clip_path->fill_rule,
					       clip_path->tolerance,
					       clip_path->antialias);
}










static cairo_status_t
_cairo_surface_set_clip_path (cairo_surface_t	*surface,
			      cairo_clip_path_t	*clip_path,
			      unsigned int	serial)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    assert (surface->backend->intersect_clip_path != NULL);

    status = surface->backend->intersect_clip_path (surface,
						    NULL,
						    CAIRO_FILL_RULE_WINDING,
						    0,
						    CAIRO_ANTIALIAS_DEFAULT);
    if (status)
	return status;

    status = _cairo_surface_set_clip_path_recursive (surface, clip_path);
    if (status)
	return status;

    surface->current_clip_serial = serial;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_set_clip (cairo_surface_t *surface, cairo_clip_t *clip)
{
    unsigned int serial = 0;

    if (!surface)
	return CAIRO_STATUS_NULL_POINTER;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (clip) {
	serial = clip->serial;
	if (serial == 0)
	    clip = NULL;
    }

    surface->clip = clip;

    if (serial == _cairo_surface_get_current_clip_serial (surface))
	return CAIRO_STATUS_SUCCESS;

    if (clip) {
	if (clip->path)
	    return _cairo_surface_set_clip_path (surface,
						 clip->path,
						 clip->serial);

	if (clip->has_region)
	    return _cairo_surface_set_clip_region (surface,
						   &clip->region,
						   clip->serial);
    }

    return _cairo_surface_reset_clip (surface);
}

























cairo_status_t
_cairo_surface_get_extents (cairo_surface_t         *surface,
			    cairo_rectangle_int_t   *rectangle)
{
    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    return surface->backend->get_extents (surface, rectangle);
}

cairo_status_t
_cairo_surface_show_glyphs (cairo_surface_t	*surface,
			    cairo_operator_t	 op,
			    cairo_pattern_t	*source,
			    cairo_glyph_t	*glyphs,
			    int			 num_glyphs,
			    cairo_scaled_font_t	*scaled_font)
{
    cairo_status_t status;
    cairo_scaled_font_t *dev_scaled_font = scaled_font;
    cairo_pattern_union_t dev_source;
    cairo_matrix_t font_matrix;

    assert (! surface->is_snapshot);

    if (!num_glyphs)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_surface_copy_pattern_for_destination (source,
						          surface,
							  &dev_source.base);
    if (status)
	return status;

    cairo_scaled_font_get_font_matrix (scaled_font, &font_matrix);

    if (_cairo_surface_has_device_transform (surface) &&
	! _cairo_matrix_is_integer_translation (&surface->device_transform, NULL, NULL))
    {
	cairo_font_options_t *font_options;
	cairo_matrix_t dev_ctm;

	font_options = cairo_font_options_create ();

	cairo_scaled_font_get_ctm (scaled_font, &dev_ctm);
	cairo_matrix_multiply (&dev_ctm, &dev_ctm, &surface->device_transform);
	cairo_scaled_font_get_font_options (scaled_font, font_options);
	dev_scaled_font = cairo_scaled_font_create (cairo_scaled_font_get_font_face (scaled_font),
						    &font_matrix,
						    &dev_ctm,
						    font_options);
	cairo_font_options_destroy (font_options);
    }
    status = cairo_scaled_font_status (dev_scaled_font);
    if (status) {
	_cairo_pattern_fini (&dev_source.base);
	return status;
    }

    CAIRO_MUTEX_LOCK (dev_scaled_font->mutex);

    status = CAIRO_INT_STATUS_UNSUPPORTED;

    if (surface->backend->show_glyphs)
	status = surface->backend->show_glyphs (surface, op, &dev_source.base,
						glyphs, num_glyphs,
                                                dev_scaled_font);

    if (status == CAIRO_INT_STATUS_UNSUPPORTED)
	status = _cairo_surface_fallback_show_glyphs (surface, op, &dev_source.base,
						      glyphs, num_glyphs,
						      dev_scaled_font);

    CAIRO_MUTEX_UNLOCK (dev_scaled_font->mutex);

    if (dev_scaled_font != scaled_font)
	cairo_scaled_font_destroy (dev_scaled_font);

    _cairo_pattern_fini (&dev_source.base);

    return status;
}






cairo_status_t
_cairo_surface_old_show_glyphs (cairo_scaled_font_t	*scaled_font,
				cairo_operator_t	 op,
				cairo_pattern_t		*pattern,
				cairo_surface_t		*dst,
				int			 source_x,
				int			 source_y,
				int			 dest_x,
				int			 dest_y,
				unsigned int		 width,
				unsigned int		 height,
				cairo_glyph_t		*glyphs,
				int			 num_glyphs)
{
    cairo_status_t status;

    assert (! dst->is_snapshot);

    if (dst->status)
	return dst->status;

    if (dst->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (dst->backend->old_show_glyphs) {
	status = dst->backend->old_show_glyphs (scaled_font,
						op, pattern, dst,
						source_x, source_y,
                                                dest_x, dest_y,
						width, height,
						glyphs, num_glyphs);
    } else
	status = CAIRO_INT_STATUS_UNSUPPORTED;

    return status;
}

static cairo_status_t
_cairo_surface_composite_fixup_unbounded_internal (cairo_surface_t         *dst,
						   cairo_rectangle_int_t   *src_rectangle,
						   cairo_rectangle_int_t   *mask_rectangle,
						   int			    dst_x,
						   int			    dst_y,
						   unsigned int		    width,
						   unsigned int		    height)
{
    cairo_rectangle_int_t dst_rectangle;
    cairo_rectangle_int_t drawn_rectangle;
    cairo_bool_t has_drawn_region = FALSE;
    cairo_bool_t has_clear_region = FALSE;
    cairo_region_t drawn_region;
    cairo_region_t clear_region;
    cairo_status_t status;

    


    dst_rectangle.x = dst_x;
    dst_rectangle.y = dst_y;
    dst_rectangle.width = width;
    dst_rectangle.height = height;

    drawn_rectangle = dst_rectangle;

    if (src_rectangle)
        _cairo_rectangle_intersect (&drawn_rectangle, src_rectangle);

    if (mask_rectangle)
        _cairo_rectangle_intersect (&drawn_rectangle, mask_rectangle);

    

    _cairo_region_init_rect (&drawn_region, &drawn_rectangle);
    _cairo_region_init_rect (&clear_region, &dst_rectangle);

    has_drawn_region = TRUE;
    has_clear_region = TRUE;

    if (_cairo_region_subtract (&clear_region, &clear_region, &drawn_region)
	!= CAIRO_STATUS_SUCCESS)
    {
        status = CAIRO_STATUS_NO_MEMORY;
        goto CLEANUP_REGIONS;
    }

    status = _cairo_surface_fill_region (dst, CAIRO_OPERATOR_SOURCE,
                                         CAIRO_COLOR_TRANSPARENT,
                                         &clear_region);

CLEANUP_REGIONS:
    if (has_drawn_region)
        _cairo_region_fini (&drawn_region);
    if (has_clear_region)
        _cairo_region_fini (&clear_region);

    return status;
}

























cairo_status_t
_cairo_surface_composite_fixup_unbounded (cairo_surface_t            *dst,
					  cairo_surface_attributes_t *src_attr,
					  int                         src_width,
					  int                         src_height,
					  cairo_surface_attributes_t *mask_attr,
					  int                         mask_width,
					  int                         mask_height,
					  int			      src_x,
					  int			      src_y,
					  int			      mask_x,
					  int			      mask_y,
					  int			      dst_x,
					  int			      dst_y,
					  unsigned int		      width,
					  unsigned int		      height)
{
    cairo_rectangle_int_t src_tmp, mask_tmp;
    cairo_rectangle_int_t *src_rectangle = NULL;
    cairo_rectangle_int_t *mask_rectangle = NULL;

    assert (! dst->is_snapshot);

    


    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width = src_width;
	src_tmp.height = src_height;

	src_rectangle = &src_tmp;
    }

    if (mask_attr &&
	_cairo_matrix_is_integer_translation (&mask_attr->matrix, NULL, NULL) &&
	mask_attr->extend == CAIRO_EXTEND_NONE)
    {
	mask_tmp.x = (dst_x - (mask_x + mask_attr->x_offset));
	mask_tmp.y = (dst_y - (mask_y + mask_attr->y_offset));
	mask_tmp.width = mask_width;
	mask_tmp.height = mask_height;

	mask_rectangle = &mask_tmp;
    }

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src_rectangle, mask_rectangle,
							      dst_x, dst_y, width, height);
}
























cairo_status_t
_cairo_surface_composite_shape_fixup_unbounded (cairo_surface_t            *dst,
						cairo_surface_attributes_t *src_attr,
						int                         src_width,
						int                         src_height,
						int                         mask_width,
						int                         mask_height,
						int			    src_x,
						int			    src_y,
						int			    mask_x,
						int			    mask_y,
						int			    dst_x,
						int			    dst_y,
						unsigned int		    width,
						unsigned int		    height)
{
    cairo_rectangle_int_t src_tmp, mask_tmp;
    cairo_rectangle_int_t *src_rectangle = NULL;
    cairo_rectangle_int_t *mask_rectangle = NULL;

    assert (! dst->is_snapshot);

    


    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width = src_width;
	src_tmp.height = src_height;

	src_rectangle = &src_tmp;
    }

    mask_tmp.x = dst_x - mask_x;
    mask_tmp.y = dst_y - mask_y;
    mask_tmp.width = mask_width;
    mask_tmp.height = mask_height;

    mask_rectangle = &mask_tmp;

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src_rectangle, mask_rectangle,
							      dst_x, dst_y, width, height);
}










static cairo_status_t
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t *pattern,
                                             cairo_surface_t *destination,
                                             cairo_pattern_t *pattern_out)
{
    cairo_status_t status;

    status = _cairo_pattern_init_copy (pattern_out, pattern);
    if (status)
	return status;

    if (_cairo_surface_has_device_transform (destination)) {
	cairo_matrix_t device_to_surface = destination->device_transform;

	status = cairo_matrix_invert (&device_to_surface);
	


	assert (status == CAIRO_STATUS_SUCCESS);

	_cairo_pattern_transform (pattern_out, &device_to_surface);
    }

    return CAIRO_STATUS_SUCCESS;
}



