



































#ifndef _CAIRO_WIN32_H_
#define _CAIRO_WIN32_H_

#include "cairo.h"

#if CAIRO_HAS_WIN32_SURFACE

#include <windows.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_win32_surface_create (HDC hdc);

cairo_public cairo_surface_t *
cairo_win32_surface_create_with_alpha (HDC hdc);

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

cairo_public HDC
cairo_win32_get_dc_with_clip (cairo_t *cr);

cairo_public cairo_surface_t *
cairo_win32_surface_get_image (cairo_surface_t *surface);

cairo_public cairo_status_t
cairo_win32_surface_set_can_convert_to_dib (cairo_surface_t *surface, cairo_bool_t can_convert);

cairo_public cairo_status_t
cairo_win32_surface_get_can_convert_to_dib (cairo_surface_t *surface, cairo_bool_t *can_convert);

BYTE cairo_win32_get_system_text_quality (void);

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

#if CAIRO_HAS_DWRITE_FONT




cairo_public cairo_font_face_t *
cairo_dwrite_font_face_create_for_dwrite_fontface(void *dwrite_font, void *dwrite_font_face);

void
cairo_dwrite_scaled_font_allow_manual_show_glyphs(cairo_scaled_font_t *dwrite_scaled_font, cairo_bool_t allowed);

void
cairo_dwrite_scaled_font_set_force_GDI_classic(cairo_scaled_font_t *dwrite_scaled_font, cairo_bool_t force);

cairo_bool_t
cairo_dwrite_scaled_font_get_force_GDI_classic(cairo_scaled_font_t *dwrite_scaled_font);

void
cairo_dwrite_set_cleartype_params(FLOAT gamma, FLOAT contrast, FLOAT level, int geometry, int mode);

int
cairo_dwrite_get_cleartype_rendering_mode();

#endif 

#if CAIRO_HAS_D2D_SURFACE

struct _cairo_device
{
    int type;
    int refcount;
};
typedef struct _cairo_device cairo_device_t;






cairo_device_t *
cairo_d2d_create_device();

cairo_device_t *
cairo_d2d_create_device_from_d3d10device(struct ID3D10Device1 *device);






int
cairo_release_device(cairo_device_t *device);






int
cairo_addref_device(cairo_device_t *device);








void
cairo_d2d_finish_device(cairo_device_t *device);




struct ID3D10Device1*
cairo_d2d_device_get_device(cairo_device_t *device);









cairo_public cairo_surface_t *
cairo_d2d_surface_create_for_hwnd(cairo_device_t *device, HWND wnd, cairo_content_t content);










cairo_public cairo_surface_t *
cairo_d2d_surface_create(cairo_device_t *device,
			 cairo_format_t format,
                         int width,
                         int height);












cairo_public cairo_surface_t *
cairo_d2d_surface_create_for_handle(cairo_device_t *device, HANDLE handle, cairo_content_t content);












cairo_public cairo_surface_t *
cairo_d2d_surface_create_for_texture(cairo_device_t *device,
				     struct ID3D10Texture2D *texture,
				     cairo_content_t content);








void cairo_d2d_present_backbuffer(cairo_surface_t *surface);












void cairo_d2d_scroll(cairo_surface_t *surface, int x, int y, cairo_rectangle_t *clip);











HDC cairo_d2d_get_dc(cairo_surface_t *surface, cairo_bool_t retain_contents);







void cairo_d2d_release_dc(cairo_surface_t *surcace, const cairo_rectangle_int_t *updated_rect);





int cairo_d2d_get_image_surface_cache_usage();






int cairo_d2d_get_surface_vram_usage(cairo_device_t *device);
#endif

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the win32 backend
#endif 

#endif 
