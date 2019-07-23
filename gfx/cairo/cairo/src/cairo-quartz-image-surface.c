



































#include "cairoint.h"

#ifdef CAIRO_HAS_QUARTZ_IMAGE_SURFACE
#include "cairo-quartz-image.h"
#endif

#include "cairo-quartz-private.h"

#define SURFACE_ERROR_NO_MEMORY (_cairo_surface_create_in_error(_cairo_error(CAIRO_STATUS_NO_MEMORY)))
#define SURFACE_ERROR_INVALID_FORMAT (_cairo_surface_create_in_error(_cairo_error(CAIRO_STATUS_INVALID_FORMAT)))

CGImageRef
_cairo_quartz_create_cgimage (cairo_format_t format,
			      unsigned int width,
			      unsigned int height,
			      unsigned int stride,
			      void *data,
			      cairo_bool_t interpolate,
			      CGColorSpaceRef colorSpaceOverride,
			      CGDataProviderReleaseDataCallback releaseCallback,
			      void *releaseInfo)
{
    CGImageRef image = NULL;
    CGDataProviderRef dataProvider = NULL;
    CGColorSpaceRef colorSpace = colorSpaceOverride;
    CGBitmapInfo bitinfo;
    int bitsPerComponent, bitsPerPixel;

    switch (format) {
	case CAIRO_FORMAT_ARGB32:
	    if (colorSpace == NULL)
		colorSpace = CGColorSpaceCreateDeviceRGB();
	    bitinfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
	    bitsPerComponent = 8;
	    bitsPerPixel = 32;
	    break;

	case CAIRO_FORMAT_RGB24:
	    if (colorSpace == NULL)
		colorSpace = CGColorSpaceCreateDeviceRGB();
	    bitinfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
	    bitsPerComponent = 8;
	    bitsPerPixel = 32;
	    break;

	
	case CAIRO_FORMAT_A8:
	    if (colorSpace == NULL)
		colorSpace = CGColorSpaceCreateDeviceGray();
	    bitinfo = kCGImageAlphaNone;
	    bitsPerComponent = 8;
	    bitsPerPixel = 8;
	    break;

	case CAIRO_FORMAT_A1:
	default:
	    return NULL;
    }

    dataProvider = CGDataProviderCreateWithData (releaseInfo,
						 data,
						 height * stride,
						 releaseCallback);

    if (!dataProvider) {
	
	if (releaseCallback)
	    releaseCallback (releaseInfo, data, height * stride);
	goto FINISH;
    }

    image = CGImageCreate (width, height,
			   bitsPerComponent,
			   bitsPerPixel,
			   stride,
			   colorSpace,
			   bitinfo,
			   dataProvider,
			   NULL,
			   interpolate,
			   kCGRenderingIntentDefault);

FINISH:

    CGDataProviderRelease (dataProvider);

    if (colorSpace != colorSpaceOverride)
	CGColorSpaceRelease (colorSpace);

    return image;
}

#ifdef CAIRO_HAS_QUARTZ_IMAGE_SURFACE

static void
DataProviderReleaseCallback (void *info, const void *data, size_t size)
{
    cairo_surface_t *surface = (cairo_surface_t *) info;
    cairo_surface_destroy (surface);
}

static cairo_surface_t *
_cairo_quartz_image_surface_create_similar (void *asurface,
					    cairo_content_t content,
					    int width,
					    int height)
{
    cairo_surface_t *result;
    cairo_surface_t *isurf = cairo_image_surface_create (_cairo_format_from_content (content),
							 width,
							 height);
    if (cairo_surface_status(isurf))
	return isurf;

    result = cairo_quartz_image_surface_create (isurf);
    cairo_surface_destroy (isurf);

    return result;
}

