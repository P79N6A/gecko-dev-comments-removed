
















  






#include <ft2build.h>
#include FT_ADVANCES_H
#include FT_INTERNAL_DEBUG_H

#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_CJK

#undef AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT

#include "afcjk.h"
#include "aferrors.h"


#ifdef AF_CONFIG_OPTION_USE_WARPER
#include "afwarp.h"
#endif


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_afcjk


  
  
  
  
  
  
  


  
  

  FT_LOCAL_DEF( void )
  af_cjk_metrics_init_widths( AF_CJKMetrics  metrics,
                              FT_Face        face,
                              FT_ULong       charcode )
  {
    
    AF_GlyphHintsRec  hints[1];


    af_glyph_hints_init( hints, face->memory );

    metrics->axis[AF_DIMENSION_HORZ].width_count = 0;
    metrics->axis[AF_DIMENSION_VERT].width_count = 0;

    {
      FT_Error          error;
      FT_UInt           glyph_index;
      int               dim;
      AF_CJKMetricsRec  dummy[1];
      AF_Scaler         scaler = &dummy->root.scaler;


      glyph_index = FT_Get_Char_Index( face, charcode );
      if ( glyph_index == 0 )
        goto Exit;

      error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
      if ( error || face->glyph->outline.n_points <= 0 )
        goto Exit;

      FT_ZERO( dummy );

      dummy->units_per_em = metrics->units_per_em;

      scaler->x_scale = 0x10000L;
      scaler->y_scale = 0x10000L;
      scaler->x_delta = 0;
      scaler->y_delta = 0;

      scaler->face        = face;
      scaler->render_mode = FT_RENDER_MODE_NORMAL;
      scaler->flags       = 0;

      af_glyph_hints_rescale( hints, (AF_ScriptMetrics)dummy );

      error = af_glyph_hints_reload( hints, &face->glyph->outline );
      if ( error )
        goto Exit;

      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_CJKAxis    axis    = &metrics->axis[dim];
        AF_AxisHints  axhints = &hints->axis[dim];
        AF_Segment    seg, limit, link;
        FT_UInt       num_widths = 0;


        error = af_latin_hints_compute_segments( hints, (AF_Dimension)dim );
        if ( error )
          goto Exit;

        af_latin_hints_link_segments( hints, (AF_Dimension)dim );

        seg   = axhints->segments;
        limit = seg + axhints->num_segments;

        for ( ; seg < limit; seg++ )
        {
          link = seg->link;

          
          if ( link && link->link == seg && link > seg )
          {
            FT_Pos  dist;


            dist = seg->pos - link->pos;
            if ( dist < 0 )
              dist = -dist;

            if ( num_widths < AF_CJK_MAX_WIDTHS )
              axis->widths[num_widths++].org = dist;
          }
        }

        af_sort_widths( num_widths, axis->widths );
        axis->width_count = num_widths;
      }

    Exit:
      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_CJKAxis  axis = &metrics->axis[dim];
        FT_Pos      stdw;


        stdw = ( axis->width_count > 0 ) ? axis->widths[0].org
                                         : AF_LATIN_CONSTANT( metrics, 50 );

        
        axis->edge_distance_threshold = stdw / 5;
        axis->standard_width          = stdw;
        axis->extra_light             = 0;
      }
    }

    af_glyph_hints_done( hints );
  }


