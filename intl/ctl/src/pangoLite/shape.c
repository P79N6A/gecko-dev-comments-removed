







































#include "pango-glyph.h"
#include "pango-engine.h"













void pangolite_shape(const gunichar2  *text, 
                 gint             length, 
                 PangoliteAnalysis    *analysis,
                 PangoliteGlyphString *glyphs)
{
  if (analysis->shape_engine)
    analysis->shape_engine->script_shape(analysis->fontCharset, text, length, 
                                         analysis, glyphs);
  else {
    pangolite_glyph_string_set_size (glyphs, 1);
    
    glyphs->glyphs[0].glyph = 0;
    glyphs->log_clusters[0] = 0;
  }
  
  g_assert (glyphs->num_glyphs > 0);
}
