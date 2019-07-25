

























#include "hb-private.h"

#include "hb-shape.h"

#include "hb-buffer-private.hh"

#include "hb-ot-shape.h"

#ifdef HAVE_GRAPHITE
#include "hb-graphite.h"
#endif

HB_BEGIN_DECLS


void
hb_shape (hb_font_t    *font,
	  hb_face_t    *face,
	  hb_buffer_t  *buffer,
	  hb_feature_t *features,
	  unsigned int  num_features)
{
#if 0 && defined(HAVE_GRAPHITE)
  hb_blob_t *silf_blob;
  silf_blob = hb_face_get_table (face, HB_GRAPHITE_TAG_Silf);
  if (hb_blob_get_length(silf_blob))
  {
    hb_graphite_shape(font, face, buffer, features, num_features);
    hb_blob_destroy(silf_blob);
    return;
  }
  hb_blob_destroy(silf_blob);
#endif

  hb_ot_shape (font, face, buffer, features, num_features);
}


HB_END_DECLS
