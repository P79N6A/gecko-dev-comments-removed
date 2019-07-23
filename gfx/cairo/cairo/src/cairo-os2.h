




































#ifndef _CAIRO_OS2_H_
#define _CAIRO_OS2_H_

#include <cairo.h>

CAIRO_BEGIN_DECLS











cairo_public void
cairo_os2_init (void);









cairo_public void
cairo_os2_fini (void);

#if CAIRO_HAS_OS2_SURFACE
















cairo_public cairo_surface_t *
cairo_os2_surface_create (HPS hps_client_window,
                          int width,
                          int height);



















cairo_public void
cairo_os2_surface_set_hwnd (cairo_surface_t *surface,
                            HWND             hwnd_client_window);






















cairo_public int
cairo_os2_surface_set_size (cairo_surface_t *surface,
                            int              new_width,
                            int              new_height,
                            int              timeout);
























cairo_public void
cairo_os2_surface_refresh_window (cairo_surface_t *surface,
                                  HPS              hps_begin_paint,
                                  PRECTL           prcl_begin_paint_rect);


















cairo_public void
cairo_os2_surface_set_manual_window_refresh (cairo_surface_t *surface,
                                             cairo_bool_t     manual_refresh);






cairo_public cairo_bool_t
cairo_os2_surface_get_manual_window_refresh (cairo_surface_t *surface);

#endif 

CAIRO_END_DECLS

#endif 
