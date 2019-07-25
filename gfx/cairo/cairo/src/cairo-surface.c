





































#include "cairoint.h"

#include "cairo-surface-fallback-private.h"
#include "cairo-clip-private.h"
#include "cairo-device-private.h"
#include "cairo-error-private.h"
#include "cairo-recording-surface-private.h"
#include "cairo-region-private.h"
#include "cairo-tee-surface-private.h"
















































#define DEFINE_NIL_SURFACE(status, name)			\
const cairo_surface_t name = {					\
    NULL,				/* backend */		\
    NULL,				/* device */		\
    CAIRO_SURFACE_TYPE_IMAGE,		/* type */		\
    CAIRO_CONTENT_COLOR,		/* content */		\
    CAIRO_REFERENCE_COUNT_INVALID,	/* ref_count */		\
    status,				/* status */		\
    0,					/* unique id */		\
    FALSE,				/* finished */		\
    TRUE,				/* is_clear */		\
    FALSE,				/* has_font_options */	\
    FALSE,				/* owns_device */	\
    FALSE,				/* permit_subpixel_antialiasing */ \
    { 0, 0, 0, NULL, },			/* user_data */		\
    { 0, 0, 0, NULL, },			/* mime_data */         \
    { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },   /* device_transform */	\
    { 1.0, 0.0,	0.0, 1.0, 0.0, 0.0 },	/* device_transform_inverse */	\
    { NULL, NULL },			/* device_transform_observers */ \
    0.0,				/* x_resolution */	\
    0.0,				/* y_resolution */	\
    0.0,				/* x_fallback_resolution */	\
    0.0,				/* y_fallback_resolution */	\
    NULL,				/* snapshot_of */	\
    NULL,				/* snapshot_detach */	\
    { NULL, NULL },			/* snapshots */		\
    { NULL, NULL },			/* snapshot */		\
    { CAIRO_ANTIALIAS_DEFAULT,		/* antialias */		\
      CAIRO_SUBPIXEL_ORDER_DEFAULT,	/* subpixel_order */	\
      CAIRO_LCD_FILTER_DEFAULT,		/* lcd_filter */	\
      CAIRO_HINT_STYLE_DEFAULT,		/* hint_style */	\
      CAIRO_HINT_METRICS_DEFAULT,	/* hint_metrics */	\
      CAIRO_ROUND_GLYPH_POS_DEFAULT	/* round_glyph_positions */	\
    }					/* font_options */	\
}



static DEFINE_NIL_SURFACE(CAIRO_STATUS_NO_MEMORY, _cairo_surface_nil);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_SURFACE_TYPE_MISMATCH, _cairo_surface_nil_surface_type_mismatch);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_STATUS, _cairo_surface_nil_invalid_status);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_CONTENT, _cairo_surface_nil_invalid_content);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_FORMAT, _cairo_surface_nil_invalid_format);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_VISUAL, _cairo_surface_nil_invalid_visual);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_FILE_NOT_FOUND, _cairo_surface_nil_file_not_found);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_TEMP_FILE_ERROR, _cairo_surface_nil_temp_file_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_READ_ERROR, _cairo_surface_nil_read_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_WRITE_ERROR, _cairo_surface_nil_write_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_STRIDE, _cairo_surface_nil_invalid_stride);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_SIZE, _cairo_surface_nil_invalid_size);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_DEVICE_TYPE_MISMATCH, _cairo_surface_nil_device_type_mismatch);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_DEVICE_ERROR, _cairo_surface_nil_device_error);





















cairo_status_t
_cairo_surface_set_error (cairo_surface_t *surface,
			  cairo_status_t status)
{
    if (status == CAIRO_INT_STATUS_NOTHING_TO_DO)
	status = CAIRO_STATUS_SUCCESS;

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
    static cairo_atomic_int_t unique_id;

#if CAIRO_NO_MUTEX
    if (++unique_id == 0)
	unique_id = 1;
    return unique_id;
#else
    cairo_atomic_int_t old, id;

    do {
	old = _cairo_atomic_uint_get (&unique_id);
	id = old + 1;
	if (id == 0)
	    id = 1;
    } while (! _cairo_atomic_uint_cmpxchg (&unique_id, old, id));

    return id;
#endif
}













cairo_device_t *
cairo_surface_get_device (cairo_surface_t *surface)
{
    if (unlikely (surface->status))
	return _cairo_device_create_in_error (surface->status);

    return surface->device;
}

static cairo_bool_t
_cairo_surface_has_snapshots (cairo_surface_t *surface)
{
    return ! cairo_list_is_empty (&surface->snapshots);
}

static cairo_bool_t
_cairo_surface_has_mime_data (cairo_surface_t *surface)
{
    return surface->mime_data.num_elements != 0;
}

