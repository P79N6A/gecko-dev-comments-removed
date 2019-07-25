

























#ifndef HB_FT_H
#define HB_FT_H

#include "hb.h"

#include "hb-font.h"

#include <ft2build.h>
#include FT_FREETYPE_H

HB_BEGIN_DECLS



hb_face_t *
hb_ft_face_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy);

hb_face_t *
hb_ft_face_create_cached (FT_Face ft_face);

hb_font_t *
hb_ft_font_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy);




void
hb_ft_font_set_funcs (hb_font_t *font);

FT_Face
hb_ft_font_get_face (hb_font_t *font);


HB_END_DECLS

#endif 
