




































#ifndef CAIRO_QUARTZ_PRIVATE_H
#define CAIRO_QUARTZ_PRIVATE_H

#include <cairoint.h>

#ifdef CAIRO_HAS_QUARTZ_SURFACE
#include <cairo-quartz.h>

typedef struct cairo_quartz_surface {
    cairo_surface_t base;

    void *imageData;

    CGContextRef cgContext;
    CGAffineTransform cgContextBaseCTM;

    cairo_rectangle_int16_t extents;

    


    CGAffineTransform imageTransform;
    CGImageRef sourceImage;
    CGShadingRef sourceShading;
    CGPatternRef sourcePattern;
} cairo_quartz_surface_t;
#endif 

#if CAIRO_HAS_ATSUI_FONT
ATSUStyle
_cairo_atsui_scaled_font_get_atsu_style (cairo_scaled_font_t *sfont);

ATSUFontID
_cairo_atsui_scaled_font_get_atsu_font_id (cairo_scaled_font_t *sfont);
#endif 

#endif 