#define AF_CJK_MAX_TEST_CHARACTERS  32


  
  

  enum
  {
    AF_CJK_BLUE_TYPE_FILL,
    AF_CJK_BLUE_TYPE_UNFILL,
    AF_CJK_BLUE_TYPE_MAX
  };


  
  static const FT_ULong af_cjk_hani_blue_chars[AF_CJK_BLUE_MAX]
                                              [AF_CJK_BLUE_TYPE_MAX]
                                              [AF_CJK_MAX_TEST_CHARACTERS] =
  {
    {
      {
        0x4ED6, 0x4EEC, 0x4F60, 0x4F86, 0x5011, 0x5230, 0x548C, 0x5730,
        0x5BF9, 0x5C0D, 0x5C31, 0x5E2D, 0x6211, 0x65F6, 0x6642, 0x6703,
        0x6765, 0x70BA, 0x80FD, 0x8230, 0x8AAA, 0x8BF4, 0x8FD9, 0x9019,
        0x9F4A 
      },
      {
        0x519B, 0x540C, 0x5DF2, 0x613F, 0x65E2, 0x661F, 0x662F, 0x666F,
        0x6C11, 0x7167, 0x73B0, 0x73FE, 0x7406, 0x7528, 0x7F6E, 0x8981,
        0x8ECD, 0x90A3, 0x914D, 0x91CC, 0x958B, 0x96F7, 0x9732, 0x9762,
        0x987E 
      }
    },
    {
      {
        0x4E2A, 0x4E3A, 0x4EBA, 0x4ED6, 0x4EE5, 0x4EEC, 0x4F60, 0x4F86,
        0x500B, 0x5011, 0x5230, 0x548C, 0x5927, 0x5BF9, 0x5C0D, 0x5C31,
        0x6211, 0x65F6, 0x6642, 0x6709, 0x6765, 0x70BA, 0x8981, 0x8AAA,
        0x8BF4 
      },
      {
        0x4E3B, 0x4E9B, 0x56E0, 0x5B83, 0x60F3, 0x610F, 0x7406, 0x751F,
        0x7576, 0x770B, 0x7740, 0x7F6E, 0x8005, 0x81EA, 0x8457, 0x88E1,
        0x8FC7, 0x8FD8, 0x8FDB, 0x9032, 0x904E, 0x9053, 0x9084, 0x91CC,
        0x9762 
      }
    },
#ifndef AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT
      { {0x0000}, {0x0000} },
      { {0x0000}, {0x0000} }
#else
    {
      {
        0x4E9B, 0x4EEC, 0x4F60, 0x4F86, 0x5011, 0x5230, 0x548C, 0x5730,
        0x5979, 0x5C06, 0x5C07, 0x5C31, 0x5E74, 0x5F97, 0x60C5, 0x6700,
        0x6837, 0x6A23, 0x7406, 0x80FD, 0x8AAA, 0x8BF4, 0x8FD9, 0x9019,
        0x901A 
      },
      {
        0x5373, 0x5417, 0x5427, 0x542C, 0x5462, 0x54C1, 0x54CD, 0x55CE,
        0x5E08, 0x5E2B, 0x6536, 0x65AD, 0x65B7, 0x660E, 0x773C, 0x9593,
        0x95F4, 0x9645, 0x9648, 0x9650, 0x9664, 0x9673, 0x968F, 0x969B,
        0x96A8 
      }
    },
    {
      {
        0x4E8B, 0x524D, 0x5B78, 0x5C06, 0x5C07, 0x60C5, 0x60F3, 0x6216,
        0x653F, 0x65AF, 0x65B0, 0x6837, 0x6A23, 0x6C11, 0x6C92, 0x6CA1,
        0x7136, 0x7279, 0x73B0, 0x73FE, 0x7403, 0x7B2C, 0x7D93, 0x8C01,
        0x8D77 
      },
      {
        0x4F8B, 0x5225, 0x522B, 0x5236, 0x52A8, 0x52D5, 0x5417, 0x55CE,
        0x589E, 0x6307, 0x660E, 0x671D, 0x671F, 0x6784, 0x7269, 0x786E,
        0x79CD, 0x8ABF, 0x8C03, 0x8CBB, 0x8D39, 0x90A3, 0x90FD, 0x9593,
        0x95F4 
      }
    }
#endif 
  };


  

  static void
  af_cjk_metrics_init_blues( AF_CJKMetrics   metrics,
                             FT_Face         face,
                             const FT_ULong  blue_chars
                                               [AF_CJK_BLUE_MAX]
                                               [AF_CJK_BLUE_TYPE_MAX]
                                               [AF_CJK_MAX_TEST_CHARACTERS] )
  {
    FT_Pos        fills[AF_CJK_MAX_TEST_CHARACTERS];
    FT_Pos        flats[AF_CJK_MAX_TEST_CHARACTERS];

    FT_Int        num_fills;
    FT_Int        num_flats;

    FT_Int        bb;
    AF_CJKBlue    blue;
    FT_Error      error;
    AF_CJKAxis    axis;
    FT_GlyphSlot  glyph = face->glyph;

#ifdef FT_DEBUG_LEVEL_TRACE
    FT_String*  cjk_blue_name[AF_CJK_BLUE_MAX] = {
      (FT_String*)"top",
      (FT_String*)"bottom",
      (FT_String*)"left",
      (FT_String*)"right"
    };
    FT_String*  cjk_blue_type_name[AF_CJK_BLUE_TYPE_MAX] = {
      (FT_String*)"filled",
      (FT_String*)"unfilled"
    };
#endif


    
    
    

    FT_TRACE5(( "cjk blue zones computation\n" ));
    FT_TRACE5(( "------------------------------------------------\n" ));

    for ( bb = 0; bb < AF_CJK_BLUE_MAX; bb++ )
    {
      FT_Int   fill_type;
      FT_Pos*  blue_ref;
      FT_Pos*  blue_shoot;


      num_fills = 0;
      num_flats = 0;

      for ( fill_type = 0; fill_type < AF_CJK_BLUE_TYPE_MAX; fill_type++ )
      {
        const FT_ULong*  p     = blue_chars[bb][fill_type];
        const FT_ULong*  limit = p + AF_CJK_MAX_TEST_CHARACTERS;
        FT_Bool          fill  = FT_BOOL(
                                   fill_type == AF_CJK_BLUE_TYPE_FILL );


        FT_TRACE5(( "cjk blue %s/%s\n", cjk_blue_name[bb],
                                        cjk_blue_type_name[fill_type] ));


        for ( ; p < limit && *p; p++ )
        {
          FT_UInt     glyph_index;
          FT_Pos      best_pos; 
          FT_Int      best_point;
          FT_Vector*  points;


          FT_TRACE5(( "  U+%lX...", *p ));

          
          glyph_index = FT_Get_Char_Index( face, *p );
          if ( glyph_index == 0 )
          {
            FT_TRACE5(( "unavailable\n" ));
            continue;
          }

          error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
          if ( error || glyph->outline.n_points <= 0 )
          {
            FT_TRACE5(( "no outline\n" ));
            continue;
          }

          
          points     = glyph->outline.points;
          best_point = -1;
          best_pos   = 0;  

          {
            FT_Int  nn;
            FT_Int  first = 0;
            FT_Int  last  = -1;


            for ( nn = 0;
                  nn < glyph->outline.n_contours;
                  first = last + 1, nn++ )
            {
              FT_Int  pp;


              last = glyph->outline.contours[nn];

              
              
              
              
              if ( last <= first )
                continue;

              switch ( bb )
              {
              case AF_CJK_BLUE_TOP:
                for ( pp = first; pp <= last; pp++ )
                  if ( best_point < 0 || points[pp].y > best_pos )
                  {
                    best_point = pp;
                    best_pos   = points[pp].y;
                  }
                break;

              case AF_CJK_BLUE_BOTTOM:
                for ( pp = first; pp <= last; pp++ )
                  if ( best_point < 0 || points[pp].y < best_pos )
                  {
                    best_point = pp;
                    best_pos   = points[pp].y;
                  }
                break;

              case AF_CJK_BLUE_LEFT:
                for ( pp = first; pp <= last; pp++ )
                  if ( best_point < 0 || points[pp].x < best_pos )
                  {
                    best_point = pp;
                    best_pos   = points[pp].x;
                  }
                break;

              case AF_CJK_BLUE_RIGHT:
                for ( pp = first; pp <= last; pp++ )
                  if ( best_point < 0 || points[pp].x > best_pos )
                  {
                    best_point = pp;
                    best_pos   = points[pp].x;
                  }
                break;

              default:
                ;
              }
            }
            FT_TRACE5(( "best_pos=%5ld\n", best_pos ));
          }

          if ( fill )
            fills[num_fills++] = best_pos;
          else
            flats[num_flats++] = best_pos;
        }
      }

      if ( num_flats == 0 && num_fills == 0 )
      {
        



        FT_TRACE5(( "empty\n" ));
        continue;
      }

      
      
      
      af_sort_pos( num_flats, flats );
      af_sort_pos( num_fills, fills );

      if ( AF_CJK_BLUE_TOP == bb || AF_CJK_BLUE_BOTTOM == bb )
        axis = &metrics->axis[AF_DIMENSION_VERT];
      else
        axis = &metrics->axis[AF_DIMENSION_HORZ];

      blue       = & axis->blues[axis->blue_count];
      blue_ref   = & blue->ref.org;
      blue_shoot = & blue->shoot.org;

      axis->blue_count++;
      if ( num_flats == 0 )
      {
        *blue_ref   = fills[num_fills / 2];
        *blue_shoot = fills[num_fills / 2];
      }
      else if ( num_fills == 0 )
      {
        *blue_ref   = flats[num_flats / 2];
        *blue_shoot = flats[num_flats / 2];
      }
      else
      {
        *blue_ref   = fills[num_fills / 2];
        *blue_shoot = flats[num_flats / 2];
      }

      
      
      if ( *blue_shoot != *blue_ref )
      {
        FT_Pos   ref       = *blue_ref;
        FT_Pos   shoot     = *blue_shoot;
        FT_Bool  under_ref = FT_BOOL( shoot < ref );


        if ( (AF_CJK_BLUE_TOP == bb || AF_CJK_BLUE_RIGHT == bb) ^ under_ref )
          *blue_shoot = *blue_ref = ( shoot + ref ) / 2;
      }

      blue->flags = 0;
      if ( AF_CJK_BLUE_TOP == bb )
        blue->flags |= AF_CJK_BLUE_IS_TOP;
      else if ( AF_CJK_BLUE_RIGHT == bb )
        blue->flags |= AF_CJK_BLUE_IS_RIGHT;

      FT_TRACE5(( "-- cjk %s bluezone ref = %ld shoot = %ld\n",
                  cjk_blue_name[bb], *blue_ref, *blue_shoot ));
    }

    return;
  }


  
  FT_LOCAL_DEF( void )
  af_cjk_metrics_check_digits( AF_CJKMetrics  metrics,
                               FT_Face        face )
  {
    FT_UInt   i;
    FT_Bool   started = 0, same_width = 1;
    FT_Fixed  advance, old_advance = 0;


    
    
    for ( i = 0x30; i <= 0x39; i++ )
    {
      FT_UInt  glyph_index;


      glyph_index = FT_Get_Char_Index( face, i );
      if ( glyph_index == 0 )
        continue;

      if ( FT_Get_Advance( face, glyph_index,
                           FT_LOAD_NO_SCALE         |
                           FT_LOAD_NO_HINTING       |
                           FT_LOAD_IGNORE_TRANSFORM,
                           &advance ) )
        continue;

      if ( started )
      {
        if ( advance != old_advance )
        {
          same_width = 0;
          break;
        }
      }
      else
      {
        old_advance = advance;
        started     = 1;
      }
    }

    metrics->root.digits_have_same_width = same_width;
  }


  FT_LOCAL_DEF( FT_Error )
  af_cjk_metrics_init( AF_CJKMetrics  metrics,
                       FT_Face        face )
  {
    FT_CharMap  oldmap = face->charmap;


    metrics->units_per_em = face->units_per_EM;

    if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) )
      face->charmap = NULL;
    else
    {
      af_cjk_metrics_init_widths( metrics, face, 0x7530 );
      af_cjk_metrics_init_blues( metrics, face, af_cjk_hani_blue_chars );
      af_cjk_metrics_check_digits( metrics, face );
    }

    FT_Set_Charmap( face, oldmap );

    return AF_Err_Ok;
  }


  static void
  af_cjk_metrics_scale_dim( AF_CJKMetrics  metrics,
                            AF_Scaler      scaler,
                            AF_Dimension   dim )
  {
    FT_Fixed    scale;
    FT_Pos      delta;
    AF_CJKAxis  axis;
    FT_UInt     nn;


    axis = &metrics->axis[dim];

    if ( dim == AF_DIMENSION_HORZ )
    {
      scale = scaler->x_scale;
      delta = scaler->x_delta;
    }
    else
    {
      scale = scaler->y_scale;
      delta = scaler->y_delta;
    }

    if ( axis->org_scale == scale && axis->org_delta == delta )
      return;

    axis->org_scale = scale;
    axis->org_delta = delta;

    axis->scale = scale;
    axis->delta = delta;

    
    for ( nn = 0; nn < axis->blue_count; nn++ )
    {
      AF_CJKBlue  blue = &axis->blues[nn];
      FT_Pos      dist;


      blue->ref.cur   = FT_MulFix( blue->ref.org, scale ) + delta;
      blue->ref.fit   = blue->ref.cur;
      blue->shoot.cur = FT_MulFix( blue->shoot.org, scale ) + delta;
      blue->shoot.fit = blue->shoot.cur;
      blue->flags    &= ~AF_CJK_BLUE_ACTIVE;

      
      dist = FT_MulFix( blue->ref.org - blue->shoot.org, scale );
      if ( dist <= 48 && dist >= -48 )
      {
        FT_Pos  delta1, delta2;


        blue->ref.fit  = FT_PIX_ROUND( blue->ref.cur );

        
        delta1 = FT_DivFix( blue->ref.fit, scale ) - blue->shoot.org;
        delta2 = delta1;
        if ( delta1 < 0 )
          delta2 = -delta2;

        delta2 = FT_MulFix( delta2, scale );

        FT_TRACE5(( "delta: %d", delta1 ));
        if ( delta2 < 32 )
          delta2 = 0;
#if 0
        else if ( delta2 < 64 )
          delta2 = 32 + ( ( ( delta2 - 32 ) + 16 ) & ~31 );
#endif
        else
          delta2 = FT_PIX_ROUND( delta2 );
        FT_TRACE5(( "/%d\n", delta2 ));

        if ( delta1 < 0 )
          delta2 = -delta2;

        blue->shoot.fit = blue->ref.fit - delta2;

        FT_TRACE5(( ">> active cjk blue zone %c%d[%ld/%ld]: "
                     "ref: cur=%.2f fit=%.2f shoot: cur=%.2f fit=%.2f\n",
                       ( dim == AF_DIMENSION_HORZ ) ? 'H' : 'V',
                       nn, blue->ref.org, blue->shoot.org,
                       blue->ref.cur / 64.0, blue->ref.fit / 64.0,
                       blue->shoot.cur / 64.0, blue->shoot.fit / 64.0 ));

        blue->flags |= AF_CJK_BLUE_ACTIVE;
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_cjk_metrics_scale( AF_CJKMetrics  metrics,
                        AF_Scaler      scaler )
  {
    metrics->root.scaler = *scaler;

    af_cjk_metrics_scale_dim( metrics, scaler, AF_DIMENSION_HORZ );
    af_cjk_metrics_scale_dim( metrics, scaler, AF_DIMENSION_VERT );
  }


  
  
  
  
  
  
  

  static FT_Error
  af_cjk_hints_compute_segments( AF_GlyphHints  hints,
                                 AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    FT_Error      error;
    AF_Segment    seg;


    error = af_latin_hints_compute_segments( hints, dim );
    if ( error )
      return error;

    
    
    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Point  pt   = seg->first;
      AF_Point  last = seg->last;
      AF_Flags  f0   = (AF_Flags)(pt->flags & AF_FLAG_CONTROL);
      AF_Flags  f1;


      seg->flags &= ~AF_EDGE_ROUND;

      for ( ; pt != last; f0 = f1 )
      {
        pt = pt->next;
        f1 = (AF_Flags)(pt->flags & AF_FLAG_CONTROL);

        if ( !f0 && !f1 )
          break;

        if ( pt == last )
          seg->flags |= AF_EDGE_ROUND;
      }
    }

    return AF_Err_Ok;
  }


  static void
  af_cjk_hints_link_segments( AF_GlyphHints  hints,
                              AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Direction  major_dir     = axis->major_dir;
    AF_Segment    seg1, seg2;
    FT_Pos        len_threshold;
    FT_Pos        dist_threshold;


    len_threshold = AF_LATIN_CONSTANT( hints->metrics, 8 );

    dist_threshold = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                                  : hints->y_scale;
    dist_threshold = FT_DivFix( 64 * 3, dist_threshold );

    
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      
      if ( seg1->first == seg1->last )
        continue;

      if ( seg1->dir != major_dir )
        continue;

      for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        if ( seg2 != seg1 && seg1->dir + seg2->dir == 0 )
        {
          FT_Pos  dist = seg2->pos - seg1->pos;


          if ( dist < 0 )
            continue;

          {
            FT_Pos  min = seg1->min_coord;
            FT_Pos  max = seg1->max_coord;
            FT_Pos  len;


            if ( min < seg2->min_coord )
              min = seg2->min_coord;

            if ( max > seg2->max_coord )
              max = seg2->max_coord;

            len = max - min;
            if ( len >= len_threshold )
            {
              if ( dist * 8 < seg1->score * 9                        &&
                   ( dist * 8 < seg1->score * 7 || seg1->len < len ) )
              {
                seg1->score = dist;
                seg1->len   = len;
                seg1->link  = seg2;
              }

              if ( dist * 8 < seg2->score * 9                        &&
                   ( dist * 8 < seg2->score * 7 || seg2->len < len ) )
              {
                seg2->score = dist;
                seg2->len   = len;
                seg2->link  = seg1;
              }
            }
          }
        }
    }

    








    {
      AF_Segment  link1, link2;


      for ( seg1 = segments; seg1 < segment_limit; seg1++ )
      {
        link1 = seg1->link;
        if ( !link1 || link1->link != seg1 || link1->pos <= seg1->pos )
          continue;

        if ( seg1->score >= dist_threshold )
          continue;

        for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        {
          if ( seg2->pos > seg1->pos || seg1 == seg2 )
            continue;

          link2 = seg2->link;
          if ( !link2 || link2->link != seg2 || link2->pos < link1->pos )
            continue;

          if ( seg1->pos == seg2->pos && link1->pos == link2->pos )
            continue;

          if ( seg2->score <= seg1->score || seg1->score * 4 <= seg2->score )
            continue;

          

          if ( seg1->len >= seg2->len * 3 )
          {
            AF_Segment  seg;


            for ( seg = segments; seg < segment_limit; seg++ )
            {
              AF_Segment  link = seg->link;


              if ( link == seg2 )
              {
                seg->link  = 0;
                seg->serif = link1;
              }
              else if ( link == link2 )
              {
                seg->link  = 0;
                seg->serif = seg1;
              }
            }
          }
          else
          {
            seg1->link = link1->link = 0;

            break;
          }
        }
      }
    }

    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      seg2 = seg1->link;

      if ( seg2 )
      {
        seg2->num_linked++;
        if ( seg2->link != seg1 )
        {
          seg1->link = 0;

          if ( seg2->score < dist_threshold || seg1->score < seg2->score * 4 )
            seg1->serif = seg2->link;
          else
            seg2->num_linked--;
        }
      }
    }
  }


  static FT_Error
  af_cjk_hints_compute_edges( AF_GlyphHints  hints,
                              AF_Dimension   dim )
  {
    AF_AxisHints  axis   = &hints->axis[dim];
    FT_Error      error  = AF_Err_Ok;
    FT_Memory     memory = hints->memory;
    AF_CJKAxis    laxis  = &((AF_CJKMetrics)hints->metrics)->axis[dim];

    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Segment    seg;

    FT_Fixed      scale;
    FT_Pos        edge_distance_threshold;


    axis->num_edges = 0;

    scale = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                         : hints->y_scale;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    edge_distance_threshold = FT_MulFix( laxis->edge_distance_threshold,
                                         scale );
    if ( edge_distance_threshold > 64 / 4 )
      edge_distance_threshold = FT_DivFix( 64 / 4, scale );
    else
      edge_distance_threshold = laxis->edge_distance_threshold;

    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Edge  found = 0;
      FT_Pos   best  = 0xFFFFU;
      FT_Int   ee;


      
      for ( ee = 0; ee < axis->num_edges; ee++ )
      {
        AF_Edge  edge = axis->edges + ee;
        FT_Pos   dist;


        if ( edge->dir != seg->dir )
          continue;

        dist = seg->pos - edge->fpos;
        if ( dist < 0 )
          dist = -dist;

        if ( dist < edge_distance_threshold && dist < best )
        {
          AF_Segment  link = seg->link;


          
          
          if ( link )
          {
            AF_Segment  seg1 = edge->first;
            AF_Segment  link1;
            FT_Pos      dist2 = 0;


            do
            {
              link1 = seg1->link;
              if ( link1 )
              {
                dist2 = AF_SEGMENT_DIST( link, link1 );
                if ( dist2 >= edge_distance_threshold )
                  break;
              }

            } while ( ( seg1 = seg1->edge_next ) != edge->first );

            if ( dist2 >= edge_distance_threshold )
              continue;
          }

          best  = dist;
          found = edge;
        }
      }

      if ( !found )
      {
        AF_Edge  edge;


        
        
        error = af_axis_hints_new_edge( axis, seg->pos,
                                        (AF_Direction)seg->dir,
                                        memory, &edge );
        if ( error )
          goto Exit;

        
        FT_ZERO( edge );

        edge->first    = seg;
        edge->last     = seg;
        edge->fpos     = seg->pos;
        edge->opos     = edge->pos = FT_MulFix( seg->pos, scale );
        seg->edge_next = seg;
        edge->dir      = seg->dir;
      }
      else
      {
        
        
        seg->edge_next         = found->first;
        found->last->edge_next = seg;
        found->last            = seg;
      }
    }

    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    

    {
      AF_Edge  edges      = axis->edges;
      AF_Edge  edge_limit = edges + axis->num_edges;
      AF_Edge  edge;


      for ( edge = edges; edge < edge_limit; edge++ )
      {
        seg = edge->first;
        if ( seg )
          do
          {
            seg->edge = edge;
            seg       = seg->edge_next;

          } while ( seg != edge->first );
      }

      
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        FT_Int  is_round    = 0;  
        FT_Int  is_straight = 0;  


        seg = edge->first;

        do
        {
          FT_Bool  is_serif;


          
          if ( seg->flags & AF_EDGE_ROUND )
            is_round++;
          else
            is_straight++;

          
          
          is_serif = (FT_Bool)( seg->serif && seg->serif->edge != edge );

          if ( seg->link || is_serif )
          {
            AF_Edge     edge2;
            AF_Segment  seg2;


            edge2 = edge->link;
            seg2  = seg->link;

            if ( is_serif )
            {
              seg2  = seg->serif;
              edge2 = edge->serif;
            }

            if ( edge2 )
            {
              FT_Pos  edge_delta;
              FT_Pos  seg_delta;


              edge_delta = edge->fpos - edge2->fpos;
              if ( edge_delta < 0 )
                edge_delta = -edge_delta;

              seg_delta = AF_SEGMENT_DIST( seg, seg2 );

              if ( seg_delta < edge_delta )
                edge2 = seg2->edge;
            }
            else
              edge2 = seg2->edge;

            if ( is_serif )
            {
              edge->serif   = edge2;
              edge2->flags |= AF_EDGE_SERIF;
            }
            else
              edge->link  = edge2;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );

        
        edge->flags = AF_EDGE_NORMAL;

        if ( is_round > 0 && is_round >= is_straight )
          edge->flags |= AF_EDGE_ROUND;

        
        
        

        if ( edge->serif && edge->link )
          edge->serif = 0;
      }
    }

  Exit:
    return error;
  }


  static FT_Error
  af_cjk_hints_detect_features( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    FT_Error  error;


    error = af_cjk_hints_compute_segments( hints, dim );
    if ( !error )
    {
      af_cjk_hints_link_segments( hints, dim );

      error = af_cjk_hints_compute_edges( hints, dim );
    }
    return error;
  }


  FT_LOCAL_DEF( void )
  af_cjk_hints_compute_blue_edges( AF_GlyphHints  hints,
                                   AF_CJKMetrics  metrics,
                                   AF_Dimension   dim )
  {
    AF_AxisHints  axis       = &hints->axis[dim];
    AF_Edge       edge       = axis->edges;
    AF_Edge       edge_limit = edge + axis->num_edges;
    AF_CJKAxis    cjk        = &metrics->axis[dim];
    FT_Fixed      scale      = cjk->scale;
    FT_Pos        best_dist0;  


    
    best_dist0 = FT_MulFix( metrics->units_per_em / 40, scale );

    if ( best_dist0 > 64 / 2 ) 
      best_dist0 = 64 / 2;

    
    

    
    
    

    for ( ; edge < edge_limit; edge++ )
    {
      FT_UInt   bb;
      AF_Width  best_blue = NULL;
      FT_Pos    best_dist = best_dist0;


      for ( bb = 0; bb < cjk->blue_count; bb++ )
      {
        AF_CJKBlue  blue = cjk->blues + bb;
        FT_Bool     is_top_right_blue, is_major_dir;


        
        if ( !( blue->flags & AF_CJK_BLUE_ACTIVE ) )
          continue;

        
        
        
        
        is_top_right_blue  =
          FT_BOOL( ( ( blue->flags & AF_CJK_BLUE_IS_TOP )   != 0 ) ||
                   ( ( blue->flags & AF_CJK_BLUE_IS_RIGHT ) != 0 ) );
        is_major_dir = FT_BOOL( edge->dir == axis->major_dir );

        
        
        
        if ( is_top_right_blue ^ is_major_dir )
        {
          FT_Pos    dist;
          AF_Width  compare;


          
          if ( FT_ABS( edge->fpos - blue->ref.org ) >
               FT_ABS( edge->fpos - blue->shoot.org ) )
            compare = &blue->shoot;
          else
            compare = &blue->ref;

          dist = edge->fpos - compare->org;
          if ( dist < 0 )
            dist = -dist;

          dist = FT_MulFix( dist, scale );
          if ( dist < best_dist )
          {
            best_dist = dist;
            best_blue = compare;
          }
        }
      }

      if ( best_blue )
        edge->blue_edge = best_blue;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_cjk_hints_init( AF_GlyphHints  hints,
                     AF_CJKMetrics  metrics )
  {
    FT_Render_Mode  mode;
    FT_UInt32       scaler_flags, other_flags;


    af_glyph_hints_rescale( hints, (AF_ScriptMetrics)metrics );

    



    hints->x_scale = metrics->axis[AF_DIMENSION_HORZ].scale;
    hints->x_delta = metrics->axis[AF_DIMENSION_HORZ].delta;
    hints->y_scale = metrics->axis[AF_DIMENSION_VERT].scale;
    hints->y_delta = metrics->axis[AF_DIMENSION_VERT].delta;

    
    mode = metrics->root.scaler.render_mode;

#ifdef AF_CONFIG_OPTION_USE_WARPER
    if ( mode == FT_RENDER_MODE_LCD || mode == FT_RENDER_MODE_LCD_V )
      metrics->root.scaler.render_mode = mode = FT_RENDER_MODE_NORMAL;
#endif

    scaler_flags = hints->scaler_flags;
    other_flags  = 0;

    



    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD )
      other_flags |= AF_LATIN_HINTS_HORZ_SNAP;

    



    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD_V )
      other_flags |= AF_LATIN_HINTS_VERT_SNAP;

    


    if ( mode != FT_RENDER_MODE_LIGHT )
      other_flags |= AF_LATIN_HINTS_STEM_ADJUST;

    if ( mode == FT_RENDER_MODE_MONO )
      other_flags |= AF_LATIN_HINTS_MONO;

    scaler_flags |= AF_SCALER_FLAG_NO_ADVANCE;

    hints->scaler_flags = scaler_flags;
    hints->other_flags  = other_flags;

    return 0;
  }


  
  
  
  
  
  
  

  
  

  static FT_Pos
  af_cjk_snap_width( AF_Width  widths,
                     FT_Int    count,
                     FT_Pos    width )
  {
    int     n;
    FT_Pos  best      = 64 + 32 + 2;
    FT_Pos  reference = width;
    FT_Pos  scaled;


    for ( n = 0; n < count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = widths[n].cur;
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    scaled = FT_PIX_ROUND( reference );

    if ( width >= reference )
    {
      if ( width < scaled + 48 )
        width = reference;
    }
    else
    {
      if ( width > scaled - 48 )
        width = reference;
    }

    return width;
  }


  

  static FT_Pos
  af_cjk_compute_stem_width( AF_GlyphHints  hints,
                             AF_Dimension   dim,
                             FT_Pos         width,
                             AF_Edge_Flags  base_flags,
                             AF_Edge_Flags  stem_flags )
  {
    AF_CJKMetrics  metrics  = (AF_CJKMetrics) hints->metrics;
    AF_CJKAxis     axis     = & metrics->axis[dim];
    FT_Pos         dist     = width;
    FT_Int         sign     = 0;
    FT_Bool        vertical = FT_BOOL( dim == AF_DIMENSION_VERT );

    FT_UNUSED( base_flags );
    FT_UNUSED( stem_flags );


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
      return width;

    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( (  vertical && !AF_LATIN_HINTS_DO_VERT_SNAP( hints ) ) ||
         ( !vertical && !AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) ) )
    {
      

      if ( axis->width_count > 0 )
      {
        if ( FT_ABS( dist - axis->widths[0].cur ) < 40 )
        {
          dist = axis->widths[0].cur;
          if ( dist < 48 )
            dist = 48;

          goto Done_Width;
        }
      }

      if ( dist < 54 )
        dist += ( 54 - dist ) / 2 ;
      else if ( dist < 3 * 64 )
      {
        FT_Pos  delta;


        delta  = dist & 63;
        dist  &= -64;

        if ( delta < 10 )
          dist += delta;
        else if ( delta < 22 )
          dist += 10;
        else if ( delta < 42 )
          dist += delta;
        else if ( delta < 54 )
          dist += 54;
        else
          dist += delta;
      }
    }
    else
    {
      

      dist = af_cjk_snap_width( axis->widths, axis->width_count, dist );

      if ( vertical )
      {
        
        

        if ( dist >= 64 )
          dist = ( dist + 16 ) & ~63;
        else
          dist = 64;
      }
      else
      {
        if ( AF_LATIN_HINTS_DO_MONO( hints ) )
        {
          
          

          if ( dist < 64 )
            dist = 64;
          else
            dist = ( dist + 32 ) & ~63;
        }
        else
        {
          
          
          

          if ( dist < 48 )
            dist = ( dist + 64 ) >> 1;

          else if ( dist < 128 )
            dist = ( dist + 22 ) & ~63;
          else
            
            dist = ( dist + 32 ) & ~63;
        }
      }
    }

  Done_Width:
    if ( sign )
      dist = -dist;

    return dist;
  }


  

  static void
  af_cjk_align_linked_edge( AF_GlyphHints  hints,
                            AF_Dimension   dim,
                            AF_Edge        base_edge,
                            AF_Edge        stem_edge )
  {
    FT_Pos  dist = stem_edge->opos - base_edge->opos;

    FT_Pos  fitted_width = af_cjk_compute_stem_width(
                             hints, dim, dist,
                             (AF_Edge_Flags)base_edge->flags,
                             (AF_Edge_Flags)stem_edge->flags );


    stem_edge->pos = base_edge->pos + fitted_width;
  }


  static void
  af_cjk_align_serif_edge( AF_GlyphHints  hints,
                           AF_Edge        base,
                           AF_Edge        serif )
  {
    FT_UNUSED( hints );

    serif->pos = base->pos + ( serif->opos - base->opos );
  }


  
  
  
  
  
  
  
  
  


