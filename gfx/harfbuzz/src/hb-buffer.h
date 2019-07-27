




























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_BUFFER_H
#define HB_BUFFER_H

#include "hb-common.h"
#include "hb-unicode.h"
#include "hb-font.h"

HB_BEGIN_DECLS


typedef struct hb_glyph_info_t {
  hb_codepoint_t codepoint;
  hb_mask_t      mask;
  uint32_t       cluster;

  
  hb_var_int_t   var1;
  hb_var_int_t   var2;
} hb_glyph_info_t;

typedef struct hb_glyph_position_t {
  hb_position_t  x_advance;
  hb_position_t  y_advance;
  hb_position_t  x_offset;
  hb_position_t  y_offset;

  
  hb_var_int_t   var;
} hb_glyph_position_t;


typedef struct hb_segment_properties_t {
  hb_direction_t  direction;
  hb_script_t     script;
  hb_language_t   language;
  
  void           *reserved1;
  void           *reserved2;
} hb_segment_properties_t;

#define HB_SEGMENT_PROPERTIES_DEFAULT {HB_DIRECTION_INVALID, \
				       HB_SCRIPT_INVALID, \
				       HB_LANGUAGE_INVALID, \
				       NULL, \
				       NULL}

hb_bool_t
hb_segment_properties_equal (const hb_segment_properties_t *a,
			     const hb_segment_properties_t *b);

unsigned int
hb_segment_properties_hash (const hb_segment_properties_t *p);







typedef struct hb_buffer_t hb_buffer_t;

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


typedef enum {
  HB_BUFFER_CONTENT_TYPE_INVALID = 0,
  HB_BUFFER_CONTENT_TYPE_UNICODE,
  HB_BUFFER_CONTENT_TYPE_GLYPHS
} hb_buffer_content_type_t;

void
hb_buffer_set_content_type (hb_buffer_t              *buffer,
			    hb_buffer_content_type_t  content_type);

hb_buffer_content_type_t
hb_buffer_get_content_type (hb_buffer_t *buffer);


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
hb_buffer_set_segment_properties (hb_buffer_t *buffer,
				  const hb_segment_properties_t *props);

void
hb_buffer_get_segment_properties (hb_buffer_t *buffer,
				  hb_segment_properties_t *props);

void
hb_buffer_guess_segment_properties (hb_buffer_t *buffer);


typedef enum { 
  HB_BUFFER_FLAG_DEFAULT			= 0x00000000u,
  HB_BUFFER_FLAG_BOT				= 0x00000001u, 
  HB_BUFFER_FLAG_EOT				= 0x00000002u, 
  HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES	= 0x00000004u
} hb_buffer_flags_t;

void
hb_buffer_set_flags (hb_buffer_t       *buffer,
		     hb_buffer_flags_t  flags);

hb_buffer_flags_t
hb_buffer_get_flags (hb_buffer_t *buffer);



#define HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT 0xFFFDu



void
hb_buffer_set_replacement_codepoint (hb_buffer_t    *buffer,
				     hb_codepoint_t  replacement);

hb_codepoint_t
hb_buffer_get_replacement_codepoint (hb_buffer_t    *buffer);




void
hb_buffer_reset (hb_buffer_t *buffer);


void
hb_buffer_clear_contents (hb_buffer_t *buffer);


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
hb_buffer_add (hb_buffer_t    *buffer,
	       hb_codepoint_t  codepoint,
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


void
hb_buffer_add_codepoints (hb_buffer_t          *buffer,
			  const hb_codepoint_t *text,
			  int                   text_length,
			  unsigned int          item_offset,
			  int                   item_length);



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





void
hb_buffer_normalize_glyphs (hb_buffer_t *buffer);






typedef enum { 
  HB_BUFFER_SERIALIZE_FLAG_DEFAULT		= 0x00000000u,
  HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS		= 0x00000001u,
  HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS		= 0x00000002u,
  HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES	= 0x00000004u
} hb_buffer_serialize_flags_t;

typedef enum {
  HB_BUFFER_SERIALIZE_FORMAT_TEXT	= HB_TAG('T','E','X','T'),
  HB_BUFFER_SERIALIZE_FORMAT_JSON	= HB_TAG('J','S','O','N'),
  HB_BUFFER_SERIALIZE_FORMAT_INVALID	= HB_TAG_NONE
} hb_buffer_serialize_format_t;


hb_buffer_serialize_format_t
hb_buffer_serialize_format_from_string (const char *str, int len);

const char *
hb_buffer_serialize_format_to_string (hb_buffer_serialize_format_t format);

const char **
hb_buffer_serialize_list_formats (void);


unsigned int
hb_buffer_serialize_glyphs (hb_buffer_t *buffer,
			    unsigned int start,
			    unsigned int end,
			    char *buf,
			    unsigned int buf_size,
			    unsigned int *buf_consumed, 
			    hb_font_t *font, 
			    hb_buffer_serialize_format_t format,
			    hb_buffer_serialize_flags_t flags);

hb_bool_t
hb_buffer_deserialize_glyphs (hb_buffer_t *buffer,
			      const char *buf,
			      int buf_len, 
			      const char **end_ptr, 
			      hb_font_t *font, 
			      hb_buffer_serialize_format_t format);


HB_END_DECLS

#endif 
