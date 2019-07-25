

























#ifndef HB_FT_H
#define HB_FT_H

#include "hb.h"

#include "hb-font.h"

#include <ft2build.h>
#include FT_FREETYPE_H

HB_BEGIN_DECLS


hb_font_funcs_t *
hb_ft_get_font_funcs (void);

hb_face_t *
hb_ft_face_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy);


hb_face_t *
hb_ft_face_create_cached (FT_Face ft_face);

hb_font_t *
hb_ft_font_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy);


HB_END_DECLS

#endif 
