


































#ifndef CAIRO_WIN32_PRIVATE_H
#define CAIRO_WIN32_PRIVATE_H

#include <cairo-win32.h>
#include <cairoint.h>

#ifndef SHADEBLENDCAPS
#define SHADEBLENDCAPS 120
#endif
#ifndef SB_NONE
#define SB_NONE 0
#endif

#define WIN32_FONT_LOGICAL_SCALE 1

typedef struct _cairo_win32_surface {
    cairo_surface_t base;

    cairo_format_t format;

    HDC dc;

    

    HBITMAP bitmap;
    cairo_bool_t is_dib;

    






    HBITMAP saved_dc_bitmap;

    cairo_surface_t *image;

    cairo_rectangle_int16_t clip_rect;

    HRGN saved_clip;

    cairo_rectangle_int16_t extents;

    
    uint32_t flags;
} cairo_win32_surface_t;


enum {
    
    CAIRO_WIN32_SURFACE_IS_DISPLAY = (1<<1),

    
    CAIRO_WIN32_SURFACE_CAN_BITBLT = (1<<2),

    
    CAIRO_WIN32_SURFACE_CAN_ALPHABLEND = (1<<3),

    
    CAIRO_WIN32_SURFACE_CAN_STRETCHBLT = (1<<4),

    
    CAIRO_WIN32_SURFACE_CAN_STRETCHDIB = (1<<5)
};

cairo_status_t
_cairo_win32_print_gdi_error (const char *context);

cairo_bool_t
_cairo_surface_is_win32 (cairo_surface_t *surface);

void
_cairo_win32_initialize (void);

#endif 
