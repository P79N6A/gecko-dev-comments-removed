

























#ifndef HB_OT_LAYOUT_PRIVATE_H
#define HB_OT_LAYOUT_PRIVATE_H

#include "hb-private.h"

#include "hb-ot-layout.h"

#include "hb-font-private.hh"
#include "hb-buffer-private.hh"


HB_BEGIN_DECLS

typedef unsigned int hb_ot_layout_class_t;





struct hb_ot_layout_t
{
  hb_blob_t *gdef_blob;
  hb_blob_t *gsub_blob;
  hb_blob_t *gpos_blob;

  const struct GDEF *gdef;
  const struct GSUB *gsub;
  const struct GPOS *gpos;

  struct
  {
    unsigned char *klasses;
    unsigned int len;
  } new_gdef;
};

struct hb_ot_layout_context_t
{
  hb_face_t *face;
  hb_font_t *font;

  union info_t
  {
    struct gpos_t
    {
      unsigned int last;        
      hb_position_t anchor_x;   
      hb_position_t anchor_y;   
    } gpos;
  } info;

  
  
  inline hb_position_t scale_x (int16_t v) { return (int64_t) this->font->x_scale * v / this->face->units_per_em; }
  inline hb_position_t scale_y (int16_t v) { return (int64_t) this->font->y_scale * v / this->face->units_per_em; }
};


HB_INTERNAL hb_ot_layout_t *
_hb_ot_layout_new (hb_face_t *face);

HB_INTERNAL void
_hb_ot_layout_free (hb_ot_layout_t *layout);






HB_INTERNAL hb_bool_t
_hb_ot_layout_has_new_glyph_classes (hb_face_t *face);

HB_INTERNAL void
_hb_ot_layout_set_glyph_property (hb_face_t      *face,
				  hb_codepoint_t  glyph,
				  unsigned int    property);

HB_INTERNAL void
_hb_ot_layout_set_glyph_class (hb_face_t                  *face,
			       hb_codepoint_t              glyph,
			       hb_ot_layout_glyph_class_t  klass);

HB_INTERNAL hb_bool_t
_hb_ot_layout_check_glyph_property (hb_face_t    *face,
				    hb_internal_glyph_info_t *ginfo,
				    unsigned int  lookup_flags,
				    unsigned int *property);

HB_INTERNAL hb_bool_t
_hb_ot_layout_skip_mark (hb_face_t    *face,
			 hb_internal_glyph_info_t *ginfo,
			 unsigned int  lookup_flags,
			 unsigned int *property);

HB_END_DECLS

#endif 
