































#ifndef CAIRO_XLIB_PRIVATE_H
#define CAIRO_XLIB_PRIVATE_H

#include "cairoint.h"
#include "cairo-xlib.h"

typedef struct _cairo_xlib_screen_info cairo_xlib_screen_info_t;
typedef struct _cairo_xlib_hook cairo_xlib_hook_t;

struct _cairo_xlib_hook {
    cairo_xlib_hook_t *next;
    void (*func) (Display *display, void *data);
    void *data;
    void *key;
};

struct _cairo_xlib_screen_info {
    cairo_xlib_screen_info_t *next;

    Display *display;
    Screen *screen;
    cairo_bool_t has_render;

    cairo_font_options_t font_options;

    cairo_xlib_hook_t *close_display_hooks;
};

cairo_private cairo_xlib_screen_info_t *
_cairo_xlib_screen_info_get (Display *display, Screen *screen);

cairo_private cairo_bool_t
_cairo_xlib_add_close_display_hook (Display *display, void (*func) (Display *, void *), void *data, void *key);
cairo_private void
_cairo_xlib_remove_close_display_hook (Display *display, void *key);

#if CAIRO_HAS_XLIB_XRENDER_SURFACE

#include "cairo-xlib-xrender.h"
slim_hidden_proto (cairo_xlib_surface_create_with_xrender_format);

#endif

#endif 
