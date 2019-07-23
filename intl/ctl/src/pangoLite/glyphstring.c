







































#include <glib.h>
#include "pango-glyph.h"








PangoliteGlyphString *
pangolite_glyph_string_new(void)
{
  PangoliteGlyphString *string = g_new(PangoliteGlyphString, 1);
  
  string->num_glyphs = 0;
  string->space = 0;
  string->glyphs = NULL;
  string->log_clusters = NULL;
  return string;
}








void
pangolite_glyph_string_set_size(PangoliteGlyphString *string, gint new_len)
{
  g_return_if_fail (new_len >= 0);

  while (new_len > string->space) {
    if (string->space == 0)
      string->space = 1;
    else
      string->space *= 2;
    
    if (string->space < 0)
      g_error("%s: glyph string length overflows maximum integer size", 
              "pangolite_glyph_string_set_size");
  }
  
  string->glyphs = g_realloc(string->glyphs, 
                             string->space * sizeof(PangoliteGlyphInfo));
  string->log_clusters = g_realloc(string->log_clusters, 
                                   string->space * sizeof (gint));
  string->num_glyphs = new_len;
}







void
pangolite_glyph_string_free(PangoliteGlyphString *string)
{
  g_free(string->glyphs);
  g_free(string->log_clusters);
  g_free(string);
}
