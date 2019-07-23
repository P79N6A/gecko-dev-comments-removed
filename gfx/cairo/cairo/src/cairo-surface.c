





































#include "cairoint.h"

#include "cairo-surface-fallback-private.h"
#include "cairo-clip-private.h"
#include "cairo-meta-surface-private.h"

#define DEFINE_NIL_SURFACE(status, name)			\
const cairo_surface_t name = {					\
    NULL,				/* backend */		\
    CAIRO_SURFACE_TYPE_IMAGE,		/* type */		\
    CAIRO_CONTENT_COLOR,		/* content */		\
    CAIRO_REFERENCE_COUNT_INVALID,	/* ref_count */		\
    status,				/* status */		\
    FALSE,				/* finished */		\
    0,					/* unique id */		\
    { 0, 0, 0, NULL, },			/* user_data */		\
    { 0, 0, 0, NULL, },			/* mime_data */         \
    { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },   /* device_transform */	\
    { 1.0, 0.0,	0.0, 1.0, 0.0, 0.0 },	/* device_transform_inverse */	\
    0.0,				/* x_resolution */	\
    0.0,				/* y_resolution */	\
    0.0,				/* x_fallback_resolution */	\
    0.0,				/* y_fallback_resolution */	\
    NULL,				/* clip */		\
    0,					/* next_clip_serial */	\
    0,					/* current_clip_serial */	\
    NULL,				/* snapshot_of */	\
    NULL,				/* snapshot_detach */	\
    { 0,	/* size */					\
      0,	/* num_elements */				\
      0,	/* element_size */				\
      NULL,	/* elements */					\
    },					/* snapshots */		\
    FALSE,				/* has_font_options */	\
    { CAIRO_ANTIALIAS_DEFAULT,		/* antialias */		\
      CAIRO_SUBPIXEL_ORDER_DEFAULT,	/* subpixel_order */	\
      CAIRO_HINT_STYLE_DEFAULT,		/* hint_style */	\
      CAIRO_HINT_METRICS_DEFAULT	/* hint_metrics */	\
    }					/* font_options */	\
}

static DEFINE_NIL_SURFACE(CAIRO_STATUS_NO_MEMORY, _cairo_surface_nil);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_SURFACE_TYPE_MISMATCH, _cairo_surface_nil_surface_type_mismatch);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_CONTENT, _cairo_surface_nil_invalid_content);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_FORMAT, _cairo_surface_nil_invalid_format);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_VISUAL, _cairo_surface_nil_invalid_visual);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_FILE_NOT_FOUND, _cairo_surface_nil_file_not_found);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_TEMP_FILE_ERROR, _cairo_surface_nil_temp_file_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_READ_ERROR, _cairo_surface_nil_read_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_WRITE_ERROR, _cairo_surface_nil_write_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_STRIDE, _cairo_surface_nil_invalid_stride);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_SIZE, _cairo_surface_nil_invalid_size);

static cairo_status_t
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t **pattern,
					     cairo_surface_t *destination,
					     cairo_pattern_t *pattern_copy);





















cairo_status_t
_cairo_surface_set_error (cairo_surface_t *surface,
			  cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS || status >= CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    

    _cairo_status_set_error (&surface->status, status);

    return _cairo_error (status);
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

static unsigned int
_cairo_surface_allocate_unique_id (void)
{
    static unsigned int unique_id;

#if CAIRO_NO_MUTEX
    if (++unique_id == 0)
	unique_id = 1;
    return unique_id;
#else
    unsigned int old, id;

    do {
	old = _cairo_atomic_uint_get (&unique_id);
	id = old + 1;
	if (id == 0)
	    id = 1;
    } while (! _cairo_atomic_uint_cmpxchg (&unique_id, old, id));

    return id;
#endif
}

static cairo_bool_t
_cairo_surface_has_snapshots (cairo_surface_t *surface)
{
    return surface->snapshots.num_elements != 0;
}

static void
_cairo_surface_detach_snapshots (cairo_surface_t *surface)
{
    cairo_surface_t **snapshots;
    unsigned int i;

    if (! _cairo_surface_has_snapshots (surface))
	return;

    

    snapshots = _cairo_array_index (&surface->snapshots, 0);
    for (i = 0; i < surface->snapshots.num_elements; i++) {
	snapshots[i]->snapshot_of = NULL;

	if (snapshots[i]->snapshot_detach != NULL)
	    snapshots[i]->snapshot_detach (snapshots[i]);

	cairo_surface_destroy (snapshots[i]);
    }
    surface->snapshots.num_elements = 0;

    assert (! _cairo_surface_has_snapshots (surface));
}

cairo_status_t
_cairo_surface_attach_snapshot (cairo_surface_t *surface,
				cairo_surface_t *snapshot,
				cairo_surface_func_t detach_func)
{
    cairo_status_t status;

    assert (surface != snapshot);

    if (snapshot->snapshot_of != NULL)
	_cairo_surface_detach_snapshot (snapshot);

    snapshot->snapshot_of = surface;
    snapshot->snapshot_detach = detach_func;

    status = _cairo_array_append (&surface->snapshots, &snapshot);
    if (unlikely (status))
	return status;

    cairo_surface_reference (snapshot);
    return CAIRO_STATUS_SUCCESS;
}

cairo_surface_t *
_cairo_surface_has_snapshot (cairo_surface_t *surface,
			     const cairo_surface_backend_t *backend,
			     cairo_content_t content)
{
    cairo_surface_t **snapshots;
    unsigned int i;

    snapshots = _cairo_array_index (&surface->snapshots, 0);
    for (i = 0; i < surface->snapshots.num_elements; i++) {
	if (snapshots[i]->backend == backend &&
	    snapshots[i]->content == content)
	{
	    return snapshots[i];
	}
    }

    return NULL;
}

void
_cairo_surface_detach_snapshot (cairo_surface_t *snapshot)
{
    cairo_surface_t *surface;
    cairo_surface_t **snapshots;
    unsigned int i;

    assert (snapshot->snapshot_of != NULL);
    surface = snapshot->snapshot_of;

    snapshots = _cairo_array_index (&surface->snapshots, 0);
    for (i = 0; i < surface->snapshots.num_elements; i++) {
	if (snapshots[i] == snapshot)
	    break;
    }
    assert (i < surface->snapshots.num_elements);

    surface->snapshots.num_elements--;
    memmove (&snapshots[i],
	     &snapshots[i+1],
	     sizeof (cairo_surface_t *)*(surface->snapshots.num_elements - i));

    snapshot->snapshot_of = NULL;

    if (snapshot->snapshot_detach != NULL)
	snapshot->snapshot_detach (snapshot);

    cairo_surface_destroy (snapshot);
}

static cairo_bool_t
_cairo_surface_is_writable (cairo_surface_t *surface)
{
    return ! surface->finished &&
	   surface->snapshot_of == NULL &&
	   ! _cairo_surface_has_snapshots (surface);
}

static void
_cairo_surface_begin_modification (cairo_surface_t *surface)
{
    assert (surface->status == CAIRO_STATUS_SUCCESS);
    assert (! surface->finished);
    assert (surface->snapshot_of == NULL);

    _cairo_surface_detach_snapshots (surface);
}

void
_cairo_surface_init (cairo_surface_t			*surface,
		     const cairo_surface_backend_t	*backend,
		     cairo_content_t			 content)
{
    CAIRO_MUTEX_INITIALIZE ();

    surface->backend = backend;
    surface->content = content;
    surface->type = backend->type;

    CAIRO_REFERENCE_COUNT_INIT (&surface->ref_count, 1);
    surface->status = CAIRO_STATUS_SUCCESS;
    surface->finished = FALSE;
    surface->unique_id = _cairo_surface_allocate_unique_id ();

    _cairo_user_data_array_init (&surface->user_data);
    _cairo_user_data_array_init (&surface->mime_data);

    cairo_matrix_init_identity (&surface->device_transform);
    cairo_matrix_init_identity (&surface->device_transform_inverse);

    surface->x_resolution = CAIRO_SURFACE_RESOLUTION_DEFAULT;
    surface->y_resolution = CAIRO_SURFACE_RESOLUTION_DEFAULT;

    surface->x_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;
    surface->y_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;

    surface->clip = NULL;
    surface->next_clip_serial = 0;
    surface->current_clip_serial = 0;

    _cairo_array_init (&surface->snapshots, sizeof (cairo_surface_t *));
    surface->snapshot_of = NULL;

    surface->has_font_options = FALSE;
}

cairo_surface_t *
_cairo_surface_create_similar_scratch (cairo_surface_t *other,
				       cairo_content_t	content,
				       int		width,
				       int		height)
{
    cairo_surface_t *surface = NULL;

    if (other->status)
	return _cairo_surface_create_in_error (other->status);

    if (other->backend->create_similar) {
	surface = other->backend->create_similar (other, content, width, height);
	if (surface != NULL && surface->status)
	    return surface;
    }

    if (surface == NULL) {
	surface =
	    cairo_image_surface_create (_cairo_format_from_content (content),
					width, height);
    }

    
    if (unlikely (surface->status))
	return surface;

    if (other->has_font_options || other->backend != surface->backend) {
	cairo_font_options_t options;

	cairo_surface_get_font_options (other, &options);
	_cairo_surface_set_font_options (surface, &options);
    }

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
	return _cairo_surface_create_in_error (other->status);

    if (! CAIRO_CONTENT_VALID (content))
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_CONTENT));

    return _cairo_surface_create_similar_solid (other, content,
						width, height,
						CAIRO_COLOR_TRANSPARENT);
}
slim_hidden_def (cairo_surface_create_similar);

