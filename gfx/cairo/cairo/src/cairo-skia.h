



































#ifndef CAIRO_SKIA_H
#define CAIRO_SKIA_H

#include "cairo.h"

#if CAIRO_HAS_SKIA_SURFACE

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_skia_surface_create (cairo_format_t format,
			   int width,
			   int height);

cairo_public cairo_surface_t *
cairo_skia_surface_create_for_data (unsigned char *data,
				    cairo_format_t format,
				    int width,
				    int height,
				    int stride);

cairo_public unsigned char *
cairo_skia_surface_get_data (cairo_surface_t *surface);

cairo_public cairo_format_t
cairo_skia_surface_get_format (cairo_surface_t *surface);

cairo_public int
cairo_skia_surface_get_width (cairo_surface_t *surface);

cairo_public int
cairo_skia_surface_get_height (cairo_surface_t *surface);

cairo_public int
cairo_skia_surface_get_stride (cairo_surface_t *surface);

cairo_public cairo_surface_t *
cairo_skia_surface_get_image (cairo_surface_t *surface);

CAIRO_END_DECLS

#else

# error Cairo was not compiled with support for the Skia backend

#endif

#endif
