





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2font.h"
#include "cf2error.h"


#define CF2_MAX_SIZE  cf2_intToFixed( 2000 )    /* max ppem */


  





  static FT_Error
  cf2_checkTransform( const CF2_Matrix*  transform,
                      CF2_Int            unitsPerEm )
  {
    CF2_Fixed  maxScale;


    FT_ASSERT( unitsPerEm > 0 );

    FT_ASSERT( transform->a > 0 && transform->d > 0 );
    FT_ASSERT( transform->b == 0 && transform->c == 0 );
    FT_ASSERT( transform->tx == 0 && transform->ty == 0 );

    if ( unitsPerEm > 0x7FFF )
      return FT_THROW( Glyph_Too_Big );

    maxScale = FT_DivFix( CF2_MAX_SIZE, cf2_intToFixed( unitsPerEm ) );

    if ( transform->a > maxScale || transform->d > maxScale )
      return FT_THROW( Glyph_Too_Big );

    return FT_Err_Ok;
  }


  static void
  cf2_setGlyphWidth( CF2_Outline  outline,
                     CF2_Fixed    width )
  {
    CFF_Decoder*  decoder = outline->decoder;


    FT_ASSERT( decoder );

    decoder->glyph_width = cf2_fixedToInt( width );
  }


  
  static void
  cf2_free_instance( void*  ptr )
  {
    CF2_Font  font = (CF2_Font)ptr;


    if ( font )
    {
      FT_Memory  memory = font->memory;


      (void)memory;
    }
  }


  
  
  
  
  
  

  static void
  cf2_builder_moveTo( CF2_OutlineCallbacks      callbacks,
                      const CF2_CallbackParams  params )
  {
    
    CF2_Outline   outline = (CF2_Outline)callbacks;
    CFF_Builder*  builder;

    (void)params;        


    FT_ASSERT( outline && outline->decoder );
    FT_ASSERT( params->op == CF2_PathOpMoveTo );

    builder = &outline->decoder->builder;

    
    cff_builder_close_contour( builder );
    builder->path_begun = 0;
  }


  static void
  cf2_builder_lineTo( CF2_OutlineCallbacks      callbacks,
                      const CF2_CallbackParams  params )
  {
    
    CF2_Outline   outline = (CF2_Outline)callbacks;
    CFF_Builder*  builder;


    FT_ASSERT( outline && outline->decoder );
    FT_ASSERT( params->op == CF2_PathOpLineTo );

    builder = &outline->decoder->builder;

    if ( !builder->path_begun )
    {
      
      
      cff_builder_start_point( builder,
                               params->pt0.x,
                               params->pt0.y );
    }

    
    cff_builder_add_point1( builder,
                            params->pt1.x,
                            params->pt1.y );
  }


  static void
  cf2_builder_cubeTo( CF2_OutlineCallbacks      callbacks,
                      const CF2_CallbackParams  params )
  {
    
    CF2_Outline   outline = (CF2_Outline)callbacks;
    CFF_Builder*  builder;


    FT_ASSERT( outline && outline->decoder );
    FT_ASSERT( params->op == CF2_PathOpCubeTo );

    builder = &outline->decoder->builder;

    if ( !builder->path_begun )
    {
      
      
      cff_builder_start_point( builder,
                               params->pt0.x,
                               params->pt0.y );
    }

    
    cff_check_points( builder, 3 );

    cff_builder_add_point( builder,
                           params->pt1.x,
                           params->pt1.y, 0 );
    cff_builder_add_point( builder,
                           params->pt2.x,
                           params->pt2.y, 0 );
    cff_builder_add_point( builder,
                           params->pt3.x,
                           params->pt3.y, 1 );
  }


  static void
  cf2_outline_init( CF2_Outline  outline,
                    FT_Memory    memory,
                    FT_Error*    error )
  {
    FT_MEM_ZERO( outline, sizeof ( CF2_OutlineRec ) );

    outline->root.memory = memory;
    outline->root.error  = error;

    outline->root.moveTo = cf2_builder_moveTo;
    outline->root.lineTo = cf2_builder_lineTo;
    outline->root.cubeTo = cf2_builder_cubeTo;
  }


  
  static void
  cf2_getScaleAndHintFlag( CFF_Decoder*  decoder,
                           CF2_Fixed*    x_scale,
                           CF2_Fixed*    y_scale,
                           FT_Bool*      hinted,
                           FT_Bool*      scaled )
  {
    FT_ASSERT( decoder && decoder->builder.glyph );

    
    *hinted = decoder->builder.glyph->hint;
    *scaled = decoder->builder.glyph->scaled;

    if ( *hinted )
    {
      *x_scale = FT_DivFix( decoder->builder.glyph->x_scale,
                            cf2_intToFixed( 64 ) );
      *y_scale = FT_DivFix( decoder->builder.glyph->y_scale,
                            cf2_intToFixed( 64 ) );
    }
    else
    {
      
      

      *x_scale = 0x0400;   
      *y_scale = 0x0400;
    }
  }


  
  
  static FT_UShort
  cf2_getUnitsPerEm( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->builder.face );
    FT_ASSERT( decoder->builder.face->root.units_per_EM );

    return decoder->builder.face->root.units_per_EM;
  }


  
  FT_LOCAL_DEF( FT_Error )
  cf2_decoder_parse_charstrings( CFF_Decoder*  decoder,
                                 FT_Byte*      charstring_base,
                                 FT_ULong      charstring_len )
  {
    FT_Memory  memory;
    FT_Error   error = FT_Err_Ok;
    CF2_Font   font;


    FT_ASSERT( decoder && decoder->cff );

    memory = decoder->builder.memory;

    
    font = (CF2_Font)decoder->cff->cf2_instance.data;

    
    if ( decoder->cff->cf2_instance.data == NULL )
    {
      decoder->cff->cf2_instance.finalizer =
        (FT_Generic_Finalizer)cf2_free_instance;

      if ( FT_ALLOC( decoder->cff->cf2_instance.data,
                     sizeof ( CF2_FontRec ) ) )
        return FT_THROW( Out_Of_Memory );

      font = (CF2_Font)decoder->cff->cf2_instance.data;

      font->memory = memory;

      
      cf2_outline_init( &font->outline, font->memory, &font->error );
    }

    
    
    font->decoder         = decoder;
    font->outline.decoder = decoder;

    {
      

      CFF_Builder*  builder = &decoder->builder;
      CFF_Driver    driver  = (CFF_Driver)FT_FACE_DRIVER( builder->face );

      
      FT_Error       error2 = FT_Err_Ok;
      CF2_BufferRec  buf;
      CF2_Matrix     transform;
      CF2_F16Dot16   glyphWidth;

      FT_Bool  hinted;
      FT_Bool  scaled;


      
      
      FT_ASSERT( charstring_base + charstring_len >= charstring_base );

      FT_ZERO( &buf );
      buf.start =
      buf.ptr   = charstring_base;
      buf.end   = charstring_base + charstring_len;

      FT_ZERO( &transform );

      cf2_getScaleAndHintFlag( decoder,
                               &transform.a,
                               &transform.d,
                               &hinted,
                               &scaled );

      font->renderingFlags = 0;
      if ( hinted )
        font->renderingFlags |= CF2_FlagsHinted;
      if ( scaled && !driver->no_stem_darkening )
        font->renderingFlags |= CF2_FlagsDarkened;

      
      
      font->unitsPerEm = (CF2_Int)cf2_getUnitsPerEm( decoder );

      error2 = cf2_checkTransform( &transform, font->unitsPerEm );
      if ( error2 )
        return error2;

      error2 = cf2_getGlyphOutline( font, &buf, &transform, &glyphWidth );
      if ( error2 )
        return FT_ERR( Invalid_File_Format );

      cf2_setGlyphWidth( &font->outline, glyphWidth );

      return FT_Err_Ok;
    }
  }


  
  FT_LOCAL_DEF( CFF_SubFont )
  cf2_getSubfont( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    return decoder->current_subfont;
  }


  
  FT_LOCAL_DEF( CF2_Fixed )
  cf2_getPpemY( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder                          &&
               decoder->builder.face            &&
               decoder->builder.face->root.size );
    FT_ASSERT( decoder->builder.face->root.size->metrics.y_ppem );

    return cf2_intToFixed(
             decoder->builder.face->root.size->metrics.y_ppem );
  }


  
  
  
  FT_LOCAL_DEF( CF2_Fixed )
  cf2_getStdVW( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    return cf2_intToFixed(
             decoder->current_subfont->private_dict.standard_height );
  }


  FT_LOCAL_DEF( CF2_Fixed )
  cf2_getStdHW( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    return cf2_intToFixed(
             decoder->current_subfont->private_dict.standard_width );
  }


  
  FT_LOCAL_DEF( void )
  cf2_getBlueMetrics( CFF_Decoder*  decoder,
                      CF2_Fixed*    blueScale,
                      CF2_Fixed*    blueShift,
                      CF2_Fixed*    blueFuzz )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    *blueScale = FT_DivFix(
                   decoder->current_subfont->private_dict.blue_scale,
                   cf2_intToFixed( 1000 ) );
    *blueShift = cf2_intToFixed(
                   decoder->current_subfont->private_dict.blue_shift );
    *blueFuzz  = cf2_intToFixed(
                   decoder->current_subfont->private_dict.blue_fuzz );
  }


  
  
  FT_LOCAL_DEF( void )
  cf2_getBlueValues( CFF_Decoder*  decoder,
                     size_t*       count,
                     FT_Pos*      *data )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    *count = decoder->current_subfont->private_dict.num_blue_values;
    *data  = (FT_Pos*)
               &decoder->current_subfont->private_dict.blue_values;
  }


  FT_LOCAL_DEF( void )
  cf2_getOtherBlues( CFF_Decoder*  decoder,
                     size_t*       count,
                     FT_Pos*      *data )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    *count = decoder->current_subfont->private_dict.num_other_blues;
    *data  = (FT_Pos*)
               &decoder->current_subfont->private_dict.other_blues;
  }


  FT_LOCAL_DEF( void )
  cf2_getFamilyBlues( CFF_Decoder*  decoder,
                      size_t*       count,
                      FT_Pos*      *data )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    *count = decoder->current_subfont->private_dict.num_family_blues;
    *data  = (FT_Pos*)
               &decoder->current_subfont->private_dict.family_blues;
  }


  FT_LOCAL_DEF( void )
  cf2_getFamilyOtherBlues( CFF_Decoder*  decoder,
                           size_t*       count,
                           FT_Pos*      *data )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    *count = decoder->current_subfont->private_dict.num_family_other_blues;
    *data  = (FT_Pos*)
               &decoder->current_subfont->private_dict.family_other_blues;
  }


  FT_LOCAL_DEF( CF2_Int )
  cf2_getLanguageGroup( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    return decoder->current_subfont->private_dict.language_group;
  }


  
  
  FT_LOCAL_DEF( CF2_Int )
  cf2_initGlobalRegionBuffer( CFF_Decoder*  decoder,
                              CF2_UInt      idx,
                              CF2_Buffer    buf )
  {
    FT_ASSERT( decoder && decoder->globals );

    FT_ZERO( buf );

    idx += decoder->globals_bias;
    if ( idx >= decoder->num_globals )
      return TRUE;     

    buf->start =
    buf->ptr   = decoder->globals[idx];
    buf->end   = decoder->globals[idx + 1];

    return FALSE;      
  }


  
  
  FT_LOCAL_DEF( FT_Error )
  cf2_getSeacComponent( CFF_Decoder*  decoder,
                        CF2_UInt      code,
                        CF2_Buffer    buf )
  {
    CF2_Int   gid;
    FT_Byte*  charstring;
    FT_ULong  len;
    FT_Error  error;


    FT_ASSERT( decoder );

    FT_ZERO( buf );

    gid = cff_lookup_glyph_by_stdcharcode( decoder->cff, code );
    if ( gid < 0 )
      return FT_THROW( Invalid_Glyph_Format );

    error = cff_get_glyph_data( decoder->builder.face,
                                gid,
                                &charstring,
                                &len );
    
    if ( error )
      return error;

    
    FT_ASSERT( charstring + len >= charstring );

    buf->start = charstring;
    buf->end   = charstring + len;
    buf->ptr   = buf->start;

    return FT_Err_Ok;
  }


  FT_LOCAL_DEF( void )
  cf2_freeSeacComponent( CFF_Decoder*  decoder,
                         CF2_Buffer    buf )
  {
    FT_ASSERT( decoder );

    cff_free_glyph_data( decoder->builder.face,
                         (FT_Byte**)&buf->start,
                         (FT_ULong)( buf->end - buf->start ) );
  }


  FT_LOCAL_DEF( CF2_Int )
  cf2_initLocalRegionBuffer( CFF_Decoder*  decoder,
                             CF2_UInt      idx,
                             CF2_Buffer    buf )
  {
    FT_ASSERT( decoder && decoder->locals );

    FT_ZERO( buf );

    idx += decoder->locals_bias;
    if ( idx >= decoder->num_locals )
      return TRUE;     

    buf->start =
    buf->ptr   = decoder->locals[idx];
    buf->end   = decoder->locals[idx + 1];

    return FALSE;      
  }


  FT_LOCAL_DEF( CF2_Fixed )
  cf2_getDefaultWidthX( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    return cf2_intToFixed(
             decoder->current_subfont->private_dict.default_width );
  }


  FT_LOCAL_DEF( CF2_Fixed )
  cf2_getNominalWidthX( CFF_Decoder*  decoder )
  {
    FT_ASSERT( decoder && decoder->current_subfont );

    return cf2_intToFixed(
             decoder->current_subfont->private_dict.nominal_width );
  }


  FT_LOCAL_DEF( void )
  cf2_outline_reset( CF2_Outline  outline )
  {
    CFF_Decoder*  decoder = outline->decoder;


    FT_ASSERT( decoder );

    outline->root.windingMomentum = 0;

    FT_GlyphLoader_Rewind( decoder->builder.loader );
  }


  FT_LOCAL_DEF( void )
  cf2_outline_close( CF2_Outline  outline )
  {
    CFF_Decoder*  decoder = outline->decoder;


    FT_ASSERT( decoder );

    cff_builder_close_contour( &decoder->builder );

    FT_GlyphLoader_Add( decoder->builder.loader );
  }



