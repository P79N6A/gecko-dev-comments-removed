

























#ifndef HB_OT_LAYOUT_PRIVATE_HH
#define HB_OT_LAYOUT_PRIVATE_HH

#include "hb-private.h"

#include "hb-ot-layout.h"
#include "hb-ot-head-private.hh"

#include "hb-font-private.h"
#include "hb-buffer-private.hh"

HB_BEGIN_DECLS



#define props_cache() var1.u16[1] /* glyph_props cache */



typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0x0001,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 0x0002,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 0x0004,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 0x0008,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 0x0010
} hb_ot_layout_glyph_class_t;






struct hb_ot_layout_t
{
  hb_blob_t *gdef_blob;
  hb_blob_t *gsub_blob;
  hb_blob_t *gpos_blob;

  const struct GDEF *gdef;
  const struct GSUB *gsub;
  const struct GPOS *gpos;
};

struct hb_ot_layout_context_t
{
  hb_face_t *face;
  hb_font_t *font;

  
  inline hb_position_t scale_x (int16_t v) { return scale (v, this->font->x_scale); }
  inline hb_position_t scale_y (int16_t v) { return scale (v, this->font->y_scale); }

  private:
  inline hb_position_t scale (int16_t v, unsigned int scale) { return v * (int64_t) scale / this->face->head_table->get_upem (); }
};


HB_INTERNAL hb_ot_layout_t *
_hb_ot_layout_new (hb_face_t *face);

HB_INTERNAL void
_hb_ot_layout_free (hb_ot_layout_t *layout);






HB_INTERNAL unsigned int
_hb_ot_layout_get_glyph_property (hb_face_t       *face,
				  hb_glyph_info_t *info);

HB_INTERNAL hb_bool_t
_hb_ot_layout_check_glyph_property (hb_face_t    *face,
				    hb_glyph_info_t *ginfo,
				    unsigned int  lookup_props,
				    unsigned int *property_out);

HB_INTERNAL hb_bool_t
_hb_ot_layout_skip_mark (hb_face_t    *face,
			 hb_glyph_info_t *ginfo,
			 unsigned int  lookup_props,
			 unsigned int *property_out);


HB_END_DECLS

#endif 
