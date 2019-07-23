




































#ifndef CAIRO_OS2_PRIVATE_H
#define CAIRO_OS2_PRIVATE_H

#define INCL_DOS
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPI
#ifdef __WATCOMC__
# include <os2.h>
#else
# include <os2emx.h>
#endif

#include <cairo-os2.h>
#include <cairoint.h>

typedef struct _cairo_os2_surface
{
    cairo_surface_t        base;

    
    HMTX                   hmtx_use_private_fields;
    
    HPS                    hps_client_window;
    HWND                   hwnd_client_window;
    BITMAPINFO2            bitmap_info;
    unsigned char         *pixels;
    cairo_image_surface_t *image_surface;
    int                    pixel_array_lend_count;
    HEV                    hev_pixel_array_came_back;

    RECTL                  rcl_dirty_area;
    cairo_bool_t           dirty_area_present;

    
    cairo_bool_t           blit_as_changes;
} cairo_os2_surface_t;

#endif 