static cairo_status_t
_cairo_quartz_image_surface_finish (void *asurface)
{
    cairo_quartz_image_surface_t *surface = (cairo_quartz_image_surface_t *) asurface;

    
    CGImageRelease (surface->image);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_quartz_image_surface_acquire_source_image (void *asurface,
						  cairo_image_surface_t **image_out,
						  void **image_extra)
{
    cairo_quartz_image_surface_t *surface = (cairo_quartz_image_surface_t *) asurface;

    *image_out = surface->imageSurface;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_quartz_image_surface_acquire_dest_image (void *asurface,
						cairo_rectangle_int_t *interest_rect,
						cairo_image_surface_t **image_out,
						cairo_rectangle_int_t *image_rect,
						void **image_extra)
{
    cairo_quartz_image_surface_t *surface = (cairo_quartz_image_surface_t *) asurface;

    *image_out = surface->imageSurface;
    *image_rect = surface->extents;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
   
}

static cairo_int_status_t
_cairo_quartz_image_surface_get_extents (void *asurface,
					 cairo_rectangle_int_t *extents)
{
    cairo_quartz_image_surface_t *surface = (cairo_quartz_image_surface_t *) asurface;

    *extents = surface->extents;

    return CAIRO_STATUS_SUCCESS;
}





static cairo_status_t
_cairo_quartz_image_surface_flush (void *asurface)
{
    cairo_quartz_image_surface_t *surface = (cairo_quartz_image_surface_t *) asurface;
    CGImageRef oldImage = surface->image;
    CGImageRef newImage = NULL;

    
    cairo_surface_reference ((cairo_surface_t*) surface->imageSurface);

    newImage = _cairo_quartz_create_cgimage (surface->imageSurface->format,
					     surface->imageSurface->width,
					     surface->imageSurface->height,
					     surface->imageSurface->stride,
					     surface->imageSurface->data,
					     TRUE,
					     NULL,
					     DataProviderReleaseCallback,
					     surface->imageSurface);

    surface->image = newImage;
    CGImageRelease (oldImage);

    return CAIRO_STATUS_SUCCESS;
}

static const cairo_surface_backend_t cairo_quartz_image_surface_backend = {
    CAIRO_SURFACE_TYPE_QUARTZ_IMAGE,
    _cairo_quartz_image_surface_create_similar,
    _cairo_quartz_image_surface_finish,
    _cairo_quartz_image_surface_acquire_source_image,
    NULL, 
    _cairo_quartz_image_surface_acquire_dest_image,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_quartz_image_surface_get_extents,
    NULL, 
    NULL, 
    _cairo_quartz_image_surface_flush,
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
















cairo_surface_t *
cairo_quartz_image_surface_create (cairo_surface_t *surface)
{
    cairo_quartz_image_surface_t *qisurf;

    CGImageRef image;

    cairo_image_surface_t *image_surface;
    int width, height, stride;
    cairo_format_t format;
    unsigned char *data;

    if (cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE)
	return SURFACE_ERROR_NO_MEMORY;

    image_surface = (cairo_image_surface_t*) surface;
    width = image_surface->width;
    height = image_surface->height;
    stride = image_surface->stride;
    format = image_surface->format;
    data = image_surface->data;

    if (!_cairo_quartz_verify_surface_size(width, height))
	return SURFACE_ERROR_NO_MEMORY;

    if (width == 0 || height == 0)
	return SURFACE_ERROR_NO_MEMORY;

    if (format != CAIRO_FORMAT_ARGB32 && format != CAIRO_FORMAT_RGB24)
	return SURFACE_ERROR_INVALID_FORMAT;

    qisurf = malloc(sizeof(cairo_quartz_image_surface_t));
    if (qisurf == NULL)
	return SURFACE_ERROR_NO_MEMORY;

    memset (qisurf, 0, sizeof(cairo_quartz_image_surface_t));

    



    cairo_surface_reference (surface);

    image = _cairo_quartz_create_cgimage (format,
					  width, height,
					  stride,
					  data,
					  TRUE,
					  NULL,
					  DataProviderReleaseCallback,
					  surface);

    if (!image) {
	free (qisurf);
	return SURFACE_ERROR_NO_MEMORY;
    }

    _cairo_surface_init (&qisurf->base,
			 &cairo_quartz_image_surface_backend,
			 _cairo_content_from_format (format));

    qisurf->extents.x = qisurf->extents.y = 0;
    qisurf->extents.width = width;
    qisurf->extents.height = height;

    qisurf->image = image;
    qisurf->imageSurface = image_surface;

    return &qisurf->base;
}


cairo_surface_t *
cairo_quartz_image_surface_get_image (cairo_surface_t *asurface)
{
    cairo_quartz_image_surface_t *surface = (cairo_quartz_image_surface_t*) asurface;

    if (cairo_surface_get_type(asurface) != CAIRO_SURFACE_TYPE_QUARTZ_IMAGE)
	return NULL;

    return (cairo_surface_t*) surface->imageSurface;
}

#endif 