cairo_surface_t *
_cairo_surface_create_similar_solid (cairo_surface_t	 *other,
				     cairo_content_t	  content,
				     int		  width,
				     int		  height,
				     const cairo_color_t *color)
{
    cairo_status_t status;
    cairo_surface_t *surface;
    cairo_solid_pattern_t solid_pattern;

    surface = _cairo_surface_create_similar_scratch (other, content,
						     width, height);
    if (surface->status)
	return surface;

    _cairo_pattern_init_solid (&solid_pattern, color, content);

    status = _cairo_surface_paint (surface,
				   color == CAIRO_COLOR_TRANSPARENT ?
				   CAIRO_OPERATOR_CLEAR : CAIRO_OPERATOR_SOURCE,
				   &solid_pattern.base, NULL);

    _cairo_pattern_fini (&solid_pattern.base);

    if (unlikely (status)) {
	cairo_surface_destroy (surface);
	return _cairo_surface_create_in_error (status);
    }

    return surface;
}

cairo_surface_t *
_cairo_surface_create_solid_pattern_surface (cairo_surface_t	   *other,
					     const cairo_solid_pattern_t *solid_pattern)
{
    if (other->backend->create_solid_pattern_surface != NULL) {
	cairo_surface_t *surface;

	surface = other->backend->create_solid_pattern_surface (other,
								solid_pattern);
	if (surface)
	    return surface;
    }

    return _cairo_surface_create_similar_solid (other,
						solid_pattern->content,
						1, 1,
						&solid_pattern->color);
}

cairo_int_status_t
_cairo_surface_repaint_solid_pattern_surface (cairo_surface_t	    *other,
					      cairo_surface_t       *solid_surface,
					      const cairo_solid_pattern_t *solid_pattern)
{
    






    if (other->backend->create_solid_pattern_surface != NULL &&
	! other->backend->can_repaint_solid_pattern_surface (solid_surface,
							     solid_pattern))
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    return _cairo_surface_paint (solid_surface,
				 CAIRO_OPERATOR_SOURCE,
				 &solid_pattern->base,
				 NULL);
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
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    _cairo_reference_count_inc (&surface->ref_count);

    return surface;
}
slim_hidden_def (cairo_surface_reference);









void
cairo_surface_destroy (cairo_surface_t *surface)
{
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    if (! _cairo_reference_count_dec_and_test (&surface->ref_count))
	return;

    assert (surface->snapshot_of == NULL);

    if (! surface->finished)
	cairo_surface_finish (surface);

    _cairo_user_data_array_fini (&surface->user_data);
    _cairo_user_data_array_fini (&surface->mime_data);
    _cairo_array_fini (&surface->snapshots);

    free (surface);
}
slim_hidden_def(cairo_surface_destroy);








cairo_status_t
_cairo_surface_reset (cairo_surface_t *surface)
{
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return CAIRO_STATUS_SUCCESS;

    assert (CAIRO_REFERENCE_COUNT_GET_VALUE (&surface->ref_count) == 1);

    _cairo_user_data_array_fini (&surface->user_data);
    _cairo_user_data_array_fini (&surface->mime_data);

    if (surface->backend->reset != NULL) {
	cairo_status_t status = surface->backend->reset (surface);
	if (unlikely (status))
	    return _cairo_surface_set_error (surface, status);
    }

    _cairo_surface_init (surface, surface->backend, surface->content);

    return CAIRO_STATUS_SUCCESS;
}












unsigned int
cairo_surface_get_reference_count (cairo_surface_t *surface)
{
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&surface->ref_count);
}




















void
cairo_surface_finish (cairo_surface_t *surface)
{
    cairo_status_t status;

    if (surface == NULL)
	return;

    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return;

    if (surface->finished)
	return;

    cairo_surface_flush (surface);

    
    if (surface->backend->finish) {
	status = surface->backend->finish (surface);
	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }

    surface->finished = TRUE;

    if (surface->snapshot_of != NULL)
	_cairo_surface_detach_snapshot (surface);
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
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface->status;

    return _cairo_user_data_array_set_data (&surface->user_data,
					    key, user_data, destroy);
}














