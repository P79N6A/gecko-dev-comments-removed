


































#ifndef CAIRO_WIN32_PRIVATE_H
#define CAIRO_WIN32_PRIVATE_H

#include "cairo-win32.h"
#include "cairoint.h"
#include "cairo-surface-clipper-private.h"

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

    cairo_rectangle_int_t extents;

    




    cairo_rectangle_int_t clip_rect;
    HRGN initial_clip_rgn;
    cairo_bool_t had_simple_clip;
    cairo_region_t *clip_region;

    
    cairo_surface_clipper_t clipper;

    
    uint32_t flags;

    
    cairo_paginated_mode_t paginated_mode;
    cairo_content_t content;
    cairo_bool_t path_empty;
    cairo_bool_t has_ctm;
    cairo_matrix_t ctm;
    cairo_bool_t has_gdi_ctm;
    cairo_matrix_t gdi_ctm;
    HBRUSH brush, old_brush;
    cairo_scaled_font_subsets_t *font_subsets;
} cairo_win32_surface_t;


enum {
    
    CAIRO_WIN32_SURFACE_FOR_PRINTING = (1<<0),

    
    CAIRO_WIN32_SURFACE_IS_DISPLAY = (1<<1),

    
    CAIRO_WIN32_SURFACE_CAN_BITBLT = (1<<2),

    
    CAIRO_WIN32_SURFACE_CAN_ALPHABLEND = (1<<3),

    
    CAIRO_WIN32_SURFACE_CAN_STRETCHBLT = (1<<4),

    
    CAIRO_WIN32_SURFACE_CAN_STRETCHDIB = (1<<5),

    
    CAIRO_WIN32_SURFACE_CAN_RECT_GRADIENT = (1<<6),

    
    CAIRO_WIN32_SURFACE_CAN_CHECK_JPEG = (1<<7),

    
    CAIRO_WIN32_SURFACE_CAN_CHECK_PNG = (1<<8),

    
    CAIRO_WIN32_SURFACE_CAN_CONVERT_TO_DIB = (1<<9),
};

cairo_status_t
_cairo_win32_print_gdi_error (const char *context);

cairo_bool_t
_cairo_surface_is_win32 (cairo_surface_t *surface);

cairo_bool_t
_cairo_surface_is_win32_printing (cairo_surface_t *surface);

cairo_status_t
_cairo_win32_surface_finish (void *abstract_surface);

cairo_bool_t
_cairo_win32_surface_get_extents (void		          *abstract_surface,
				  cairo_rectangle_int_t   *rectangle);

uint32_t
_cairo_win32_flags_for_dc (HDC dc);

cairo_status_t
_cairo_win32_surface_set_clip_region (void           *abstract_surface,
				      cairo_region_t *region);

cairo_int_status_t
_cairo_win32_surface_show_glyphs (void			*surface,
				  cairo_operator_t	 op,
				  const cairo_pattern_t	*source,
				  cairo_glyph_t		*glyphs,
				  int			 num_glyphs,
				  cairo_scaled_font_t	*scaled_font,
				  cairo_clip_t		*clip,
				  int			*remaining_glyphs);

cairo_surface_t *
_cairo_win32_surface_create_similar (void	    *abstract_src,
				     cairo_content_t content,
				     int	     width,
				     int	     height);

cairo_status_t
_cairo_win32_surface_clone_similar (void *abstract_surface,
				    cairo_surface_t *src,
				    cairo_content_t content,
				    int src_x,
				    int src_y,
				    int width,
				    int height,
				    int *clone_offset_x,
				    int *clone_offset_y,
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

cairo_int_status_t
_cairo_win32_save_initial_clip (HDC dc, cairo_win32_surface_t *surface);

cairo_int_status_t
_cairo_win32_restore_initial_clip (cairo_win32_surface_t *surface);

void
_cairo_win32_debug_dump_hrgn (HRGN rgn, char *header);

cairo_bool_t
_cairo_win32_scaled_font_is_type1 (cairo_scaled_font_t *scaled_font);

cairo_bool_t
_cairo_win32_scaled_font_is_bitmap (cairo_scaled_font_t *scaled_font);

BYTE
_cairo_win32_get_system_text_quality (void);

#ifdef WINCE


#define ETO_GLYPH_INDEX 0
#define ETO_PDY 0
#define HALFTONE COLORONCOLOR
#define GM_ADVANCED 2
#define MWT_IDENTITY 1

inline int SetGraphicsMode(HDC hdc, int iMode) {return 1;}
inline int GetGraphicsMode(HDC hdc)            {return 1;} 
inline void GdiFlush()                         {}
inline BOOL SetWorldTransform(HDC hdc, CONST XFORM *lpXform) { return FALSE; }
inline BOOL GetWorldTransform(HDC hdc, LPXFORM lpXform )     { return FALSE; }
inline BOOL ModifyWorldTransform(HDC hdc, CONST XFORM * lpxf, DWORD mode) { return 1; }

#endif

#ifdef CAIRO_HAS_DWRITE_FONT
CAIRO_BEGIN_DECLS

cairo_int_status_t
_cairo_dwrite_show_glyphs_on_surface(void			*surface,
				     cairo_operator_t		 op,
				     const cairo_pattern_t	*source,
				     cairo_glyph_t		*glyphs,
				     int			 num_glyphs,
				     cairo_scaled_font_t	*scaled_font,
				     cairo_clip_t               *clip);

cairo_int_status_t
_cairo_dwrite_scaled_font_create_win32_scaled_font(cairo_scaled_font_t *scaled_font,
                                                   cairo_scaled_font_t **new_font);

CAIRO_END_DECLS
#endif 
#endif 
