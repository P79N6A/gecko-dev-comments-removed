

























#include "hb-private.hh"

#include "hb-shaper-private.hh"
#include "hb-shape-plan-private.hh"
#include "hb-buffer-private.hh"
#include "hb-font-private.hh"


static const char **static_shaper_list;

static
void free_static_shaper_list (void)
{
  free (static_shaper_list);
}

const char **
hb_shape_list_shapers (void)
{
retry:
  const char **shaper_list = (const char **) hb_atomic_ptr_get (&static_shaper_list);

  if (unlikely (!shaper_list))
  {
    
    shaper_list = (const char **) calloc (1 + HB_SHAPERS_COUNT, sizeof (const char *));
    if (unlikely (!shaper_list)) {
      static const char *nil_shaper_list[] = {NULL};
      return nil_shaper_list;
    }

    const hb_shaper_pair_t *shapers = _hb_shapers_get ();
    unsigned int i;
    for (i = 0; i < HB_SHAPERS_COUNT; i++)
      shaper_list[i] = shapers[i].name;
    shaper_list[i] = NULL;

    if (!hb_atomic_ptr_cmpexch (&static_shaper_list, NULL, shaper_list)) {
      free (shaper_list);
      goto retry;
    }

#ifdef HAVE_ATEXIT
    atexit (free_static_shaper_list); 
#endif
  }

  return shaper_list;
}


hb_bool_t
hb_shape_full (hb_font_t          *font,
	       hb_buffer_t        *buffer,
	       const hb_feature_t *features,
	       unsigned int        num_features,
	       const char * const *shaper_list)
{
  if (unlikely (!buffer->len))
    return true;

  buffer->guess_properties ();

  hb_shape_plan_t *shape_plan = hb_shape_plan_create_cached (font->face, &buffer->props, features, num_features, shaper_list);
  hb_bool_t res = hb_shape_plan_execute (shape_plan, font, buffer, features, num_features);
  hb_shape_plan_destroy (shape_plan);
  return res;
}

void
hb_shape (hb_font_t           *font,
	  hb_buffer_t         *buffer,
	  const hb_feature_t  *features,
	  unsigned int         num_features)
{
  hb_shape_full (font, buffer, features, num_features, NULL);
}
