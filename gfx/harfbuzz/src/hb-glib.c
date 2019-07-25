

























#include "hb-private.h"

#include "hb-glib.h"

#include "hb-unicode-private.h"

#include <glib.h>

HB_BEGIN_DECLS


static hb_codepoint_t hb_glib_get_mirroring (hb_codepoint_t unicode) { g_unichar_get_mirror_char (unicode, &unicode); return unicode; }
static hb_category_t hb_glib_get_general_category (hb_codepoint_t unicode) { return g_unichar_type (unicode); }
static hb_script_t hb_glib_get_script (hb_codepoint_t unicode) { return g_unichar_get_script (unicode); }
static unsigned int hb_glib_get_combining_class (hb_codepoint_t unicode) { return g_unichar_combining_class (unicode); }
static unsigned int hb_glib_get_eastasian_width (hb_codepoint_t unicode) { return g_unichar_iswide (unicode); }


static hb_unicode_funcs_t glib_ufuncs = {
  HB_REFERENCE_COUNT_INVALID, 
  TRUE, 
  {
    hb_glib_get_general_category,
    hb_glib_get_combining_class,
    hb_glib_get_mirroring,
    hb_glib_get_script,
    hb_glib_get_eastasian_width
  }
};

hb_unicode_funcs_t *
hb_glib_get_unicode_funcs (void)
{
  return &glib_ufuncs;
}


HB_END_DECLS
