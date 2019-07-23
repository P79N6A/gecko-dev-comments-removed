

















#ifndef __T1DECODE_H__
#define __T1DECODE_H__


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  FT_CALLBACK_TABLE
  const T1_Decoder_FuncsRec  t1_decoder_funcs;


  FT_LOCAL( FT_Error )
  t1_decoder_parse_glyph( T1_Decoder  decoder,
                          FT_UInt     glyph_index );

  FT_LOCAL( FT_Error )
  t1_decoder_parse_charstrings( T1_Decoder  decoder,
                                FT_Byte*    base,
                                FT_UInt     len );

  FT_LOCAL( FT_Error )
  t1_decoder_init( T1_Decoder           decoder,
                   FT_Face              face,
                   FT_Size              size,
                   FT_GlyphSlot         slot,
                   FT_Byte**            glyph_names,
                   PS_Blend             blend,
                   FT_Bool              hinting,
                   FT_Render_Mode       hint_mode,
                   T1_Decoder_Callback  parse_glyph );

  FT_LOCAL( void )
  t1_decoder_done( T1_Decoder  decoder );


FT_END_HEADER

#endif 