static void
_cairo_surface_detach_mime_data (cairo_surface_t *surface)
{
    if (! _cairo_surface_has_mime_data (surface))
	return;

    _cairo_user_data_array_fini (&surface->mime_data);
    _cairo_user_data_array_init (&surface->mime_data);
}

static void
_cairo_surface_detach_snapshots (cairo_surface_t *surface)
{
    while (_cairo_surface_has_snapshots (surface)) {
	_cairo_surface_detach_snapshot (cairo_list_first_entry (&surface->snapshots,
								cairo_surface_t,
								snapshot));
    }
}

void
_cairo_surface_detach_snapshot (cairo_surface_t *snapshot)
{
    assert (snapshot->snapshot_of != NULL);

    snapshot->snapshot_of = NULL;
    cairo_list_del (&snapshot->snapshot);

    if (snapshot->snapshot_detach != NULL)
	snapshot->snapshot_detach (snapshot);

    cairo_surface_destroy (snapshot);
}

void
_cairo_surface_attach_snapshot (cairo_surface_t *surface,
				 cairo_surface_t *snapshot,
				 cairo_surface_func_t detach_func)
{
    assert (surface != snapshot);
    assert (snapshot->snapshot_of != surface);

    cairo_surface_reference (snapshot);

    if (snapshot->snapshot_of != NULL)
	_cairo_surface_detach_snapshot (snapshot);

    snapshot->snapshot_of = surface;
    snapshot->snapshot_detach = detach_func;

    cairo_list_add (&snapshot->snapshot, &surface->snapshots);

    assert (_cairo_surface_has_snapshot (surface, snapshot->backend) == snapshot);
}

cairo_surface_t *
_cairo_surface_has_snapshot (cairo_surface_t *surface,
			     const cairo_surface_backend_t *backend)
{
    cairo_surface_t *snapshot;

    cairo_list_foreach_entry (snapshot, cairo_surface_t,
			      &surface->snapshots, snapshot)
    {
	
	if (snapshot->backend == backend)
	    return snapshot;
    }

    return NULL;
}

static cairo_bool_t
_cairo_surface_is_writable (cairo_surface_t *surface)
{
    return ! surface->finished &&
	   surface->snapshot_of == NULL &&
	   ! _cairo_surface_has_snapshots (surface) &&
	   ! _cairo_surface_has_mime_data (surface);
}

static void
_cairo_surface_begin_modification (cairo_surface_t *surface)
{
    assert (surface->status == CAIRO_STATUS_SUCCESS);
    assert (! surface->finished);
    assert (surface->snapshot_of == NULL);

    _cairo_surface_detach_snapshots (surface);
    _cairo_surface_detach_mime_data (surface);
}

void
_cairo_surface_init (cairo_surface_t			*surface,
		     const cairo_surface_backend_t	*backend,
		     cairo_device_t			*device,
		     cairo_content_t			 content)
{
    CAIRO_MUTEX_INITIALIZE ();

    surface->backend = backend;
    surface->device = cairo_device_reference (device);
    surface->content = content;
    surface->type = backend->type;

    CAIRO_REFERENCE_COUNT_INIT (&surface->ref_count, 1);
    surface->status = CAIRO_STATUS_SUCCESS;
    surface->unique_id = _cairo_surface_allocate_unique_id ();
    surface->finished = FALSE;
    surface->is_clear = FALSE;
    surface->owns_device = (device != NULL);
    surface->has_font_options = FALSE;
    surface->permit_subpixel_antialiasing = TRUE;

    _cairo_user_data_array_init (&surface->user_data);
    _cairo_user_data_array_init (&surface->mime_data);

    cairo_matrix_init_identity (&surface->device_transform);
    cairo_matrix_init_identity (&surface->device_transform_inverse);
    cairo_list_init (&surface->device_transform_observers);

    surface->x_resolution = CAIRO_SURFACE_RESOLUTION_DEFAULT;
    surface->y_resolution = CAIRO_SURFACE_RESOLUTION_DEFAULT;

    surface->x_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;
    surface->y_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;

    cairo_list_init (&surface->snapshots);
    surface->snapshot_of = NULL;
}

static void
_cairo_surface_copy_similar_properties (cairo_surface_t *surface,
					cairo_surface_t *other)
{
    if (other->has_font_options || other->backend != surface->backend) {
	cairo_font_options_t options;

	cairo_surface_get_font_options (other, &options);
	_cairo_surface_set_font_options (surface, &options);
    }

    surface->permit_subpixel_antialiasing = other->permit_subpixel_antialiasing;

    cairo_surface_set_fallback_resolution (surface,
					   other->x_fallback_resolution,
					   other->y_fallback_resolution);
}

