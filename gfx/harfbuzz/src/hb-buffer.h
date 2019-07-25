




























#ifndef HB_BUFFER_H
#define HB_BUFFER_H

#include "hb-common.h"
#include "hb-unicode.h"

HB_BEGIN_DECLS


typedef struct _hb_buffer_t hb_buffer_t;

typedef struct _hb_glyph_info_t {
  hb_codepoint_t codepoint;
  hb_mask_t      mask;
  uint32_t       cluster;

  
  hb_var_int_t   var1;
  hb_var_int_t   var2;
} hb_glyph_info_t;

typedef struct _hb_glyph_position_t {
  hb_position_t  x_advance;
  hb_position_t  y_advance;
  hb_position_t  x_offset;
  hb_position_t  y_offset;

  
  hb_var_int_t   var;
} hb_glyph_position_t;


hb_buffer_t *
hb_buffer_create (void);

hb_buffer_t *
hb_buffer_get_empty (void);

hb_buffer_t *
hb_buffer_reference (hb_buffer_t *buffer);

void
hb_buffer_destroy (hb_buffer_t *buffer);

hb_bool_t
hb_buffer_set_user_data (hb_buffer_t        *buffer,
			 hb_user_data_key_t *key,
			 void *              data,
			 hb_destroy_func_t   destroy,
			 hb_bool_t           replace);

void *
hb_buffer_get_user_data (hb_buffer_t        *buffer,
			 hb_user_data_key_t *key);


void
hb_buffer_set_unicode_funcs (hb_buffer_t        *buffer,
			     hb_unicode_funcs_t *unicode_funcs);

hb_unicode_funcs_t *
hb_buffer_get_unicode_funcs (hb_buffer_t        *buffer);

void
hb_buffer_set_direction (hb_buffer_t    *buffer,
			 hb_direction_t  direction);

hb_direction_t
hb_buffer_get_direction (hb_buffer_t *buffer);

void
hb_buffer_set_script (hb_buffer_t *buffer,
		      hb_script_t  script);

hb_script_t
hb_buffer_get_script (hb_buffer_t *buffer);

void
hb_buffer_set_language (hb_buffer_t   *buffer,
			hb_language_t  language);

hb_language_t
hb_buffer_get_language (hb_buffer_t *buffer);




void
hb_buffer_reset (hb_buffer_t *buffer);


hb_bool_t
hb_buffer_pre_allocate (hb_buffer_t  *buffer,
		        unsigned int  size);



hb_bool_t
hb_buffer_allocation_successful (hb_buffer_t  *buffer);

void
hb_buffer_reverse (hb_buffer_t *buffer);

void
hb_buffer_reverse_clusters (hb_buffer_t *buffer);

void
hb_buffer_guess_properties (hb_buffer_t *buffer);




void
hb_buffer_add (hb_buffer_t    *buffer,
	       hb_codepoint_t  codepoint,
	       hb_mask_t       mask,
	       unsigned int    cluster);

void
hb_buffer_add_utf8 (hb_buffer_t  *buffer,
		    const char   *text,
		    int           text_length,
		    unsigned int  item_offset,
		    int           item_length);

void
hb_buffer_add_utf16 (hb_buffer_t    *buffer,
		     const uint16_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length);

void
hb_buffer_add_utf32 (hb_buffer_t    *buffer,
		     const uint32_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length);



hb_bool_t
hb_buffer_set_length (hb_buffer_t  *buffer,
		      unsigned int  length);


unsigned int
hb_buffer_get_length (hb_buffer_t *buffer);




hb_glyph_info_t *
hb_buffer_get_glyph_infos (hb_buffer_t  *buffer,
                           unsigned int *length);


hb_glyph_position_t *
hb_buffer_get_glyph_positions (hb_buffer_t  *buffer,
                               unsigned int *length);


HB_END_DECLS

#endif 
