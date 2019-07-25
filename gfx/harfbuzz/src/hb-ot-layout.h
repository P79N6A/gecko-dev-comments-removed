

























#ifndef HB_OT_LAYOUT_H
#define HB_OT_LAYOUT_H

#include "hb-common.h"
#include "hb-buffer.h"
#include "hb-font.h"

#include "hb-ot-tag.h"

HB_BEGIN_DECLS

#define HB_OT_TAG_GDEF HB_TAG('G','D','E','F')
#define HB_OT_TAG_GSUB HB_TAG('G','S','U','B')
#define HB_OT_TAG_GPOS HB_TAG('G','P','O','S')





typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0x0000,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 0x0002,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 0x0004,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 0x0008,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 0x0010
} hb_ot_layout_glyph_class_t;




hb_bool_t
hb_ot_layout_has_glyph_classes (hb_face_t *face);

hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (hb_face_t      *face,
			      hb_codepoint_t  glyph);

void
hb_ot_layout_set_glyph_class (hb_face_t                 *face,
			      hb_codepoint_t             glyph,
			      hb_ot_layout_glyph_class_t klass);

void
hb_ot_layout_build_glyph_classes (hb_face_t      *face,
				  hb_codepoint_t *glyphs,
				  unsigned char  *klasses,
				  uint16_t        count);



unsigned int
hb_ot_layout_get_attach_points (hb_face_t      *face,
				hb_codepoint_t  glyph,
				unsigned int    start_offset,
				unsigned int   *point_count ,
				unsigned int   *point_array );


unsigned int
hb_ot_layout_get_lig_carets (hb_font_t      *font,
			     hb_face_t      *face,
			     hb_codepoint_t  glyph,
			     unsigned int    start_offset,
			     unsigned int   *caret_count ,
			     int            *caret_array );






#define HB_OT_LAYOUT_NO_SCRIPT_INDEX		((unsigned int) 0xFFFF)
#define HB_OT_LAYOUT_NO_FEATURE_INDEX		((unsigned int) 0xFFFF)
#define HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX	((unsigned int) 0xFFFF)

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
				  unsigned int   *script_index);

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
hb_ot_layout_feature_get_lookup_indexes (hb_face_t    *face,
					 hb_tag_t      table_tag,
					 unsigned int  feature_index,
					 unsigned int  start_offset,
					 unsigned int *lookup_count ,
					 unsigned int *lookup_indexes );






hb_bool_t
hb_ot_layout_has_substitution (hb_face_t *face);

hb_bool_t
hb_ot_layout_substitute_lookup (hb_face_t    *face,
				hb_buffer_t  *buffer,
				unsigned int  lookup_index,
				hb_mask_t     mask);





hb_bool_t
hb_ot_layout_has_positioning (hb_face_t *face);

hb_bool_t
hb_ot_layout_position_lookup (hb_font_t    *font,
			      hb_face_t    *face,
			      hb_buffer_t  *buffer,
			      unsigned int  lookup_index,
			      hb_mask_t     mask);


void
hb_ot_layout_position_finish (hb_font_t    *font,
			      hb_face_t    *face,
			      hb_buffer_t  *buffer);


HB_END_DECLS

#endif 