cairo_surface_t *
_cairo_surface_create_similar_scratch (cairo_surface_t *other,
				       cairo_content_t	content,
				       int		width,
				       int		height)
{
    cairo_surface_t *surface;

    if (unlikely (other->status))
	return _cairo_surface_create_in_error (other->status);

    if (other->backend->create_similar == NULL)
	return NULL;

    surface = other->backend->create_similar (other,
					      content, width, height);
    if (surface == NULL || surface->status)
	return surface;

    _cairo_surface_copy_similar_properties (surface, other);

    return surface;
}


























cairo_surface_t *
cairo_surface_create_similar (cairo_surface_t  *other,
			      cairo_content_t	content,
			      int		width,
			      int		height)
{
    if (unlikely (other->status))
	return _cairo_surface_create_in_error (other->status);
    if (unlikely (other->finished))
	return _cairo_surface_create_in_error (CAIRO_STATUS_SURFACE_FINISHED);

    if (unlikely (! CAIRO_CONTENT_VALID (content)))
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_CONTENT));

    return _cairo_surface_create_similar_solid (other,
						content, width, height,
						CAIRO_COLOR_TRANSPARENT,
						TRUE);
}

cairo_surface_t *
_cairo_surface_create_similar_solid (cairo_surface_t	 *other,
				     cairo_content_t	  content,
				     int		  width,
				     int		  height,
				     const cairo_color_t *color,
				     cairo_bool_t allow_fallback)
{
    cairo_status_t status;
    cairo_surface_t *surface;
    cairo_solid_pattern_t pattern;

    surface = _cairo_surface_create_similar_scratch (other, content,
						     width, height);
    if (surface == NULL && allow_fallback)
	surface = _cairo_image_surface_create_with_content (content,
							    width, height);
    if (surface == NULL || surface->status)
	return surface;

    _cairo_pattern_init_solid (&pattern, color);
    status = _cairo_surface_paint (surface,
				   color == CAIRO_COLOR_TRANSPARENT ?
				   CAIRO_OPERATOR_CLEAR : CAIRO_OPERATOR_SOURCE,
				   &pattern.base, NULL);
    if (unlikely (status)) {
	cairo_surface_destroy (surface);
	surface = _cairo_surface_create_in_error (status);
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
						_cairo_color_get_content (&solid_pattern->color),
						1, 1,
						&solid_pattern->color,
						FALSE);
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

    
    assert (! CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    _cairo_user_data_array_fini (&surface->user_data);
    _cairo_user_data_array_fini (&surface->mime_data);

    if (surface->owns_device)
        cairo_device_destroy (surface->device);

    free (surface);
}
slim_hidden_def(cairo_surface_destroy);












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

    
    _cairo_surface_detach_snapshots (surface);
    if (surface->snapshot_of != NULL)
	_cairo_surface_detach_snapshot (surface);

    cairo_surface_flush (surface);
    surface->finished = TRUE;

    
    if (surface->backend->finish) {
	status = surface->backend->finish (surface);
	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_finish);













void
_cairo_surface_release_device_reference (cairo_surface_t *surface)
{
    assert (surface->owns_device);

    cairo_device_destroy (surface->device);
    surface->owns_device = FALSE;
}













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
                             unsigned long		*length)
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
                             unsigned long		 length,
			     cairo_destroy_func_t	 destroy,
			     void			*closure)
{
    cairo_status_t status;
    cairo_mime_data_t *mime_data;

    if (unlikely (surface->status))
	return surface->status;
    if (surface->finished)
	return _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));

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
		                           _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
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
slim_hidden_def (cairo_surface_mark_dirty);

















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
	status = _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
	return;
    }

    


    assert (! _cairo_surface_has_snapshots (surface));
    assert (! _cairo_surface_has_mime_data (surface));

    surface->is_clear = FALSE;

    if (surface->backend->mark_dirty_rectangle != NULL) {
	




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
	status = _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
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

    _cairo_observers_notify (&surface->device_transform_observers, surface);
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
	status = _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->device_transform.x0 = x_offset;
    surface->device_transform.y0 = y_offset;

    surface->device_transform_inverse = surface->device_transform;
    status = cairo_matrix_invert (&surface->device_transform_inverse);
    
    assert (status == CAIRO_STATUS_SUCCESS);

    _cairo_observers_notify (&surface->device_transform_observers, surface);
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
	status = _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_SURFACE_FINISHED));
	return;
    }

    if (x_pixels_per_inch <= 0 || y_pixels_per_inch <= 0) {
	


	status = _cairo_surface_set_error (surface, CAIRO_STATUS_INVALID_MATRIX);
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
_cairo_recording_surface_clone_similar (cairo_surface_t  *surface,
					cairo_surface_t  *src,
					int               src_x,
					int               src_y,
					int               width,
					int               height,
					int              *clone_offset_x,
					int              *clone_offset_y,
					cairo_surface_t **clone_out)
{
    cairo_recording_surface_t *recorder = (cairo_recording_surface_t *) src;
    cairo_surface_t *similar;
    cairo_status_t status;

    similar = _cairo_surface_has_snapshot (src, surface->backend);
    if (similar != NULL) {
	*clone_out = cairo_surface_reference (similar);
	*clone_offset_x = 0;
	*clone_offset_y = 0;
	return CAIRO_STATUS_SUCCESS;
    }

    if (recorder->unbounded ||
	width*height*8 < recorder->extents.width*recorder->extents.height)
    {
	similar = _cairo_surface_create_similar_solid (surface,
						       src->content,
						       width, height,
                                                       CAIRO_COLOR_TRANSPARENT,
                                                       FALSE);
	if (similar == NULL)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	if (unlikely (similar->status))
	    return similar->status;

	cairo_surface_set_device_offset (similar, -src_x, -src_y);

	status = _cairo_recording_surface_replay (src, similar);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}
    } else {
	similar = _cairo_surface_create_similar_scratch (surface,
							 src->content,
							 recorder->extents.width,
							 recorder->extents.height);
	if (similar == NULL)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	if (unlikely (similar->status))
	    return similar->status;

	status = _cairo_recording_surface_replay (src, similar);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}

	_cairo_surface_attach_snapshot (src, similar, NULL);

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
    _cairo_surface_release_source_image (acquire_data->src,
					 acquire_data->image,
					 acquire_data->image_extra);
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

    struct acquire_source_image_data *data = malloc (sizeof (*data));
    if (unlikely (data == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    data->src = src;
    data->image = image;
    data->image_extra = image_extra;

    surface = (cairo_image_surface_t*)
	_cairo_image_surface_create_with_pixman_format (image->data,
							image->pixman_format,
							image->width,
							image->height,
							image->stride);
    status = surface->base.status;
    if (status) {
	free (data);
	return status;
    }

    status = _cairo_user_data_array_set_data (&surface->base.user_data,
					      &wrap_image_key,
					      data,
					      _wrap_release_source_image);
    if (status) {
	cairo_surface_destroy (&surface->base);
	free (data);
	return status;
    }

    pixman_image_set_component_alpha (
	surface->pixman_image,
	pixman_image_get_component_alpha (image->pixman_image));

    *out = surface;
    return CAIRO_STATUS_SUCCESS;
}





















cairo_status_t
_cairo_surface_clone_similar (cairo_surface_t  *surface,
			      cairo_surface_t  *src,
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

#if CAIRO_HAS_TEE_SURFACE

    if (src->type == CAIRO_SURFACE_TYPE_TEE) {
	cairo_surface_t *match;

	match = _cairo_tee_surface_find_match (src,
					       surface->backend,
					       src->content);
	if (match != NULL)
	    src = match;
    }

#endif

    if (surface->backend->clone_similar != NULL) {
	status = surface->backend->clone_similar (surface, src,
						  src_x, src_y,
						  width, height,
						  clone_offset_x,
						  clone_offset_y,
						  clone_out);
	if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	    if (_cairo_surface_is_image (src))
		return CAIRO_INT_STATUS_UNSUPPORTED;

	    
	    if (_cairo_surface_is_recording (src)) {
		return _cairo_recording_surface_clone_similar (surface, src,
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
















cairo_bool_t
_cairo_surface_is_similar (cairo_surface_t *surface_a,
	                   cairo_surface_t *surface_b)
{
    if (surface_a->backend != surface_b->backend)
	return FALSE;

    if (surface_a->backend->is_similar != NULL)
	return surface_a->backend->is_similar (surface_a, surface_b);

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
			  unsigned int		height,
			  cairo_region_t	*clip_region)
{
    cairo_int_status_t status;

    if (unlikely (dst->status))
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
					  width, height,
					  clip_region);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (dst, status);
    }

    return _cairo_surface_set_error (dst,
	    _cairo_surface_fallback_composite (op,
					      src, mask, dst,
					      src_x, src_y,
					      mask_x, mask_y,
					      dst_x, dst_y,
					      width, height,
					      clip_region));
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

    
    if (op == CAIRO_OPERATOR_IN &&
	_cairo_color_equal (color, CAIRO_COLOR_WHITE))
    {
	return CAIRO_STATUS_SUCCESS;
    }

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

    status =  _cairo_surface_fill_rectangles (surface,
					      op, color, rects, num_rects);

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
	status = surface->backend->fill_rectangles (surface,
						    op, color,
						    rects, num_rects);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (surface, status);
    }

    return _cairo_surface_set_error (surface,
	    _cairo_surface_fallback_fill_rectangles (surface,
						     op, color,
						     rects, num_rects));
}

static cairo_status_t
_pattern_has_error (const cairo_pattern_t *pattern)
{
    const cairo_surface_pattern_t *spattern;

    if (unlikely (pattern->status))
	return pattern->status;

    if (pattern->type != CAIRO_PATTERN_TYPE_SURFACE)
	return CAIRO_STATUS_SUCCESS;

    spattern = (const cairo_surface_pattern_t *) pattern;
    if (unlikely (spattern->surface->status))
	return spattern->surface->status;

    if (unlikely (spattern->surface->finished))
	return _cairo_error (CAIRO_STATUS_SURFACE_FINISHED);

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_paint (cairo_surface_t	*surface,
		      cairo_operator_t	 op,
		      const cairo_pattern_t *source,
		      cairo_clip_t	    *clip)
{
    cairo_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_CLEAR && surface->is_clear)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_OVER &&
	_cairo_pattern_is_clear (source))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->paint != NULL) {
	status = surface->backend->paint (surface, op, source, clip);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_paint (surface, op, source, clip);

 FINISH:
    surface->is_clear = op == CAIRO_OPERATOR_CLEAR && clip == NULL;

    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_mask (cairo_surface_t		*surface,
		     cairo_operator_t		 op,
		     const cairo_pattern_t	*source,
		     const cairo_pattern_t	*mask,
		     cairo_clip_t		*clip)
{
    cairo_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_CLEAR && surface->is_clear)
	return CAIRO_STATUS_SUCCESS;

    
    if (_cairo_pattern_is_clear (mask) &&
	_cairo_operator_bounded_by_mask (op))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    if (op == CAIRO_OPERATOR_OVER &&
	_cairo_pattern_is_clear (source))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    status = _pattern_has_error (mask);
    if (unlikely (status))
	return status;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->mask != NULL) {
	status = surface->backend->mask (surface, op, source, mask, clip);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_mask (surface, op, source, mask, clip);

 FINISH:
    surface->is_clear = FALSE;

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
			    const cairo_stroke_style_t    *stroke_style,
			    const cairo_matrix_t	    *stroke_ctm,
			    const cairo_matrix_t	    *stroke_ctm_inverse,
			    double		     stroke_tolerance,
			    cairo_antialias_t	     stroke_antialias,
			    cairo_clip_t	    *clip)
{
    cairo_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (surface->is_clear &&
	fill_op == CAIRO_OPERATOR_CLEAR &&
	stroke_op == CAIRO_OPERATOR_CLEAR)
    {
	return CAIRO_STATUS_SUCCESS;
    }

    status = _pattern_has_error (fill_source);
    if (unlikely (status))
	return status;

    status = _pattern_has_error (stroke_source);
    if (unlikely (status))
	return status;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->fill_stroke) {
	cairo_matrix_t dev_ctm = *stroke_ctm;
	cairo_matrix_t dev_ctm_inverse = *stroke_ctm_inverse;

	status = surface->backend->fill_stroke (surface,
						fill_op, fill_source, fill_rule,
						fill_tolerance, fill_antialias,
						path,
						stroke_op, stroke_source,
						stroke_style,
						&dev_ctm, &dev_ctm_inverse,
						stroke_tolerance, stroke_antialias,
						clip);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    goto FINISH;
    }

    status = _cairo_surface_fill (surface, fill_op, fill_source, path,
				  fill_rule, fill_tolerance, fill_antialias,
				  clip);
    if (unlikely (status))
	goto FINISH;

    status = _cairo_surface_stroke (surface, stroke_op, stroke_source, path,
				    stroke_style, stroke_ctm, stroke_ctm_inverse,
				    stroke_tolerance, stroke_antialias,
				    clip);
    if (unlikely (status))
	goto FINISH;

  FINISH:
    surface->is_clear = FALSE;

    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_stroke (cairo_surface_t		*surface,
		       cairo_operator_t		 op,
		       const cairo_pattern_t	*source,
		       cairo_path_fixed_t	*path,
		       const cairo_stroke_style_t	*stroke_style,
		       const cairo_matrix_t		*ctm,
		       const cairo_matrix_t		*ctm_inverse,
		       double			 tolerance,
		       cairo_antialias_t	 antialias,
		       cairo_clip_t		*clip)
{
    cairo_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_CLEAR && surface->is_clear)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_OVER &&
	_cairo_pattern_is_clear (source))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->stroke != NULL) {
	status = surface->backend->stroke (surface, op, source,
					   path, stroke_style,
					   ctm, ctm_inverse,
					   tolerance, antialias,
					   clip);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_stroke (surface, op, source,
                                             path, stroke_style,
                                             ctm, ctm_inverse,
                                             tolerance, antialias,
					     clip);

 FINISH:
    surface->is_clear = FALSE;

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
		     cairo_clip_t	*clip)
{
    cairo_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_CLEAR && surface->is_clear)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_OVER &&
	_cairo_pattern_is_clear (source))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->fill != NULL) {
	status = surface->backend->fill (surface, op, source,
					 path, fill_rule,
					 tolerance, antialias,
					 clip);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_fill (surface, op, source,
                                           path, fill_rule,
                                           tolerance, antialias,
					   clip);

 FINISH:
    surface->is_clear = FALSE;

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
				     int			num_traps,
				     cairo_region_t		*clip_region)
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
						     traps, num_traps,
						     clip_region);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (dst, status);
    }

    return  _cairo_surface_set_error (dst,
	    _cairo_surface_fallback_composite_trapezoids (op, pattern, dst,
							  antialias,
							  src_x, src_y,
							  dst_x, dst_y,
							  width, height,
							  traps, num_traps,
							  clip_region));
}

