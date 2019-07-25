


































#include "cairoint.h"
#include "cairo-device-private.h"
#include "cairo-error-private.h"


























































static const cairo_device_t _nil_device = {
    CAIRO_REFERENCE_COUNT_INVALID,
    CAIRO_STATUS_NO_MEMORY,
};

static const cairo_device_t _mismatch_device = {
    CAIRO_REFERENCE_COUNT_INVALID,
    CAIRO_STATUS_DEVICE_TYPE_MISMATCH,
};

static const cairo_device_t _invalid_device = {
    CAIRO_REFERENCE_COUNT_INVALID,
    CAIRO_STATUS_DEVICE_ERROR,
};

cairo_device_t *
_cairo_device_create_in_error (cairo_status_t status)
{
    switch (status) {
    case CAIRO_STATUS_NO_MEMORY:
	return (cairo_device_t *) &_nil_device;
    case CAIRO_STATUS_DEVICE_ERROR:
	return (cairo_device_t *) &_invalid_device;
    case CAIRO_STATUS_DEVICE_TYPE_MISMATCH:
	return (cairo_device_t *) &_mismatch_device;

    case CAIRO_STATUS_SUCCESS:
    case CAIRO_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	
    case CAIRO_STATUS_SURFACE_TYPE_MISMATCH:
    case CAIRO_STATUS_INVALID_STATUS:
    case CAIRO_STATUS_INVALID_FORMAT:
    case CAIRO_STATUS_INVALID_VISUAL:
    case CAIRO_STATUS_READ_ERROR:
    case CAIRO_STATUS_WRITE_ERROR:
    case CAIRO_STATUS_FILE_NOT_FOUND:
    case CAIRO_STATUS_TEMP_FILE_ERROR:
    case CAIRO_STATUS_INVALID_STRIDE:
    case CAIRO_STATUS_INVALID_SIZE:
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
    case CAIRO_STATUS_INVALID_CONTENT:
    default:
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_device_t *) &_nil_device;
    }
}

void
_cairo_device_init (cairo_device_t *device,
		    const cairo_device_backend_t *backend)
{
    CAIRO_REFERENCE_COUNT_INIT (&device->ref_count, 1);
    device->status = CAIRO_STATUS_SUCCESS;

    device->backend = backend;

    CAIRO_RECURSIVE_MUTEX_INIT (device->mutex);
    device->mutex_depth = 0;

    device->finished = FALSE;

    _cairo_user_data_array_init (&device->user_data);
}
















cairo_device_t *
cairo_device_reference (cairo_device_t *device)
{
    if (device == NULL ||
	CAIRO_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return device;
    }

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&device->ref_count));
    _cairo_reference_count_inc (&device->ref_count);

    return device;
}
slim_hidden_def (cairo_device_reference);













cairo_status_t
cairo_device_status (cairo_device_t *device)
{
    if (device == NULL)
	return CAIRO_STATUS_NULL_POINTER;

    return device->status;
}
















void
cairo_device_flush (cairo_device_t *device)
{
    cairo_status_t status;

    if (device == NULL || device->status)
	return;

    if (device->backend->flush != NULL) {
	status = device->backend->flush (device);
	if (unlikely (status))
	    status = _cairo_device_set_error (device, status);
    }
}
slim_hidden_def (cairo_device_flush);




















void
cairo_device_finish (cairo_device_t *device)
{
    if (device == NULL ||
	CAIRO_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return;
    }

    if (device->finished)
	return;

    cairo_device_flush (device);

    device->finished = TRUE;

    if (device->backend->finish != NULL)
	device->backend->finish (device);
}
slim_hidden_def (cairo_device_finish);













void
cairo_device_destroy (cairo_device_t *device)
{
    cairo_user_data_array_t user_data;

    if (device == NULL ||
	CAIRO_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return;
    }

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&device->ref_count));
    if (! _cairo_reference_count_dec_and_test (&device->ref_count))
	return;

    cairo_device_finish (device);

    assert (device->mutex_depth == 0);
    CAIRO_MUTEX_FINI (device->mutex);

    user_data = device->user_data;

    device->backend->destroy (device);

    _cairo_user_data_array_fini (&user_data);

}
slim_hidden_def (cairo_device_destroy);












cairo_device_type_t
cairo_device_get_type (cairo_device_t *device)
{
    if (device == NULL ||
	CAIRO_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return (cairo_device_type_t) -1;
    }

    return device->backend->type;
}































cairo_status_t
cairo_device_acquire (cairo_device_t *device)
{
    if (device == NULL)
	return CAIRO_STATUS_SUCCESS;

    if (unlikely (device->status))
	return device->status;

    if (unlikely (device->finished))
	return _cairo_device_set_error (device, CAIRO_STATUS_SURFACE_FINISHED); 

    CAIRO_MUTEX_LOCK (device->mutex);
    if (device->mutex_depth++ == 0) {
	if (device->backend->lock != NULL)
	    device->backend->lock (device);
    }

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_device_acquire);










void
cairo_device_release (cairo_device_t *device)
{
    if (device == NULL)
	return;

    assert (device->mutex_depth > 0);

    if (--device->mutex_depth == 0) {
	if (device->backend->unlock != NULL)
	    device->backend->unlock (device);
    }

    CAIRO_MUTEX_UNLOCK (device->mutex);
}
slim_hidden_def (cairo_device_release);

cairo_status_t
_cairo_device_set_error (cairo_device_t *device,
			 cairo_status_t  status)
{
    if (status == CAIRO_STATUS_SUCCESS || status >= CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    

    _cairo_status_set_error (&device->status, status);

    return _cairo_error (status);
}












unsigned int
cairo_device_get_reference_count (cairo_device_t *device)
{
    if (device == NULL ||
	CAIRO_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&device->ref_count);
}















void *
cairo_device_get_user_data (cairo_device_t		 *device,
			    const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&device->user_data,
					    key);
}



















cairo_status_t
cairo_device_set_user_data (cairo_device_t		 *device,
			    const cairo_user_data_key_t *key,
			    void			 *user_data,
			    cairo_destroy_func_t	  destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
	return device->status;

    return _cairo_user_data_array_set_data (&device->user_data,
					    key, user_data, destroy);
}
