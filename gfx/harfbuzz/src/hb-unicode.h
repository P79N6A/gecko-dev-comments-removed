





























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_UNICODE_H
#define HB_UNICODE_H

#include "hb-common.h"

HB_BEGIN_DECLS





typedef enum
{
  HB_UNICODE_GENERAL_CATEGORY_CONTROL,			
  HB_UNICODE_GENERAL_CATEGORY_FORMAT,			
  HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED,		
  HB_UNICODE_GENERAL_CATEGORY_PRIVATE_USE,		
  HB_UNICODE_GENERAL_CATEGORY_SURROGATE,		
  HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER,		
  HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER,		
  HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER,		
  HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER,		
  HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER,		
  HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK,		
  HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK,		
  HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK,		
  HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER,		
  HB_UNICODE_GENERAL_CATEGORY_LETTER_NUMBER,		
  HB_UNICODE_GENERAL_CATEGORY_OTHER_NUMBER,		
  HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION,	
  HB_UNICODE_GENERAL_CATEGORY_DASH_PUNCTUATION,		
  HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION,	
  HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION,	
  HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION,	
  HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION,	
  HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION,		
  HB_UNICODE_GENERAL_CATEGORY_CURRENCY_SYMBOL,		
  HB_UNICODE_GENERAL_CATEGORY_MODIFIER_SYMBOL,		
  HB_UNICODE_GENERAL_CATEGORY_MATH_SYMBOL,		
  HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL,		
  HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR,		
  HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR,	
  HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR		
} hb_unicode_general_category_t;








typedef enum
{
  HB_UNICODE_COMBINING_CLASS_NOT_REORDERED	= 0,
  HB_UNICODE_COMBINING_CLASS_OVERLAY		= 1,
  HB_UNICODE_COMBINING_CLASS_NUKTA		= 7,
  HB_UNICODE_COMBINING_CLASS_KANA_VOICING	= 8,
  HB_UNICODE_COMBINING_CLASS_VIRAMA		= 9,

  
  HB_UNICODE_COMBINING_CLASS_CCC10	=  10,
  HB_UNICODE_COMBINING_CLASS_CCC11	=  11,
  HB_UNICODE_COMBINING_CLASS_CCC12	=  12,
  HB_UNICODE_COMBINING_CLASS_CCC13	=  13,
  HB_UNICODE_COMBINING_CLASS_CCC14	=  14,
  HB_UNICODE_COMBINING_CLASS_CCC15	=  15,
  HB_UNICODE_COMBINING_CLASS_CCC16	=  16,
  HB_UNICODE_COMBINING_CLASS_CCC17	=  17,
  HB_UNICODE_COMBINING_CLASS_CCC18	=  18,
  HB_UNICODE_COMBINING_CLASS_CCC19	=  19,
  HB_UNICODE_COMBINING_CLASS_CCC20	=  20,
  HB_UNICODE_COMBINING_CLASS_CCC21	=  21,
  HB_UNICODE_COMBINING_CLASS_CCC22	=  22,
  HB_UNICODE_COMBINING_CLASS_CCC23	=  23,
  HB_UNICODE_COMBINING_CLASS_CCC24	=  24,
  HB_UNICODE_COMBINING_CLASS_CCC25	=  25,
  HB_UNICODE_COMBINING_CLASS_CCC26	=  26,

  
  HB_UNICODE_COMBINING_CLASS_CCC27	=  27,
  HB_UNICODE_COMBINING_CLASS_CCC28	=  28,
  HB_UNICODE_COMBINING_CLASS_CCC29	=  29,
  HB_UNICODE_COMBINING_CLASS_CCC30	=  30,
  HB_UNICODE_COMBINING_CLASS_CCC31	=  31,
  HB_UNICODE_COMBINING_CLASS_CCC32	=  32,
  HB_UNICODE_COMBINING_CLASS_CCC33	=  33,
  HB_UNICODE_COMBINING_CLASS_CCC34	=  34,
  HB_UNICODE_COMBINING_CLASS_CCC35	=  35,

  
  HB_UNICODE_COMBINING_CLASS_CCC36	=  36,

  
  HB_UNICODE_COMBINING_CLASS_CCC84	=  84,
  HB_UNICODE_COMBINING_CLASS_CCC91	=  91,

  
  HB_UNICODE_COMBINING_CLASS_CCC103	= 103,
  HB_UNICODE_COMBINING_CLASS_CCC107	= 107,

  
  HB_UNICODE_COMBINING_CLASS_CCC118	= 118,
  HB_UNICODE_COMBINING_CLASS_CCC122	= 122,

  
  HB_UNICODE_COMBINING_CLASS_CCC129	= 129,
  HB_UNICODE_COMBINING_CLASS_CCC130	= 130,
  HB_UNICODE_COMBINING_CLASS_CCC133	= 132,


  HB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW_LEFT	= 200,
  HB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW		= 202,
  HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE		= 214,
  HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE_RIGHT	= 216,
  HB_UNICODE_COMBINING_CLASS_BELOW_LEFT			= 218,
  HB_UNICODE_COMBINING_CLASS_BELOW			= 220,
  HB_UNICODE_COMBINING_CLASS_BELOW_RIGHT		= 222,
  HB_UNICODE_COMBINING_CLASS_LEFT			= 224,
  HB_UNICODE_COMBINING_CLASS_RIGHT			= 226,
  HB_UNICODE_COMBINING_CLASS_ABOVE_LEFT			= 228,
  HB_UNICODE_COMBINING_CLASS_ABOVE			= 230,
  HB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT		= 232,
  HB_UNICODE_COMBINING_CLASS_DOUBLE_BELOW		= 233,
  HB_UNICODE_COMBINING_CLASS_DOUBLE_ABOVE		= 234,

  HB_UNICODE_COMBINING_CLASS_IOTA_SUBSCRIPT		= 240,

  HB_UNICODE_COMBINING_CLASS_INVALID	= 255
} hb_unicode_combining_class_t;






typedef struct hb_unicode_funcs_t hb_unicode_funcs_t;





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








typedef hb_unicode_combining_class_t	(*hb_unicode_combining_class_func_t)	(hb_unicode_funcs_t *ufuncs,
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



















typedef unsigned int			(*hb_unicode_decompose_compatibility_func_t)	(hb_unicode_funcs_t *ufuncs,
											 hb_codepoint_t      u,
											 hb_codepoint_t     *decomposed,
											 void               *user_data);


#define HB_UNICODE_MAX_DECOMPOSITION_LEN (18+1) /* codepoints */



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

void
hb_unicode_funcs_set_decompose_compatibility_func (hb_unicode_funcs_t *ufuncs,
						   hb_unicode_decompose_compatibility_func_t decompose_compatibility_func,
						   void *user_data, hb_destroy_func_t destroy);



hb_unicode_combining_class_t
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

unsigned int
hb_unicode_decompose_compatibility (hb_unicode_funcs_t *ufuncs,
				    hb_codepoint_t      u,
				    hb_codepoint_t     *decomposed);

HB_END_DECLS

#endif 