cairo_span_renderer_t *
_cairo_surface_create_span_renderer (cairo_operator_t		 op,
				     const cairo_pattern_t	*pattern,
				     cairo_surface_t		*dst,
				     cairo_antialias_t	         antialias,
				     const cairo_composite_rectangles_t *rects,
				     cairo_region_t		*clip_region)
{
    assert (dst->snapshot_of == NULL);

    if (unlikely (dst->status))
	return _cairo_span_renderer_create_in_error (dst->status);

    if (unlikely (dst->finished))
	return _cairo_span_renderer_create_in_error (CAIRO_STATUS_SURFACE_FINISHED);

    if (dst->backend->create_span_renderer) {
	return dst->backend->create_span_renderer (op,
						   pattern, dst,
						   antialias,
						   rects,
						   clip_region);
    }
    ASSERT_NOT_REACHED;
    return _cairo_span_renderer_create_in_error (CAIRO_INT_STATUS_UNSUPPORTED);
}

cairo_bool_t
_cairo_surface_check_span_renderer (cairo_operator_t		 op,
				    const cairo_pattern_t	*pattern,
				    cairo_surface_t		*dst,
				    cairo_antialias_t	         antialias)
{
    assert (dst->snapshot_of == NULL);
    assert (dst->status == CAIRO_STATUS_SUCCESS);
    assert (! dst->finished);

    
    if (antialias == CAIRO_ANTIALIAS_NONE)
	return FALSE;

    if (dst->backend->check_span_renderer != NULL)
	return dst->backend->check_span_renderer (op, pattern, dst, antialias);

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

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
		                                 CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    _cairo_surface_begin_modification (surface);

    
    if (surface->backend->show_page == NULL)
	return;

    status_ignored = _cairo_surface_set_error (surface,
			                 surface->backend->show_page (surface));
}
slim_hidden_def (cairo_surface_show_page);

























