

























#ifndef HB_CORETEXT_H
#define HB_CORETEXT_H

#include "hb.h"

#include <ApplicationServices/ApplicationServices.h>

HB_BEGIN_DECLS


CTFontRef
hb_coretext_font_get_ct_font (hb_font_t *font);


HB_END_DECLS

#endif 
