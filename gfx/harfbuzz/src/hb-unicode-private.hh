





























#ifndef HB_UNICODE_PRIVATE_HH
#define HB_UNICODE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-unicode.h"
#include "hb-object-private.hh"







#define HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS \
  HB_UNICODE_FUNC_IMPLEMENT (combining_class) \
  HB_UNICODE_FUNC_IMPLEMENT (eastasian_width) \
  HB_UNICODE_FUNC_IMPLEMENT (general_category) \
  HB_UNICODE_FUNC_IMPLEMENT (mirroring) \
  HB_UNICODE_FUNC_IMPLEMENT (script) \
  HB_UNICODE_FUNC_IMPLEMENT (compose) \
  HB_UNICODE_FUNC_IMPLEMENT (decompose) \
  /* ^--- Add new callbacks here */


#define HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS_SIMPLE \
  HB_UNICODE_FUNC_IMPLEMENT (unsigned int, combining_class) \
  HB_UNICODE_FUNC_IMPLEMENT (unsigned int, eastasian_width) \
  HB_UNICODE_FUNC_IMPLEMENT (hb_unicode_general_category_t, general_category) \
  HB_UNICODE_FUNC_IMPLEMENT (hb_codepoint_t, mirroring) \
  HB_UNICODE_FUNC_IMPLEMENT (hb_script_t, script) \
  /* ^--- Add new simple callbacks here */

struct _hb_unicode_funcs_t {
  hb_object_header_t header;

  hb_unicode_funcs_t *parent;

  bool immutable;

  

  struct {
#define HB_UNICODE_FUNC_IMPLEMENT(name) hb_unicode_##name##_func_t name;
    HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
  } func;

  struct {
#define HB_UNICODE_FUNC_IMPLEMENT(name) void *name;
    HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
  } user_data;

  struct {
#define HB_UNICODE_FUNC_IMPLEMENT(name) hb_destroy_func_t name;
    HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
  } destroy;
};


#ifdef HAVE_GLIB
extern HB_INTERNAL hb_unicode_funcs_t _hb_glib_unicode_funcs;
#define _hb_unicode_funcs_default _hb_glib_unicode_funcs
#elif defined(HAVE_ICU)
extern HB_INTERNAL hb_unicode_funcs_t _hb_icu_unicode_funcs;
#define _hb_unicode_funcs_default _hb_icu_unicode_funcs
#else
extern HB_INTERNAL hb_unicode_funcs_t _hb_unicode_funcs_nil;
#define _hb_unicode_funcs_default _hb_unicode_funcs_nil
#endif




#endif 
