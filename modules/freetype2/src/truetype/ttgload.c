

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_TAGS_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_DRIVER_H

#include "ttgload.h"
#include "ttpload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"
#include "ttsubpix.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttgload


  
  
  
  
#define ARGS_ARE_WORDS             0x0001
#define ARGS_ARE_XY_VALUES         0x0002
#define ROUND_XY_TO_GRID           0x0004
#define WE_HAVE_A_SCALE            0x0008

#define MORE_COMPONENTS            0x0020
#define WE_HAVE_AN_XY_SCALE        0x0040
#define WE_HAVE_A_2X2              0x0080
#define WE_HAVE_INSTR              0x0100
#define USE_MY_METRICS             0x0200
#define OVERLAP_COMPOUND           0x0400
#define SCALED_COMPONENT_OFFSET    0x0800
#define UNSCALED_COMPONENT_OFFSET  0x1000


  
  
  
  
  FT_LOCAL_DEF( void )
  TT_Get_HMetrics( TT_Face     face,
                   FT_UInt     idx,
                   FT_Short*   lsb,
                   FT_UShort*  aw )
  {
    ( (SFNT_Service)face->sfnt )->get_metrics( face, 0, idx, lsb, aw );

    FT_TRACE5(( "  advance width (font units): %d\n", *aw ));
    FT_TRACE5(( "  left side bearing (font units): %d\n", *lsb ));
  }


  
  
  
  
  
  FT_LOCAL_DEF( void )
  TT_Get_VMetrics( TT_Face     face,
                   FT_UInt     idx,
                   FT_Pos      yMax,
                   FT_Short*   tsb,
                   FT_UShort*  ah )
  {
    if ( face->vertical_info )
      ( (SFNT_Service)face->sfnt )->get_metrics( face, 1, idx, tsb, ah );

    else if ( face->os2.version != 0xFFFFU )
    {
      *tsb = (FT_Short)( face->os2.sTypoAscender - yMax );
      *ah  = face->os2.sTypoAscender - face->os2.sTypoDescender;
    }

    else
    {
      *tsb = (FT_Short)( face->horizontal.Ascender - yMax );
      *ah  = face->horizontal.Ascender - face->horizontal.Descender;
    }

    FT_TRACE5(( "  advance height (font units): %d\n", *ah ));
    FT_TRACE5(( "  top side bearing (font units): %d\n", *tsb ));
  }


  static FT_Error
  tt_get_metrics( TT_Loader  loader,
                  FT_UInt    glyph_index )
  {
    TT_Face    face   = (TT_Face)loader->face;
#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    TT_Driver  driver = (TT_Driver)FT_FACE_DRIVER( face );
#endif

    FT_Error   error;
    FT_Stream  stream = loader->stream;

    FT_Short   left_bearing = 0, top_bearing = 0;
    FT_UShort  advance_width = 0, advance_height = 0;

    
    
    FT_ULong  pos = FT_STREAM_POS();


    TT_Get_HMetrics( face, glyph_index,
                     &left_bearing,
                     &advance_width );
    TT_Get_VMetrics( face, glyph_index,
                     loader->bbox.yMax,
                     &top_bearing,
                     &advance_height );

    if ( FT_STREAM_SEEK( pos ) )
      return error;

    loader->left_bearing = left_bearing;
    loader->advance      = advance_width;
    loader->top_bearing  = top_bearing;
    loader->vadvance     = advance_height;

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    if ( driver->interpreter_version == TT_INTERPRETER_VERSION_38 )
    {
      if ( loader->exec )
        loader->exec->sph_tweak_flags = 0;

      
      if ( loader->exec && loader->exec->ignore_x_mode )
        sph_set_tweaks( loader, glyph_index );
    }
#endif 

    if ( !loader->linear_def )
    {
      loader->linear_def = 1;
      loader->linear     = advance_width;
    }

    return FT_Err_Ok;
  }


#ifdef FT_CONFIG_OPTION_INCREMENTAL

  static void
  tt_get_metrics_incr_overrides( TT_Loader  loader,
                                 FT_UInt    glyph_index )
  {
    TT_Face  face = (TT_Face)loader->face;

    FT_Short   left_bearing = 0, top_bearing = 0;
    FT_UShort  advance_width = 0, advance_height = 0;


    
    
    if ( face->root.internal->incremental_interface                           &&
         face->root.internal->incremental_interface->funcs->get_glyph_metrics )
    {
      FT_Incremental_MetricsRec  metrics;
      FT_Error                   error;


      metrics.bearing_x = loader->left_bearing;
      metrics.bearing_y = 0;
      metrics.advance   = loader->advance;
      metrics.advance_v = 0;

      error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                face->root.internal->incremental_interface->object,
                glyph_index, FALSE, &metrics );
      if ( error )
        goto Exit;

      left_bearing  = (FT_Short)metrics.bearing_x;
      advance_width = (FT_UShort)metrics.advance;

#if 0

      
      metrics.bearing_x = 0;
      metrics.bearing_y = loader->top_bearing;
      metrics.advance   = loader->vadvance;

      error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                face->root.internal->incremental_interface->object,
                glyph_index, TRUE, &metrics );
      if ( error )
        goto Exit;

      top_bearing    = (FT_Short)metrics.bearing_y;
      advance_height = (FT_UShort)metrics.advance;

#endif 

      loader->left_bearing = left_bearing;
      loader->advance      = advance_width;
      loader->top_bearing  = top_bearing;
      loader->vadvance     = advance_height;

      if ( !loader->linear_def )
      {
        loader->linear_def = 1;
        loader->linear     = advance_width;
      }
    }

  Exit:
    return;
  }

