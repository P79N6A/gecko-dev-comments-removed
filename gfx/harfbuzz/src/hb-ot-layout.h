

























#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_LAYOUT_H
#define HB_OT_LAYOUT_H

#include "hb.h"

#include "hb-ot-tag.h"

HB_BEGIN_DECLS


#define HB_OT_TAG_GDEF HB_TAG('G','D','E','F')
#define HB_OT_TAG_GSUB HB_TAG('G','S','U','B')
#define HB_OT_TAG_GPOS HB_TAG('G','P','O','S')
#define HB_OT_TAG_JSTF HB_TAG('J','S','T','F')






hb_bool_t
hb_ot_layout_has_glyph_classes (hb_face_t *face);

typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 1,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 2,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 3,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 4
} hb_ot_layout_glyph_class_t;

hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (hb_face_t      *face,
			      hb_codepoint_t  glyph);

void
hb_ot_layout_get_glyphs_in_class (hb_face_t                  *face,
				  hb_ot_layout_glyph_class_t  klass,
				  hb_set_t                   *glyphs );




unsigned int
hb_ot_layout_get_attach_points (hb_face_t      *face,
				hb_codepoint_t  glyph,
				unsigned int    start_offset,
				unsigned int   *point_count ,
				unsigned int   *point_array );


unsigned int
hb_ot_layout_get_ligature_carets (hb_font_t      *font,
				  hb_direction_t  direction,
				  hb_codepoint_t  glyph,
				  unsigned int    start_offset,
				  unsigned int   *caret_count ,
				  hb_position_t  *caret_array );






#define HB_OT_LAYOUT_NO_SCRIPT_INDEX		0xFFFFu
#define HB_OT_LAYOUT_NO_FEATURE_INDEX		0xFFFFu
#define HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX	0xFFFFu

unsigned int
hb_ot_layout_table_get_script_tags (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int  start_offset,
				    unsigned int *script_count ,
				    hb_tag_t     *script_tags );

hb_bool_t
hb_ot_layout_table_find_script (hb_face_t    *face,
				hb_tag_t      table_tag,
				hb_tag_t      script_tag,
				unsigned int *script_index);


hb_bool_t
hb_ot_layout_table_choose_script (hb_face_t      *face,
				  hb_tag_t        table_tag,
				  const hb_tag_t *script_tags,
				  unsigned int   *script_index,
				  hb_tag_t       *chosen_script);

unsigned int
hb_ot_layout_table_get_feature_tags (hb_face_t    *face,
				     hb_tag_t      table_tag,
				     unsigned int  start_offset,
				     unsigned int *feature_count ,
				     hb_tag_t     *feature_tags );

unsigned int
hb_ot_layout_script_get_language_tags (hb_face_t    *face,
				       hb_tag_t      table_tag,
				       unsigned int  script_index,
				       unsigned int  start_offset,
				       unsigned int *language_count ,
				       hb_tag_t     *language_tags );

hb_bool_t
hb_ot_layout_script_find_language (hb_face_t    *face,
				   hb_tag_t      table_tag,
				   unsigned int  script_index,
				   hb_tag_t      language_tag,
				   unsigned int *language_index);

hb_bool_t
hb_ot_layout_language_get_required_feature_index (hb_face_t    *face,
						  hb_tag_t      table_tag,
						  unsigned int  script_index,
						  unsigned int  language_index,
						  unsigned int *feature_index);

hb_bool_t
hb_ot_layout_language_get_required_feature (hb_face_t    *face,
					    hb_tag_t      table_tag,
					    unsigned int  script_index,
					    unsigned int  language_index,
					    unsigned int *feature_index,
					    hb_tag_t     *feature_tag);

unsigned int
hb_ot_layout_language_get_feature_indexes (hb_face_t    *face,
					   hb_tag_t      table_tag,
					   unsigned int  script_index,
					   unsigned int  language_index,
					   unsigned int  start_offset,
					   unsigned int *feature_count ,
					   unsigned int *feature_indexes );

