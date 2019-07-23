

















#ifndef __SVPSCMAP_H__
#define __SVPSCMAP_H__

#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_POSTSCRIPT_CMAPS  "postscript-cmaps"


  


  typedef FT_UInt32
  (*PS_Unicode_ValueFunc)( const char*  glyph_name );

  


  typedef const char*
  (*PS_Macintosh_NameFunc)( FT_UInt  name_index );

  


  typedef const char*
  (*PS_Adobe_Std_StringsFunc)( FT_UInt  string_index );


  



  typedef struct  PS_UniMap_
  {
    FT_UInt32  unicode;      
    FT_UInt    glyph_index;

  } PS_UniMap;


  typedef struct PS_UnicodesRec_*  PS_Unicodes;

  typedef struct  PS_UnicodesRec_
  {
    FT_CMapRec  cmap;
    FT_UInt     num_maps;
    PS_UniMap*  maps;

  } PS_UnicodesRec;


  



  typedef const char*
  (*PS_GetGlyphNameFunc)( FT_Pointer  data,
                          FT_UInt     string_index );

  



  typedef void
  (*PS_FreeGlyphNameFunc)( FT_Pointer  data,
                           const char*  name );

  typedef FT_Error
  (*PS_Unicodes_InitFunc)( FT_Memory             memory,
                           PS_Unicodes           unicodes,
                           FT_UInt               num_glyphs,
                           PS_GetGlyphNameFunc   get_glyph_name,
                           PS_FreeGlyphNameFunc  free_glyph_name,
                           FT_Pointer            glyph_data );

  typedef FT_UInt
  (*PS_Unicodes_CharIndexFunc)( PS_Unicodes  unicodes,
                                FT_UInt32    unicode );

  typedef FT_ULong
  (*PS_Unicodes_CharNextFunc)( PS_Unicodes  unicodes,
                               FT_UInt32   *unicode );


  FT_DEFINE_SERVICE( PsCMaps )
  {
    PS_Unicode_ValueFunc       unicode_value;

    PS_Unicodes_InitFunc       unicodes_init;
    PS_Unicodes_CharIndexFunc  unicodes_char_index;
    PS_Unicodes_CharNextFunc   unicodes_char_next;

    PS_Macintosh_NameFunc      macintosh_name;
    PS_Adobe_Std_StringsFunc   adobe_std_strings;
    const unsigned short*      adobe_std_encoding;
    const unsigned short*      adobe_expert_encoding;
  };

  


FT_END_HEADER


#endif 