#endif 


  
  
  
  
  static void
  translate_array( FT_UInt     n,
                   FT_Vector*  coords,
                   FT_Pos      delta_x,
                   FT_Pos      delta_y )
  {
    FT_UInt  k;


    if ( delta_x )
      for ( k = 0; k < n; k++ )
        coords[k].x += delta_x;

    if ( delta_y )
      for ( k = 0; k < n; k++ )
        coords[k].y += delta_y;
  }


  
  
  
  
  
  
  

  FT_CALLBACK_DEF( FT_Error )
  TT_Access_Glyph_Frame( TT_Loader  loader,
                         FT_UInt    glyph_index,
                         FT_ULong   offset,
                         FT_UInt    byte_count )
  {
    FT_Error   error;
    FT_Stream  stream = loader->stream;

    
    FT_UNUSED( glyph_index );


    FT_TRACE4(( "Glyph %ld\n", glyph_index ));

    
    if ( FT_STREAM_SEEK( offset ) || FT_FRAME_ENTER( byte_count ) )
      return error;

    loader->cursor = stream->cursor;
    loader->limit  = stream->limit;

    return FT_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  TT_Forget_Glyph_Frame( TT_Loader  loader )
  {
    FT_Stream  stream = loader->stream;


    FT_FRAME_EXIT();
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Glyph_Header( TT_Loader  loader )
  {
    FT_Byte*  p     = loader->cursor;
    FT_Byte*  limit = loader->limit;


    if ( p + 10 > limit )
      return FT_THROW( Invalid_Outline );

    loader->n_contours = FT_NEXT_SHORT( p );

    loader->bbox.xMin = FT_NEXT_SHORT( p );
    loader->bbox.yMin = FT_NEXT_SHORT( p );
    loader->bbox.xMax = FT_NEXT_SHORT( p );
    loader->bbox.yMax = FT_NEXT_SHORT( p );

    FT_TRACE5(( "  # of contours: %d\n", loader->n_contours ));
    FT_TRACE5(( "  xMin: %4d  xMax: %4d\n", loader->bbox.xMin,
                                            loader->bbox.xMax ));
    FT_TRACE5(( "  yMin: %4d  yMax: %4d\n", loader->bbox.yMin,
                                            loader->bbox.yMax ));
    loader->cursor = p;

    return FT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Simple_Glyph( TT_Loader  load )
  {
    FT_Error        error;
    FT_Byte*        p          = load->cursor;
    FT_Byte*        limit      = load->limit;
    FT_GlyphLoader  gloader    = load->gloader;
    FT_Int          n_contours = load->n_contours;
    FT_Outline*     outline;
    FT_UShort       n_ins;
    FT_Int          n_points;
    FT_ULong        tmp;

    FT_Byte         *flag, *flag_limit;
    FT_Byte         c, count;
    FT_Vector       *vec, *vec_limit;
    FT_Pos          x;
    FT_Short        *cont, *cont_limit, prev_cont;
    FT_Int          xy_size = 0;


    
    error = FT_GLYPHLOADER_CHECK_POINTS( gloader, 0, n_contours );
    if ( error )
      goto Fail;

    
    cont       = gloader->current.outline.contours;
    cont_limit = cont + n_contours;

    
    if ( n_contours >= 0xFFF || p + ( n_contours + 1 ) * 2 > limit )
      goto Invalid_Outline;

    prev_cont = FT_NEXT_SHORT( p );

    if ( n_contours > 0 )
      cont[0] = prev_cont;

    if ( prev_cont < 0 )
      goto Invalid_Outline;

    for ( cont++; cont < cont_limit; cont++ )
    {
      cont[0] = FT_NEXT_SHORT( p );
      if ( cont[0] <= prev_cont )
      {
        
        goto Invalid_Outline;
      }
      prev_cont = cont[0];
    }

    n_points = 0;
    if ( n_contours > 0 )
    {
      n_points = cont[-1] + 1;
      if ( n_points < 0 )
        goto Invalid_Outline;
    }

    
    error = FT_GLYPHLOADER_CHECK_POINTS( gloader, n_points + 4, 0 );
    if ( error )
      goto Fail;

    
    load->glyph->control_len  = 0;
    load->glyph->control_data = 0;

    if ( p + 2 > limit )
      goto Invalid_Outline;

    n_ins = FT_NEXT_USHORT( p );

    FT_TRACE5(( "  Instructions size: %u\n", n_ins ));

    
    if ( ( limit - p ) < n_ins )
    {
      FT_TRACE0(( "TT_Load_Simple_Glyph: instruction count mismatch\n" ));
      error = FT_THROW( Too_Many_Hints );
      goto Fail;
    }

#ifdef TT_USE_BYTECODE_INTERPRETER

    if ( IS_HINTED( load->load_flags ) )
    {
      
      

      tmp   = load->exec->glyphSize;
      error = Update_Max( load->exec->memory,
                          &tmp,
                          sizeof ( FT_Byte ),
                          (void*)&load->exec->glyphIns,
                          n_ins );

      load->exec->glyphSize = (FT_UShort)tmp;
      if ( error )
        return error;

      load->glyph->control_len  = n_ins;
      load->glyph->control_data = load->exec->glyphIns;

      FT_MEM_COPY( load->exec->glyphIns, p, (FT_Long)n_ins );
    }

#endif 

    p += n_ins;

    outline = &gloader->current.outline;

    
    flag       = (FT_Byte*)outline->tags;
    flag_limit = flag + n_points;

    FT_ASSERT( flag != NULL );

    while ( flag < flag_limit )
    {
      if ( p + 1 > limit )
        goto Invalid_Outline;

      *flag++ = c = FT_NEXT_BYTE( p );
      if ( c & 8 )
      {
        if ( p + 1 > limit )
          goto Invalid_Outline;

        count = FT_NEXT_BYTE( p );
        if ( flag + (FT_Int)count > flag_limit )
          goto Invalid_Outline;

        for ( ; count > 0; count-- )
          *flag++ = c;
      }
    }

    

    vec       = outline->points;
    vec_limit = vec + n_points;
    flag      = (FT_Byte*)outline->tags;
    x         = 0;

    if ( p + xy_size > limit )
      goto Invalid_Outline;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos   y = 0;
      FT_Byte  f = *flag;


      if ( f & 2 )
      {
        if ( p + 1 > limit )
          goto Invalid_Outline;

        y = (FT_Pos)FT_NEXT_BYTE( p );
        if ( ( f & 16 ) == 0 )
          y = -y;
      }
      else if ( ( f & 16 ) == 0 )
      {
        if ( p + 2 > limit )
          goto Invalid_Outline;

        y = (FT_Pos)FT_NEXT_SHORT( p );
      }

      x     += y;
      vec->x = x;
      
      *flag  = (FT_Byte)( f & ~( 2 | 16 ) );
    }

    

    vec       = gloader->current.outline.points;
    vec_limit = vec + n_points;
    flag      = (FT_Byte*)outline->tags;
    x         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos   y = 0;
      FT_Byte  f = *flag;


      if ( f & 4 )
      {
        if ( p + 1 > limit )
          goto Invalid_Outline;

        y = (FT_Pos)FT_NEXT_BYTE( p );
        if ( ( f & 32 ) == 0 )
          y = -y;
      }
      else if ( ( f & 32 ) == 0 )
      {
        if ( p + 2 > limit )
          goto Invalid_Outline;

        y = (FT_Pos)FT_NEXT_SHORT( p );
      }

      x     += y;
      vec->y = x;
      
      *flag  = (FT_Byte)( f & FT_CURVE_TAG_ON );
    }

    outline->n_points   = (FT_UShort)n_points;
    outline->n_contours = (FT_Short) n_contours;

    load->cursor = p;

  Fail:
    return error;

  Invalid_Outline:
    error = FT_THROW( Invalid_Outline );
    goto Fail;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Composite_Glyph( TT_Loader  loader )
  {
    FT_Error        error;
    FT_Byte*        p       = loader->cursor;
    FT_Byte*        limit   = loader->limit;
    FT_GlyphLoader  gloader = loader->gloader;
    FT_SubGlyph     subglyph;
    FT_UInt         num_subglyphs;


    num_subglyphs = 0;

    do
    {
      FT_Fixed  xx, xy, yy, yx;
      FT_UInt   count;


      
      error = FT_GlyphLoader_CheckSubGlyphs( gloader, num_subglyphs + 1 );
      if ( error )
        goto Fail;

      
      if ( p + 4 > limit )
        goto Invalid_Composite;

      subglyph = gloader->current.subglyphs + num_subglyphs;

      subglyph->arg1 = subglyph->arg2 = 0;

      subglyph->flags = FT_NEXT_USHORT( p );
      subglyph->index = FT_NEXT_USHORT( p );

      
      count = 2;
      if ( subglyph->flags & ARGS_ARE_WORDS )
        count += 2;
      if ( subglyph->flags & WE_HAVE_A_SCALE )
        count += 2;
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
        count += 4;
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
        count += 8;

      if ( p + count > limit )
        goto Invalid_Composite;

      
      if ( subglyph->flags & ARGS_ARE_WORDS )
      {
        subglyph->arg1 = FT_NEXT_SHORT( p );
        subglyph->arg2 = FT_NEXT_SHORT( p );
      }
      else
      {
        subglyph->arg1 = FT_NEXT_CHAR( p );
        subglyph->arg2 = FT_NEXT_CHAR( p );
      }

      
      xx = yy = 0x10000L;
      xy = yx = 0;

      if ( subglyph->flags & WE_HAVE_A_SCALE )
      {
        xx = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
        yy = xx;
      }
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
      {
        xx = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
        yy = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
      }
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
      {
        xx = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
        yx = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
        xy = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
        yy = (FT_Fixed)FT_NEXT_SHORT( p ) << 2;
      }

      subglyph->transform.xx = xx;
      subglyph->transform.xy = xy;
      subglyph->transform.yx = yx;
      subglyph->transform.yy = yy;

      num_subglyphs++;

    } while ( subglyph->flags & MORE_COMPONENTS );

    gloader->current.num_subglyphs = num_subglyphs;

#ifdef TT_USE_BYTECODE_INTERPRETER

    {
      FT_Stream  stream = loader->stream;


      
      
      
      
      loader->ins_pos = (FT_ULong)( FT_STREAM_POS() +
                                    p - limit );
    }

#endif

    loader->cursor = p;

  Fail:
    return error;

  Invalid_Composite:
    error = FT_THROW( Invalid_Composite );
    goto Fail;
  }


  FT_LOCAL_DEF( void )
  TT_Init_Glyph_Loading( TT_Face  face )
  {
    face->access_glyph_frame   = TT_Access_Glyph_Frame;
    face->read_glyph_header    = TT_Load_Glyph_Header;
    face->read_simple_glyph    = TT_Load_Simple_Glyph;
    face->read_composite_glyph = TT_Load_Composite_Glyph;
    face->forget_glyph_frame   = TT_Forget_Glyph_Frame;
  }


  static void
  tt_prepare_zone( TT_GlyphZone  zone,
                   FT_GlyphLoad  load,
                   FT_UInt       start_point,
                   FT_UInt       start_contour )
  {
    zone->n_points    = (FT_UShort)( load->outline.n_points - start_point );
    zone->n_contours  = (FT_Short) ( load->outline.n_contours -
                                       start_contour );
    zone->org         = load->extra_points + start_point;
    zone->cur         = load->outline.points + start_point;
    zone->orus        = load->extra_points2 + start_point;
    zone->tags        = (FT_Byte*)load->outline.tags + start_point;
    zone->contours    = (FT_UShort*)load->outline.contours + start_contour;
    zone->first_point = (FT_UShort)start_point;
  }


  
  
  
  
  
  
  
  
  
  static FT_Error
  TT_Hint_Glyph( TT_Loader  loader,
                 FT_Bool    is_composite )
  {
#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    TT_Face    face   = (TT_Face)loader->face;
    TT_Driver  driver = (TT_Driver)FT_FACE_DRIVER( face );
#endif

    TT_GlyphZone  zone = &loader->zone;

#ifdef TT_USE_BYTECODE_INTERPRETER
    FT_UInt       n_ins;
#else
    FT_UNUSED( is_composite );
#endif


#ifdef TT_USE_BYTECODE_INTERPRETER
    if ( loader->glyph->control_len > 0xFFFFL )
    {
      FT_TRACE1(( "TT_Hint_Glyph: too long instructions" ));
      FT_TRACE1(( " (0x%lx byte) is truncated\n",
                 loader->glyph->control_len ));
    }
    n_ins = (FT_UInt)( loader->glyph->control_len );

    
    if ( n_ins > 0 )
      FT_ARRAY_COPY( zone->org, zone->cur, zone->n_points );

    
    loader->exec->GS = ((TT_Size)loader->size)->GS;

    
    
    if ( is_composite )
    {
      loader->exec->metrics.x_scale = 1 << 16;
      loader->exec->metrics.y_scale = 1 << 16;

      FT_ARRAY_COPY( zone->orus, zone->cur, zone->n_points );
    }
    else
    {
      loader->exec->metrics.x_scale =
        ((TT_Size)loader->size)->metrics.x_scale;
      loader->exec->metrics.y_scale =
        ((TT_Size)loader->size)->metrics.y_scale;
    }
#endif

    
    zone->cur[zone->n_points - 4].x =
      FT_PIX_ROUND( zone->cur[zone->n_points - 4].x );
    zone->cur[zone->n_points - 3].x =
      FT_PIX_ROUND( zone->cur[zone->n_points - 3].x );
    zone->cur[zone->n_points - 2].y =
      FT_PIX_ROUND( zone->cur[zone->n_points - 2].y );
    zone->cur[zone->n_points - 1].y =
      FT_PIX_ROUND( zone->cur[zone->n_points - 1].y );

#ifdef TT_USE_BYTECODE_INTERPRETER

    if ( n_ins > 0 )
    {
      FT_Bool   debug;
      FT_Error  error;

      FT_GlyphLoader  gloader         = loader->gloader;
      FT_Outline      current_outline = gloader->current.outline;


      TT_Set_CodeRange( loader->exec, tt_coderange_glyph,
                        loader->exec->glyphIns, n_ins );

      loader->exec->is_composite = is_composite;
      loader->exec->pts          = *zone;

      debug = FT_BOOL( !( loader->load_flags & FT_LOAD_NO_SCALE ) &&
                       ((TT_Size)loader->size)->debug             );

      error = TT_Run_Context( loader->exec, debug );
      if ( error && loader->exec->pedantic_hinting )
        return error;

      
      current_outline.tags[0] |=
        ( loader->exec->GS.scan_type << 5 ) | FT_CURVE_TAG_HAS_SCANMODE;
    }

#endif

    
    loader->pp1 = zone->cur[zone->n_points - 4];
    loader->pp2 = zone->cur[zone->n_points - 3];
    loader->pp3 = zone->cur[zone->n_points - 2];
    loader->pp4 = zone->cur[zone->n_points - 1];

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    if ( driver->interpreter_version == TT_INTERPRETER_VERSION_38 )
    {
      if ( loader->exec->sph_tweak_flags & SPH_TWEAK_DEEMBOLDEN )
        FT_Outline_EmboldenXY( &loader->gloader->current.outline, -24, 0 );

      else if ( loader->exec->sph_tweak_flags & SPH_TWEAK_EMBOLDEN )
        FT_Outline_EmboldenXY( &loader->gloader->current.outline, 24, 0 );
    }
#endif 

    return FT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  static FT_Error
  TT_Process_Simple_Glyph( TT_Loader  loader )
  {
    FT_GlyphLoader  gloader = loader->gloader;
    FT_Error        error   = FT_Err_Ok;
    FT_Outline*     outline;
    FT_Int          n_points;


    outline  = &gloader->current.outline;
    n_points = outline->n_points;

    

    outline->points[n_points    ] = loader->pp1;
    outline->points[n_points + 1] = loader->pp2;
    outline->points[n_points + 2] = loader->pp3;
    outline->points[n_points + 3] = loader->pp4;

    outline->tags[n_points    ] = 0;
    outline->tags[n_points + 1] = 0;
    outline->tags[n_points + 2] = 0;
    outline->tags[n_points + 3] = 0;

    n_points += 4;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

    if ( ((TT_Face)loader->face)->doblend )
    {
      
      FT_Vector*  deltas;
      FT_Memory   memory = loader->face->memory;
      FT_Int      i;


      error = TT_Vary_Get_Glyph_Deltas( (TT_Face)(loader->face),
                                        loader->glyph_index,
                                        &deltas,
                                        n_points );
      if ( error )
        return error;

      for ( i = 0; i < n_points; ++i )
      {
        outline->points[i].x += deltas[i].x;
        outline->points[i].y += deltas[i].y;
      }

      FT_FREE( deltas );
    }

#endif 

    if ( IS_HINTED( loader->load_flags ) )
    {
      tt_prepare_zone( &loader->zone, &gloader->current, 0, 0 );

      FT_ARRAY_COPY( loader->zone.orus, loader->zone.cur,
                     loader->zone.n_points + 4 );
    }

    {
#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
      TT_Face    face   = (TT_Face)loader->face;
      TT_Driver  driver = (TT_Driver)FT_FACE_DRIVER( face );

      FT_String*  family         = face->root.family_name;
      FT_Int      ppem           = loader->size->metrics.x_ppem;
      FT_String*  style          = face->root.style_name;
      FT_Int      x_scale_factor = 1000;
#endif

      FT_Vector*  vec   = outline->points;
      FT_Vector*  limit = outline->points + n_points;

      FT_Fixed  x_scale = 0; 
      FT_Fixed  y_scale = 0;

      FT_Bool  do_scale = FALSE;


#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING

      if ( driver->interpreter_version == TT_INTERPRETER_VERSION_38 )
      {
        
        if ( IS_HINTED( loader->load_flags ) )
          x_scale_factor = sph_test_tweak_x_scaling( face,
                                                     family,
                                                     ppem,
                                                     style,
                                                     loader->glyph_index );
        
        if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 ||
             x_scale_factor != 1000                         )
        {
          x_scale = FT_MulDiv( ((TT_Size)loader->size)->metrics.x_scale,
                               x_scale_factor, 1000 );
          y_scale = ((TT_Size)loader->size)->metrics.y_scale;

          
          
          if ( x_scale_factor != 1000 && ppem > 11 )
            FT_Outline_EmboldenXY( outline,
                                   FT_MulFix( 1280 * ppem,
                                              1000 - x_scale_factor ),
                                   0 );
          do_scale = TRUE;
        }
      }
      else

#endif 

      {
        
        if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          x_scale = ((TT_Size)loader->size)->metrics.x_scale;
          y_scale = ((TT_Size)loader->size)->metrics.y_scale;

          do_scale = TRUE;
        }
      }

      if ( do_scale )
      {
        for ( ; vec < limit; vec++ )
        {
          vec->x = FT_MulFix( vec->x, x_scale );
          vec->y = FT_MulFix( vec->y, y_scale );
        }

        loader->pp1 = outline->points[n_points - 4];
        loader->pp2 = outline->points[n_points - 3];
        loader->pp3 = outline->points[n_points - 2];
        loader->pp4 = outline->points[n_points - 1];
      }
    }

    if ( IS_HINTED( loader->load_flags ) )
    {
      loader->zone.n_points += 4;

      error = TT_Hint_Glyph( loader, 0 );
    }

    return error;
  }


  
  
  
  
  
  
  
  
  
  static FT_Error
  TT_Process_Composite_Component( TT_Loader    loader,
                                  FT_SubGlyph  subglyph,
                                  FT_UInt      start_point,
                                  FT_UInt      num_base_points )
  {
    FT_GlyphLoader  gloader    = loader->gloader;
    FT_Vector*      base_vec   = gloader->base.outline.points;
    FT_UInt         num_points = gloader->base.outline.n_points;
    FT_Bool         have_scale;
    FT_Pos          x, y;


    have_scale = FT_BOOL( subglyph->flags & ( WE_HAVE_A_SCALE     |
                                              WE_HAVE_AN_XY_SCALE |
                                              WE_HAVE_A_2X2       ) );

    
    if ( have_scale )
    {
      FT_UInt  i;


      for ( i = num_base_points; i < num_points; i++ )
        FT_Vector_Transform( base_vec + i, &subglyph->transform );
    }

    
    if ( !( subglyph->flags & ARGS_ARE_XY_VALUES ) )
    {
      FT_UInt     k = subglyph->arg1;
      FT_UInt     l = subglyph->arg2;
      FT_Vector*  p1;
      FT_Vector*  p2;


      
      

      
      k += start_point;
      l += num_base_points;
      if ( k >= num_base_points ||
           l >= num_points      )
        return FT_THROW( Invalid_Composite );

      p1 = gloader->base.outline.points + k;
      p2 = gloader->base.outline.points + l;

      x = p1->x - p2->x;
      y = p1->y - p2->y;
    }
    else
    {
      x = subglyph->arg1;
      y = subglyph->arg2;

      if ( !x && !y )
        return FT_Err_Ok;

      
      
      

      if ( have_scale &&
#ifdef TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED
           !( subglyph->flags & UNSCALED_COMPONENT_OFFSET ) )
#else
            ( subglyph->flags & SCALED_COMPONENT_OFFSET ) )
#endif
      {

#if 0

        
        
        
        
        int  a = subglyph->transform.xx > 0 ?  subglyph->transform.xx
                                            : -subglyph->transform.xx;
        int  b = subglyph->transform.yx > 0 ?  subglyph->transform.yx
                                            : -subglyph->transform.yx;
        int  c = subglyph->transform.xy > 0 ?  subglyph->transform.xy
                                            : -subglyph->transform.xy;
        int  d = subglyph->transform.yy > 0 ? subglyph->transform.yy
                                            : -subglyph->transform.yy;
        int  m = a > b ? a : b;
        int  n = c > d ? c : d;


        if ( a - b <= 33 && a - b >= -33 )
          m *= 2;
        if ( c - d <= 33 && c - d >= -33 )
          n *= 2;
        x = FT_MulFix( x, m );
        y = FT_MulFix( y, n );

#else 

        
        
        
        
        FT_Fixed  mac_xscale = FT_Hypot( subglyph->transform.xx,
                                         subglyph->transform.xy );
        FT_Fixed  mac_yscale = FT_Hypot( subglyph->transform.yy,
                                         subglyph->transform.yx );


        x = FT_MulFix( x, mac_xscale );
        y = FT_MulFix( y, mac_yscale );

#endif 

      }

      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        FT_Fixed  x_scale = ((TT_Size)loader->size)->metrics.x_scale;
        FT_Fixed  y_scale = ((TT_Size)loader->size)->metrics.y_scale;


        x = FT_MulFix( x, x_scale );
        y = FT_MulFix( y, y_scale );

        if ( subglyph->flags & ROUND_XY_TO_GRID )
        {
          x = FT_PIX_ROUND( x );
          y = FT_PIX_ROUND( y );
        }
      }
    }

    if ( x || y )
      translate_array( num_points - num_base_points,
                       base_vec + num_base_points,
                       x, y );

    return FT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  static FT_Error
  TT_Process_Composite_Glyph( TT_Loader  loader,
                              FT_UInt    start_point,
                              FT_UInt    start_contour )
  {
    FT_Error     error;
    FT_Outline*  outline;
    FT_UInt      i;


    outline = &loader->gloader->base.outline;

    
    error = FT_GLYPHLOADER_CHECK_POINTS( loader->gloader,
                                         outline->n_points + 4,
                                         0 );
    if ( error )
      return error;

    outline->points[outline->n_points    ] = loader->pp1;
    outline->points[outline->n_points + 1] = loader->pp2;
    outline->points[outline->n_points + 2] = loader->pp3;
    outline->points[outline->n_points + 3] = loader->pp4;

    outline->tags[outline->n_points    ] = 0;
    outline->tags[outline->n_points + 1] = 0;
    outline->tags[outline->n_points + 2] = 0;
    outline->tags[outline->n_points + 3] = 0;

#ifdef TT_USE_BYTECODE_INTERPRETER

    {
      FT_Stream  stream = loader->stream;
      FT_UShort  n_ins, max_ins;
      FT_ULong   tmp;


      
      
      if ( FT_STREAM_SEEK( loader->ins_pos ) ||
           FT_READ_USHORT( n_ins )           )
        return error;

      FT_TRACE5(( "  Instructions size = %d\n", n_ins ));

      
      max_ins = ((TT_Face)loader->face)->max_profile.maxSizeOfInstructions;
      if ( n_ins > max_ins )
      {
        
        
        if ( (FT_Int)n_ins > loader->byte_len )
        {
          FT_TRACE1(( "TT_Process_Composite_Glyph:"
                      " too many instructions (%d) for glyph with length %d\n",
                      n_ins, loader->byte_len ));
          return FT_THROW( Too_Many_Hints );
        }

        tmp   = loader->exec->glyphSize;
        error = Update_Max( loader->exec->memory,
                            &tmp,
                            sizeof ( FT_Byte ),
                            (void*)&loader->exec->glyphIns,
                            n_ins );

        loader->exec->glyphSize = (FT_UShort)tmp;
        if ( error )
          return error;
      }
      else if ( n_ins == 0 )
        return FT_Err_Ok;

      if ( FT_STREAM_READ( loader->exec->glyphIns, n_ins ) )
        return error;

      loader->glyph->control_data = loader->exec->glyphIns;
      loader->glyph->control_len  = n_ins;
    }

#endif

    tt_prepare_zone( &loader->zone, &loader->gloader->base,
                     start_point, start_contour );

    
    
    for ( i = 0; i < loader->zone.n_points; i++ )
      loader->zone.tags[i] &= ~FT_CURVE_TAG_TOUCH_BOTH;

    loader->zone.n_points += 4;

    return TT_Hint_Glyph( loader, 1 );
  }


  



















































































