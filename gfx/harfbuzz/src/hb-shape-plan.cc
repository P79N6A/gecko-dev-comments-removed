

























#include "hb-shape-plan-private.hh"
#include "hb-shaper-private.hh"
#include "hb-font-private.hh"
#include "hb-buffer-private.hh"

#define HB_SHAPER_IMPLEMENT(shaper) \
	HB_SHAPER_DATA_ENSURE_DECLARE(shaper, face) \
	HB_SHAPER_DATA_ENSURE_DECLARE(shaper, font)
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT


static void
hb_shape_plan_plan (hb_shape_plan_t    *shape_plan,
		    const hb_feature_t *user_features,
		    unsigned int        num_user_features,
		    const char * const *shaper_list)
{
  const hb_shaper_pair_t *shapers = _hb_shapers_get ();

#define HB_SHAPER_PLAN(shaper) \
	HB_STMT_START { \
	  if (hb_##shaper##_shaper_face_data_ensure (shape_plan->face_unsafe)) { \
	    HB_SHAPER_DATA (shaper, shape_plan) = \
	      HB_SHAPER_DATA_CREATE_FUNC (shaper, shape_plan) (shape_plan, user_features, num_user_features); \
	    shape_plan->shaper_func = _hb_##shaper##_shape; \
	    shape_plan->shaper_name = #shaper; \
	    return; \
	  } \
	} HB_STMT_END

  if (likely (!shaper_list)) {
    for (unsigned int i = 0; i < HB_SHAPERS_COUNT; i++)
      if (0)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (shapers[i].func == _hb_##shaper##_shape) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  } else {
    for (; *shaper_list; shaper_list++)
      if (0)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (0 == strcmp (*shaper_list, #shaper)) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  }

#undef HB_SHAPER_PLAN
}




















hb_shape_plan_t *
hb_shape_plan_create (hb_face_t                     *face,
		      const hb_segment_properties_t *props,
		      const hb_feature_t            *user_features,
		      unsigned int                   num_user_features,
		      const char * const            *shaper_list)
{
  hb_shape_plan_t *shape_plan;
  hb_feature_t *features = NULL;

  if (unlikely (!face))
    face = hb_face_get_empty ();
  if (unlikely (!props || hb_object_is_inert (face)))
    return hb_shape_plan_get_empty ();
  if (num_user_features && !(features = (hb_feature_t *) malloc (num_user_features * sizeof (hb_feature_t))))
    return hb_shape_plan_get_empty ();
  if (!(shape_plan = hb_object_create<hb_shape_plan_t> ())) {
    free (features);
    return hb_shape_plan_get_empty ();
  }

  assert (props->direction != HB_DIRECTION_INVALID);

  hb_face_make_immutable (face);
  shape_plan->default_shaper_list = shaper_list == NULL;
  shape_plan->face_unsafe = face;
  shape_plan->props = *props;
  shape_plan->num_user_features = num_user_features;
  shape_plan->user_features = features;
  if (num_user_features)
    memcpy (features, user_features, num_user_features * sizeof (hb_feature_t));

  hb_shape_plan_plan (shape_plan, user_features, num_user_features, shaper_list);

  return shape_plan;
}










hb_shape_plan_t *
hb_shape_plan_get_empty (void)
{
  static const hb_shape_plan_t _hb_shape_plan_nil = {
    HB_OBJECT_HEADER_STATIC,

    true, 
    NULL, 
    HB_SEGMENT_PROPERTIES_DEFAULT, 

    NULL, 
    NULL, 

    NULL, 
    0,    

    {
#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_INVALID,
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
    }
  };

  return const_cast<hb_shape_plan_t *> (&_hb_shape_plan_nil);
}











hb_shape_plan_t *
hb_shape_plan_reference (hb_shape_plan_t *shape_plan)
{
  return hb_object_reference (shape_plan);
}









void
hb_shape_plan_destroy (hb_shape_plan_t *shape_plan)
{
  if (!hb_object_destroy (shape_plan)) return;

#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_DESTROY(shaper, shape_plan);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

  free (shape_plan->user_features);

  free (shape_plan);
}















hb_bool_t
hb_shape_plan_set_user_data (hb_shape_plan_t    *shape_plan,
			     hb_user_data_key_t *key,
			     void *              data,
			     hb_destroy_func_t   destroy,
			     hb_bool_t           replace)
{
  return hb_object_set_user_data (shape_plan, key, data, destroy, replace);
}












void *
hb_shape_plan_get_user_data (hb_shape_plan_t    *shape_plan,
			     hb_user_data_key_t *key)
{
  return hb_object_get_user_data (shape_plan, key);
}
















hb_bool_t
hb_shape_plan_execute (hb_shape_plan_t    *shape_plan,
		       hb_font_t          *font,
		       hb_buffer_t        *buffer,
		       const hb_feature_t *features,
		       unsigned int        num_features)
{
  if (unlikely (hb_object_is_inert (shape_plan) ||
		hb_object_is_inert (font) ||
		hb_object_is_inert (buffer)))
    return false;

  assert (shape_plan->face_unsafe == font->face);
  assert (hb_segment_properties_equal (&shape_plan->props, &buffer->props));

#define HB_SHAPER_EXECUTE(shaper) \
	HB_STMT_START { \
	  return HB_SHAPER_DATA (shaper, shape_plan) && \
		 hb_##shaper##_shaper_font_data_ensure (font) && \
		 _hb_##shaper##_shape (shape_plan, font, buffer, features, num_features); \
	} HB_STMT_END

  if (0)
    ;
#define HB_SHAPER_IMPLEMENT(shaper) \
  else if (shape_plan->shaper_func == _hb_##shaper##_shape) \
    HB_SHAPER_EXECUTE (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

#undef HB_SHAPER_EXECUTE

  return false;
}






#if 0
static unsigned int
hb_shape_plan_hash (const hb_shape_plan_t *shape_plan)
{
  return hb_segment_properties_hash (&shape_plan->props) +
	 shape_plan->default_shaper_list ? 0 : (intptr_t) shape_plan->shaper_func;
}
#endif






struct hb_shape_plan_proposal_t
{
  const hb_segment_properties_t  props;
  const char * const            *shaper_list;
  const hb_feature_t            *user_features;
  unsigned int                   num_user_features;
  hb_shape_func_t               *shaper_func;
};

static inline hb_bool_t
hb_shape_plan_user_features_match (const hb_shape_plan_t          *shape_plan,
				   const hb_shape_plan_proposal_t *proposal)
{
  if (proposal->num_user_features != shape_plan->num_user_features) return false;
  for (unsigned int i = 0, n = proposal->num_user_features; i < n; i++)
    if (proposal->user_features[i].tag   != shape_plan->user_features[i].tag   ||
        proposal->user_features[i].value != shape_plan->user_features[i].value ||
        proposal->user_features[i].start != shape_plan->user_features[i].start ||
        proposal->user_features[i].end   != shape_plan->user_features[i].end) return false;
  return true;
}

static hb_bool_t
hb_shape_plan_matches (const hb_shape_plan_t          *shape_plan,
		       const hb_shape_plan_proposal_t *proposal)
{
  return hb_segment_properties_equal (&shape_plan->props, &proposal->props) &&
	 hb_shape_plan_user_features_match (shape_plan, proposal) &&
	 ((shape_plan->default_shaper_list && proposal->shaper_list == NULL) ||
	  (shape_plan->shaper_func == proposal->shaper_func));
}

static inline hb_bool_t
hb_non_global_user_features_present (const hb_feature_t *user_features,
				     unsigned int        num_user_features)
{
  while (num_user_features)
    if (user_features->start != 0 || user_features->end != (unsigned int) -1)
      return true;
    else
      num_user_features--, user_features++;
  return false;
}















hb_shape_plan_t *
hb_shape_plan_create_cached (hb_face_t                     *face,
			     const hb_segment_properties_t *props,
			     const hb_feature_t            *user_features,
			     unsigned int                   num_user_features,
			     const char * const            *shaper_list)
{
  hb_shape_plan_proposal_t proposal = {
    *props,
    shaper_list,
    user_features,
    num_user_features,
    NULL
  };

  if (shaper_list) {
    
#define HB_SHAPER_PLAN(shaper) \
	  HB_STMT_START { \
	    if (hb_##shaper##_shaper_face_data_ensure (face)) \
	      proposal.shaper_func = _hb_##shaper##_shape; \
	  } HB_STMT_END

    for (const char * const *shaper_item = shaper_list; *shaper_item; shaper_item++)
      if (0)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (0 == strcmp (*shaper_item, #shaper)) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

#undef HB_SHAPER_PLAN

    if (unlikely (!proposal.shaper_list))
      return hb_shape_plan_get_empty ();
  }


retry:
  hb_face_t::plan_node_t *cached_plan_nodes = (hb_face_t::plan_node_t *) hb_atomic_ptr_get (&face->shape_plans);
  for (hb_face_t::plan_node_t *node = cached_plan_nodes; node; node = node->next)
    if (hb_shape_plan_matches (node->shape_plan, &proposal))
      return hb_shape_plan_reference (node->shape_plan);

  

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, props, user_features, num_user_features, shaper_list);

  

  if (hb_non_global_user_features_present (user_features, num_user_features))
    return shape_plan;

  hb_face_t::plan_node_t *node = (hb_face_t::plan_node_t *) calloc (1, sizeof (hb_face_t::plan_node_t));
  if (unlikely (!node))
    return shape_plan;

  node->shape_plan = shape_plan;
  node->next = cached_plan_nodes;

  if (!hb_atomic_ptr_cmpexch (&face->shape_plans, cached_plan_nodes, node)) {
    hb_shape_plan_destroy (shape_plan);
    free (node);
    goto retry;
  }

  return hb_shape_plan_reference (shape_plan);
}











const char *
hb_shape_plan_get_shaper (hb_shape_plan_t *shape_plan)
{
  return shape_plan->shaper_name;
}