void
cairo_surface_get_mime_data (cairo_surface_t		*surface,
                             const char			*mime_type,
                             const unsigned char       **data,
                             unsigned int		*length)
{
    cairo_user_data_slot_t *slots;
    int i, num_slots;

    *data = NULL;
    *length = 0;
    if (unlikely (surface->status))
	return;

    




    num_slots = surface->mime_data.num_elements;
    slots = _cairo_array_index (&surface->mime_data, 0);
    for (i = 0; i < num_slots; i++) {
	if (strcmp ((char *) slots[i].key, mime_type) == 0) {
	    cairo_mime_data_t *mime_data = slots[i].user_data;

	    *data = mime_data->data;
	    *length = mime_data->length;
	    return;
	}
    }
}
slim_hidden_def (cairo_surface_get_mime_data);

static void
_cairo_mime_data_destroy (void *ptr)
{
    cairo_mime_data_t *mime_data = ptr;

    if (! _cairo_reference_count_dec_and_test (&mime_data->ref_count))
	return;

    if (mime_data->destroy && mime_data->closure)
	mime_data->destroy (mime_data->closure);

    free (mime_data);
}





















cairo_status_t
cairo_surface_set_mime_data (cairo_surface_t		*surface,
                             const char			*mime_type,
                             const unsigned char	*data,
                             unsigned int		 length,
			     cairo_destroy_func_t	 destroy,
			     void			*closure)
{
    cairo_status_t status;
    cairo_mime_data_t *mime_data;

    if (unlikely (surface->status))
	return surface->status;

    status = _cairo_intern_string (&mime_type, -1);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    if (data != NULL) {
	mime_data = malloc (sizeof (cairo_mime_data_t));
	if (unlikely (mime_data == NULL))
	    return _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_NO_MEMORY));

	CAIRO_REFERENCE_COUNT_INIT (&mime_data->ref_count, 1);

	mime_data->data = (unsigned char *) data;
	mime_data->length = length;
	mime_data->destroy = destroy;
	mime_data->closure = closure;
    } else
	mime_data = NULL;

    status = _cairo_user_data_array_set_data (&surface->mime_data,
					      (cairo_user_data_key_t *) mime_type,
					      mime_data,
					      _cairo_mime_data_destroy);
    if (unlikely (status)) {
	if (mime_data != NULL)
	    free (mime_data);

	return _cairo_surface_set_error (surface, status);
    }

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_surface_set_mime_data);

static void
_cairo_mime_data_reference (const void *key, void *elt, void *closure)
{
    cairo_mime_data_t *mime_data = elt;

    _cairo_reference_count_inc (&mime_data->ref_count);
}

cairo_status_t
_cairo_surface_copy_mime_data (cairo_surface_t *dst,
			       cairo_surface_t *src)
{
    cairo_status_t status;

    if (dst->status)
	return dst->status;

    if (src->status)
	return _cairo_surface_set_error (dst, src->status);

    
    status = _cairo_user_data_array_copy (&dst->mime_data, &src->mime_data);
    if (unlikely (status))
	return _cairo_surface_set_error (dst, status);

    
    _cairo_user_data_array_foreach (&dst->mime_data,
				    _cairo_mime_data_reference,
				    NULL);

    return CAIRO_STATUS_SUCCESS;
}
















void
_cairo_surface_set_font_options (cairo_surface_t       *surface,
				 cairo_font_options_t  *options)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface,
		                           CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

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

    if (surface->status) {
	_cairo_font_options_init_default (options);
	return;
    }

    if (! surface->has_font_options) {
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
    cairo_status_t status;

    if (surface->status)
	return;

    if (surface->finished)
	return;

    
    _cairo_surface_detach_snapshots (surface);

    if (surface->backend->flush) {
	status = surface->backend->flush (surface);
	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_flush);









void
cairo_surface_mark_dirty (cairo_surface_t *surface)
{
    cairo_surface_mark_dirty_rectangle (surface, 0, 0, -1, -1);
}

















void
cairo_surface_mark_dirty_rectangle (cairo_surface_t *surface,
				    int              x,
				    int              y,
				    int              width,
				    int              height)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    


    assert (! _cairo_surface_has_snapshots (surface));

    




    surface->current_clip_serial = -1;

    if (surface->backend->mark_dirty_rectangle) {
	




	status = surface->backend->mark_dirty_rectangle (surface,
                                                         x + surface->device_transform.x0,
                                                         y + surface->device_transform.y0,
							 width, height);

	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_mark_dirty_rectangle);


















void
_cairo_surface_set_device_scale (cairo_surface_t *surface,
				 double		  sx,
				 double		  sy)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->device_transform.xx = sx;
    surface->device_transform.yy = sy;
    surface->device_transform.xy = 0.0;
    surface->device_transform.yx = 0.0;

    surface->device_transform_inverse = surface->device_transform;
    status = cairo_matrix_invert (&surface->device_transform_inverse);
    
    assert (status == CAIRO_STATUS_SUCCESS);
}



















void
cairo_surface_set_device_offset (cairo_surface_t *surface,
				 double           x_offset,
				 double           y_offset)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->device_transform.x0 = x_offset;
    surface->device_transform.y0 = y_offset;

    surface->device_transform_inverse = surface->device_transform;
    status = cairo_matrix_invert (&surface->device_transform_inverse);
    
    assert (status == CAIRO_STATUS_SUCCESS);
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
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->x_fallback_resolution = x_pixels_per_inch;
    surface->y_fallback_resolution = y_pixels_per_inch;
}
slim_hidden_def (cairo_surface_set_fallback_resolution);













void
cairo_surface_get_fallback_resolution (cairo_surface_t	*surface,
				       double		*x_pixels_per_inch,
				       double		*y_pixels_per_inch)
{
    if (x_pixels_per_inch)
	*x_pixels_per_inch = surface->x_fallback_resolution;
    if (y_pixels_per_inch)
	*y_pixels_per_inch = surface->y_fallback_resolution;
}

int
_cairo_surface_get_text_path_fill_threshold (const cairo_surface_t *surface)
{
    return surface->backend->fill == NULL ? 10240 : 256;
}

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
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    assert (!surface->finished);

    if (surface->backend->acquire_source_image == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = surface->backend->acquire_source_image (surface,
						     image_out, image_extra);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    _cairo_debug_check_image_surface_is_defined (&(*image_out)->base);

    return CAIRO_STATUS_SUCCESS;
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
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    if (surface->backend->acquire_dest_image == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = surface->backend->acquire_dest_image (surface,
						   interest_rect,
						   image_out,
						   image_rect,
						   image_extra);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    _cairo_debug_check_image_surface_is_defined (&(*image_out)->base);

    return CAIRO_STATUS_SUCCESS;
}













void
_cairo_surface_release_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int_t   *interest_rect,
				   cairo_image_surface_t   *image,
				   cairo_rectangle_int_t   *image_rect,
				   void                    *image_extra)
{
    assert (_cairo_surface_is_writable (surface));

    if (surface->backend->release_dest_image)
	surface->backend->release_dest_image (surface, interest_rect,
					      image, image_rect, image_extra);
}

