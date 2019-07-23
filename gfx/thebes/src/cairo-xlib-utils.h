




































#ifndef CAIROXLIBUTILS_H_
#define CAIROXLIBUTILS_H_

#include "cairo.h"
#include <X11/Xlib.h>

CAIRO_BEGIN_DECLS












typedef cairo_bool_t (* cairo_xlib_drawing_callback)
    (void *closure,
     Display *dpy,
     Drawable drawable,
     Visual *visual,
     short offset_x, short offset_y,
     XRectangle* clip_rects, unsigned int num_rects);





typedef struct {
    cairo_surface_t *surface;
    cairo_bool_t    uniform_alpha;
    cairo_bool_t    uniform_color;
    double          alpha; 
    double          r, g, b; 
} cairo_xlib_drawing_result_t;







typedef enum _cairo_xlib_drawing_opacity {
    CAIRO_XLIB_DRAWING_OPAQUE,
    CAIRO_XLIB_DRAWING_TRANSPARENT
} cairo_xlib_drawing_opacity_t;





















typedef enum {
    CAIRO_XLIB_DRAWING_SUPPORTS_OFFSET = 0x01,
    CAIRO_XLIB_DRAWING_SUPPORTS_CLIP_RECT = 0x02,
    CAIRO_XLIB_DRAWING_SUPPORTS_CLIP_LIST = 0x04,
    CAIRO_XLIB_DRAWING_SUPPORTS_ALTERNATE_DISPLAY = 0x08,
    CAIRO_XLIB_DRAWING_SUPPORTS_NONDEFAULT_VISUAL = 0x10
} cairo_xlib_drawing_support_t;

























void cairo_draw_with_xlib (cairo_t *cr,
                           cairo_xlib_drawing_callback callback,
                           void *closure,
                           Display *dpy,
                           unsigned int width, unsigned int height,
                           cairo_xlib_drawing_opacity_t is_opaque,
                           cairo_xlib_drawing_support_t capabilities,
                           cairo_xlib_drawing_result_t *result);

CAIRO_END_DECLS

#endif 
