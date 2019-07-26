

























#include "hb-ot-shape-complex-private.hh"


static const hb_tag_t tibetan_features[] =
{
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
  HB_TAG_NONE
};

static void
collect_features_tibetan (hb_ot_shape_planner_t *plan)
{
  for (const hb_tag_t *script_features = tibetan_features; script_features && *script_features; script_features++)
    plan->map.add_global_bool_feature (*script_features);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_tibetan =
{
  "default",
  collect_features_tibetan,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_DEFAULT,
  true, 
};