static cairo_status_t
_cairo_meta_surface_clone_similar (cairo_surface_t  *surface,
			           cairo_surface_t  *src,
				   cairo_content_t   content,
				   int               src_x,
				   int               src_y,
				   int               width,
				   int               height,
				   int              *clone_offset_x,
				   int              *clone_offset_y,
				   cairo_surface_t **clone_out)
{
    cairo_meta_surface_t *meta = (cairo_meta_surface_t *) src;
    cairo_surface_t *similar;
    cairo_status_t status;

    similar = _cairo_surface_has_snapshot (src,
					   surface->backend,
					   src->content & content);
    if (similar != NULL) {
	*clone_out = cairo_surface_reference (similar);
	*clone_offset_x = 0;
	*clone_offset_y = 0;
	return CAIRO_STATUS_SUCCESS;
    }

    if (width*height*8 < meta->extents.width*meta->extents.height) {
	similar = cairo_surface_create_similar (surface,
						src->content & content,
						width, height);
	status = similar->status;
	if (unlikely (status))
	    return status;

	cairo_surface_set_device_offset (similar, -src_x, -src_y);

	status = cairo_meta_surface_replay (src, similar);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}

    } else {
	similar = cairo_surface_create_similar (surface,
						src->content & content,
						meta->extents.width,
						meta->extents.height);
	status = similar->status;
	if (unlikely (status))
	    return status;

	status = cairo_meta_surface_replay (src, similar);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}

	status = _cairo_surface_attach_snapshot (src, similar, NULL);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}

	src_x = src_y = 0;
    }

    *clone_out = similar;
    *clone_offset_x = src_x;
    *clone_offset_y = src_y;
    return CAIRO_STATUS_SUCCESS;
}

struct acquire_source_image_data
{
    cairo_surface_t *src;
    cairo_image_surface_t *image;
    void *image_extra;
};

static void
_wrap_release_source_image (void *data)
{
    struct acquire_source_image_data *acquire_data = data;
    _cairo_surface_release_source_image (acquire_data->src, acquire_data->image, acquire_data->image_extra);
    free(data);
}

static cairo_status_t
_wrap_image (cairo_surface_t *src,
	     cairo_image_surface_t *image,
	     void *image_extra,
	     cairo_image_surface_t **out)
{
    static cairo_user_data_key_t wrap_image_key;
    cairo_image_surface_t *surface;
    cairo_status_t status;

    struct acquire_source_image_data *data = malloc(sizeof(*data));
    data->src = src;
    data->image = image;
    data->image_extra = image_extra;

    surface = (cairo_image_surface_t*)cairo_image_surface_create_for_data (image->data,
	    image->format,
	    image->width,
	    image->height,
	    image->stride);
    status = surface->base.status;
    if (status)
	return status;

    status = _cairo_user_data_array_set_data (&surface->base.user_data,
	    &wrap_image_key,
	    data,
	    _wrap_release_source_image);
    if (status) {
	cairo_surface_destroy (&surface->base);
	return status;
    }

    pixman_image_set_component_alpha (surface->pixman_image,
            pixman_image_get_component_alpha (image->pixman_image));

    *out = surface;
    return CAIRO_STATUS_SUCCESS;
}























cairo_status_t
_cairo_surface_clone_similar (cairo_surface_t  *surface,
			      cairo_surface_t  *src,
			      cairo_content_t	content,
			      int               src_x,
			      int               src_y,
			      int               width,
			      int               height,
			      int              *clone_offset_x,
			      int              *clone_offset_y,
			      cairo_surface_t **clone_out)
{
    cairo_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;
    cairo_image_surface_t *image;
    void *image_extra;

    if (unlikely (surface->status))
	return surface->status;

    if (unlikely (surface->finished))
	return _cairo_error (CAIRO_STATUS_SURFACE_FINISHED);

    if (surface->backend->clone_similar) {
	status = surface->backend->clone_similar (surface, src,
						  content,
						  src_x, src_y,
						  width, height,
						  clone_offset_x,
						  clone_offset_y,
						  clone_out);

	if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	    if (_cairo_surface_is_image (src))
		return CAIRO_INT_STATUS_UNSUPPORTED;

	    
	    if (_cairo_surface_is_meta (src)) {
		return _cairo_meta_surface_clone_similar (surface, src,
							  content,
							  src_x, src_y,
							  width, height,
							  clone_offset_x,
							  clone_offset_y,
							  clone_out);
	    }

	    
	    status = _cairo_surface_acquire_source_image (src, &image, &image_extra);
	    if (status == CAIRO_STATUS_SUCCESS) {
		status = _wrap_image(src, image, image_extra, &image);
		if (status != CAIRO_STATUS_SUCCESS) {
		    _cairo_surface_release_source_image (src, image, image_extra);
		} else {
		    status =
		    surface->backend->clone_similar (surface, &image->base,
						     content,
						     src_x, src_y,
						     width, height,
						     clone_offset_x,
						     clone_offset_y,
						     clone_out);
		    cairo_surface_destroy(&image->base);
		}
	    }
	}
    }

    
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	status =
	    _cairo_surface_fallback_clone_similar (surface, src,
						   content,
						   src_x, src_y,
						   width, height,
						   clone_offset_x,
						   clone_offset_y,
						   clone_out);
    }

    
    if (unlikely (status))
	return status;

    

    if (*clone_out != src) {
	(*clone_out)->device_transform = src->device_transform;
	(*clone_out)->device_transform_inverse = src->device_transform_inverse;
    }

    return status;
}


#include "cairo-meta-surface-private.h"















