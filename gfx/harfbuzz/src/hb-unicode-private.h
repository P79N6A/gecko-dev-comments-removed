

























#ifndef HB_UNICODE_PRIVATE_H
#define HB_UNICODE_PRIVATE_H

#include "hb-private.h"

#include "hb-unicode.h"

HB_BEGIN_DECLS






struct _hb_unicode_funcs_t {
  hb_reference_count_t ref_count;

  hb_bool_t immutable;

  struct {
    hb_unicode_get_general_category_func_t	get_general_category;
    hb_unicode_get_combining_class_func_t	get_combining_class;
    hb_unicode_get_mirroring_func_t		get_mirroring;
    hb_unicode_get_script_func_t		get_script;
    hb_unicode_get_eastasian_width_func_t	get_eastasian_width;
  } v;
};

extern HB_INTERNAL hb_unicode_funcs_t _hb_unicode_funcs_nil;


HB_INTERNAL hb_direction_t
_hb_script_get_horizontal_direction (hb_script_t script);


HB_END_DECLS

#endif 
