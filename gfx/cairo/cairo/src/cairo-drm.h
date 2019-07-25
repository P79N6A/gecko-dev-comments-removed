































#ifndef CAIRO_DRM_H
#define CAIRO_DRM_H

#include "cairo.h"

#if CAIRO_HAS_DRM_SURFACE

CAIRO_BEGIN_DECLS

typedef struct _cairo_drm_device cairo_drm_device_t;

struct udev_device;

cairo_public cairo_drm_device_t *
cairo_drm_device_get (struct udev_device *device);

cairo_public cairo_drm_device_t *
cairo_drm_device_get_for_fd (int fd);

cairo_public cairo_drm_device_t *
cairo_drm_device_default (void);

cairo_public cairo_drm_device_t *
cairo_drm_device_reference (cairo_drm_device_t *device);

cairo_public cairo_status_t
cairo_drm_device_status (cairo_drm_device_t *device);

cairo_public int
cairo_drm_device_get_fd (cairo_drm_device_t *device);

cairo_public void
cairo_drm_device_throttle (cairo_drm_device_t *device);

cairo_public void
cairo_drm_device_destroy (cairo_drm_device_t *device);


cairo_public cairo_surface_t *
cairo_drm_surface_create (cairo_drm_device_t *device,
			  cairo_content_t content,
			  int width, int height);

cairo_public cairo_surface_t *
cairo_drm_surface_create_for_name (cairo_drm_device_t *device,
				   unsigned int name,
	                           cairo_format_t format,
				   int width, int height, int stride);

cairo_public cairo_surface_t *
cairo_drm_surface_create_from_cacheable_image (cairo_drm_device_t *device,
	                                       cairo_surface_t *surface);

cairo_public cairo_status_t
cairo_drm_surface_enable_scan_out (cairo_surface_t *surface);

cairo_public cairo_drm_device_t *
cairo_drm_surface_get_device (cairo_surface_t *abstract_surface);

cairo_public unsigned int
cairo_drm_surface_get_handle (cairo_surface_t *surface);

cairo_public unsigned int
cairo_drm_surface_get_name (cairo_surface_t *surface);

cairo_public cairo_format_t
cairo_drm_surface_get_format (cairo_surface_t *surface);

cairo_public int
cairo_drm_surface_get_width (cairo_surface_t *surface);

cairo_public int
cairo_drm_surface_get_height (cairo_surface_t *surface);

cairo_public int
cairo_drm_surface_get_stride (cairo_surface_t *surface);













cairo_public cairo_surface_t *
cairo_drm_surface_map (cairo_surface_t *surface);

cairo_public void
cairo_drm_surface_unmap (cairo_surface_t *drm_surface,
	                 cairo_surface_t *image_surface);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the DRM backend
#endif 

#endif 