cairo_surface_t *
_cairo_surface_snapshot (cairo_surface_t *surface)
{
    cairo_surface_t *snapshot;
    cairo_status_t status;

    if (surface->status)
	return _cairo_surface_create_in_error (surface->status);

    if (surface->finished)
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_SURFACE_FINISHED));

    if (surface->snapshot_of != NULL)
	return cairo_surface_reference (surface);

    snapshot = _cairo_surface_has_snapshot (surface,
					    surface->backend,
					    surface->content);
    if (snapshot != NULL)
	return cairo_surface_reference (snapshot);

    if (surface->backend->snapshot != NULL) {
	snapshot = surface->backend->snapshot (surface);
	if (unlikely (snapshot->status))
	    return snapshot;

	
	if (snapshot->backend != surface->backend) {
	    cairo_surface_t *previous;

	    previous = _cairo_surface_has_snapshot (surface,
		                                    snapshot->backend,
						    snapshot->content);
	    if (previous != NULL) {
		cairo_surface_destroy (snapshot);
		return cairo_surface_reference (previous);
	    }
	}
    }

    if (snapshot == NULL) {
	snapshot = _cairo_surface_has_snapshot (surface,
						&_cairo_image_surface_backend,
						surface->content);
	if (snapshot != NULL)
	    return cairo_surface_reference (snapshot);

	snapshot = _cairo_surface_fallback_snapshot (surface);
	if (unlikely (snapshot->status))
	    return snapshot;
    }

    status = _cairo_surface_copy_mime_data (snapshot, surface);
    if (unlikely (status)) {
	cairo_surface_destroy (snapshot);
	return _cairo_surface_create_in_error (status);
    }

    snapshot->device_transform = surface->device_transform;
    snapshot->device_transform_inverse = surface->device_transform_inverse;

    status = _cairo_surface_attach_snapshot (surface, snapshot, NULL);
    if (unlikely (status)) {
	cairo_surface_destroy (snapshot);
	return _cairo_surface_create_in_error (status);
    }

    return snapshot;
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
			  const cairo_pattern_t	*src,
			  const cairo_pattern_t	*mask,
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

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    if (mask) {
	


	assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);
    }

    if (dst->backend->composite) {
	status = dst->backend->composite (op,
					  src, mask, dst,
                                          src_x, src_y,
                                          mask_x, mask_y,
                                          dst_x, dst_y,
					  width, height);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (dst, status);
    }

    return _cairo_surface_set_error (dst,
	    _cairo_surface_fallback_composite (op,
					      src, mask, dst,
					      src_x, src_y,
					      mask_x, mask_y,
					      dst_x, dst_y,
					      width, height));
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

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

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
    int num_rects;
    cairo_rectangle_int_t stack_rects[CAIRO_STACK_ARRAY_LENGTH (cairo_rectangle_int_t)];
    cairo_rectangle_int_t *rects = stack_rects;
    cairo_status_t status;
    int i;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    num_rects = cairo_region_num_rectangles (region);
    if (num_rects == 0)
	return CAIRO_STATUS_SUCCESS;

    if (num_rects > ARRAY_LENGTH (stack_rects)) {
	rects = _cairo_malloc_ab (num_rects,
				  sizeof (cairo_rectangle_int_t));
	if (rects == NULL) {
	    return _cairo_surface_set_error (surface,
					     _cairo_error (CAIRO_STATUS_NO_MEMORY));
	}
    }

    for (i = 0; i < num_rects; i++)
	cairo_region_get_rectangle (region, i, &rects[i]);

    status =  _cairo_surface_fill_rectangles (surface, op,
					      color, rects, num_rects);

    if (rects != stack_rects)
	free (rects);

    return _cairo_surface_set_error (surface, status);
}

















cairo_status_t
_cairo_surface_fill_rectangles (cairo_surface_t		*surface,
				cairo_operator_t         op,
				const cairo_color_t	*color,
				cairo_rectangle_int_t	*rects,
				int			 num_rects)
{
    cairo_int_status_t status;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    if (num_rects == 0)
	return CAIRO_STATUS_SUCCESS;

    if (surface->backend->fill_rectangles) {
	status = surface->backend->fill_rectangles (surface, op, color,
						    rects, num_rects);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (surface, status);
    }

    return _cairo_surface_set_error (surface,
	    _cairo_surface_fallback_fill_rectangles (surface, op, color,
						     rects, num_rects));
}

