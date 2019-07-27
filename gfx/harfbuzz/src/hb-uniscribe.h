

























#ifndef HB_UNISCRIBE_H
#define HB_UNISCRIBE_H

#include "hb.h"

#include <windows.h>

HB_BEGIN_DECLS


LOGFONTW *
hb_uniscribe_font_get_logfontw (hb_font_t *font);

HFONT
hb_uniscribe_font_get_hfont (hb_font_t *font);


HB_END_DECLS

#endif 
