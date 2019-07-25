





























#ifndef HB_UNICODE_H
#define HB_UNICODE_H

#include "hb-common.h"

HB_BEGIN_DECLS






typedef struct _hb_unicode_funcs_t hb_unicode_funcs_t;





hb_unicode_funcs_t *
hb_unicode_funcs_get_default (void);


hb_unicode_funcs_t *
hb_unicode_funcs_create (hb_unicode_funcs_t *parent);

hb_unicode_funcs_t *
hb_unicode_funcs_get_empty (void);

hb_unicode_funcs_t *
hb_unicode_funcs_reference (hb_unicode_funcs_t *ufuncs);

void
hb_unicode_funcs_destroy (hb_unicode_funcs_t *ufuncs);

hb_bool_t
hb_unicode_funcs_set_user_data (hb_unicode_funcs_t *ufuncs,
			        hb_user_data_key_t *key,
			        void *              data,
			        hb_destroy_func_t   destroy,
				hb_bool_t           replace);


void *
hb_unicode_funcs_get_user_data (hb_unicode_funcs_t *ufuncs,
			        hb_user_data_key_t *key);


void
hb_unicode_funcs_make_immutable (hb_unicode_funcs_t *ufuncs);

hb_bool_t
hb_unicode_funcs_is_immutable (hb_unicode_funcs_t *ufuncs);

hb_unicode_funcs_t *
hb_unicode_funcs_get_parent (hb_unicode_funcs_t *ufuncs);








typedef unsigned int			(*hb_unicode_combining_class_func_t)	(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      unicode,
										 void               *user_data);
typedef unsigned int			(*hb_unicode_eastasian_width_func_t)	(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      unicode,
										 void               *user_data);
typedef hb_unicode_general_category_t	(*hb_unicode_general_category_func_t)	(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      unicode,
										 void               *user_data);
typedef hb_codepoint_t			(*hb_unicode_mirroring_func_t)		(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      unicode,
										 void               *user_data);
typedef hb_script_t			(*hb_unicode_script_func_t)		(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      unicode,
										 void               *user_data);

typedef hb_bool_t			(*hb_unicode_compose_func_t)		(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      a,
										 hb_codepoint_t      b,
										 hb_codepoint_t     *ab,
										 void               *user_data);
typedef hb_bool_t			(*hb_unicode_decompose_func_t)		(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      ab,
										 hb_codepoint_t     *a,
										 hb_codepoint_t     *b,
										 void               *user_data);



void
hb_unicode_funcs_set_combining_class_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_combining_class_func_t combining_class_func,
					   void *user_data, hb_destroy_func_t destroy);

void
hb_unicode_funcs_set_eastasian_width_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_eastasian_width_func_t eastasian_width_func,
					   void *user_data, hb_destroy_func_t destroy);

void
hb_unicode_funcs_set_general_category_func (hb_unicode_funcs_t *ufuncs,
					    hb_unicode_general_category_func_t general_category_func,
					    void *user_data, hb_destroy_func_t destroy);

void
hb_unicode_funcs_set_mirroring_func (hb_unicode_funcs_t *ufuncs,
				     hb_unicode_mirroring_func_t mirroring_func,
				     void *user_data, hb_destroy_func_t destroy);

void
hb_unicode_funcs_set_script_func (hb_unicode_funcs_t *ufuncs,
				  hb_unicode_script_func_t script_func,
				  void *user_data, hb_destroy_func_t destroy);

void
hb_unicode_funcs_set_compose_func (hb_unicode_funcs_t *ufuncs,
				   hb_unicode_compose_func_t compose_func,
				   void *user_data, hb_destroy_func_t destroy);

void
hb_unicode_funcs_set_decompose_func (hb_unicode_funcs_t *ufuncs,
				     hb_unicode_decompose_func_t decompose_func,
				     void *user_data, hb_destroy_func_t destroy);




unsigned int
hb_unicode_combining_class (hb_unicode_funcs_t *ufuncs,
			    hb_codepoint_t unicode);

unsigned int
hb_unicode_eastasian_width (hb_unicode_funcs_t *ufuncs,
			    hb_codepoint_t unicode);

hb_unicode_general_category_t
hb_unicode_general_category (hb_unicode_funcs_t *ufuncs,
			     hb_codepoint_t unicode);

hb_codepoint_t
hb_unicode_mirroring (hb_unicode_funcs_t *ufuncs,
		      hb_codepoint_t unicode);

hb_script_t
hb_unicode_script (hb_unicode_funcs_t *ufuncs,
		   hb_codepoint_t unicode);

hb_bool_t
hb_unicode_compose (hb_unicode_funcs_t *ufuncs,
		    hb_codepoint_t      a,
		    hb_codepoint_t      b,
		    hb_codepoint_t     *ab);
hb_bool_t
hb_unicode_decompose (hb_unicode_funcs_t *ufuncs,
		      hb_codepoint_t      ab,
		      hb_codepoint_t     *a,
		      hb_codepoint_t     *b);

HB_END_DECLS

#endif 
