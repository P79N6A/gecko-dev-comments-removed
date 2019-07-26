

















#ifndef __CFFGLOAD_H__
#define __CFFGLOAD_H__


#include <ft2build.h>
#include FT_FREETYPE_H
#include "cffobjs.h"


FT_BEGIN_HEADER


#define CFF_MAX_OPERANDS        48
#define CFF_MAX_SUBRS_CALLS     32
#define CFF_MAX_TRANS_ELEMENTS  32


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  CFF_Builder_
  {
    FT_Memory       memory;
    TT_Face         face;
    CFF_GlyphSlot   glyph;
    FT_GlyphLoader  loader;
    FT_Outline*     base;
    FT_Outline*     current;

    FT_Pos          pos_x;
    FT_Pos          pos_y;

    FT_Vector       left_bearing;
    FT_Vector       advance;

    FT_BBox         bbox;          
    FT_Bool         path_begun;
    FT_Bool         load_points;
    FT_Bool         no_recurse;

    FT_Bool         metrics_only;

    void*           hints_funcs;    
    void*           hints_globals;  

  } CFF_Builder;


  FT_LOCAL( FT_Error )
  cff_check_points( CFF_Builder*  builder,
                    FT_Int        count );

  FT_LOCAL( void )
  cff_builder_add_point( CFF_Builder*  builder,
                         FT_Pos        x,
                         FT_Pos        y,
                         FT_Byte       flag );
  FT_LOCAL( FT_Error )
  cff_builder_add_point1( CFF_Builder*  builder,
                          FT_Pos        x,
                          FT_Pos        y );
  FT_LOCAL( FT_Error )
  cff_builder_start_point( CFF_Builder*  builder,
                           FT_Pos        x,
                           FT_Pos        y );
  FT_LOCAL( void )
  cff_builder_close_contour( CFF_Builder*  builder );


  FT_LOCAL( FT_Int )
  cff_lookup_glyph_by_stdcharcode( CFF_Font  cff,
                                   FT_Int    charcode );
  FT_LOCAL( FT_Error )
  cff_get_glyph_data( TT_Face    face,
                      FT_UInt    glyph_index,
                      FT_Byte**  pointer,
                      FT_ULong*  length );
  FT_LOCAL( void )
  cff_free_glyph_data( TT_Face    face,
                       FT_Byte**  pointer,
                       FT_ULong   length );


  

  typedef struct  CFF_Decoder_Zone_
  {
    FT_Byte*  base;
    FT_Byte*  limit;
    FT_Byte*  cursor;

  } CFF_Decoder_Zone;


  typedef struct  CFF_Decoder_
  {
    CFF_Builder        builder;
    CFF_Font           cff;

    FT_Fixed           stack[CFF_MAX_OPERANDS + 1];
    FT_Fixed*          top;

    CFF_Decoder_Zone   zones[CFF_MAX_SUBRS_CALLS + 1];
    CFF_Decoder_Zone*  zone;

    FT_Int             flex_state;
    FT_Int             num_flex_vectors;
    FT_Vector          flex_vectors[7];

    FT_Pos             glyph_width;
    FT_Pos             nominal_width;

    FT_Bool            read_width;
    FT_Bool            width_only;
    FT_Int             num_hints;
    FT_Fixed           buildchar[CFF_MAX_TRANS_ELEMENTS];

    FT_UInt            num_locals;
    FT_UInt            num_globals;

    FT_Int             locals_bias;
    FT_Int             globals_bias;

    FT_Byte**          locals;
    FT_Byte**          globals;

    FT_Byte**          glyph_names;   
    FT_UInt            num_glyphs;    

    FT_Render_Mode     hint_mode;

    FT_Bool            seac;

    CFF_SubFont        current_subfont; 

  } CFF_Decoder;


  FT_LOCAL( void )
  cff_decoder_init( CFF_Decoder*    decoder,
                    TT_Face         face,
                    CFF_Size        size,
                    CFF_GlyphSlot   slot,
                    FT_Bool         hinting,
                    FT_Render_Mode  hint_mode );

  FT_LOCAL( FT_Error )
  cff_decoder_prepare( CFF_Decoder*  decoder,
                       CFF_Size      size,
                       FT_UInt       glyph_index );

#if 0  

  
  FT_LOCAL( FT_Error )
  cff_compute_max_advance( TT_Face  face,
                           FT_Int*  max_advance );

#endif 

#ifdef CFF_CONFIG_OPTION_OLD_ENGINE
  FT_LOCAL( FT_Error )
  cff_decoder_parse_charstrings( CFF_Decoder*  decoder,
                                 FT_Byte*      charstring_base,
                                 FT_ULong      charstring_len );
#endif

  FT_LOCAL( FT_Error )
  cff_slot_load( CFF_GlyphSlot  glyph,
                 CFF_Size       size,
                 FT_UInt        glyph_index,
                 FT_Int32       load_flags );


FT_END_HEADER

#endif 



