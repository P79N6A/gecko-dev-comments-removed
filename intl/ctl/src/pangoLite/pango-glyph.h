







































#ifndef __PANGO_GLYPH_H__
#define __PANGO_GLYPH_H__

#include "pango-types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PangoliteGlyphInfo PangoliteGlyphInfo;
typedef struct _PangoliteGlyphString PangoliteGlyphString;


typedef gint32 PangoliteGlyphUnit;




struct _PangoliteGlyphInfo
{
  PangoliteGlyph glyph;
  gint           is_cluster_start;
};




struct _PangoliteGlyphString {
  gint num_glyphs;

  PangoliteGlyphInfo *glyphs;

  




  gint *log_clusters;

  
  gint space;
};

PangoliteGlyphString *pangolite_glyph_string_new(void);
void pangolite_glyph_string_set_size(PangoliteGlyphString *string, gint new_len);
void pangolite_glyph_string_free(PangoliteGlyphString *string);




void pangolite_shape(const gunichar2  *text,
                 gint             length,
                 PangoliteAnalysis    *analysis,
                 PangoliteGlyphString *glyphs);

#ifdef __cplusplus
}
#endif 

#endif
