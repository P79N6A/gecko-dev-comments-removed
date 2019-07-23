































#ifndef CAIRO_XLIB_SURFACE_PRIVATE_H
#define CAIRO_XLIB_SURFACE_PRIVATE_H

#include "cairo-xlib.h"

#include "cairo-surface-private.h"

typedef struct _cairo_xlib_surface cairo_xlib_surface_t;

struct _cairo_xlib_surface {
    cairo_surface_t base;

    Display *dpy;
    cairo_xlib_screen_info_t *screen_info;

    GC gc;
    Drawable drawable;
    Screen *screen;
    cairo_bool_t owns_pixmap;
    Visual *visual;

    int use_pixmap;

    int render_major;
    int render_minor;

    














    cairo_bool_t buggy_repeat;

    int width;
    int height;
    int depth;

    Picture dst_picture, src_picture;

    unsigned int clip_dirty;
    cairo_bool_t have_clip_rects;
    XRectangle embedded_clip_rects[4];
    XRectangle *clip_rects;
    int num_clip_rects;

    XRenderPictFormat *xrender_format;
    cairo_filter_t filter;
    int repeat;
    XTransform xtransform;
};

enum {
    CAIRO_XLIB_SURFACE_CLIP_DIRTY_GC      = 0x01,
    CAIRO_XLIB_SURFACE_CLIP_DIRTY_PICTURE = 0x02,
    CAIRO_XLIB_SURFACE_CLIP_DIRTY_ALL     = 0x03
};

#endif 
