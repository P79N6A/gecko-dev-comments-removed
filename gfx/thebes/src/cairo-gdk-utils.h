




































#ifndef CAIROGDKUTILS_H_
#define CAIROGDKUTILS_H_

#include "cairo.h"
#include <gdk/gdk.h>

CAIRO_BEGIN_DECLS












typedef cairo_bool_t (* cairo_gdk_drawing_callback)
    (void *closure,
     cairo_surface_t *surface,
     short offset_x, short offset_y,
     GdkRectangle * clip_rects, unsigned int num_rects);





typedef struct {
    cairo_surface_t *surface;
    cairo_bool_t    uniform_alpha;
    cairo_bool_t    uniform_color;
    double          alpha; 
    double          r, g, b; 
} cairo_gdk_drawing_result_t;







typedef enum _cairo_gdk_drawing_opacity {
    CAIRO_GDK_DRAWING_OPAQUE,
    CAIRO_GDK_DRAWING_TRANSPARENT
} cairo_gdk_drawing_opacity_t;





















typedef enum {
    CAIRO_GDK_DRAWING_SUPPORTS_OFFSET = 0x01,
    CAIRO_GDK_DRAWING_SUPPORTS_CLIP_RECT = 0x02,
    CAIRO_GDK_DRAWING_SUPPORTS_CLIP_LIST = 0x04,
    CAIRO_GDK_DRAWING_SUPPORTS_ALTERNATE_SCREEN = 0x08,
    CAIRO_GDK_DRAWING_SUPPORTS_NONDEFAULT_VISUAL = 0x10
} cairo_gdk_drawing_support_t;

























void cairo_draw_with_gdk (cairo_t *cr,
                          cairo_gdk_drawing_callback callback,
                          void * closure,
                          unsigned int width, unsigned int height,
                          cairo_gdk_drawing_opacity_t is_opaque,
                          cairo_gdk_drawing_support_t capabilities,
                          cairo_gdk_drawing_result_t *result);

CAIRO_END_DECLS

#endif 
