

























#ifndef HB_FONT_PRIVATE_H
#define HB_FONT_PRIVATE_H

#include "hb-private.h"

#include "hb-font.h"

#include "hb-ot-head-private.hh"

HB_BEGIN_DECLS





struct _hb_font_funcs_t {
  hb_reference_count_t ref_count;

  hb_bool_t immutable;

  struct {
    hb_font_get_glyph_func_t		get_glyph;
    hb_font_get_contour_point_func_t	get_contour_point;
    hb_font_get_glyph_metrics_func_t	get_glyph_metrics;
    hb_font_get_kerning_func_t		get_kerning;
  } v;
};

extern HB_INTERNAL hb_font_funcs_t _hb_font_funcs_nil;






struct _hb_face_t {
  hb_reference_count_t ref_count;

  hb_get_table_func_t  get_table;
  hb_destroy_func_t    destroy;
  void                *user_data;

  unsigned int         units_per_em;

  struct hb_ot_layout_t *ot_layout;
};






struct _hb_font_t {
  hb_reference_count_t ref_count;

  unsigned int x_scale;
  unsigned int y_scale;

  unsigned int x_ppem;
  unsigned int y_ppem;

  hb_font_funcs_t   *klass;
  hb_destroy_func_t  destroy;
  void              *user_data;
};


HB_END_DECLS

#endif 
