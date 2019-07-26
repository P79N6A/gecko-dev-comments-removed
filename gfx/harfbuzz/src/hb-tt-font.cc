

























#include "hb-font-private.hh" 

#include "hb-open-type-private.hh"

#include "hb-ot-hhea-table.hh"
#include "hb-ot-hmtx-table.hh"

#include "hb-blob.h"

#include <string.h>



#if 0
struct hb_tt_font_t
{
  const struct hhea *hhea;
  hb_blob_t *hhea_blob;
};


static hb_tt_font_t *
_hb_tt_font_create (hb_font_t *font)
{
  
  hb_tt_font_t *tt = (hb_tt_font_t *) calloc (1, sizeof (hb_tt_font_t));

  tt->hhea_blob = Sanitizer<hhea>::sanitize (font->face->reference_table (HB_OT_TAG_hhea));
  tt->hhea = Sanitizer<hhea>::lock_instance (tt->hhea_blob);

  return tt;
}

static void
_hb_tt_font_destroy (hb_tt_font_t *tt)
{
  hb_blob_destroy (tt->hhea_blob);

  free (tt);
}

static inline const hhea&
_get_hhea (hb_face_t *face)
{
  return likely (face->tt && face->tt->hhea) ? *face->tt->hhea : Null(hhea);
}






#endif
