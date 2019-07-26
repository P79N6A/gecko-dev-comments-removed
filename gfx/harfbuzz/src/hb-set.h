

























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_SET_H
#define HB_SET_H

#include "hb-common.h"

HB_BEGIN_DECLS


typedef struct hb_set_t hb_set_t;


hb_set_t *
hb_set_create (void);

hb_set_t *
hb_set_get_empty (void);

hb_set_t *
hb_set_reference (hb_set_t *set);

void
hb_set_destroy (hb_set_t *set);

hb_bool_t
hb_set_set_user_data (hb_set_t           *set,
		      hb_user_data_key_t *key,
		      void *              data,
		      hb_destroy_func_t   destroy,
		      hb_bool_t           replace);

void *
hb_set_get_user_data (hb_set_t           *set,
		      hb_user_data_key_t *key);



hb_bool_t
hb_set_allocation_successful (const hb_set_t *set);

void
hb_set_clear (hb_set_t *set);

hb_bool_t
hb_set_is_empty (const hb_set_t *set);

hb_bool_t
hb_set_has (const hb_set_t *set,
	    hb_codepoint_t  codepoint);



void
hb_set_add (hb_set_t       *set,
	    hb_codepoint_t  codepoint);

void
hb_set_add_range (hb_set_t       *set,
		  hb_codepoint_t  first,
		  hb_codepoint_t  last);

void
hb_set_del (hb_set_t       *set,
	    hb_codepoint_t  codepoint);

void
hb_set_del_range (hb_set_t       *set,
		  hb_codepoint_t  first,
		  hb_codepoint_t  last);

hb_bool_t
hb_set_is_equal (const hb_set_t *set,
		 const hb_set_t *other);

void
hb_set_set (hb_set_t       *set,
	    const hb_set_t *other);

void
hb_set_union (hb_set_t       *set,
	      const hb_set_t *other);

void
hb_set_intersect (hb_set_t       *set,
		  const hb_set_t *other);

void
hb_set_subtract (hb_set_t       *set,
		 const hb_set_t *other);

void
hb_set_symmetric_difference (hb_set_t       *set,
			     const hb_set_t *other);

void
hb_set_invert (hb_set_t *set);

unsigned int
hb_set_get_population (const hb_set_t *set);


hb_codepoint_t
hb_set_get_min (const hb_set_t *set);


hb_codepoint_t
hb_set_get_max (const hb_set_t *set);


hb_bool_t
hb_set_next (const hb_set_t *set,
	     hb_codepoint_t *codepoint);


hb_bool_t
hb_set_next_range (const hb_set_t *set,
		   hb_codepoint_t *first,
		   hb_codepoint_t *last);


HB_END_DECLS

#endif 