cairo_status_t
_cairo_surface_paint (cairo_surface_t	*surface,
		      cairo_operator_t	 op,
		      const cairo_pattern_t *source,
		      cairo_rectangle_int_t *extents)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    if (surface->status)
	return surface->status;

    _cairo_surface_begin_modification (surface);

    status = _cairo_surface_copy_pattern_for_destination (&source,
							  surface,
							  &dev_source.base);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    if (surface->backend->paint) {
	status = surface->backend->paint (surface, op, source, extents);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_paint (surface, op, source);

 FINISH:
    if (source == &dev_source.base)
	_cairo_pattern_fini (&dev_source.base);

    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_mask (cairo_surface_t		*surface,
		     cairo_operator_t		 op,
		     const cairo_pattern_t	*source,
		     const cairo_pattern_t	*mask,
		     cairo_rectangle_int_t      *extents)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_pattern_union_t dev_mask;

    if (surface->status)
	return surface->status;

    _cairo_surface_begin_modification (surface);

    status = _cairo_surface_copy_pattern_for_destination (&source,
							  surface,
							  &dev_source.base);
    if (unlikely (status))
	goto FINISH;

    status = _cairo_surface_copy_pattern_for_destination (&mask,
							  surface,
							  &dev_mask.base);
    if (unlikely (status))
	goto CLEANUP_SOURCE;

    if (surface->backend->mask) {
	status = surface->backend->mask (surface, op, source, mask, extents);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto CLEANUP_MASK;
    }

    status = _cairo_surface_fallback_mask (surface, op, source, mask);

 CLEANUP_MASK:
    if (mask == &dev_mask.base)
	_cairo_pattern_fini (&dev_mask.base);
 CLEANUP_SOURCE:
    if (source == &dev_source.base)
	_cairo_pattern_fini (&dev_source.base);
 FINISH:

    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_fill_stroke (cairo_surface_t	    *surface,
			    cairo_operator_t	     fill_op,
			    const cairo_pattern_t   *fill_source,
			    cairo_fill_rule_t	     fill_rule,
			    double		     fill_tolerance,
			    cairo_antialias_t	     fill_antialias,
			    cairo_path_fixed_t	    *path,
			    cairo_operator_t	     stroke_op,
			    const cairo_pattern_t   *stroke_source,
			    cairo_stroke_style_t    *stroke_style,
			    cairo_matrix_t	    *stroke_ctm,
			    cairo_matrix_t	    *stroke_ctm_inverse,
			    double		     stroke_tolerance,
			    cairo_antialias_t	     stroke_antialias,
			    cairo_rectangle_int_t   *extents)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->fill_stroke) {
	cairo_pattern_union_t dev_stroke_source;
	cairo_pattern_union_t dev_fill_source;
	cairo_matrix_t dev_ctm = *stroke_ctm;
	cairo_matrix_t dev_ctm_inverse = *stroke_ctm_inverse;

	status = _cairo_surface_copy_pattern_for_destination (&stroke_source,
							      surface,
							      &dev_stroke_source.base);
	if (unlikely (status))
	    return _cairo_surface_set_error (surface, status);

	status = _cairo_surface_copy_pattern_for_destination (&fill_source,
							      surface,
							      &dev_fill_source.base);
	if (unlikely (status)) {
	    if (stroke_source == &dev_stroke_source.base)
		_cairo_pattern_fini (&dev_stroke_source.base);

	    return _cairo_surface_set_error (surface, status);
	}

	status = surface->backend->fill_stroke (surface,
						fill_op, fill_source, fill_rule,
						fill_tolerance, fill_antialias,
						path,
						stroke_op, stroke_source,
						stroke_style,
						&dev_ctm, &dev_ctm_inverse,
						stroke_tolerance, stroke_antialias,
						extents);

	if (stroke_source == &dev_stroke_source.base)
	    _cairo_pattern_fini (&dev_stroke_source.base);

	if (fill_source == &dev_fill_source.base)
	    _cairo_pattern_fini (&dev_fill_source.base);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (surface, status);
    }

    status = _cairo_surface_fill (surface, fill_op, fill_source, path,
				  fill_rule, fill_tolerance, fill_antialias, NULL);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    status = _cairo_surface_stroke (surface, stroke_op, stroke_source, path,
				    stroke_style, stroke_ctm, stroke_ctm_inverse,
				    stroke_tolerance, stroke_antialias, NULL);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_stroke (cairo_surface_t		*surface,
		       cairo_operator_t		 op,
		       const cairo_pattern_t	*source,
		       cairo_path_fixed_t	*path,
		       cairo_stroke_style_t	*stroke_style,
		       cairo_matrix_t		*ctm,
		       cairo_matrix_t		*ctm_inverse,
		       double			 tolerance,
		       cairo_antialias_t	 antialias,
		       cairo_rectangle_int_t    *extents)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_path_fixed_t *dev_path = path;
    cairo_path_fixed_t real_dev_path;
    cairo_matrix_t dev_ctm = *ctm;
    cairo_matrix_t dev_ctm_inverse = *ctm_inverse;

    if (surface->status)
	return surface->status;

    _cairo_surface_begin_modification (surface);

    status = _cairo_surface_copy_pattern_for_destination (&source,
							  surface,
							  &dev_source.base);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    if (surface->backend->stroke) {
	status = surface->backend->stroke (surface, op, source,
					   path, stroke_style,
					   &dev_ctm, &dev_ctm_inverse,
					   tolerance, antialias, extents);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_stroke (surface, op, source,
                                             path, stroke_style,
                                             &dev_ctm, &dev_ctm_inverse,
                                             tolerance, antialias);

 FINISH:
    if (dev_path == &real_dev_path)
        _cairo_path_fixed_fini (&real_dev_path);

    if (source == &dev_source.base)
	_cairo_pattern_fini (&dev_source.base);

    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_fill (cairo_surface_t	*surface,
		     cairo_operator_t	 op,
		     const cairo_pattern_t *source,
		     cairo_path_fixed_t	*path,
		     cairo_fill_rule_t	 fill_rule,
		     double		 tolerance,
		     cairo_antialias_t	 antialias,
		     cairo_rectangle_int_t *extents)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    if (surface->status)
	return surface->status;

    _cairo_surface_begin_modification (surface);

    status = _cairo_surface_copy_pattern_for_destination (&source,
							  surface,
							  &dev_source.base);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    if (surface->backend->fill) {
	status = surface->backend->fill (surface, op, source,
					 path, fill_rule,
					 tolerance, antialias, extents);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_fill (surface, op, source,
                                           path, fill_rule,
                                           tolerance, antialias);

 FINISH:
    if (source == &dev_source.base)
	_cairo_pattern_fini (&dev_source.base);

    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_composite_trapezoids (cairo_operator_t		op,
				     const cairo_pattern_t	*pattern,
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

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    


    assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);

    if (dst->backend->composite_trapezoids) {
	status = dst->backend->composite_trapezoids (op,
						     pattern, dst,
						     antialias,
						     src_x, src_y,
                                                     dst_x, dst_y,
						     width, height,
						     traps, num_traps);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (dst, status);
    }

    return  _cairo_surface_set_error (dst,
	    _cairo_surface_fallback_composite_trapezoids (op, pattern, dst,
							  antialias,
							  src_x, src_y,
							  dst_x, dst_y,
							  width, height,
							  traps, num_traps));
}

cairo_span_renderer_t *
_cairo_surface_create_span_renderer (cairo_operator_t		op,
				     const cairo_pattern_t     	*pattern,
				     cairo_surface_t		*dst,
				     cairo_antialias_t	        antialias,
				     const cairo_composite_rectangles_t *rects)
{
    assert (dst->snapshot_of == NULL);

    if (dst->status)
	return _cairo_span_renderer_create_in_error (dst->status);

    if (dst->finished)
	return _cairo_span_renderer_create_in_error (CAIRO_STATUS_SURFACE_FINISHED);

    if (dst->backend->create_span_renderer) {
	return dst->backend->create_span_renderer (op,
						   pattern, dst,
						   antialias,
						   rects);
    }
    ASSERT_NOT_REACHED;
    return _cairo_span_renderer_create_in_error (CAIRO_INT_STATUS_UNSUPPORTED);
}

cairo_bool_t
_cairo_surface_check_span_renderer   (cairo_operator_t		op,
				      const cairo_pattern_t     *pattern,
				      cairo_surface_t		*dst,
				      cairo_antialias_t	        antialias,
				      const cairo_composite_rectangles_t *rects)
{
    cairo_int_status_t status;

    assert (dst->snapshot_of == NULL);

    if (dst->status)
	return FALSE;

    if (dst->finished) {
	status = _cairo_surface_set_error (dst, CAIRO_STATUS_SURFACE_FINISHED);
	return FALSE;
    }

    if (dst->backend->check_span_renderer) {
	return dst->backend->check_span_renderer (op,
						  pattern, dst,
						  antialias,
						  rects);
    }
    return FALSE;
}















void
cairo_surface_copy_page (cairo_surface_t *surface)
{
    cairo_status_t status_ignored;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
		                                 CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    
    if (surface->backend->copy_page == NULL)
	return;

    status_ignored = _cairo_surface_set_error (surface,
			                 surface->backend->copy_page (surface));
}
slim_hidden_def (cairo_surface_copy_page);













void
cairo_surface_show_page (cairo_surface_t *surface)
{
    cairo_status_t status_ignored;

    if (surface->status)
	return;

    _cairo_surface_begin_modification (surface);

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
		                                 CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    
    if (surface->backend->show_page == NULL)
	return;

    status_ignored = _cairo_surface_set_error (surface,
			                 surface->backend->show_page (surface));
}
slim_hidden_def (cairo_surface_show_page);












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
	return _cairo_surface_set_error (surface,CAIRO_STATUS_SURFACE_FINISHED);

    surface->current_clip_serial = 0;

    if (surface->backend->intersect_clip_path) {
	status = surface->backend->intersect_clip_path (surface,
							NULL,
							CAIRO_FILL_RULE_WINDING,
							0,
							CAIRO_ANTIALIAS_DEFAULT);
	if (unlikely (status))
	    return _cairo_surface_set_error (surface, status);
    }

    if (surface->backend->set_clip_region != NULL) {
	status = surface->backend->set_clip_region (surface, NULL);
	if (unlikely (status))
	    return _cairo_surface_set_error (surface, status);
    }

    return CAIRO_STATUS_SUCCESS;
}











cairo_status_t
_cairo_surface_set_clip_region (cairo_surface_t	    *surface,
				cairo_region_t	    *region,
				unsigned int	     serial)
{
    cairo_status_t status;
    
    if (surface->status)
	return surface->status;

    if (surface->finished)
	return _cairo_surface_set_error (surface,CAIRO_STATUS_SURFACE_FINISHED);

    assert (surface->backend->set_clip_region != NULL);

    status = surface->backend->set_clip_region (surface, region);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    surface->current_clip_serial = serial;

    return CAIRO_STATUS_SUCCESS;
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
	return _cairo_surface_set_error (surface,CAIRO_STATUS_SURFACE_FINISHED);

    assert (surface->backend->intersect_clip_path != NULL);

    status = surface->backend->intersect_clip_path (surface,
						    dev_path,
						    fill_rule,
						    tolerance,
						    antialias);

    return _cairo_surface_set_error (surface, status);
}

