



























#ifndef HB_FACE_PRIVATE_HH
#define HB_FACE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-font.h"
#include "hb-object-private.hh"
#include "hb-shaper-private.hh"
#include "hb-shape-plan-private.hh"






struct hb_face_t {
  hb_object_header_t header;
  ASSERT_POD ();

  hb_bool_t immutable;

  hb_reference_table_func_t  reference_table_func;
  void                      *user_data;
  hb_destroy_func_t          destroy;

  unsigned int index;
  mutable unsigned int upem;
  mutable unsigned int num_glyphs;

  struct hb_shaper_data_t shaper_data;

  struct plan_node_t {
    hb_shape_plan_t *shape_plan;
    plan_node_t *next;
  } *shape_plans;


  inline hb_blob_t *reference_table (hb_tag_t tag) const
  {
    hb_blob_t *blob;

    if (unlikely (!this || !reference_table_func))
      return hb_blob_get_empty ();

    blob = reference_table_func (const_cast<hb_face_t *> (this), tag, user_data);
    if (unlikely (!blob))
      return hb_blob_get_empty ();

    return blob;
  }

  inline HB_PURE_FUNC unsigned int get_upem (void) const
  {
    if (unlikely (!upem))
      load_upem ();
    return upem;
  }

  inline unsigned int get_num_glyphs (void) const
  {
    if (unlikely (num_glyphs == (unsigned int) -1))
      load_num_glyphs ();
    return num_glyphs;
  }

  private:
  HB_INTERNAL void load_upem (void) const;
  HB_INTERNAL void load_num_glyphs (void) const;
};

extern HB_INTERNAL const hb_face_t _hb_face_nil;

#define HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS
#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_PROTOTYPE(shaper, face);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
#undef HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS


#endif 
