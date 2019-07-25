




































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







typedef enum _cairo_gdk_drawing_opacity {
    CAIRO_GDK_DRAWING_OPAQUE,
    CAIRO_GDK_DRAWING_TRANSPARENT
} cairo_gdk_drawing_opacity_t;








typedef enum {
    CAIRO_GDK_DRAWING_SUPPORTS_CLIP_RECT = 0x02,
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