static cairo_status_t
_cairo_surface_set_clip_path_recursive (cairo_surface_t *surface,
					cairo_clip_path_t *clip_path)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (clip_path == NULL)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_surface_set_clip_path_recursive (surface, clip_path->prev);
    if (unlikely (status))
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
	return _cairo_surface_set_error (surface,CAIRO_STATUS_SURFACE_FINISHED);

    assert (surface->backend->intersect_clip_path != NULL);

    status = surface->backend->intersect_clip_path (surface,
						    NULL,
						    CAIRO_FILL_RULE_WINDING,
						    0,
						    CAIRO_ANTIALIAS_DEFAULT);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    status = _cairo_surface_set_clip_path_recursive (surface, clip_path);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    surface->current_clip_serial = serial;

    return CAIRO_STATUS_SUCCESS;
}










static cairo_status_t
_cairo_surface_set_empty_clip_path (cairo_surface_t *surface,
	                            unsigned int serial)
{
    cairo_path_fixed_t path;
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    _cairo_path_fixed_init (&path);

    status = surface->backend->intersect_clip_path (surface,
						    &path,
						    CAIRO_FILL_RULE_WINDING,
						    0,
						    CAIRO_ANTIALIAS_DEFAULT);

    if (status == CAIRO_STATUS_SUCCESS)
	surface->current_clip_serial = serial;

    _cairo_path_fixed_fini (&path);

    return _cairo_surface_set_error (surface, status);
}









static cairo_status_t
_cairo_surface_set_empty_clip_region (cairo_surface_t *surface,
				      unsigned int     serial)
{
    cairo_region_t *region;
    cairo_status_t  status;

    if (surface->status)
	return surface->status;

    region = cairo_region_create ();
    status = region->status;

    if (status == CAIRO_STATUS_SUCCESS)
	status = _cairo_surface_set_clip_region (surface, region, serial);

    cairo_region_destroy (region);

    return _cairo_surface_set_error (surface, status);
}

cairo_clip_t *
_cairo_surface_get_clip (cairo_surface_t *surface)
{
    return surface->clip;
}

cairo_status_t
_cairo_surface_set_clip (cairo_surface_t *surface, cairo_clip_t *clip)
{
    unsigned int serial = 0;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return _cairo_surface_set_error (surface,CAIRO_STATUS_SURFACE_FINISHED);

    if (clip) {
	serial = clip->serial;
	if (serial == 0)
	    clip = NULL;
    }

    surface->clip = clip;

    if (serial == _cairo_surface_get_current_clip_serial (surface))
	return CAIRO_STATUS_SUCCESS;

    if (clip) {
	if (clip->all_clipped) {
	    if (surface->backend->intersect_clip_path != NULL)
		return _cairo_surface_set_empty_clip_path (surface,
						           clip->serial);

	    if (surface->backend->set_clip_region != NULL)
		return _cairo_surface_set_empty_clip_region (surface,
							     clip->serial);
	} else {
	    if (clip->path)
		return _cairo_surface_set_clip_path (surface,
						     clip->path,
						     clip->serial);

	    if (clip->region)
		return _cairo_surface_set_clip_region (surface,
						       clip->region,
						       clip->serial);
	}
    }

    return _cairo_surface_reset_clip (surface);
}

























cairo_int_status_t
_cairo_surface_get_extents (cairo_surface_t         *surface,
			    cairo_rectangle_int_t   *extents)
{
    cairo_int_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return _cairo_surface_set_error (surface,CAIRO_STATUS_SURFACE_FINISHED);

    if (surface->backend->get_extents) {
	status = _cairo_surface_set_error (surface,
					   surface->backend->get_extents (surface, extents));
    }

    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	extents->x      = CAIRO_RECT_INT_MIN;
	extents->y      = CAIRO_RECT_INT_MIN;
	extents->width  = CAIRO_RECT_INT_MAX - CAIRO_RECT_INT_MIN;
	extents->height = CAIRO_RECT_INT_MAX - CAIRO_RECT_INT_MIN;
    }

    return status;
}






















cairo_bool_t
cairo_surface_has_show_text_glyphs (cairo_surface_t	    *surface)
{
    cairo_status_t status_ignored;

    if (surface->status)
	return FALSE;

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
						   CAIRO_STATUS_SURFACE_FINISHED);
	return FALSE;
    }

    if (surface->backend->has_show_text_glyphs)
	return surface->backend->has_show_text_glyphs (surface);
    else
	return surface->backend->show_text_glyphs != NULL;
}
slim_hidden_def (cairo_surface_has_show_text_glyphs);
















cairo_status_t
_cairo_surface_show_text_glyphs (cairo_surface_t	    *surface,
				 cairo_operator_t	     op,
				 const cairo_pattern_t	    *source,
				 const char		    *utf8,
				 int			     utf8_len,
				 cairo_glyph_t		    *glyphs,
				 int			     num_glyphs,
				 const cairo_text_cluster_t *clusters,
				 int			     num_clusters,
				 cairo_text_cluster_flags_t  cluster_flags,
				 cairo_scaled_font_t	    *scaled_font,
				 cairo_rectangle_int_t      *extents)
{
    cairo_status_t status;
    cairo_scaled_font_t *dev_scaled_font = scaled_font;
    cairo_pattern_union_t dev_source;

    if (surface->status)
	return surface->status;

    if (num_glyphs == 0 && utf8_len == 0)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    status = _cairo_surface_copy_pattern_for_destination (&source,
						          surface,
							  &dev_source.base);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    if (_cairo_surface_has_device_transform (surface) &&
	! _cairo_matrix_is_integer_translation (&surface->device_transform, NULL, NULL))
    {
	cairo_font_options_t font_options;
	cairo_matrix_t dev_ctm, font_matrix;

	cairo_scaled_font_get_font_matrix (scaled_font, &font_matrix);
	cairo_scaled_font_get_ctm (scaled_font, &dev_ctm);
	cairo_matrix_multiply (&dev_ctm, &dev_ctm, &surface->device_transform);
	cairo_scaled_font_get_font_options (scaled_font, &font_options);
	dev_scaled_font = cairo_scaled_font_create (cairo_scaled_font_get_font_face (scaled_font),
						    &font_matrix,
						    &dev_ctm,
						    &font_options);
    }
    status = cairo_scaled_font_status (dev_scaled_font);
    if (unlikely (status)) {
	if (source == &dev_source.base)
	    _cairo_pattern_fini (&dev_source.base);

	return _cairo_surface_set_error (surface, status);
    }

    status = CAIRO_INT_STATUS_UNSUPPORTED;

    

    if (clusters) {
	

	if (surface->backend->show_text_glyphs) {
	    status = surface->backend->show_text_glyphs (surface, op,
							 source,
							 utf8, utf8_len,
							 glyphs, num_glyphs,
							 clusters, num_clusters, cluster_flags,
							 dev_scaled_font, extents);
	}
	if (status == CAIRO_INT_STATUS_UNSUPPORTED && surface->backend->show_glyphs) {
	    int remaining_glyphs = num_glyphs;
	    status = surface->backend->show_glyphs (surface, op,
						    source,
						    glyphs, num_glyphs,
						    dev_scaled_font,
						    &remaining_glyphs, extents);
	    glyphs += num_glyphs - remaining_glyphs;
	    num_glyphs = remaining_glyphs;
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED && remaining_glyphs == 0)
		status = CAIRO_STATUS_SUCCESS;
	}
    } else {
	
	if (surface->backend->show_glyphs) {
	    int remaining_glyphs = num_glyphs;
	    status = surface->backend->show_glyphs (surface, op,
						    source,
						    glyphs, num_glyphs,
						    dev_scaled_font,
						    &remaining_glyphs, extents);
	    glyphs += num_glyphs - remaining_glyphs;
	    num_glyphs = remaining_glyphs;
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED && remaining_glyphs == 0)
		status = CAIRO_STATUS_SUCCESS;
	} else if (surface->backend->show_text_glyphs) {
	    







	    status = surface->backend->show_text_glyphs (surface, op,
							 source,
							 utf8, utf8_len,
							 glyphs, num_glyphs,
							 clusters, num_clusters, cluster_flags,
							 dev_scaled_font, extents);
	}
    }

    if (status == CAIRO_INT_STATUS_UNSUPPORTED)
	status = _cairo_surface_fallback_show_glyphs (surface, op,
						      source,
						      glyphs, num_glyphs,
						      dev_scaled_font);

    if (dev_scaled_font != scaled_font)
	cairo_scaled_font_destroy (dev_scaled_font);

    if (source == &dev_source.base)
	_cairo_pattern_fini (&dev_source.base);

    return _cairo_surface_set_error (surface, status);
}