cairo_bool_t
_cairo_surface_get_extents (cairo_surface_t         *surface,
			    cairo_rectangle_int_t   *extents)
{
    cairo_bool_t bounded;

    bounded = FALSE;
    if (surface->backend->get_extents != NULL)
	bounded = surface->backend->get_extents (surface, extents);

    if (! bounded)
	_cairo_unbounded_rectangle_init (extents);

    return bounded;
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














void
cairo_surface_set_subpixel_antialiasing (cairo_surface_t *surface,
                                         cairo_subpixel_antialiasing_t enabled)
{
    if (surface->status)
        return;

    if (surface->finished) {
        _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
        return;
    }

    surface->permit_subpixel_antialiasing =
        enabled == CAIRO_SUBPIXEL_ANTIALIASING_ENABLED;
}
slim_hidden_def (cairo_surface_set_subpixel_antialiasing);











cairo_subpixel_antialiasing_t
cairo_surface_get_subpixel_antialiasing (cairo_surface_t *surface)
{
    if (surface->status)
        return CAIRO_SUBPIXEL_ANTIALIASING_DISABLED;

    return surface->permit_subpixel_antialiasing ?
        CAIRO_SUBPIXEL_ANTIALIASING_ENABLED : CAIRO_SUBPIXEL_ANTIALIASING_DISABLED;
}
slim_hidden_def (cairo_surface_get_subpixel_antialiasing);
















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
				 cairo_clip_t		    *clip)
{
    cairo_status_t status;
    cairo_scaled_font_t *dev_scaled_font = scaled_font;

    if (unlikely (surface->status))
	return surface->status;

    if (num_glyphs == 0 && utf8_len == 0)
	return CAIRO_STATUS_SUCCESS;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_CLEAR && surface->is_clear)
	return CAIRO_STATUS_SUCCESS;

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    _cairo_surface_begin_modification (surface);

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
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    status = CAIRO_INT_STATUS_UNSUPPORTED;

    

    if (clusters) {
	

	if (surface->backend->show_text_glyphs != NULL) {
	    status = surface->backend->show_text_glyphs (surface, op,
							 source,
							 utf8, utf8_len,
							 glyphs, num_glyphs,
							 clusters, num_clusters, cluster_flags,
							 dev_scaled_font,
							 clip);
	}
	if (status == CAIRO_INT_STATUS_UNSUPPORTED &&
	    surface->backend->show_glyphs)
	{
	    int remaining_glyphs = num_glyphs;
	    status = surface->backend->show_glyphs (surface, op,
						    source,
						    glyphs, num_glyphs,
						    dev_scaled_font,
						    clip,
						    &remaining_glyphs);
	    glyphs += num_glyphs - remaining_glyphs;
	    num_glyphs = remaining_glyphs;
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED && remaining_glyphs == 0)
		status = CAIRO_STATUS_SUCCESS;
	}
    } else {
	
	if (surface->backend->show_glyphs != NULL) {
	    int remaining_glyphs = num_glyphs;
	    status = surface->backend->show_glyphs (surface, op,
						    source,
						    glyphs, num_glyphs,
						    dev_scaled_font,
						    clip,
						    &remaining_glyphs);
	    glyphs += num_glyphs - remaining_glyphs;
	    num_glyphs = remaining_glyphs;
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED && remaining_glyphs == 0)
		status = CAIRO_STATUS_SUCCESS;
	} else if (surface->backend->show_text_glyphs != NULL) {
	    







	    status = surface->backend->show_text_glyphs (surface, op,
							 source,
							 utf8, utf8_len,
							 glyphs, num_glyphs,
							 clusters, num_clusters, cluster_flags,
							 dev_scaled_font,
							 clip);
	}
    }

    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	status = _cairo_surface_fallback_show_glyphs (surface, op,
						      source,
						      glyphs, num_glyphs,
						      dev_scaled_font,
						      clip);
    }

    if (dev_scaled_font != scaled_font)
	cairo_scaled_font_destroy (dev_scaled_font);

    surface->is_clear = FALSE;

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
				int			 num_glyphs,
				cairo_region_t		*clip_region)
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
						glyphs, num_glyphs,
						clip_region);
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
						   unsigned int		    height,
						   cairo_region_t	    *clip_region)
{
    cairo_rectangle_int_t dst_rectangle;
    cairo_region_t clear_region;
    cairo_status_t status;

    


    dst_rectangle.x = dst_x;
    dst_rectangle.y = dst_y;
    dst_rectangle.width = width;
    dst_rectangle.height = height;

    _cairo_region_init_rectangle (&clear_region, &dst_rectangle);

    if (clip_region != NULL) {
	status = cairo_region_intersect (&clear_region, clip_region);
	if (unlikely (status))
	    goto CLEANUP_REGIONS;
    }

    if (src_rectangle != NULL) {
        if (! _cairo_rectangle_intersect (&dst_rectangle, src_rectangle))
	    goto EMPTY;
    }

    if (mask_rectangle != NULL) {
        if (! _cairo_rectangle_intersect (&dst_rectangle, mask_rectangle))
	    goto EMPTY;
    }

    
    status = cairo_region_subtract_rectangle (&clear_region, &dst_rectangle);
    if (unlikely (status) || cairo_region_is_empty (&clear_region))
        goto CLEANUP_REGIONS;

  EMPTY:
    status = _cairo_surface_fill_region (dst, CAIRO_OPERATOR_CLEAR,
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
					  unsigned int		      height,
					  cairo_region_t	     *clip_region)
{
    cairo_rectangle_int_t src_tmp, mask_tmp;
    cairo_rectangle_int_t *src_rectangle = NULL;
    cairo_rectangle_int_t *mask_rectangle = NULL;

    if (unlikely (dst->status))
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
							      dst_x, dst_y, width, height,
							      clip_region);
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
						unsigned int		    height,
						cairo_region_t	    *clip_region)
{
    cairo_rectangle_int_t src_tmp, *src= NULL;
    cairo_rectangle_int_t mask;

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    


    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width  = src_width;
	src_tmp.height = src_height;

	src = &src_tmp;
    }

    mask.x = dst_x - mask_x;
    mask.y = dst_y - mask_y;
    mask.width  = mask_width;
    mask.height = mask_height;

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src, &mask,
							      dst_x, dst_y, width, height,
							      clip_region);
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