#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING

#define TT_LOADER_SET_PP( loader )                                          \
          do                                                                \
          {                                                                 \
            FT_Bool  subpixel_  = loader->exec ? loader->exec->subpixel     \
                                               : 0;                         \
            FT_Bool  grayscale_ = loader->exec ? loader->exec->grayscale    \
                                               : 0;                         \
            FT_Bool  use_aw_2_  = (FT_Bool)( subpixel_ && grayscale_ );     \
                                                                            \
                                                                            \
            (loader)->pp1.x = (loader)->bbox.xMin - (loader)->left_bearing; \
            (loader)->pp1.y = 0;                                            \
            (loader)->pp2.x = (loader)->pp1.x + (loader)->advance;          \
            (loader)->pp2.y = 0;                                            \
                                                                            \
            (loader)->pp3.x = use_aw_2_ ? (loader)->advance / 2 : 0;        \
            (loader)->pp3.y = (loader)->bbox.yMax + (loader)->top_bearing;  \
            (loader)->pp4.x = use_aw_2_ ? (loader)->advance / 2 : 0;        \
            (loader)->pp4.y = (loader)->pp3.y - (loader)->vadvance;         \
          } while ( 0 )

#else 

#define TT_LOADER_SET_PP( loader )                                          \
          do                                                                \
          {                                                                 \
            (loader)->pp1.x = (loader)->bbox.xMin - (loader)->left_bearing; \
            (loader)->pp1.y = 0;                                            \
            (loader)->pp2.x = (loader)->pp1.x + (loader)->advance;          \
            (loader)->pp2.y = 0;                                            \
                                                                            \
            (loader)->pp3.x = 0;                                            \
            (loader)->pp3.y = (loader)->bbox.yMax + (loader)->top_bearing;  \
            (loader)->pp4.x = 0;                                            \
            (loader)->pp4.y = (loader)->pp3.y - (loader)->vadvance;         \
          } while ( 0 )

