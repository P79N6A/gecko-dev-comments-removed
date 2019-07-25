

























#ifndef HB_SHAPE_PLAN_H
#define HB_SHAPE_PLAN_H



#include "hb-private.hh"

#include "hb-buffer-private.hh"


typedef struct hb_shape_plan_t hb_shape_plan;





HB_INTERNAL hb_shape_plan_t *
hb_shape_plan_create (hb_face_t                     *face,
		      const hb_segment_properties_t *props,
		      const hb_feature_t            *user_features,
		      unsigned int                   num_user_features,
		      const char * const            *shaper_list);

HB_INTERNAL hb_shape_plan_t *
hb_shape_plan_create_cached (hb_face_t                     *face,
			     const hb_segment_properties_t *props,
			     const hb_feature_t            *user_features,
			     unsigned int                   num_user_features,
			     const char * const            *shaper_list);

HB_INTERNAL hb_shape_plan_t *
hb_shape_plan_get_empty (void);

HB_INTERNAL hb_shape_plan_t *
hb_shape_plan_reference (hb_shape_plan_t *shape_plan);

HB_INTERNAL void
hb_shape_plan_destroy (hb_shape_plan_t *shape_plan);


HB_INTERNAL hb_bool_t
hb_shape_plan_execute (hb_shape_plan      *shape_plan,
		       hb_font_t          *font,
		       hb_buffer_t        *buffer,
		       const hb_feature_t *features,
		       unsigned int        num_features);


#endif 
