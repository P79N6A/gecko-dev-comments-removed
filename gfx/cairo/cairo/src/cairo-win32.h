



































#ifndef _CAIRO_WIN32_H_
#define _CAIRO_WIN32_H_

#include "cairo.h"

#if CAIRO_HAS_WIN32_SURFACE

#include <windows.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_win32_surface_create (HDC hdc);

cairo_public cairo_surface_t *
cairo_win32_printing_surface_create (HDC hdc);

cairo_public cairo_surface_t *
cairo_win32_surface_create_with_ddb (HDC hdc,
                                     cairo_format_t format,
                                     int width,
                                     int height);

cairo_public cairo_surface_t *
cairo_win32_surface_create_with_dib (cairo_format_t format,
                                     int width,
                                     int height);

cairo_public HDC
cairo_win32_surface_get_dc (cairo_surface_t *surface);

cairo_public cairo_surface_t *
cairo_win32_surface_get_image (cairo_surface_t *surface);

cairo_public cairo_status_t
cairo_win32_surface_set_can_convert_to_dib (cairo_surface_t *surface, cairo_bool_t can_convert);

cairo_public cairo_status_t
cairo_win32_surface_get_can_convert_to_dib (cairo_surface_t *surface, cairo_bool_t *can_convert);


#if CAIRO_HAS_WIN32_FONT





cairo_public cairo_font_face_t *
cairo_win32_font_face_create_for_logfontw (LOGFONTW *logfont);

cairo_public cairo_font_face_t *
cairo_win32_font_face_create_for_hfont (HFONT font);

cairo_public cairo_font_face_t *
cairo_win32_font_face_create_for_logfontw_hfont (LOGFONTW *logfont, HFONT font);

cairo_public cairo_status_t
cairo_win32_scaled_font_select_font (cairo_scaled_font_t *scaled_font,
				     HDC                  hdc);

cairo_public void
cairo_win32_scaled_font_done_font (cairo_scaled_font_t *scaled_font);

cairo_public double
cairo_win32_scaled_font_get_metrics_factor (cairo_scaled_font_t *scaled_font);

cairo_public void
cairo_win32_scaled_font_get_logical_to_device (cairo_scaled_font_t *scaled_font,
					       cairo_matrix_t *logical_to_device);

cairo_public void
cairo_win32_scaled_font_get_device_to_logical (cairo_scaled_font_t *scaled_font,
					       cairo_matrix_t *device_to_logical);

#endif 

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the win32 backend
#endif 

#endif 