#endif 


  
  
  
  
  
  
  
  
  
  static FT_Error
  load_truetype_glyph( TT_Loader  loader,
                       FT_UInt    glyph_index,
                       FT_UInt    recurse_count,
                       FT_Bool    header_only )
  {
    FT_Error        error        = FT_Err_Ok;
    FT_Fixed        x_scale, y_scale;
    FT_ULong        offset;
    TT_Face         face         = (TT_Face)loader->face;
    FT_GlyphLoader  gloader      = loader->gloader;
    FT_Bool         opened_frame = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    FT_Vector*      deltas       = NULL;
#endif

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    FT_StreamRec    inc_stream;
    FT_Data         glyph_data;
    FT_Bool         glyph_data_loaded = 0;
#endif


    
    
    if ( recurse_count > 1                                   &&
         recurse_count > face->max_profile.maxComponentDepth )
    {
      error = FT_THROW( Invalid_Composite );
      goto Exit;
    }

    
    if ( glyph_index >= (FT_UInt)face->root.num_glyphs )
    {
      error = FT_THROW( Invalid_Glyph_Index );
      goto Exit;
    }

    loader->glyph_index = glyph_index;

    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
    {
      x_scale = ((TT_Size)loader->size)->metrics.x_scale;
      y_scale = ((TT_Size)loader->size)->metrics.y_scale;
    }
    else
    {
      x_scale = 0x10000L;
      y_scale = 0x10000L;
    }

    
    
    

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    
    
    
    if ( face->root.internal->incremental_interface )
    {
      error = face->root.internal->incremental_interface->funcs->get_glyph_data(
                face->root.internal->incremental_interface->object,
                glyph_index, &glyph_data );
      if ( error )
        goto Exit;

      glyph_data_loaded = 1;
      offset            = 0;
      loader->byte_len  = glyph_data.length;

      FT_MEM_ZERO( &inc_stream, sizeof ( inc_stream ) );
      FT_Stream_OpenMemory( &inc_stream,
                            glyph_data.pointer, glyph_data.length );

      loader->stream = &inc_stream;
    }
    else

#endif 

      offset = tt_face_get_location( face, glyph_index,
                                     (FT_UInt*)&loader->byte_len );

    if ( loader->byte_len > 0 )
    {
#ifdef FT_CONFIG_OPTION_INCREMENTAL
      
      if ( !loader->glyf_offset                        &&
           !face->root.internal->incremental_interface )
#else
      if ( !loader->glyf_offset )
#endif 
      {
        FT_TRACE2(( "no `glyf' table but non-zero `loca' entry\n" ));
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      error = face->access_glyph_frame( loader, glyph_index,
                                        loader->glyf_offset + offset,
                                        loader->byte_len );
      if ( error )
        goto Exit;

      opened_frame = 1;

      
      error = face->read_glyph_header( loader );
      if ( error )
        goto Exit;

      
      
      
      error = tt_get_metrics( loader, glyph_index );
      if ( error )
        goto Exit;

      if ( header_only )
        goto Exit;
    }

    if ( loader->byte_len == 0 || loader->n_contours == 0 )
    {
      loader->bbox.xMin = 0;
      loader->bbox.xMax = 0;
      loader->bbox.yMin = 0;
      loader->bbox.yMax = 0;

      error = tt_get_metrics( loader, glyph_index );
      if ( error )
        goto Exit;

      if ( header_only )
        goto Exit;

      
      
      TT_LOADER_SET_PP( loader );

#ifdef FT_CONFIG_OPTION_INCREMENTAL
      tt_get_metrics_incr_overrides( loader, glyph_index );
#endif

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

      if ( ((TT_Face)(loader->face))->doblend )
      {
        
        FT_Memory  memory = loader->face->memory;


        error = TT_Vary_Get_Glyph_Deltas( (TT_Face)(loader->face),
                                          glyph_index, &deltas, 4 );
        if ( error )
          goto Exit;

        loader->pp1.x += deltas[0].x;
        loader->pp1.y += deltas[0].y;
        loader->pp2.x += deltas[1].x;
        loader->pp2.y += deltas[1].y;

        loader->pp3.x += deltas[2].x;
        loader->pp3.y += deltas[2].y;
        loader->pp4.x += deltas[3].x;
        loader->pp4.y += deltas[3].y;

        FT_FREE( deltas );
      }

#endif 

      
      
      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        

        loader->pp3.x = FT_MulFix( loader->pp3.x, x_scale );
        loader->pp3.y = FT_MulFix( loader->pp3.y, y_scale );
        loader->pp4.x = FT_MulFix( loader->pp4.x, x_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

      error = FT_Err_Ok;
      goto Exit;
    }

    
    
    TT_LOADER_SET_PP( loader );

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    tt_get_metrics_incr_overrides( loader, glyph_index );
#endif

    
    
    

    

    if ( loader->n_contours > 0 )
    {
      error = face->read_simple_glyph( loader );
      if ( error )
        goto Exit;

      
      face->forget_glyph_frame( loader );
      opened_frame = 0;

      error = TT_Process_Simple_Glyph( loader );
      if ( error )
        goto Exit;

      FT_GlyphLoader_Add( gloader );
    }

    
    
    

    
    else if ( loader->n_contours == -1 )
    {
      FT_UInt   start_point;
      FT_UInt   start_contour;
      FT_ULong  ins_pos;  


      start_point   = gloader->base.outline.n_points;
      start_contour = gloader->base.outline.n_contours;

      
      error = face->read_composite_glyph( loader );
      if ( error )
        goto Exit;

      
      ins_pos = loader->ins_pos;

      
      face->forget_glyph_frame( loader );
      opened_frame = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

      if ( face->doblend )
      {
        FT_Int       i, limit;
        FT_SubGlyph  subglyph;
        FT_Memory    memory = face->root.memory;


        
        

        if ( ( error = TT_Vary_Get_Glyph_Deltas(
                         face,
                         glyph_index,
                         &deltas,
                         gloader->current.num_subglyphs + 4 ) ) != 0 )
          goto Exit;

        subglyph = gloader->current.subglyphs + gloader->base.num_subglyphs;
        limit    = gloader->current.num_subglyphs;

        for ( i = 0; i < limit; ++i, ++subglyph )
        {
          if ( subglyph->flags & ARGS_ARE_XY_VALUES )
          {
            
            
            
            subglyph->arg1 += (FT_Int16)deltas[i].x;
            subglyph->arg2 += (FT_Int16)deltas[i].y;
          }
        }

        loader->pp1.x += deltas[i + 0].x;
        loader->pp1.y += deltas[i + 0].y;
        loader->pp2.x += deltas[i + 1].x;
        loader->pp2.y += deltas[i + 1].y;

        loader->pp3.x += deltas[i + 2].x;
        loader->pp3.y += deltas[i + 2].y;
        loader->pp4.x += deltas[i + 3].x;
        loader->pp4.y += deltas[i + 3].y;

        FT_FREE( deltas );
      }

#endif 

      
      
      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        

        loader->pp3.x = FT_MulFix( loader->pp3.x, x_scale );
        loader->pp3.y = FT_MulFix( loader->pp3.y, y_scale );
        loader->pp4.x = FT_MulFix( loader->pp4.x, x_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

      
      
      
      if ( loader->load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_GlyphLoader_Add( gloader );
        loader->glyph->format = FT_GLYPH_FORMAT_COMPOSITE;

        goto Exit;
      }

      
      
      

      {
        FT_UInt      n, num_base_points;
        FT_SubGlyph  subglyph       = 0;

        FT_UInt      num_points     = start_point;
        FT_UInt      num_subglyphs  = gloader->current.num_subglyphs;
        FT_UInt      num_base_subgs = gloader->base.num_subglyphs;

        FT_Stream    old_stream     = loader->stream;
        FT_Int       old_byte_len   = loader->byte_len;


        FT_GlyphLoader_Add( gloader );

        
        for ( n = 0; n < num_subglyphs; n++ )
        {
          FT_Vector  pp[4];


          
          
          
          
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          pp[0] = loader->pp1;
          pp[1] = loader->pp2;
          pp[2] = loader->pp3;
          pp[3] = loader->pp4;

          num_base_points = gloader->base.outline.n_points;

          error = load_truetype_glyph( loader, subglyph->index,
                                       recurse_count + 1, FALSE );
          if ( error )
            goto Exit;

          
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          
          if ( !( subglyph->flags & USE_MY_METRICS ) )
          {
            loader->pp1 = pp[0];
            loader->pp2 = pp[1];
            loader->pp3 = pp[2];
            loader->pp4 = pp[3];
          }

          num_points = gloader->base.outline.n_points;

          if ( num_points == num_base_points )
            continue;

          
          
          
          
          
          
          error = TT_Process_Composite_Component( loader,
                                                  subglyph,
                                                  start_point,
                                                  num_base_points );
          if ( error )
            goto Exit;
        }

        loader->stream   = old_stream;
        loader->byte_len = old_byte_len;

        
        loader->ins_pos = ins_pos;
        if ( IS_HINTED( loader->load_flags ) &&
#ifdef TT_USE_BYTECODE_INTERPRETER
             subglyph->flags & WE_HAVE_INSTR &&
#endif
             num_points > start_point )
        {
          error = TT_Process_Composite_Glyph( loader,
                                              start_point,
                                              start_contour );
          if ( error )
            goto Exit;
        }
      }
    }
    else
    {
      
      error = FT_THROW( Invalid_Outline );
      goto Exit;
    }

    
    
    

  Exit:

    if ( opened_frame )
      face->forget_glyph_frame( loader );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( glyph_data_loaded )
      face->root.internal->incremental_interface->funcs->free_glyph_data(
        face->root.internal->incremental_interface->object,
        &glyph_data );

#endif

    return error;
  }


  static FT_Error
  compute_glyph_metrics( TT_Loader  loader,
                         FT_UInt    glyph_index )
  {
    TT_Face    face   = (TT_Face)loader->face;
#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    TT_Driver  driver = (TT_Driver)FT_FACE_DRIVER( face );
#endif

    FT_BBox       bbox;
    FT_Fixed      y_scale;
    TT_GlyphSlot  glyph = loader->glyph;
    TT_Size       size  = (TT_Size)loader->size;


    y_scale = 0x10000L;
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      y_scale = size->root.metrics.y_scale;

    if ( glyph->format != FT_GLYPH_FORMAT_COMPOSITE )
      FT_Outline_Get_CBox( &glyph->outline, &bbox );
    else
      bbox = loader->bbox;

    
    
    glyph->linearHoriAdvance = loader->linear;

    glyph->metrics.horiBearingX = bbox.xMin;
    glyph->metrics.horiBearingY = bbox.yMax;
    glyph->metrics.horiAdvance  = loader->pp2.x - loader->pp1.x;

    
    if ( !face->postscript.isFixedPitch  &&
         IS_HINTED( loader->load_flags ) )
    {
      FT_Byte*  widthp;


      widthp = tt_face_get_device_metrics( face,
                                           size->root.metrics.x_ppem,
                                           glyph_index );

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING

      if ( driver->interpreter_version == TT_INTERPRETER_VERSION_38 )
      {
        FT_Bool  ignore_x_mode;


        ignore_x_mode = FT_BOOL( FT_LOAD_TARGET_MODE( loader->load_flags ) !=
                                 FT_RENDER_MODE_MONO );

        if ( widthp                                                   &&
             ( ( ignore_x_mode && loader->exec->compatible_widths ) ||
                !ignore_x_mode                                      ||
                SPH_OPTION_BITMAP_WIDTHS                            ) )
          glyph->metrics.horiAdvance = *widthp << 6;
      }
      else

#endif 

      {
        if ( widthp )
          glyph->metrics.horiAdvance = *widthp << 6;
      }
    }

    
    glyph->metrics.width  = bbox.xMax - bbox.xMin;
    glyph->metrics.height = bbox.yMax - bbox.yMin;

    
    
    
    {
      FT_Pos  top;      
      FT_Pos  advance;  


      
      if ( face->vertical_info                   &&
           face->vertical.number_Of_VMetrics > 0 )
      {
        top = (FT_Short)FT_DivFix( loader->pp3.y - bbox.yMax,
                                   y_scale );

        if ( loader->pp3.y <= loader->pp4.y )
          advance = 0;
        else
          advance = (FT_UShort)FT_DivFix( loader->pp3.y - loader->pp4.y,
                                          y_scale );
      }
      else
      {
        FT_Pos  height;


        
        

        
        
        
        

        height = (FT_Short)FT_DivFix( bbox.yMax - bbox.yMin,
                                      y_scale );
        if ( face->os2.version != 0xFFFFU )
          advance = (FT_Pos)( face->os2.sTypoAscender -
                              face->os2.sTypoDescender );
        else
          advance = (FT_Pos)( face->horizontal.Ascender -
                              face->horizontal.Descender );

        top = ( advance - height ) / 2;
      }

#ifdef FT_CONFIG_OPTION_INCREMENTAL
      {
        FT_Incremental_InterfaceRec*  incr;
        FT_Incremental_MetricsRec     metrics;
        FT_Error                      error;


        incr = face->root.internal->incremental_interface;

        
        
        if ( incr && incr->funcs->get_glyph_metrics )
        {
          metrics.bearing_x = 0;
          metrics.bearing_y = top;
          metrics.advance   = advance;

          error = incr->funcs->get_glyph_metrics( incr->object,
                                                  glyph_index,
                                                  TRUE,
                                                  &metrics );
          if ( error )
            return error;

          top     = metrics.bearing_y;
          advance = metrics.advance;
        }
      }

      

#endif 

      glyph->linearVertAdvance = advance;

      
      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        top     = FT_MulFix( top,     y_scale );
        advance = FT_MulFix( advance, y_scale );
      }

      
      
      
      glyph->metrics.vertBearingX = glyph->metrics.horiBearingX -
                                      glyph->metrics.horiAdvance / 2;
      glyph->metrics.vertBearingY = top;
      glyph->metrics.vertAdvance  = advance;
    }

    return 0;
  }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  static FT_Error
  load_sbit_image( TT_Size       size,
                   TT_GlyphSlot  glyph,
                   FT_UInt       glyph_index,
                   FT_Int32      load_flags )
  {
    TT_Face             face;
    SFNT_Service        sfnt;
    FT_Stream           stream;
    FT_Error            error;
    TT_SBit_MetricsRec  metrics;


    face   = (TT_Face)glyph->face;
    sfnt   = (SFNT_Service)face->sfnt;
    stream = face->root.stream;

    error = sfnt->load_sbit_image( face,
                                   size->strike_index,
                                   glyph_index,
                                   (FT_Int)load_flags,
                                   stream,
                                   &glyph->bitmap,
                                   &metrics );
    if ( !error )
    {
      glyph->outline.n_points   = 0;
      glyph->outline.n_contours = 0;

      glyph->metrics.width  = (FT_Pos)metrics.width  << 6;
      glyph->metrics.height = (FT_Pos)metrics.height << 6;

      glyph->metrics.horiBearingX = (FT_Pos)metrics.horiBearingX << 6;
      glyph->metrics.horiBearingY = (FT_Pos)metrics.horiBearingY << 6;
      glyph->metrics.horiAdvance  = (FT_Pos)metrics.horiAdvance  << 6;

      glyph->metrics.vertBearingX = (FT_Pos)metrics.vertBearingX << 6;
      glyph->metrics.vertBearingY = (FT_Pos)metrics.vertBearingY << 6;
      glyph->metrics.vertAdvance  = (FT_Pos)metrics.vertAdvance  << 6;

      glyph->format = FT_GLYPH_FORMAT_BITMAP;

      if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
      {
        glyph->bitmap_left = metrics.vertBearingX;
        glyph->bitmap_top  = metrics.vertBearingY;
      }
      else
      {
        glyph->bitmap_left = metrics.horiBearingX;
        glyph->bitmap_top  = metrics.horiBearingY;
      }
    }

    return error;
  }

#endif 


  static FT_Error
  tt_loader_init( TT_Loader     loader,
                  TT_Size       size,
                  TT_GlyphSlot  glyph,
                  FT_Int32      load_flags,
                  FT_Bool       glyf_table_only )
  {
    FT_Error  error;

    TT_Face    face;
    FT_Stream  stream;
#ifdef TT_USE_BYTECODE_INTERPRETER
    FT_Bool    pedantic = FT_BOOL( load_flags & FT_LOAD_PEDANTIC );
#endif


    face   = (TT_Face)glyph->face;
    stream = face->root.stream;

    FT_MEM_ZERO( loader, sizeof ( TT_LoaderRec ) );

#ifdef TT_USE_BYTECODE_INTERPRETER

    
    if ( IS_HINTED( load_flags ) && !glyf_table_only )
    {
      TT_ExecContext  exec;
      FT_Bool         grayscale;

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
      TT_Driver  driver = (TT_Driver)FT_FACE_DRIVER( face );

      FT_Bool  subpixel = FALSE;

#if 0
      
      FT_Bool  compatible_widths;
      FT_Bool  symmetrical_smoothing;
      FT_Bool  bgr;
      FT_Bool  subpixel_positioned;
#endif
#endif 

      FT_Bool  reexecute = FALSE;


      if ( size->bytecode_ready < 0 || size->cvt_ready < 0 )
      {
        error = tt_size_ready_bytecode( size, pedantic );
        if ( error )
          return error;
      }
      else if ( size->bytecode_ready )
        return size->bytecode_ready;
      else if ( size->cvt_ready )
        return size->cvt_ready;

      
      exec = size->debug ? size->context
                         : ( (TT_Driver)FT_FACE_DRIVER( face ) )->context;
      if ( !exec )
        return FT_THROW( Could_Not_Find_Context );

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING

      if ( driver->interpreter_version == TT_INTERPRETER_VERSION_38 )
      {
        subpixel = FT_BOOL( ( FT_LOAD_TARGET_MODE( load_flags ) !=
                              FT_RENDER_MODE_MONO               )  &&
                            SPH_OPTION_SET_SUBPIXEL                );

        if ( subpixel )
          grayscale = FALSE;
        else if ( SPH_OPTION_SET_GRAYSCALE )
        {
          grayscale = TRUE;
          subpixel  = FALSE;
        }
        else
          grayscale = FALSE;

        if ( FT_IS_TRICKY( glyph->face ) )
          subpixel = FALSE;

        exec->ignore_x_mode      = subpixel || grayscale;
        exec->rasterizer_version = SPH_OPTION_SET_RASTERIZER_VERSION;
        if ( exec->sph_tweak_flags & SPH_TWEAK_RASTERIZER_35 )
          exec->rasterizer_version = TT_INTERPRETER_VERSION_35;

#if 1
        exec->compatible_widths     = SPH_OPTION_SET_COMPATIBLE_WIDTHS;
        exec->symmetrical_smoothing = FALSE;
        exec->bgr                   = FALSE;
        exec->subpixel_positioned   = TRUE;
#else 
        exec->compatible_widths =
          FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) !=
                   TT_LOAD_COMPATIBLE_WIDTHS );
        exec->symmetrical_smoothing =
          FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) !=
                   TT_LOAD_SYMMETRICAL_SMOOTHING );
        exec->bgr =
          FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) !=
                   TT_LOAD_BGR );
        exec->subpixel_positioned =
          FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) !=
                   TT_LOAD_SUBPIXEL_POSITIONED );
