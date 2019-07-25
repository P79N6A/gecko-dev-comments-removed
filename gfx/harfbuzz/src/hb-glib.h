



























#ifndef HB_GLIB_H
#define HB_GLIB_H

#include "hb.h"
#include <glib.h>

HB_BEGIN_DECLS


hb_script_t
hb_glib_script_to_script (GUnicodeScript script);

GUnicodeScript
hb_glib_script_from_script (hb_script_t script);


hb_unicode_funcs_t *
hb_glib_get_unicode_funcs (void);


HB_END_DECLS

#endif 
