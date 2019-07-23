




































#include "cairo-gdk-utils.h"

#include <stdlib.h>

#if   HAVE_STDINT_H
#include <stdint.h>
#elif HAVE_INTTYPES_H
#include <inttypes.h>
#elif HAVE_SYS_INT_TYPES_H
#include <sys/int_types.h>
#endif
















static cairo_bool_t
_convert_coord_to_short (double coord, short *v)
{
    *v = (short)coord;
    
    return *v == coord;
}

static cairo_bool_t
_convert_coord_to_unsigned_short (double coord, unsigned short *v)
{
    *v = (unsigned short)coord;
    
    return *v == coord;
}


void cairo_draw_with_gdk (cairo_t *cr,
                           cairo_gdk_drawing_callback callback,
                           void * closure,
                           unsigned int width, unsigned int height,
                           cairo_gdk_drawing_opacity_t is_opaque,
                           cairo_gdk_drawing_support_t capabilities,
                           cairo_gdk_drawing_result_t *result)
{
    double device_offset_x, device_offset_y;
    short offset_x = 0, offset_y = 0;
    
    cairo_surface_t * target = cairo_get_target (cr);
    cairo_matrix_t matrix;

    cairo_surface_get_device_offset (target, &device_offset_x, &device_offset_y);
    cairo_get_matrix (cr, &matrix);

    _convert_coord_to_short (matrix.x0 + device_offset_x, &offset_x);
    _convert_coord_to_short (matrix.y0 + device_offset_y, &offset_y);

    cairo_surface_flush (target);
    callback (closure, target, offset_x, offset_y, NULL, 0);
    cairo_surface_mark_dirty (target);
}


