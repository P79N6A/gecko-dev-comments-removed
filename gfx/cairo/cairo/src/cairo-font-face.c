







































#include "cairoint.h"



const cairo_font_face_t _cairo_font_face_nil = {
    { 0 },				
    CAIRO_STATUS_NO_MEMORY,		
    CAIRO_REFERENCE_COUNT_INVALID,	
    { 0, 0, 0, NULL },			
    NULL
};

cairo_status_t
_cairo_font_face_set_error (cairo_font_face_t *font_face,
	                    cairo_status_t     status)
{
    if (status == CAIRO_STATUS_SUCCESS)
	return status;

    

    _cairo_status_set_error (&font_face->status, status);

    return _cairo_error (status);
}

void
_cairo_font_face_init (cairo_font_face_t               *font_face,
		       const cairo_font_face_backend_t *backend)
{
    CAIRO_MUTEX_INITIALIZE ();

    font_face->status = CAIRO_STATUS_SUCCESS;
    CAIRO_REFERENCE_COUNT_INIT (&font_face->ref_count, 1);
    font_face->backend = backend;

    _cairo_user_data_array_init (&font_face->user_data);
}















cairo_font_face_t *
cairo_font_face_reference (cairo_font_face_t *font_face)
{
    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return font_face;

    



    _cairo_reference_count_inc (&font_face->ref_count);

    return font_face;
}
slim_hidden_def (cairo_font_face_reference);









void
cairo_font_face_destroy (cairo_font_face_t *font_face)
{
    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&font_face->ref_count));

    if (! _cairo_reference_count_dec_and_test (&font_face->ref_count))
	return;

    if (font_face->backend->destroy)
	font_face->backend->destroy (font_face);

    



    if (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&font_face->ref_count))
	return;

    _cairo_user_data_array_fini (&font_face->user_data);

    free (font_face);
}
slim_hidden_def (cairo_font_face_destroy);












cairo_font_type_t
cairo_font_face_get_type (cairo_font_face_t *font_face)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return CAIRO_FONT_TYPE_TOY;

    return font_face->backend->type;
}












unsigned int
cairo_font_face_get_reference_count (cairo_font_face_t *font_face)
{
    if (font_face == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&font_face->ref_count);
}











cairo_status_t
cairo_font_face_status (cairo_font_face_t *font_face)
{
    return font_face->status;
}













void *
cairo_font_face_get_user_data (cairo_font_face_t	   *font_face,
			       const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&font_face->user_data,
					    key);
}
slim_hidden_def (cairo_font_face_get_user_data);

















cairo_status_t
cairo_font_face_set_user_data (cairo_font_face_t	   *font_face,
			       const cairo_user_data_key_t *key,
			       void			   *user_data,
			       cairo_destroy_func_t	    destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return font_face->status;

    return _cairo_user_data_array_set_data (&font_face->user_data,
					    key, user_data, destroy);
}
slim_hidden_def (cairo_font_face_set_user_data);

void
_cairo_unscaled_font_init (cairo_unscaled_font_t               *unscaled_font,
			   const cairo_unscaled_font_backend_t *backend)
{
    CAIRO_REFERENCE_COUNT_INIT (&unscaled_font->ref_count, 1);
    unscaled_font->backend = backend;
}

cairo_unscaled_font_t *
_cairo_unscaled_font_reference (cairo_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return NULL;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&unscaled_font->ref_count));

    _cairo_reference_count_inc (&unscaled_font->ref_count);

    return unscaled_font;
}

void
_cairo_unscaled_font_destroy (cairo_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&unscaled_font->ref_count));

    if (! _cairo_reference_count_dec_and_test (&unscaled_font->ref_count))
	return;

    unscaled_font->backend->destroy (unscaled_font);

    free (unscaled_font);
}
