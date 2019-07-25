



























#ifndef HB_FONT_PRIVATE_HH
#define HB_FONT_PRIVATE_HH

#include "hb-private.hh"

#include "hb-font.h"
#include "hb-object-private.hh"







#define HB_FONT_FUNCS_IMPLEMENT_CALLBACKS \
  HB_FONT_FUNC_IMPLEMENT (glyph) \
  HB_FONT_FUNC_IMPLEMENT (glyph_h_advance) \
  HB_FONT_FUNC_IMPLEMENT (glyph_v_advance) \
  HB_FONT_FUNC_IMPLEMENT (glyph_h_origin) \
  HB_FONT_FUNC_IMPLEMENT (glyph_v_origin) \
  HB_FONT_FUNC_IMPLEMENT (glyph_h_kerning) \
  HB_FONT_FUNC_IMPLEMENT (glyph_v_kerning) \
  HB_FONT_FUNC_IMPLEMENT (glyph_extents) \
  HB_FONT_FUNC_IMPLEMENT (glyph_contour_point) \
  /* ^--- Add new callbacks here */

struct _hb_font_funcs_t {
  hb_object_header_t header;

  hb_bool_t immutable;

  

  struct {
#define HB_FONT_FUNC_IMPLEMENT(name) hb_font_get_##name##_func_t name;
    HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
  } get;

  struct {
#define HB_FONT_FUNC_IMPLEMENT(name) void *name;
    HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
  } user_data;

  struct {
#define HB_FONT_FUNC_IMPLEMENT(name) hb_destroy_func_t name;
    HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
  } destroy;
};






struct _hb_face_t {
  hb_object_header_t header;

  hb_bool_t immutable;

  hb_reference_table_func_t  reference_table;
  void                      *user_data;
  hb_destroy_func_t          destroy;

  struct hb_ot_layout_t *ot_layout;

  unsigned int index;
  unsigned int upem;
};






struct _hb_font_t {
  hb_object_header_t header;

  hb_bool_t immutable;

  hb_font_t *parent;
  hb_face_t *face;

  int x_scale;
  int y_scale;

  unsigned int x_ppem;
  unsigned int y_ppem;

  hb_font_funcs_t   *klass;
  void              *user_data;
  hb_destroy_func_t  destroy;


  
  inline hb_position_t em_scale_x (int16_t v) { return em_scale (v, this->x_scale); }
  inline hb_position_t em_scale_y (int16_t v) { return em_scale (v, this->y_scale); }

  
  inline hb_position_t parent_scale_x_distance (hb_position_t v) {
    if (unlikely (parent && parent->x_scale != x_scale))
      return v * (int64_t) this->x_scale / this->parent->x_scale;
    return v;
  }
  inline hb_position_t parent_scale_y_distance (hb_position_t v) {
    if (unlikely (parent && parent->y_scale != y_scale))
      return v * (int64_t) this->y_scale / this->parent->y_scale;
    return v;
  }
  inline hb_position_t parent_scale_x_position (hb_position_t v) {
    return parent_scale_x_distance (v);
  }
  inline hb_position_t parent_scale_y_position (hb_position_t v) {
    return parent_scale_y_distance (v);
  }

  inline void parent_scale_distance (hb_position_t *x, hb_position_t *y) {
    *x = parent_scale_x_distance (*x);
    *y = parent_scale_y_distance (*y);
  }
  inline void parent_scale_position (hb_position_t *x, hb_position_t *y) {
    *x = parent_scale_x_position (*x);
    *y = parent_scale_y_position (*y);
  }


  private:
  inline hb_position_t em_scale (int16_t v, int scale) { return v * (int64_t) scale / hb_face_get_upem (this->face); }
};



#endif 