#endif 

      }
      else

#endif 

      {
        grayscale = FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) !=
                             FT_RENDER_MODE_MONO );
      }

      error = TT_Load_Context( exec, face, size );
      if ( error )
        return error;

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING

      if ( driver->interpreter_version == TT_INTERPRETER_VERSION_38 )
      {
        
        
        if ( subpixel != exec->subpixel )
        {
          FT_TRACE4(( "tt_loader_init: subpixel hinting change,"
                      " re-executing `prep' table\n" ));

          exec->subpixel = subpixel;
          reexecute      = TRUE;
        }

        
        
        if ( grayscale != exec->grayscale )
        {
          FT_TRACE4(( "tt_loader_init: grayscale hinting change,"
                      " re-executing `prep' table\n" ));

          exec->grayscale = grayscale;
          reexecute       = TRUE;
        }
      }
      else

#endif 

      {
        
        
        if ( grayscale != exec->grayscale )
        {
          FT_TRACE4(( "tt_loader_init: grayscale change,"
                      " re-executing `prep' table\n" ));

          exec->grayscale = grayscale;
          reexecute       = TRUE;
        }
      }

      if ( reexecute )
      {
        FT_UInt  i;


        for ( i = 0; i < size->cvt_size; i++ )
          size->cvt[i] = FT_MulFix( face->cvt[i], size->ttmetrics.scale );
        error = tt_size_run_prep( size, pedantic );
        if ( error )
          return error;
      }

      
      if ( exec->GS.instruct_control & 1 )
        load_flags |= FT_LOAD_NO_HINTING;

      
      if ( exec->GS.instruct_control & 2 )
        exec->GS = tt_default_graphics_state;

      exec->pedantic_hinting = FT_BOOL( load_flags & FT_LOAD_PEDANTIC );
      loader->exec = exec;
      loader->instructions = exec->glyphIns;
    }

