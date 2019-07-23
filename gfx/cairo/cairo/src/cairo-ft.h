



































#ifndef CAIRO_FT_H
#define CAIRO_FT_H

#include <cairo.h>

#if CAIRO_HAS_FT_FONT



#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

CAIRO_BEGIN_DECLS

cairo_public cairo_font_face_t *
cairo_ft_font_face_create_for_pattern (FcPattern *pattern);

cairo_public void
cairo_ft_font_options_substitute (const cairo_font_options_t *options,
				  FcPattern                  *pattern);

cairo_public cairo_font_face_t *
cairo_ft_font_face_create_for_ft_face (FT_Face         face,
				       int             load_flags);

cairo_public FT_Face
cairo_ft_scaled_font_lock_face (cairo_scaled_font_t *scaled_font);

cairo_public void
cairo_ft_scaled_font_unlock_face (cairo_scaled_font_t *scaled_font);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the freetype font backend
#endif 

#endif 
