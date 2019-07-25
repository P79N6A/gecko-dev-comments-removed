

























#ifndef HB_UNISCRIBE_H
#define HB_UNISCRIBE_H

#include "hb-common.h"
#include "hb-shape.h"

#define _WIN32_WINNT 0x0500
#include <windows.h>

HB_BEGIN_DECLS


hb_bool_t
hb_uniscribe_shape (hb_font_t          *font,
		    hb_buffer_t        *buffer,
		    const hb_feature_t *features,
		    unsigned int        num_features,
		    const char * const *shaper_options);

LOGFONTW *
hb_uniscribe_font_get_logfontw (hb_font_t *font);

HFONT
hb_uniscribe_font_get_hfont (hb_font_t *font);


HB_END_DECLS

#endif 
