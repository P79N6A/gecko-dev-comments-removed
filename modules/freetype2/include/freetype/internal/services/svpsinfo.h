

















#ifndef __SVPSINFO_H__
#define __SVPSINFO_H__

#include FT_INTERNAL_SERVICE_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_POSTSCRIPT_INFO  "postscript-info"


  typedef FT_Error
  (*PS_GetFontInfoFunc)( FT_Face          face,
                         PS_FontInfoRec*  afont_info );

  typedef FT_Int
  (*PS_HasGlyphNamesFunc)( FT_Face   face );

  typedef FT_Error
  (*PS_GetFontPrivateFunc)( FT_Face         face,
                            PS_PrivateRec*  afont_private );


  FT_DEFINE_SERVICE( PsInfo )
  {
    PS_GetFontInfoFunc     ps_get_font_info;
    PS_HasGlyphNamesFunc   ps_has_glyph_names;
    PS_GetFontPrivateFunc  ps_get_font_private;
  };

  


FT_END_HEADER


#endif 



