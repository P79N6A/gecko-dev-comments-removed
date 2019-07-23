




































#ifndef CAIRO_QUARTZ_PRIVATE_H
#define CAIRO_QUARTZ_PRIVATE_H

#include "cairoint.h"

#ifdef CAIRO_HAS_QUARTZ_SURFACE
#include <cairo-quartz.h>

typedef struct cairo_quartz_surface {
    cairo_surface_t base;

    void *imageData;

    cairo_surface_t *imageSurfaceEquiv;

    CGContextRef cgContext;
    CGAffineTransform cgContextBaseCTM;

    cairo_rectangle_int_t extents;

    


    CGImageRef sourceImage;
    cairo_surface_t *sourceImageSurface;
    CGAffineTransform sourceImageTransform;
    CGRect sourceImageRect;

    CGShadingRef sourceShading;
    CGPatternRef sourcePattern;
} cairo_quartz_surface_t;
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
