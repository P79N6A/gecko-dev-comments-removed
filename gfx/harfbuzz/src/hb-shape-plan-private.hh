

























#ifndef HB_SHAPE_PLAN_PRIVATE_HH
#define HB_SHAPE_PLAN_PRIVATE_HH

#include "hb-private.hh"
#include "hb-shape-plan.h"
#include "hb-object-private.hh"
#include "hb-shaper-private.hh"


struct hb_shape_plan_t
{
  hb_object_header_t header;
  ASSERT_POD ();

  hb_bool_t default_shaper_list;
  hb_face_t *face;
  hb_segment_properties_t props;

  hb_shape_func_t *shaper_func;
  const char *shaper_name;

  struct hb_shaper_data_t shaper_data;
};

#define HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS \
	, const hb_feature_t            *user_features \
	, unsigned int                   num_user_features
#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_PROTOTYPE(shaper, shape_plan);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
#undef HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS


#endif 