cairo_status_t
_cairo_surface_old_show_glyphs (cairo_scaled_font_t	*scaled_font,
				cairo_operator_t	 op,
				const cairo_pattern_t	*pattern,
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

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    if (dst->backend->old_show_glyphs) {
	status = dst->backend->old_show_glyphs (scaled_font,
						op, pattern, dst,
						source_x, source_y,
                                                dest_x, dest_y,
						width, height,
						glyphs, num_glyphs);
    } else
	status = CAIRO_INT_STATUS_UNSUPPORTED;

    return _cairo_surface_set_error (dst, status);
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
    cairo_region_t clear_region;
    cairo_status_t status;

    


    dst_rectangle.x = dst_x;
    dst_rectangle.y = dst_y;
    dst_rectangle.width = width;
    dst_rectangle.height = height;

    _cairo_region_init_rectangle (&clear_region, &dst_rectangle);

    if (src_rectangle) {
        if (! _cairo_rectangle_intersect (&dst_rectangle, src_rectangle))
	    goto EMPTY;
    }

    if (mask_rectangle) {
        if (! _cairo_rectangle_intersect (&dst_rectangle, mask_rectangle))
	    goto EMPTY;
    }

    
    status = cairo_region_subtract_rectangle (&clear_region, &dst_rectangle);
    if (unlikely (status))
        goto CLEANUP_REGIONS;

  EMPTY:
    status = _cairo_surface_fill_region (dst, CAIRO_OPERATOR_SOURCE,
                                         CAIRO_COLOR_TRANSPARENT,
                                         &clear_region);

  CLEANUP_REGIONS:
    _cairo_region_fini (&clear_region);

    return _cairo_surface_set_error (dst, status);
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

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    


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

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    


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
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t **pattern,
                                             cairo_surface_t *destination,
                                             cairo_pattern_t *pattern_copy)
{
    cairo_status_t status;

    if (! _cairo_surface_has_device_transform (destination))
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_pattern_init_copy (pattern_copy, *pattern);
    if (unlikely (status))
	return status;

    _cairo_pattern_transform (pattern_copy,
			      &destination->device_transform_inverse);


    *pattern = pattern_copy;
    return CAIRO_STATUS_SUCCESS;
}











void
_cairo_surface_set_resolution (cairo_surface_t *surface,
			       double x_res,
			       double y_res)
{
    if (surface->status)
	return;

    surface->x_resolution = x_res;
    surface->y_resolution = y_res;
}

cairo_surface_t *
_cairo_surface_create_in_error (cairo_status_t status)
{
    switch (status) {
    case CAIRO_STATUS_NO_MEMORY:
	return (cairo_surface_t *) &_cairo_surface_nil;
    case CAIRO_STATUS_SURFACE_TYPE_MISMATCH:
	return (cairo_surface_t *) &_cairo_surface_nil_surface_type_mismatch;
    case CAIRO_STATUS_INVALID_CONTENT:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_content;
    case CAIRO_STATUS_INVALID_FORMAT:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_format;
    case CAIRO_STATUS_INVALID_VISUAL:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_visual;
    case CAIRO_STATUS_READ_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_read_error;
    case CAIRO_STATUS_WRITE_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_write_error;
    case CAIRO_STATUS_FILE_NOT_FOUND:
	return (cairo_surface_t *) &_cairo_surface_nil_file_not_found;
    case CAIRO_STATUS_TEMP_FILE_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_temp_file_error;
    case CAIRO_STATUS_INVALID_STRIDE:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_stride;
    case CAIRO_STATUS_INVALID_SIZE:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_size;
    case CAIRO_STATUS_SUCCESS:
    case CAIRO_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	
    case CAIRO_STATUS_INVALID_RESTORE:
    case CAIRO_STATUS_INVALID_POP_GROUP:
    case CAIRO_STATUS_NO_CURRENT_POINT:
    case CAIRO_STATUS_INVALID_MATRIX:
    case CAIRO_STATUS_INVALID_STATUS:
    case CAIRO_STATUS_NULL_POINTER:
    case CAIRO_STATUS_INVALID_STRING:
    case CAIRO_STATUS_INVALID_PATH_DATA:
    case CAIRO_STATUS_SURFACE_FINISHED:
    case CAIRO_STATUS_PATTERN_TYPE_MISMATCH:
    case CAIRO_STATUS_INVALID_DASH:
    case CAIRO_STATUS_INVALID_DSC_COMMENT:
    case CAIRO_STATUS_INVALID_INDEX:
    case CAIRO_STATUS_CLIP_NOT_REPRESENTABLE:
    case CAIRO_STATUS_FONT_TYPE_MISMATCH:
    case CAIRO_STATUS_USER_FONT_IMMUTABLE:
    case CAIRO_STATUS_USER_FONT_ERROR:
    case CAIRO_STATUS_NEGATIVE_COUNT:
    case CAIRO_STATUS_INVALID_CLUSTERS:
    case CAIRO_STATUS_INVALID_SLANT:
    case CAIRO_STATUS_INVALID_WEIGHT:
    case CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED:
    default:
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t *) &_cairo_surface_nil;
    }
}



