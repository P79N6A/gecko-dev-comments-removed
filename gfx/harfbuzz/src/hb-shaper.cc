

























#include "hb-private.hh"
#include "hb-shaper-private.hh"
#include "hb-atomic-private.hh"


static const hb_shaper_pair_t all_shapers[] = {
#define HB_SHAPER_IMPLEMENT(name) {#name, _hb_##name##_shape},
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
};




static const hb_shaper_pair_t *static_shapers;

static inline
void free_static_shapers (void)
{
  if (unlikely (static_shapers != all_shapers))
    free ((void *) static_shapers);
}

const hb_shaper_pair_t *
_hb_shapers_get (void)
{
retry:
  hb_shaper_pair_t *shapers = (hb_shaper_pair_t *) hb_atomic_ptr_get (&static_shapers);

  if (unlikely (!shapers))
  {
    char *env = getenv ("HB_SHAPER_LIST");
    if (!env || !*env) {
      (void) hb_atomic_ptr_cmpexch (&static_shapers, NULL, &all_shapers[0]);
      return (const hb_shaper_pair_t *) all_shapers;
    }

    
    shapers = (hb_shaper_pair_t *) malloc (sizeof (all_shapers));
    if (unlikely (!shapers)) {
      (void) hb_atomic_ptr_cmpexch (&static_shapers, NULL, &all_shapers[0]);
      return (const hb_shaper_pair_t *) all_shapers;
    }

    memcpy (shapers, all_shapers, sizeof (all_shapers));

     
    unsigned int i = 0;
    char *end, *p = env;
    for (;;) {
      end = strchr (p, ',');
      if (!end)
	end = p + strlen (p);

      for (unsigned int j = i; j < ARRAY_LENGTH (all_shapers); j++)
	if (end - p == (int) strlen (shapers[j].name) &&
	    0 == strncmp (shapers[j].name, p, end - p))
	{
	  
	 struct hb_shaper_pair_t t = shapers[j];
	 memmove (&shapers[i + 1], &shapers[i], sizeof (shapers[i]) * (j - i));
	 shapers[i] = t;
	 i++;
	}

      if (!*end)
	break;
      else
	p = end + 1;
    }

    if (!hb_atomic_ptr_cmpexch (&static_shapers, NULL, shapers)) {
      free (shapers);
      goto retry;
    }

#ifdef HAVE_ATEXIT
    atexit (free_static_shapers); 
#endif
  }

  return shapers;
}
