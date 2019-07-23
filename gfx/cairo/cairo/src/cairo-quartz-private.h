


































#ifndef CAIRO_QUARTZ_PRIVATE_H
#define CAIRO_QUARTZ_PRIVATE_H

#include <cairoint.h>
#include <cairo-quartz.h>

typedef struct cairo_quartz_surface {
    cairo_surface_t base;

    CGContextRef context;

    cairo_bool_t y_grows_down;

    cairo_rectangle_int16_t extents;

    pixman_region16_t *clip_region;
} cairo_quartz_surface_t;

cairo_bool_t
_cairo_surface_is_quartz (cairo_surface_t *surface);

cairo_bool_t
_cairo_scaled_font_is_atsui (cairo_scaled_font_t *sfont);

ATSUStyle
_cairo_atsui_scaled_font_get_atsu_style (cairo_scaled_font_t *sfont);

ATSUFontID
_cairo_atsui_scaled_font_get_atsu_font_id (cairo_scaled_font_t *sfont);

#endif 
