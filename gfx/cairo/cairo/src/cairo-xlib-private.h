































#ifndef CAIRO_XLIB_PRIVATE_H
#define CAIRO_XLIB_PRIVATE_H

#include "cairoint.h"
#include "cairo-xlib.h"

typedef struct _cairo_xlib_screen_info cairo_xlib_screen_info_t;

struct _cairo_xlib_screen_info {
    cairo_xlib_screen_info_t *next;

    Display *display;
    Screen *screen;
    cairo_bool_t has_render;

    cairo_font_options_t font_options;
};

cairo_private cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_get (Display *display, Screen *screen);

#if CAIRO_HAS_XLIB_XRENDER_SURFACE

#include "cairo-xlib-xrender.h"
#if 0
slim_hidden_proto (cairo_xlib_surface_create_with_xrender_format);
#endif

#endif

#endif 
