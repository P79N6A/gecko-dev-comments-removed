

























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
hb_set_allocation_successful (hb_set_t  *set);

void
hb_set_clear (hb_set_t *set);

hb_bool_t
hb_set_empty (hb_set_t *set);

hb_bool_t
hb_set_has (hb_set_t       *set,
	    hb_codepoint_t  codepoint);



void
hb_set_add (hb_set_t       *set,
	    hb_codepoint_t  codepoint);

void
hb_set_del (hb_set_t       *set,
	    hb_codepoint_t  codepoint);

hb_bool_t
hb_set_equal (hb_set_t *set,
	      hb_set_t *other);

void
hb_set_set (hb_set_t *set,
	    hb_set_t *other);

void
hb_set_union (hb_set_t *set,
	      hb_set_t *other);

void
hb_set_intersect (hb_set_t *set,
		  hb_set_t *other);

void
hb_set_subtract (hb_set_t *set,
		 hb_set_t *other);

void
hb_set_symmetric_difference (hb_set_t *set,
			     hb_set_t *other);


hb_codepoint_t
hb_set_min (hb_set_t *set);


hb_codepoint_t
hb_set_max (hb_set_t *set);


hb_bool_t
hb_set_next (hb_set_t       *set,
	     hb_codepoint_t *codepoint);




HB_END_DECLS

#endif 
