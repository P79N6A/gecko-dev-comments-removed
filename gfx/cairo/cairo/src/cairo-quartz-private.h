




































#ifndef CAIRO_QUARTZ_PRIVATE_H
#define CAIRO_QUARTZ_PRIVATE_H

#include "cairoint.h"

#ifdef CAIRO_HAS_QUARTZ_SURFACE
#include <cairo-quartz.h>

typedef struct cairo_quartz_surface {
    cairo_surface_t base;

    CGContextRef cgContext;
    CGAffineTransform cgContextBaseCTM;

    void *imageData;
    cairo_surface_t *imageSurfaceEquiv;

    cairo_rectangle_int_t extents;

    


    CGImageRef sourceImage;
    cairo_surface_t *sourceImageSurface;
    CGAffineTransform sourceImageTransform;
    CGRect sourceImageRect;

    CGShadingRef sourceShading;
    CGPatternRef sourcePattern;

    CGInterpolationQuality oldInterpolationQuality;
} cairo_quartz_surface_t;

typedef struct cairo_quartz_image_surface {
    cairo_surface_t base;

    cairo_rectangle_int_t extents;

    CGImageRef image;
    cairo_image_surface_t *imageSurface;
} cairo_quartz_image_surface_t;

cairo_bool_t
_cairo_quartz_verify_surface_size(int width, int height);

CGImageRef
_cairo_quartz_create_cgimage (cairo_format_t format,
			      unsigned int width,
			      unsigned int height,
			      unsigned int stride,
			      void *data,
			      cairo_bool_t interpolate,
			      CGColorSpaceRef colorSpaceOverride,
			      CGDataProviderReleaseDataCallback releaseCallback,
			      void *releaseInfo);

#endif 

#if CAIRO_HAS_ATSUI_FONT
ATSUStyle
_cairo_atsui_scaled_font_get_atsu_style (cairo_scaled_font_t *sfont);

ATSUFontID
_cairo_atsui_scaled_font_get_atsu_font_id (cairo_scaled_font_t *sfont);

CGFontRef
_cairo_atsui_scaled_font_get_cg_font_ref (cairo_scaled_font_t *sfont);
#endif 

#endif 