#define AF_LIGHT_MODE_MAX_HORZ_GAP    9
#define AF_LIGHT_MODE_MAX_VERT_GAP   15
#define AF_LIGHT_MODE_MAX_DELTA_ABS  14


  static FT_Pos
  af_hint_normal_stem( AF_GlyphHints  hints,
                       AF_Edge        edge,
                       AF_Edge        edge2,
                       FT_Pos         anchor,
                       AF_Dimension   dim )
  {
    FT_Pos  org_len, cur_len, org_center;
    FT_Pos  cur_pos1, cur_pos2;
    FT_Pos  d_off1, u_off1, d_off2, u_off2, delta;
    FT_Pos  offset;
    FT_Pos  threshold = 64;


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
    {
      if ( ( edge->flags  & AF_EDGE_ROUND ) &&
           ( edge2->flags & AF_EDGE_ROUND ) )
      {
        if ( dim == AF_DIMENSION_VERT )
          threshold = 64 - AF_LIGHT_MODE_MAX_HORZ_GAP;
        else
          threshold = 64 - AF_LIGHT_MODE_MAX_VERT_GAP;
      }
      else
      {
        if ( dim == AF_DIMENSION_VERT )
          threshold = 64 - AF_LIGHT_MODE_MAX_HORZ_GAP / 3;
        else
          threshold = 64 - AF_LIGHT_MODE_MAX_VERT_GAP / 3;
      }
    }

    org_len    = edge2->opos - edge->opos;
    cur_len    = af_cjk_compute_stem_width( hints, dim, org_len,
                                            (AF_Edge_Flags)edge->flags,
                                            (AF_Edge_Flags)edge2->flags );

    org_center = ( edge->opos + edge2->opos ) / 2 + anchor;
    cur_pos1   = org_center - cur_len / 2;
    cur_pos2   = cur_pos1 + cur_len;
    d_off1     = cur_pos1 - FT_PIX_FLOOR( cur_pos1 );
    d_off2     = cur_pos2 - FT_PIX_FLOOR( cur_pos2 );
    u_off1     = 64 - d_off1;
    u_off2     = 64 - d_off2;
    delta      = 0;


    if ( d_off1 == 0 || d_off2 == 0 )
      goto Exit;

    if ( cur_len <= threshold )
    {
      if ( d_off2 < cur_len )
      {
        if ( u_off1 <= d_off2 )
          delta =  u_off1;
        else
          delta = -d_off2;
      }

      goto Exit;
    }

    if ( threshold < 64 )
    {
      if ( d_off1 >= threshold || u_off1 >= threshold ||
           d_off2 >= threshold || u_off2 >= threshold )
        goto Exit;
    }

    offset = cur_len & 63;

    if ( offset < 32 )
    {
      if ( u_off1 <= offset || d_off2 <= offset )
        goto Exit;
    }
    else
      offset = 64 - threshold;

    d_off1 = threshold - u_off1;
    u_off1 = u_off1    - offset;
    u_off2 = threshold - d_off2;
    d_off2 = d_off2    - offset;

    if ( d_off1 <= u_off1 )
      u_off1 = -d_off1;

    if ( d_off2 <= u_off2 )
      u_off2 = -d_off2;

    if ( FT_ABS( u_off1 ) <= FT_ABS( u_off2 ) )
      delta = u_off1;
    else
      delta = u_off2;

  Exit:

#if 1
    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
    {
      if ( delta > AF_LIGHT_MODE_MAX_DELTA_ABS )
        delta = AF_LIGHT_MODE_MAX_DELTA_ABS;
      else if ( delta < -AF_LIGHT_MODE_MAX_DELTA_ABS )
        delta = -AF_LIGHT_MODE_MAX_DELTA_ABS;
    }
#endif

    cur_pos1 += delta;

    if ( edge->opos < edge2->opos )
    {
      edge->pos  = cur_pos1;
      edge2->pos = cur_pos1 + cur_len;
    }
    else
    {
      edge->pos  = cur_pos1 + cur_len;
      edge2->pos = cur_pos1;
    }

    return delta;
  }


  static void
  af_cjk_hint_edges( AF_GlyphHints  hints,
                     AF_Dimension   dim )
  {
    AF_AxisHints  axis       = &hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    FT_PtrDist    n_edges;
    AF_Edge       edge;
    AF_Edge       anchor   = 0;
    FT_Pos        delta    = 0;
    FT_Int        skipped  = 0;
    FT_Bool       has_last_stem = FALSE;
    FT_Pos        last_stem_pos = 0;


    
    FT_TRACE5(( "==== cjk hinting %s edges =====\n",
          dim == AF_DIMENSION_HORZ ? "vertical" : "horizontal" ));

    if ( AF_HINTS_DO_BLUES( hints ) )
    {
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        AF_Width  blue;
        AF_Edge   edge1, edge2;


        if ( edge->flags & AF_EDGE_DONE )
          continue;

        blue  = edge->blue_edge;
        edge1 = NULL;
        edge2 = edge->link;

        if ( blue )
        {
          edge1 = edge;
        }
        else if ( edge2 && edge2->blue_edge )
        {
          blue  = edge2->blue_edge;
          edge1 = edge2;
          edge2 = edge;
        }

        if ( !edge1 )
          continue;

        FT_TRACE5(( "CJKBLUE: edge %d @%d (opos=%.2f) snapped to (%.2f), "
                 "was (%.2f)\n",
                 edge1-edges, edge1->fpos, edge1->opos / 64.0, blue->fit / 64.0,
                 edge1->pos / 64.0 ));

        edge1->pos    = blue->fit;
        edge1->flags |= AF_EDGE_DONE;

        if ( edge2 && !edge2->blue_edge )
        {
          af_cjk_align_linked_edge( hints, dim, edge1, edge2 );
          edge2->flags |= AF_EDGE_DONE;
        }

        if ( !anchor )
          anchor = edge;
      }
    }

    
    for ( edge = edges; edge < edge_limit; edge++ )
    {
      AF_Edge  edge2;


      if ( edge->flags & AF_EDGE_DONE )
        continue;

      
      edge2 = edge->link;
      if ( !edge2 )
      {
        skipped++;
        continue;
      }

      







      if ( has_last_stem                       &&
           ( edge->pos  < last_stem_pos + 64 ||
             edge2->pos < last_stem_pos + 64 ) )
      {
        skipped++;
        continue;
      }

      
      
      if ( edge2->blue_edge )
      {
        FT_TRACE5(( "ASSERTION FAILED for edge %d\n", edge2-edges ));

        af_cjk_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
        continue;
      }

      if ( edge2 < edge )
      {
        af_cjk_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
        



        has_last_stem = TRUE;
        last_stem_pos = edge->pos;
        continue;
      }

      if ( dim != AF_DIMENSION_VERT && !anchor )
      {

#if 0
        if ( fixedpitch )
        {
          AF_Edge     left  = edge;
          AF_Edge     right = edge_limit - 1;
          AF_EdgeRec  left1, left2, right1, right2;
          FT_Pos      target, center1, center2;
          FT_Pos      delta1, delta2, d1, d2;


          while ( right > left && !right->link )
            right--;

          left1  = *left;
          left2  = *left->link;
          right1 = *right->link;
          right2 = *right;

          delta  = ( ( ( hinter->pp2.x + 32 ) & -64 ) - hinter->pp2.x ) / 2;
          target = left->opos + ( right->opos - left->opos ) / 2 + delta - 16;

          delta1  = delta;
          delta1 += af_hint_normal_stem( hints, left, left->link,
                                         delta1, 0 );

          if ( left->link != right )
            af_hint_normal_stem( hints, right->link, right, delta1, 0 );

          center1 = left->pos + ( right->pos - left->pos ) / 2;

          if ( center1 >= target )
            delta2 = delta - 32;
          else
            delta2 = delta + 32;

          delta2 += af_hint_normal_stem( hints, &left1, &left2, delta2, 0 );

          if ( delta1 != delta2 )
          {
            if ( left->link != right )
              af_hint_normal_stem( hints, &right1, &right2, delta2, 0 );

            center2 = left1.pos + ( right2.pos - left1.pos ) / 2;

            d1 = center1 - target;
            d2 = center2 - target;

            if ( FT_ABS( d2 ) < FT_ABS( d1 ) )
            {
              left->pos       = left1.pos;
              left->link->pos = left2.pos;

              if ( left->link != right )
              {
                right->link->pos = right1.pos;
                right->pos       = right2.pos;
              }

              delta1 = delta2;
            }
          }

          delta               = delta1;
          right->link->flags |= AF_EDGE_DONE;
          right->flags       |= AF_EDGE_DONE;
        }
        else

#endif 

          delta = af_hint_normal_stem( hints, edge, edge2, 0,
                                       AF_DIMENSION_HORZ );
      }
      else
        af_hint_normal_stem( hints, edge, edge2, delta, dim );

#if 0
      printf( "stem (%d,%d) adjusted (%.1f,%.1f)\n",
               edge - edges, edge2 - edges,
               ( edge->pos - edge->opos ) / 64.0,
               ( edge2->pos - edge2->opos ) / 64.0 );
#endif

      anchor = edge;
      edge->flags  |= AF_EDGE_DONE;
      edge2->flags |= AF_EDGE_DONE;
      has_last_stem = TRUE;
      last_stem_pos = edge2->pos;
    }

    

    
    
    
    
    
    
    
    

    
    
    

    n_edges = edge_limit - edges;
    if ( dim == AF_DIMENSION_HORZ && ( n_edges == 6 || n_edges == 12 ) )
    {
      AF_Edge  edge1, edge2, edge3;
      FT_Pos   dist1, dist2, span;


      if ( n_edges == 6 )
      {
        edge1 = edges;
        edge2 = edges + 2;
        edge3 = edges + 4;
      }
      else
      {
        edge1 = edges + 1;
        edge2 = edges + 5;
        edge3 = edges + 9;
      }

      dist1 = edge2->opos - edge1->opos;
      dist2 = edge3->opos - edge2->opos;

      span = dist1 - dist2;
      if ( span < 0 )
        span = -span;

      if ( edge1->link == edge1 + 1 &&
           edge2->link == edge2 + 1 &&
           edge3->link == edge3 + 1 && span < 8 )
      {
        delta = edge3->pos - ( 2 * edge2->pos - edge1->pos );
        edge3->pos -= delta;
        if ( edge3->link )
          edge3->link->pos -= delta;

        
        if ( n_edges == 12 )
        {
          ( edges + 8 )->pos -= delta;
          ( edges + 11 )->pos -= delta;
        }

        edge3->flags |= AF_EDGE_DONE;
        if ( edge3->link )
          edge3->link->flags |= AF_EDGE_DONE;
      }
    }

    if ( !skipped )
      return;

    



    for ( edge = edges; edge < edge_limit; edge++ )
    {
      if ( edge->flags & AF_EDGE_DONE )
        continue;

      if ( edge->serif )
      {
        af_cjk_align_serif_edge( hints, edge->serif, edge );
        edge->flags |= AF_EDGE_DONE;
        skipped--;
      }
    }

    if ( !skipped )
      return;

    for ( edge = edges; edge < edge_limit; edge++ )
    {
      AF_Edge  before, after;


      if ( edge->flags & AF_EDGE_DONE )
        continue;

      before = after = edge;

      while ( --before >= edges )
        if ( before->flags & AF_EDGE_DONE )
          break;

      while ( ++after < edge_limit )
        if ( after->flags & AF_EDGE_DONE )
          break;

      if ( before >= edges || after < edge_limit )
      {
        if ( before < edges )
          af_cjk_align_serif_edge( hints, after, edge );
        else if ( after >= edge_limit )
          af_cjk_align_serif_edge( hints, before, edge );
        else
        {
          if ( after->fpos == before->fpos )
            edge->pos = before->pos;
          else
            edge->pos = before->pos +
                        FT_MulDiv( edge->fpos - before->fpos,
                                   after->pos - before->pos,
                                   after->fpos - before->fpos );
        }
      }
    }
  }


  static void
  af_cjk_align_edge_points( AF_GlyphHints  hints,
                            AF_Dimension   dim )
  {
    AF_AxisHints  axis       = & hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    AF_Edge       edge;
    FT_Bool       snapping;


    snapping = FT_BOOL( ( dim == AF_DIMENSION_HORZ             &&
                          AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) )  ||
                        ( dim == AF_DIMENSION_VERT             &&
                          AF_LATIN_HINTS_DO_VERT_SNAP( hints ) )  );

    for ( edge = edges; edge < edge_limit; edge++ )
    {
      
      
      AF_Segment  seg = edge->first;


      if ( snapping )
      {
        do
        {
          AF_Point  point = seg->first;


          for (;;)
          {
            if ( dim == AF_DIMENSION_HORZ )
            {
              point->x      = edge->pos;
              point->flags |= AF_FLAG_TOUCH_X;
            }
            else
            {
              point->y      = edge->pos;
              point->flags |= AF_FLAG_TOUCH_Y;
            }

            if ( point == seg->last )
              break;

            point = point->next;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }
      else
      {
        FT_Pos  delta = edge->pos - edge->opos;


        do
        {
          AF_Point  point = seg->first;


          for (;;)
          {
            if ( dim == AF_DIMENSION_HORZ )
            {
              point->x     += delta;
              point->flags |= AF_FLAG_TOUCH_X;
            }
            else
            {
              point->y     += delta;
              point->flags |= AF_FLAG_TOUCH_Y;
            }

            if ( point == seg->last )
              break;

            point = point->next;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_cjk_hints_apply( AF_GlyphHints  hints,
                      FT_Outline*    outline,
                      AF_CJKMetrics  metrics )
  {
    FT_Error  error;
    int       dim;

    FT_UNUSED( metrics );


    error = af_glyph_hints_reload( hints, outline );
    if ( error )
      goto Exit;

    
    if ( AF_HINTS_DO_HORIZONTAL( hints ) )
    {
      error = af_cjk_hints_detect_features( hints, AF_DIMENSION_HORZ );
      if ( error )
        goto Exit;

      af_cjk_hints_compute_blue_edges( hints, metrics, AF_DIMENSION_HORZ );
    }

    if ( AF_HINTS_DO_VERTICAL( hints ) )
    {
      error = af_cjk_hints_detect_features( hints, AF_DIMENSION_VERT );
      if ( error )
        goto Exit;

      af_cjk_hints_compute_blue_edges( hints, metrics, AF_DIMENSION_VERT );
    }

    
    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
      if ( ( dim == AF_DIMENSION_HORZ && AF_HINTS_DO_HORIZONTAL( hints ) ) ||
           ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_VERTICAL( hints ) )   )
      {

#ifdef AF_CONFIG_OPTION_USE_WARPER
        if ( dim == AF_DIMENSION_HORZ                                  &&
             metrics->root.scaler.render_mode == FT_RENDER_MODE_NORMAL )
        {
          AF_WarperRec  warper;
          FT_Fixed      scale;
          FT_Pos        delta;


          af_warper_compute( &warper, hints, (AF_Dimension)dim,
                             &scale, &delta );
          af_glyph_hints_scale_dim( hints, (AF_Dimension)dim,
                                    scale, delta );
          continue;
        }
#endif 

        af_cjk_hint_edges( hints, (AF_Dimension)dim );
        af_cjk_align_edge_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_strong_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_weak_points( hints, (AF_Dimension)dim );
      }
    }

#if 0
    af_glyph_hints_dump_points( hints );
    af_glyph_hints_dump_segments( hints );
    af_glyph_hints_dump_edges( hints );
#endif

    af_glyph_hints_save( hints, outline );

  Exit:
    return error;
  }


  
  
  
  
  
  
  


  

  static const AF_Script_UniRangeRec  af_cjk_uniranges[] =
  {
    AF_UNIRANGE_REC(  0x2E80UL,  0x2EFFUL ),  
    AF_UNIRANGE_REC(  0x2F00UL,  0x2FDFUL ),  
    AF_UNIRANGE_REC(  0x2FF0UL,  0x2FFFUL ),  
    AF_UNIRANGE_REC(  0x3000UL,  0x303FUL ),  
    AF_UNIRANGE_REC(  0x3040UL,  0x309FUL ),  
    AF_UNIRANGE_REC(  0x30A0UL,  0x30FFUL ),  
    AF_UNIRANGE_REC(  0x3100UL,  0x312FUL ),  
    AF_UNIRANGE_REC(  0x3130UL,  0x318FUL ),  
    AF_UNIRANGE_REC(  0x3190UL,  0x319FUL ),  
    AF_UNIRANGE_REC(  0x31A0UL,  0x31BFUL ),  
    AF_UNIRANGE_REC(  0x31C0UL,  0x31EFUL ),  
    AF_UNIRANGE_REC(  0x31F0UL,  0x31FFUL ),  
    AF_UNIRANGE_REC(  0x3200UL,  0x32FFUL ),  
    AF_UNIRANGE_REC(  0x3300UL,  0x33FFUL ),  
    AF_UNIRANGE_REC(  0x3400UL,  0x4DBFUL ),  
    AF_UNIRANGE_REC(  0x4DC0UL,  0x4DFFUL ),  
    AF_UNIRANGE_REC(  0x4E00UL,  0x9FFFUL ),  
    AF_UNIRANGE_REC(  0xA960UL,  0xA97FUL ),  
    AF_UNIRANGE_REC(  0xAC00UL,  0xD7AFUL ),  
    AF_UNIRANGE_REC(  0xD7B0UL,  0xD7FFUL ),  
    AF_UNIRANGE_REC(  0xF900UL,  0xFAFFUL ),  
    AF_UNIRANGE_REC(  0xFE10UL,  0xFE1FUL ),  
    AF_UNIRANGE_REC(  0xFE30UL,  0xFE4FUL ),  
    AF_UNIRANGE_REC(  0xFF00UL,  0xFFEFUL ),  
    AF_UNIRANGE_REC( 0x1B000UL, 0x1B0FFUL ),  
    AF_UNIRANGE_REC( 0x1D300UL, 0x1D35FUL ),  
    AF_UNIRANGE_REC( 0x1F200UL, 0x1F2FFUL ),  
    AF_UNIRANGE_REC( 0x20000UL, 0x2A6DFUL ),  
    AF_UNIRANGE_REC( 0x2A700UL, 0x2B73FUL ),  
    AF_UNIRANGE_REC( 0x2B740UL, 0x2B81FUL ),  
    AF_UNIRANGE_REC( 0x2F800UL, 0x2FA1FUL ),  
    AF_UNIRANGE_REC(       0UL,       0UL )
  };


  AF_DEFINE_SCRIPT_CLASS( af_cjk_script_class,
    AF_SCRIPT_CJK,
    af_cjk_uniranges,

    sizeof ( AF_CJKMetricsRec ),

    (AF_Script_InitMetricsFunc) af_cjk_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_cjk_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_cjk_hints_init,
    (AF_Script_ApplyHintsFunc)  af_cjk_hints_apply
  )

#else 

  static const AF_Script_UniRangeRec  af_cjk_uniranges[] =
  {
    AF_UNIRANGE_REC( 0UL, 0UL )
  };


  AF_DEFINE_SCRIPT_CLASS( af_cjk_script_class,
    AF_SCRIPT_CJK,
    af_cjk_uniranges,

    sizeof ( AF_CJKMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  )

#endif 



