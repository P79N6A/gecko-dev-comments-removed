


































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

#define WIN32_FONT_LOGICAL_SCALE 32

typedef struct _cairo_win32_surface {
    cairo_surface_t base;

    cairo_format_t format;

    HDC dc;

    

    HBITMAP bitmap;
    cairo_bool_t is_dib;

    






    HBITMAP saved_dc_bitmap;

    cairo_surface_t *image;

    cairo_rectangle_int_t clip_rect;

    HRGN saved_clip;

    cairo_rectangle_int_t extents;

    
    uint32_t flags;

    
    cairo_paginated_mode_t paginated_mode;
    cairo_content_t content;
    cairo_bool_t path_empty;
    cairo_bool_t has_ctm;
    cairo_matrix_t ctm;
    int clip_saved_dc;
    HBRUSH brush, old_brush;
} cairo_win32_surface_t;


enum {
    
    CAIRO_WIN32_SURFACE_FOR_PRINTING = (1<<0),

    
    CAIRO_WIN32_SURFACE_IS_DISPLAY = (1<<1),

    
    CAIRO_WIN32_SURFACE_CAN_BITBLT = (1<<2),

    
    CAIRO_WIN32_SURFACE_CAN_ALPHABLEND = (1<<3),

    
    CAIRO_WIN32_SURFACE_CAN_STRETCHBLT = (1<<4),

    
    CAIRO_WIN32_SURFACE_CAN_STRETCHDIB = (1<<5),

    
    CAIRO_WIN32_SURFACE_CAN_RECT_GRADIENT = (1<<6),
};

cairo_status_t
_cairo_win32_print_gdi_error (const char *context);

cairo_bool_t
_cairo_surface_is_win32 (cairo_surface_t *surface);

cairo_bool_t
_cairo_surface_is_win32_printing (cairo_surface_t *surface);

cairo_status_t
_cairo_win32_surface_finish (void *abstract_surface);

cairo_int_status_t
_cairo_win32_surface_get_extents (void		          *abstract_surface,
				  cairo_rectangle_int16_t *rectangle);

uint32_t
_cairo_win32_flags_for_dc (HDC dc);

cairo_int_status_t
_cairo_win32_surface_show_glyphs (void			*surface,
				  cairo_operator_t	 op,
				  cairo_pattern_t	*source,
				  cairo_glyph_t		*glyphs,
				  int			 num_glyphs,
				  cairo_scaled_font_t	*scaled_font);

cairo_surface_t *
_cairo_win32_surface_create_similar (void	    *abstract_src,
				     cairo_content_t content,
				     int	     width,
				     int	     height);

cairo_status_t
_cairo_win32_surface_clone_similar (void *abstract_surface,
				    cairo_surface_t *src,
				    int src_x,
				    int src_y,
				    int width,
				    int height,
				    cairo_surface_t **clone_out);

static inline void
_cairo_matrix_to_win32_xform (const cairo_matrix_t *m,
                              XFORM *xform)
{
    xform->eM11 = (FLOAT) m->xx;
    xform->eM21 = (FLOAT) m->xy;
    xform->eM12 = (FLOAT) m->yx;
    xform->eM22 = (FLOAT) m->yy;
    xform->eDx = (FLOAT) m->x0;
    xform->eDy = (FLOAT) m->y0;
}

#endif 
