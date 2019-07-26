

























#ifndef HB_SHAPER_PRIVATE_HH
#define HB_SHAPER_PRIVATE_HH

#include "hb-private.hh"

typedef hb_bool_t hb_shape_func_t (hb_shape_plan_t    *shape_plan,
				   hb_font_t          *font,
				   hb_buffer_t        *buffer,
				   const hb_feature_t *features,
				   unsigned int        num_features);

#define HB_SHAPER_IMPLEMENT(name) \
	extern "C" HB_INTERNAL hb_shape_func_t _hb_##name##_shape;
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

struct hb_shaper_pair_t {
  char name[16];
  hb_shape_func_t *func;
};

HB_INTERNAL const hb_shaper_pair_t *
_hb_shapers_get (void);



struct hb_shaper_data_t {
#define HB_SHAPER_IMPLEMENT(shaper) void *shaper;
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
};

#define HB_SHAPERS_COUNT (sizeof (hb_shaper_data_t) / sizeof (void *))


#define HB_SHAPER_DATA_SUCCEEDED ((void *) +1)


#define HB_SHAPER_DATA_INVALID ((void *) -1)
#define HB_SHAPER_DATA_IS_INVALID(data) ((void *) (data) == HB_SHAPER_DATA_INVALID)

#define HB_SHAPER_DATA_TYPE(shaper, object)		struct hb_##shaper##_shaper_##object##_data_t
#define HB_SHAPER_DATA_INSTANCE(shaper, object, instance)	(* (HB_SHAPER_DATA_TYPE(shaper, object) **) &(instance)->shaper_data.shaper)
#define HB_SHAPER_DATA(shaper, object)			HB_SHAPER_DATA_INSTANCE (shaper, object, object)
#define HB_SHAPER_DATA_CREATE_FUNC(shaper, object)	_hb_##shaper##_shaper_##object##_data_create
#define HB_SHAPER_DATA_DESTROY_FUNC(shaper, object)	_hb_##shaper##_shaper_##object##_data_destroy

#define HB_SHAPER_DATA_PROTOTYPE(shaper, object) \
	HB_SHAPER_DATA_TYPE (shaper, object); /* Type forward declaration. */ \
	extern "C" HB_INTERNAL HB_SHAPER_DATA_TYPE (shaper, object) * \
	HB_SHAPER_DATA_CREATE_FUNC (shaper, object) (hb_##object##_t *object HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS); \
	extern "C" HB_INTERNAL void \
	HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (HB_SHAPER_DATA_TYPE (shaper, object) *data)

#define HB_SHAPER_DATA_DESTROY(shaper, object) \
	if (object->shaper_data.shaper && \
	    object->shaper_data.shaper != HB_SHAPER_DATA_INVALID && \
	    object->shaper_data.shaper != HB_SHAPER_DATA_SUCCEEDED) \
	  HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (HB_SHAPER_DATA (shaper, object));

#define HB_SHAPER_DATA_ENSURE_DECLARE(shaper, object) \
static inline bool \
hb_##shaper##_shaper_##object##_data_ensure (hb_##object##_t *object) \
{\
  retry: \
  HB_SHAPER_DATA_TYPE (shaper, object) *data = (HB_SHAPER_DATA_TYPE (shaper, object) *) hb_atomic_ptr_get (&HB_SHAPER_DATA (shaper, object)); \
  if (unlikely (!data)) { \
    data = HB_SHAPER_DATA_CREATE_FUNC (shaper, object) (object); \
    if (unlikely (!data)) \
      data = (HB_SHAPER_DATA_TYPE (shaper, object) *) HB_SHAPER_DATA_INVALID; \
    if (!hb_atomic_ptr_cmpexch (&HB_SHAPER_DATA (shaper, object), NULL, data)) { \
      if (data && \
	  data != HB_SHAPER_DATA_INVALID && \
	  data != HB_SHAPER_DATA_SUCCEEDED) \
	HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (data); \
      goto retry; \
    } \
  } \
  return data != NULL && !HB_SHAPER_DATA_IS_INVALID (data); \
}


#endif 
