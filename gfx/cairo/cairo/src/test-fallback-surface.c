




















































#include "cairoint.h"

#include "test-fallback-surface.h"

typedef struct _test_fallback_surface {
    cairo_surface_t base;

    
    cairo_surface_t *backing;
} test_fallback_surface_t;

const cairo_private cairo_surface_backend_t test_fallback_surface_backend;

slim_hidden_proto (_cairo_test_fallback_surface_create);

cairo_surface_t *
_cairo_test_fallback_surface_create (cairo_content_t	content,
			       int		width,
			       int		height)
{
    test_fallback_surface_t *surface;
    cairo_surface_t *backing;

    backing = _cairo_image_surface_create_with_content (content, width, height);
    if (cairo_surface_status (backing))
	return (cairo_surface_t*) &_cairo_surface_nil;

    surface = malloc (sizeof (test_fallback_surface_t));
    if (surface == NULL) {
	cairo_surface_destroy (backing);
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    _cairo_surface_init (&surface->base, &test_fallback_surface_backend,
			 content);

    surface->backing = backing;

    return &surface->base;
}
slim_hidden_def (_cairo_test_fallback_surface_create);

static cairo_surface_t *
_test_fallback_surface_create_similar (void		*abstract_surface,
				       cairo_content_t	 content,
				       int		 width,
				       int		 height)
{
    assert (CAIRO_CONTENT_VALID (content));

    return _cairo_test_fallback_surface_create (content,
					  width, height);
}

static cairo_status_t
_test_fallback_surface_finish (void *abstract_surface)
{
    test_fallback_surface_t *surface = abstract_surface;

    cairo_surface_destroy (surface->backing);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_test_fallback_surface_acquire_source_image (void	     *abstract_surface,
					     cairo_image_surface_t **image_out,
					     void		 **image_extra)
{
    test_fallback_surface_t *surface = abstract_surface;

    return _cairo_surface_acquire_source_image (surface->backing,
						image_out, image_extra);
}

static void
_test_fallback_surface_release_source_image (void	     *abstract_surface,
					     cairo_image_surface_t	*image,
					     void		  *image_extra)
{
    test_fallback_surface_t *surface = abstract_surface;

    _cairo_surface_release_source_image (surface->backing,
					 image, image_extra);
}

static cairo_status_t
_test_fallback_surface_acquire_dest_image (void		           *abstract_surface,
					   cairo_rectangle_int_t   *interest_rect,
					   cairo_image_surface_t  **image_out,
					   cairo_rectangle_int_t   *image_rect_out,
					   void			  **image_extra)
{
    test_fallback_surface_t *surface = abstract_surface;

    return _cairo_surface_acquire_dest_image (surface->backing,
					      interest_rect,
					      image_out,
					      image_rect_out,
					      image_extra);
}

static void
_test_fallback_surface_release_dest_image (void			   *abstract_surface,
					   cairo_rectangle_int_t   *interest_rect,
					   cairo_image_surface_t   *image,
					   cairo_rectangle_int_t   *image_rect,
					   void			   *image_extra)
{
    test_fallback_surface_t *surface = abstract_surface;

    _cairo_surface_release_dest_image (surface->backing,
				       interest_rect,
				       image,
				       image_rect,
				       image_extra);
}

static cairo_status_t
_test_fallback_surface_clone_similar (void		  *abstract_surface,
				      cairo_surface_t     *src,
				      int                  src_x,
				      int                  src_y,
				      int                  width,
				      int                  height,
				      cairo_surface_t    **clone_out)
{
    test_fallback_surface_t *surface = abstract_surface;

    if (src->backend == surface->base.backend) {
	*clone_out = cairo_surface_reference (src);

	return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_int_status_t
_test_fallback_surface_get_extents (void		  *abstract_surface,
				    cairo_rectangle_int_t *rectangle)
{
    test_fallback_surface_t *surface = abstract_surface;

    return _cairo_surface_get_extents (surface->backing, rectangle);
}

const cairo_surface_backend_t test_fallback_surface_backend = {
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_FALLBACK,
    _test_fallback_surface_create_similar,
    _test_fallback_surface_finish,
    _test_fallback_surface_acquire_source_image,
    _test_fallback_surface_release_source_image,
    _test_fallback_surface_acquire_dest_image,
    _test_fallback_surface_release_dest_image,
    _test_fallback_surface_clone_similar,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _test_fallback_surface_get_extents,
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
    NULL  
};