#endif 

    
    
    

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( face->root.internal->incremental_interface )
      loader->glyf_offset = 0;
    else

#endif

    {
      error = face->goto_table( face, TTAG_glyf, stream, 0 );

      if ( FT_ERR_EQ( error, Table_Missing ) )
        loader->glyf_offset = 0;
      else if ( error )
      {
        FT_ERROR(( "tt_loader_init: could not access glyph table\n" ));
        return error;
      }
      else
        loader->glyf_offset = FT_STREAM_POS();
    }

    
    if ( !glyf_table_only )
    {
      FT_GlyphLoader  gloader = glyph->internal->loader;


      FT_GlyphLoader_Rewind( gloader );
      loader->gloader = gloader;
    }

    loader->load_flags = load_flags;

    loader->face   = (FT_Face)face;
    loader->size   = (FT_Size)size;
    loader->glyph  = (FT_GlyphSlot)glyph;
    loader->stream = stream;

    return FT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Load_Glyph( TT_Size       size,
                 TT_GlyphSlot  glyph,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags )
  {
    FT_Error      error;
    TT_LoaderRec  loader;


    FT_TRACE1(( "TT_Load_Glyph: glyph index %d\n", glyph_index ));

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    
    
    
    
    if ( size->strike_index != 0xFFFFFFFFUL      &&
         ( load_flags & FT_LOAD_NO_BITMAP ) == 0 )
    {
      error = load_sbit_image( size, glyph, glyph_index, load_flags );
      if ( !error )
      {
        if ( FT_IS_SCALABLE( glyph->face ) )
        {
          
          (void)tt_loader_init( &loader, size, glyph, load_flags, TRUE );
          (void)load_truetype_glyph( &loader, glyph_index, 0, TRUE );
          glyph->linearHoriAdvance = loader.linear;
          glyph->linearVertAdvance = loader.vadvance;

          
          
          if ( !glyph->metrics.horiAdvance && glyph->linearHoriAdvance )
            glyph->metrics.horiAdvance =
              FT_MulFix( glyph->linearHoriAdvance,
                         size->root.metrics.x_scale );
          if ( !glyph->metrics.vertAdvance && glyph->linearVertAdvance )
            glyph->metrics.vertAdvance =
              FT_MulFix( glyph->linearVertAdvance,
                         size->root.metrics.y_scale );
        }

        return FT_Err_Ok;
      }
    }

#endif 

    
    if ( !( load_flags & FT_LOAD_NO_SCALE ) && !size->ttmetrics.valid )
      return FT_THROW( Invalid_Size_Handle );

    if ( load_flags & FT_LOAD_SBITS_ONLY )
      return FT_THROW( Invalid_Argument );

    error = tt_loader_init( &loader, size, glyph, load_flags, FALSE );
    if ( error )
      return error;

    glyph->format        = FT_GLYPH_FORMAT_OUTLINE;
    glyph->num_subglyphs = 0;
    glyph->outline.flags = 0;

    
    error = load_truetype_glyph( &loader, glyph_index, 0, FALSE );
    if ( !error )
    {
      if ( glyph->format == FT_GLYPH_FORMAT_COMPOSITE )
      {
        glyph->num_subglyphs = loader.gloader->base.num_subglyphs;
        glyph->subglyphs     = loader.gloader->base.subglyphs;
      }
      else
      {
        glyph->outline        = loader.gloader->base.outline;
        glyph->outline.flags &= ~FT_OUTLINE_SINGLE_PASS;

        
        
        
        
        if ( loader.pp1.x )
          FT_Outline_Translate( &glyph->outline, -loader.pp1.x, 0 );
      }

#ifdef TT_USE_BYTECODE_INTERPRETER

      if ( IS_HINTED( load_flags ) )
      {
        if ( loader.exec->GS.scan_control )
        {
          
          switch ( loader.exec->GS.scan_type )
          {
          case 0: 
            glyph->outline.flags |= FT_OUTLINE_INCLUDE_STUBS;
            break;
          case 1: 
            
            break;
          case 4: 
            glyph->outline.flags |= FT_OUTLINE_SMART_DROPOUTS |
                                    FT_OUTLINE_INCLUDE_STUBS;
            break;
          case 5: 
            glyph->outline.flags |= FT_OUTLINE_SMART_DROPOUTS;
            break;

          default: 
            glyph->outline.flags |= FT_OUTLINE_IGNORE_DROPOUTS;
            break;
          }
        }
        else
          glyph->outline.flags |= FT_OUTLINE_IGNORE_DROPOUTS;
      }

#endif 

      error = compute_glyph_metrics( &loader, glyph_index );
    }

    
    
    
    
    if ( !( load_flags & FT_LOAD_NO_SCALE ) &&
         size->root.metrics.y_ppem < 24     )
      glyph->outline.flags |= FT_OUTLINE_HIGH_PRECISION;

    return error;
  }