static void
_rectangle_intersect_clip (cairo_rectangle_int_t *extents, cairo_clip_t *clip)
{
    const cairo_rectangle_int_t *clip_extents;
    cairo_bool_t is_empty;

    clip_extents = NULL;
    if (clip != NULL)
	clip_extents = _cairo_clip_get_extents (clip);

    if (clip_extents != NULL)
	is_empty = _cairo_rectangle_intersect (extents, clip_extents);
}

static void
_cairo_surface_operation_extents (cairo_surface_t *surface,
				  cairo_operator_t op,
				  const cairo_pattern_t *source,
				  cairo_clip_t *clip,
				  cairo_rectangle_int_t *extents)
{
    cairo_bool_t is_empty;

    is_empty = _cairo_surface_get_extents (surface, extents);

    if (_cairo_operator_bounded_by_source (op)) {
	cairo_rectangle_int_t source_extents;

	_cairo_pattern_get_extents (source, &source_extents);
	is_empty = _cairo_rectangle_intersect (extents, &source_extents);
    }

    _rectangle_intersect_clip (extents, clip);
}

cairo_status_t
_cairo_surface_paint_extents (cairo_surface_t *surface,
			      cairo_operator_t		op,
			      const cairo_pattern_t	*source,
			      cairo_clip_t		*clip,
			      cairo_rectangle_int_t	*extents)
{
    _cairo_surface_operation_extents (surface, op, source, clip, extents);
    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_mask_extents (cairo_surface_t *surface,
			     cairo_operator_t		 op,
			     const cairo_pattern_t	*source,
			     const cairo_pattern_t	*mask,
			     cairo_clip_t		*clip,
			     cairo_rectangle_int_t	*extents)
{
    cairo_bool_t is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t mask_extents;

	_cairo_pattern_get_extents (mask, &mask_extents);
	is_empty = _cairo_rectangle_intersect (extents, &mask_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_stroke_extents (cairo_surface_t *surface,
			       cairo_operator_t op,
			       const cairo_pattern_t *source,
			       cairo_path_fixed_t	*path,
			       const cairo_stroke_style_t *style,
			       const cairo_matrix_t *ctm,
			       const cairo_matrix_t *ctm_inverse,
			       double tolerance,
			       cairo_antialias_t	 antialias,
			       cairo_clip_t *clip,
			       cairo_rectangle_int_t *extents)
{
    cairo_status_t status;
    cairo_bool_t is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t mask_extents;

	status = _cairo_path_fixed_stroke_extents (path, style,
						   ctm, ctm_inverse,
						   tolerance,
						   &mask_extents);
	if (unlikely (status))
	    return status;

	is_empty = _cairo_rectangle_intersect (extents, &mask_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_fill_extents (cairo_surface_t		*surface,
			     cairo_operator_t		 op,
			     const cairo_pattern_t	*source,
			     cairo_path_fixed_t		*path,
			     cairo_fill_rule_t		 fill_rule,
			     double			 tolerance,
			     cairo_antialias_t		 antialias,
			     cairo_clip_t		*clip,
			     cairo_rectangle_int_t	*extents)
{
    cairo_bool_t is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t mask_extents;

	_cairo_path_fixed_fill_extents (path, fill_rule, tolerance,
					&mask_extents);
	is_empty = _cairo_rectangle_intersect (extents, &mask_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_glyphs_extents (cairo_surface_t *surface,
			       cairo_operator_t	   op,
			       const cairo_pattern_t *source,
			       cairo_glyph_t	  *glyphs,
			       int		   num_glyphs,
			       cairo_scaled_font_t  *scaled_font,
			       cairo_clip_t         *clip,
			       cairo_rectangle_int_t *extents)
{
    cairo_status_t	     status;
    cairo_bool_t             is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t glyph_extents;

	status = _cairo_scaled_font_glyph_device_extents (scaled_font,
							  glyphs,
							  num_glyphs,
							  &glyph_extents,
							  NULL);
	if (unlikely (status))
	    return status;

	is_empty = _cairo_rectangle_intersect (extents, &glyph_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_surface_t *
_cairo_surface_create_in_error (cairo_status_t status)
{
    switch (status) {
    case CAIRO_STATUS_NO_MEMORY:
	return (cairo_surface_t *) &_cairo_surface_nil;
    case CAIRO_STATUS_SURFACE_TYPE_MISMATCH:
	return (cairo_surface_t *) &_cairo_surface_nil_surface_type_mismatch;
    case CAIRO_STATUS_INVALID_STATUS:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_status;
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
    case CAIRO_STATUS_DEVICE_TYPE_MISMATCH:
	return (cairo_surface_t *) &_cairo_surface_nil_device_type_mismatch;
    case CAIRO_STATUS_DEVICE_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_device_error;
    case CAIRO_STATUS_SUCCESS:
    case CAIRO_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	
    case CAIRO_STATUS_INVALID_RESTORE:
    case CAIRO_STATUS_INVALID_POP_GROUP:
    case CAIRO_STATUS_NO_CURRENT_POINT:
    case CAIRO_STATUS_INVALID_MATRIX:
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