unsigned int
hb_ot_layout_language_get_feature_tags (hb_face_t    *face,
					hb_tag_t      table_tag,
					unsigned int  script_index,
					unsigned int  language_index,
					unsigned int  start_offset,
					unsigned int *feature_count ,
					hb_tag_t     *feature_tags );

hb_bool_t
hb_ot_layout_language_find_feature (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int  script_index,
				    unsigned int  language_index,
				    hb_tag_t      feature_tag,
				    unsigned int *feature_index);

unsigned int
hb_ot_layout_feature_get_lookups (hb_face_t    *face,
				  hb_tag_t      table_tag,
				  unsigned int  feature_index,
				  unsigned int  start_offset,
				  unsigned int *lookup_count ,
				  unsigned int *lookup_indexes );

unsigned int
hb_ot_layout_table_get_lookup_count (hb_face_t    *face,
				     hb_tag_t      table_tag);


void
hb_ot_layout_collect_lookups (hb_face_t      *face,
			      hb_tag_t        table_tag,
			      const hb_tag_t *scripts,
			      const hb_tag_t *languages,
			      const hb_tag_t *features,
			      hb_set_t       *lookup_indexes );

void
hb_ot_layout_lookup_collect_glyphs (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int  lookup_index,
				    hb_set_t     *glyphs_before, 
				    hb_set_t     *glyphs_input,  
				    hb_set_t     *glyphs_after,  
				    hb_set_t     *glyphs_output  );

#ifdef HB_NOT_IMPLEMENTED
typedef struct
{
  const hb_codepoint_t *before,
  unsigned int          before_length,
  const hb_codepoint_t *input,
  unsigned int          input_length,
  const hb_codepoint_t *after,
  unsigned int          after_length,
} hb_ot_layout_glyph_sequence_t;

typedef hb_bool_t
(*hb_ot_layout_glyph_sequence_func_t) (hb_font_t    *font,
				       hb_tag_t      table_tag,
				       unsigned int  lookup_index,
				       const hb_ot_layout_glyph_sequence_t *sequence,
				       void         *user_data);

void
Xhb_ot_layout_lookup_enumerate_sequences (hb_face_t    *face,
					 hb_tag_t      table_tag,
					 unsigned int  lookup_index,
					 hb_ot_layout_glyph_sequence_func_t callback,
					 void         *user_data);
#endif






hb_bool_t
hb_ot_layout_has_substitution (hb_face_t *face);

hb_bool_t
hb_ot_layout_lookup_would_substitute (hb_face_t            *face,
				      unsigned int          lookup_index,
				      const hb_codepoint_t *glyphs,
				      unsigned int          glyphs_length,
				      hb_bool_t             zero_context);

void
hb_ot_layout_lookup_substitute_closure (hb_face_t    *face,
				        unsigned int  lookup_index,
				        hb_set_t     *glyphs
					);

#ifdef HB_NOT_IMPLEMENTED

hb_bool_t
Xhb_ot_layout_lookup_substitute (hb_font_t            *font,
				unsigned int          lookup_index,
				const hb_ot_layout_glyph_sequence_t *sequence,
				unsigned int          out_size,
				hb_codepoint_t       *glyphs_out,   
				unsigned int         *clusters_out, 
				unsigned int         *out_length    );
#endif






hb_bool_t
hb_ot_layout_has_positioning (hb_face_t *face);

#ifdef HB_NOT_IMPLEMENTED

hb_bool_t
Xhb_ot_layout_lookup_position (hb_font_t            *font,
			      unsigned int          lookup_index,
			      const hb_ot_layout_glyph_sequence_t *sequence,
			      hb_glyph_position_t  *positions );
#endif



hb_bool_t
hb_ot_layout_get_size_params (hb_face_t    *face,
			      unsigned int *design_size,       
			      unsigned int *subfamily_id,      
			      unsigned int *subfamily_name_id, 
			      unsigned int *range_start,       
			      unsigned int *range_end          );


HB_END_DECLS

#endif
