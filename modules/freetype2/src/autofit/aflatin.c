

















#include <ft2build.h>
#include FT_ADVANCES_H

#include "aflatin.h"
#include "aferrors.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  
  
  
  
  
  
  

  FT_LOCAL_DEF( void )
  af_latin_metrics_init_widths( AF_LatinMetrics  metrics,
                                FT_Face          face,
                                FT_ULong         charcode )
  {
    
    AF_GlyphHintsRec  hints[1];


    af_glyph_hints_init( hints, face->memory );

    metrics->axis[AF_DIMENSION_HORZ].width_count = 0;
    metrics->axis[AF_DIMENSION_VERT].width_count = 0;

    {
      FT_Error             error;
      FT_UInt              glyph_index;
      int                  dim;
      AF_LatinMetricsRec   dummy[1];
      AF_Scaler            scaler = &dummy->root.scaler;


      glyph_index = FT_Get_Char_Index( face, charcode );
      if ( glyph_index == 0 )
        goto Exit;

      error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
      if ( error || face->glyph->outline.n_points <= 0 )
        goto Exit;

      FT_ZERO( dummy );

      dummy->units_per_em = metrics->units_per_em;
      scaler->x_scale     = scaler->y_scale = 0x10000L;
      scaler->x_delta     = scaler->y_delta = 0;
      scaler->face        = face;
      scaler->render_mode = FT_RENDER_MODE_NORMAL;
      scaler->flags       = 0;

      af_glyph_hints_rescale( hints, (AF_ScriptMetrics)dummy );

      error = af_glyph_hints_reload( hints, &face->glyph->outline );
      if ( error )
        goto Exit;

      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_LatinAxis  axis    = &metrics->axis[dim];
        AF_AxisHints  axhints = &hints->axis[dim];
        AF_Segment    seg, limit, link;
        FT_UInt       num_widths = 0;


        error = af_latin_hints_compute_segments( hints,
                                                 (AF_Dimension)dim );
        if ( error )
          goto Exit;

        af_latin_hints_link_segments( hints,
                                      (AF_Dimension)dim );

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

            if ( num_widths < AF_LATIN_MAX_WIDTHS )
              axis->widths[ num_widths++ ].org = dist;
          }
        }

        af_sort_widths( num_widths, axis->widths );
        axis->width_count = num_widths;
      }

  Exit:
      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_LatinAxis  axis = &metrics->axis[dim];
        FT_Pos        stdw;


        stdw = ( axis->width_count > 0 )
                 ? axis->widths[0].org
                 : AF_LATIN_CONSTANT( metrics, 50 );

        
        axis->edge_distance_threshold = stdw / 5;
        axis->standard_width          = stdw;
        axis->extra_light             = 0;
      }
    }

    af_glyph_hints_done( hints );
  }



#define AF_LATIN_MAX_TEST_CHARACTERS  12


  static const char af_latin_blue_chars[AF_LATIN_MAX_BLUES]
                                       [AF_LATIN_MAX_TEST_CHARACTERS + 1] =
  {
    "THEZOCQS",
    "HEZLOCUS",
    "fijkdbh",
    "xzroesc",
    "xzroesc",
    "pqgjy"
  };


  static void
  af_latin_metrics_init_blues( AF_LatinMetrics  metrics,
                               FT_Face          face )
  {
    FT_Pos        flats [AF_LATIN_MAX_TEST_CHARACTERS];
    FT_Pos        rounds[AF_LATIN_MAX_TEST_CHARACTERS];
    FT_Int        num_flats;
    FT_Int        num_rounds;
    FT_Int        bb;
    AF_LatinBlue  blue;
    FT_Error      error;
    AF_LatinAxis  axis  = &metrics->axis[AF_DIMENSION_VERT];
    FT_GlyphSlot  glyph = face->glyph;


    
    
    

    AF_LOG(( "blue zones computation\n" ));
    AF_LOG(( "------------------------------------------------\n" ));

    for ( bb = 0; bb < AF_LATIN_BLUE_MAX; bb++ )
    {
      const char*  p     = af_latin_blue_chars[bb];
      const char*  limit = p + AF_LATIN_MAX_TEST_CHARACTERS;
      FT_Pos*      blue_ref;
      FT_Pos*      blue_shoot;


      AF_LOG(( "blue %3d: ", bb ));

      num_flats  = 0;
      num_rounds = 0;

      for ( ; p < limit && *p; p++ )
      {
        FT_UInt     glyph_index;
        FT_Pos      best_y; 
        FT_Int      best_point, best_first, best_last;
        FT_Vector*  points;
        FT_Bool     round = 0;


        AF_LOG(( "'%c'", *p ));

        
        glyph_index = FT_Get_Char_Index( face, (FT_UInt)*p );
        if ( glyph_index == 0 )
          continue;

        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
        if ( error || glyph->outline.n_points <= 0 )
          continue;

        
        points      = glyph->outline.points;
        best_point  = -1;
        best_y      = 0;  
        best_first  = 0;  
        best_last   = 0;  

        {
          FT_Int  nn;
          FT_Int  first = 0;
          FT_Int  last  = -1;


          for ( nn = 0; nn < glyph->outline.n_contours; first = last+1, nn++ )
          {
            FT_Int  old_best_point = best_point;
            FT_Int  pp;


            last = glyph->outline.contours[nn];

            
            
            
            if ( last <= first )
                continue;

            if ( AF_LATIN_IS_TOP_BLUE( bb ) )
            {
              for ( pp = first; pp <= last; pp++ )
                if ( best_point < 0 || points[pp].y > best_y )
                {
                  best_point = pp;
                  best_y     = points[pp].y;
                }
            }
            else
            {
              for ( pp = first; pp <= last; pp++ )
                if ( best_point < 0 || points[pp].y < best_y )
                {
                  best_point = pp;
                  best_y     = points[pp].y;
                }
            }

            if ( best_point != old_best_point )
            {
              best_first = first;
              best_last  = last;
            }
          }
          AF_LOG(( "%5d", best_y ));
        }

        
        
        
        if ( best_point >= 0 )
        {
          FT_Int  prev, next;
          FT_Pos  dist;


          
          
          prev = best_point;
          next = prev;

          do
          {
            if ( prev > best_first )
              prev--;
            else
              prev = best_last;

            dist = points[prev].y - best_y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( prev != best_point );

          do
          {
            if ( next < best_last )
              next++;
            else
              next = best_first;

            dist = points[next].y - best_y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( next != best_point );

          
          round = FT_BOOL(
            FT_CURVE_TAG( glyph->outline.tags[prev] ) != FT_CURVE_TAG_ON ||
            FT_CURVE_TAG( glyph->outline.tags[next] ) != FT_CURVE_TAG_ON );

          AF_LOG(( "%c ", round ? 'r' : 'f' ));
        }

        if ( round )
          rounds[num_rounds++] = best_y;
        else
          flats[num_flats++]   = best_y;
      }

      AF_LOG(( "\n" ));

      if ( num_flats == 0 && num_rounds == 0 )
      {
        



        AF_LOG(( "empty\n" ));
        continue;
      }

      
      
      
      af_sort_pos( num_rounds, rounds );
      af_sort_pos( num_flats,  flats );

      blue       = & axis->blues[axis->blue_count];
      blue_ref   = & blue->ref.org;
      blue_shoot = & blue->shoot.org;

      axis->blue_count++;

      if ( num_flats == 0 )
      {
        *blue_ref   =
        *blue_shoot = rounds[num_rounds / 2];
      }
      else if ( num_rounds == 0 )
      {
        *blue_ref   =
        *blue_shoot = flats[num_flats / 2];
      }
      else
      {
        *blue_ref   = flats[num_flats / 2];
        *blue_shoot = rounds[num_rounds / 2];
      }

      
      
      
      if ( *blue_shoot != *blue_ref )
      {
        FT_Pos   ref      = *blue_ref;
        FT_Pos   shoot    = *blue_shoot;
        FT_Bool  over_ref = FT_BOOL( shoot > ref );


        if ( AF_LATIN_IS_TOP_BLUE( bb ) ^ over_ref )
          *blue_shoot = *blue_ref = ( shoot + ref ) / 2;
      }

      blue->flags = 0;
      if ( AF_LATIN_IS_TOP_BLUE( bb ) )
        blue->flags |= AF_LATIN_BLUE_TOP;

      




      if ( bb == AF_LATIN_BLUE_SMALL_TOP )
        blue->flags |= AF_LATIN_BLUE_ADJUSTMENT;

      AF_LOG(( "-- ref = %ld, shoot = %ld\n", *blue_ref, *blue_shoot ));
    }

    return;
  }


  FT_LOCAL_DEF( void )
  af_latin_metrics_check_digits( AF_LatinMetrics  metrics,
                                 FT_Face          face )
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
  af_latin_metrics_init( AF_LatinMetrics  metrics,
                         FT_Face          face )
  {
    FT_Error    error = AF_Err_Ok;
    FT_CharMap  oldmap = face->charmap;
    FT_UInt     ee;

    static const FT_Encoding  latin_encodings[] =
    {
      FT_ENCODING_UNICODE,
      FT_ENCODING_APPLE_ROMAN,
      FT_ENCODING_ADOBE_STANDARD,
      FT_ENCODING_ADOBE_LATIN_1,
      FT_ENCODING_NONE  
    };


    metrics->units_per_em = face->units_per_EM;

    
    for ( ee = 0; latin_encodings[ee] != FT_ENCODING_NONE; ee++ )
    {
      error = FT_Select_Charmap( face, latin_encodings[ee] );
      if ( !error )
        break;
    }

    if ( !error )
    {
      
      af_latin_metrics_init_widths( metrics, face, 'o' );
      af_latin_metrics_init_blues( metrics, face );
      af_latin_metrics_check_digits( metrics, face );
    }

    FT_Set_Charmap( face, oldmap );
    return AF_Err_Ok;
  }


  static void
  af_latin_metrics_scale_dim( AF_LatinMetrics  metrics,
                              AF_Scaler        scaler,
                              AF_Dimension     dim )
  {
    FT_Fixed      scale;
    FT_Pos        delta;
    AF_LatinAxis  axis;
    FT_UInt       nn;


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

    axis = &metrics->axis[dim];

    if ( axis->org_scale == scale && axis->org_delta == delta )
      return;

    axis->org_scale = scale;
    axis->org_delta = delta;

    



    {
      AF_LatinAxis  Axis = &metrics->axis[AF_DIMENSION_VERT];
      AF_LatinBlue  blue = NULL;


      for ( nn = 0; nn < Axis->blue_count; nn++ )
      {
        if ( Axis->blues[nn].flags & AF_LATIN_BLUE_ADJUSTMENT )
        {
          blue = &Axis->blues[nn];
          break;
        }
      }

      if ( blue )
      {
        FT_Pos  scaled = FT_MulFix( blue->shoot.org, scaler->y_scale );
        FT_Pos  fitted = ( scaled + 40 ) & ~63;


        if ( scaled != fitted )
        {
#if 0
          if ( dim == AF_DIMENSION_HORZ )
          {
            if ( fitted < scaled )
              scale -= scale / 50;  
          }
          else
#endif
          if ( dim == AF_DIMENSION_VERT )
          {
            scale = FT_MulDiv( scale, fitted, scaled );
          }
        }
      }
    }

    axis->scale = scale;
    axis->delta = delta;

    if ( dim == AF_DIMENSION_HORZ )
    {
      metrics->root.scaler.x_scale = scale;
      metrics->root.scaler.x_delta = delta;
    }
    else
    {
      metrics->root.scaler.y_scale = scale;
      metrics->root.scaler.y_delta = delta;
    }

    
    for ( nn = 0; nn < axis->width_count; nn++ )
    {
      AF_Width  width = axis->widths + nn;


      width->cur = FT_MulFix( width->org, scale );
      width->fit = width->cur;
    }

    
    
    axis->extra_light =
      (FT_Bool)( FT_MulFix( axis->standard_width, scale ) < 32 + 8 );

    if ( dim == AF_DIMENSION_VERT )
    {
      
      for ( nn = 0; nn < axis->blue_count; nn++ )
      {
        AF_LatinBlue  blue = &axis->blues[nn];
        FT_Pos        dist;


        blue->ref.cur   = FT_MulFix( blue->ref.org, scale ) + delta;
        blue->ref.fit   = blue->ref.cur;
        blue->shoot.cur = FT_MulFix( blue->shoot.org, scale ) + delta;
        blue->shoot.fit = blue->shoot.cur;
        blue->flags    &= ~AF_LATIN_BLUE_ACTIVE;

        
        dist = FT_MulFix( blue->ref.org - blue->shoot.org, scale );
        if ( dist <= 48 && dist >= -48 )
        {
          FT_Pos  delta1, delta2;


          delta1 = blue->shoot.org - blue->ref.org;
          delta2 = delta1;
          if ( delta1 < 0 )
            delta2 = -delta2;

          delta2 = FT_MulFix( delta2, scale );

          if ( delta2 < 32 )
            delta2 = 0;
          else if ( delta2 < 64 )
            delta2 = 32 + ( ( ( delta2 - 32 ) + 16 ) & ~31 );
          else
            delta2 = FT_PIX_ROUND( delta2 );

          if ( delta1 < 0 )
            delta2 = -delta2;

          blue->ref.fit   = FT_PIX_ROUND( blue->ref.cur );
          blue->shoot.fit = blue->ref.fit + delta2;

          blue->flags |= AF_LATIN_BLUE_ACTIVE;
        }
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_latin_metrics_scale( AF_LatinMetrics  metrics,
                          AF_Scaler        scaler )
  {
    metrics->root.scaler.render_mode = scaler->render_mode;
    metrics->root.scaler.face        = scaler->face;

    af_latin_metrics_scale_dim( metrics, scaler, AF_DIMENSION_HORZ );
    af_latin_metrics_scale_dim( metrics, scaler, AF_DIMENSION_VERT );
  }


  
  
  
  
  
  
  

  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_compute_segments( AF_GlyphHints  hints,
                                   AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    FT_Memory     memory        = hints->memory;
    FT_Error      error         = AF_Err_Ok;
    AF_Segment    segment       = NULL;
    AF_SegmentRec seg0;
    AF_Point*     contour       = hints->contours;
    AF_Point*     contour_limit = contour + hints->num_contours;
    AF_Direction  major_dir, segment_dir;


    FT_ZERO( &seg0 );
    seg0.score = 32000;
    seg0.flags = AF_EDGE_NORMAL;

    major_dir   = (AF_Direction)FT_ABS( axis->major_dir );
    segment_dir = major_dir;

    axis->num_segments = 0;

    
    if ( dim == AF_DIMENSION_HORZ )
    {
      AF_Point  point = hints->points;
      AF_Point  limit = point + hints->num_points;


      for ( ; point < limit; point++ )
      {
        point->u = point->fx;
        point->v = point->fy;
      }
    }
    else
    {
      AF_Point  point = hints->points;
      AF_Point  limit = point + hints->num_points;


      for ( ; point < limit; point++ )
      {
        point->u = point->fy;
        point->v = point->fx;
      }
    }

    
    for ( ; contour < contour_limit; contour++ )
    {
      AF_Point  point   =  contour[0];
      AF_Point  last    =  point->prev;
      int       on_edge =  0;
      FT_Pos    min_pos =  32000;  
      FT_Pos    max_pos = -32000;  
      FT_Bool   passed;


      if ( point == last )  
        continue;

      if ( FT_ABS( last->out_dir )  == major_dir &&
           FT_ABS( point->out_dir ) == major_dir )
      {
        
        last = point;

        for (;;)
        {
          point = point->prev;
          if ( FT_ABS( point->out_dir ) != major_dir )
          {
            point = point->next;
            break;
          }
          if ( point == last )
            break;
        }
      }

      last   = point;
      passed = 0;

      for (;;)
      {
        FT_Pos  u, v;


        if ( on_edge )
        {
          u = point->u;
          if ( u < min_pos )
            min_pos = u;
          if ( u > max_pos )
            max_pos = u;

          if ( point->out_dir != segment_dir || point == last )
          {
            
            segment->last = point;
            segment->pos  = (FT_Short)( ( min_pos + max_pos ) >> 1 );

            
            
            if ( ( segment->first->flags | point->flags ) &
                   AF_FLAG_CONTROL                        )
              segment->flags |= AF_EDGE_ROUND;

            
            min_pos = max_pos = point->v;

            v = segment->first->v;
            if ( v < min_pos )
              min_pos = v;
            if ( v > max_pos )
              max_pos = v;

            segment->min_coord = (FT_Short)min_pos;
            segment->max_coord = (FT_Short)max_pos;
            segment->height    = (FT_Short)( segment->max_coord -
                                             segment->min_coord );

            on_edge = 0;
            segment = NULL;
            
          }
        }

        
        if ( point == last )
        {
          if ( passed )
            break;
          passed = 1;
        }

        if ( !on_edge && FT_ABS( point->out_dir ) == major_dir )
        {
          
          segment_dir = (AF_Direction)point->out_dir;

          
          error = af_axis_hints_new_segment( axis, memory, &segment );
          if ( error )
            goto Exit;

          segment[0]        = seg0;
          segment->dir      = (FT_Char)segment_dir;
          min_pos = max_pos = point->u;
          segment->first    = point;
          segment->last     = point;
          segment->contour  = contour;
          on_edge           = 1;
        }

        point = point->next;
      }

    } 


    
    
    {
      AF_Segment  segments     = axis->segments;
      AF_Segment  segments_end = segments + axis->num_segments;


      for ( segment = segments; segment < segments_end; segment++ )
      {
        AF_Point  first   = segment->first;
        AF_Point  last    = segment->last;
        FT_Pos    first_v = first->v;
        FT_Pos    last_v  = last->v;


        if ( first == last )
          continue;

        if ( first_v < last_v )
        {
          AF_Point  p;


          p = first->prev;
          if ( p->v < first_v )
            segment->height = (FT_Short)( segment->height +
                                          ( ( first_v - p->v ) >> 1 ) );

          p = last->next;
          if ( p->v > last_v )
            segment->height = (FT_Short)( segment->height +
                                          ( ( p->v - last_v ) >> 1 ) );
        }
        else
        {
          AF_Point  p;


          p = first->prev;
          if ( p->v > first_v )
            segment->height = (FT_Short)( segment->height +
                                          ( ( p->v - first_v ) >> 1 ) );

          p = last->next;
          if ( p->v < last_v )
            segment->height = (FT_Short)( segment->height +
                                          ( ( last_v - p->v ) >> 1 ) );
        }
      }
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  af_latin_hints_link_segments( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    FT_Pos        len_threshold, len_score;
    AF_Segment    seg1, seg2;


    len_threshold = AF_LATIN_CONSTANT( hints->metrics, 8 );
    if ( len_threshold == 0 )
      len_threshold = 1;

    len_score = AF_LATIN_CONSTANT( hints->metrics, 6000 );

    
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      
      
      if ( seg1->dir != axis->major_dir || seg1->first == seg1->last )
        continue;

      for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        if ( seg1->dir + seg2->dir == 0 && seg2->pos > seg1->pos )
        {
          FT_Pos  pos1 = seg1->pos;
          FT_Pos  pos2 = seg2->pos;
          FT_Pos  dist = pos2 - pos1;


          if ( dist < 0 )
            dist = -dist;

          {
            FT_Pos  min = seg1->min_coord;
            FT_Pos  max = seg1->max_coord;
            FT_Pos  len, score;


            if ( min < seg2->min_coord )
              min = seg2->min_coord;

            if ( max > seg2->max_coord )
              max = seg2->max_coord;

            len = max - min;
            if ( len >= len_threshold )
            {
              score = dist + len_score / len;

              if ( score < seg1->score )
              {
                seg1->score = score;
                seg1->link  = seg2;
              }

              if ( score < seg2->score )
              {
                seg2->score = score;
                seg2->link  = seg1;
              }
            }
          }
        }
    }

    
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      seg2 = seg1->link;

      if ( seg2 )
      {
        if ( seg2->link != seg1 )
        {
          seg1->link  = 0;
          seg1->serif = seg2->link;
        }
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_compute_edges( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    AF_AxisHints  axis   = &hints->axis[dim];
    FT_Error      error  = AF_Err_Ok;
    FT_Memory     memory = hints->memory;
    AF_LatinAxis  laxis  = &((AF_LatinMetrics)hints->metrics)->axis[dim];

    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Segment    seg;

    AF_Direction  up_dir;
    FT_Fixed      scale;
    FT_Pos        edge_distance_threshold;
    FT_Pos        segment_length_threshold;


    axis->num_edges = 0;

    scale = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                         : hints->y_scale;

    up_dir = ( dim == AF_DIMENSION_HORZ ) ? AF_DIR_UP
                                          : AF_DIR_RIGHT;

    




    if ( dim == AF_DIMENSION_HORZ )
        segment_length_threshold = FT_DivFix( 64, hints->y_scale );
    else
        segment_length_threshold = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    edge_distance_threshold = FT_MulFix( laxis->edge_distance_threshold,
                                         scale );
    if ( edge_distance_threshold > 64 / 4 )
      edge_distance_threshold = 64 / 4;

    edge_distance_threshold = FT_DivFix( edge_distance_threshold,
                                         scale );

    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Edge  found = 0;
      FT_Int   ee;


      if ( seg->height < segment_length_threshold )
        continue;

      
      
      if ( seg->serif                                     &&
           2 * seg->height < 3 * segment_length_threshold )
        continue;

      
      for ( ee = 0; ee < axis->num_edges; ee++ )
      {
        AF_Edge  edge = axis->edges + ee;
        FT_Pos   dist;


        dist = seg->pos - edge->fpos;
        if ( dist < 0 )
          dist = -dist;

        if ( dist < edge_distance_threshold && edge->dir == seg->dir )
        {
          found = edge;
          break;
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
        edge->dir      = seg->dir;
        edge->opos     = edge->pos = FT_MulFix( seg->pos, scale );
        seg->edge_next = seg;
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
        FT_Pos  ups         = 0;  
        FT_Pos  downs       = 0;  


        seg = edge->first;

        do
        {
          FT_Bool  is_serif;


          
          if ( seg->flags & AF_EDGE_ROUND )
            is_round++;
          else
            is_straight++;

          
          if ( seg->dir == up_dir )
            ups   += seg->max_coord-seg->min_coord;
          else
            downs += seg->max_coord-seg->min_coord;

          
          
          is_serif = (FT_Bool)( seg->serif               &&
                                seg->serif->edge         &&
                                seg->serif->edge != edge );

          if ( ( seg->link && seg->link->edge != NULL ) || is_serif )
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

              seg_delta = seg->pos - seg2->pos;
              if ( seg_delta < 0 )
                seg_delta = -seg_delta;

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

#if 0
        
        edge->dir = AF_DIR_NONE;

        if ( ups > downs )
          edge->dir = (FT_Char)up_dir;

        else if ( ups < downs )
          edge->dir = (FT_Char)-up_dir;

        else if ( ups == downs )
          edge->dir = 0;  
#endif

        
        
        

        if ( edge->serif && edge->link )
          edge->serif = 0;
      }
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_detect_features( AF_GlyphHints  hints,
                                  AF_Dimension   dim )
  {
    FT_Error  error;


    error = af_latin_hints_compute_segments( hints, dim );
    if ( !error )
    {
      af_latin_hints_link_segments( hints, dim );

      error = af_latin_hints_compute_edges( hints, dim );
    }
    return error;
  }


  FT_LOCAL_DEF( void )
  af_latin_hints_compute_blue_edges( AF_GlyphHints    hints,
                                     AF_LatinMetrics  metrics )
  {
    AF_AxisHints  axis       = &hints->axis[ AF_DIMENSION_VERT ];
    AF_Edge       edge       = axis->edges;
    AF_Edge       edge_limit = edge + axis->num_edges;
    AF_LatinAxis  latin      = &metrics->axis[ AF_DIMENSION_VERT ];
    FT_Fixed      scale      = latin->scale;


    
    

    
    for ( ; edge < edge_limit; edge++ )
    {
      FT_Int    bb;
      AF_Width  best_blue = NULL;
      FT_Pos    best_dist;  


      
      best_dist = FT_MulFix( metrics->units_per_em / 40, scale );

      if ( best_dist > 64 / 2 )
        best_dist = 64 / 2;

      for ( bb = 0; bb < AF_LATIN_BLUE_MAX; bb++ )
      {
        AF_LatinBlue  blue = latin->blues + bb;
        FT_Bool       is_top_blue, is_major_dir;


        
        if ( !( blue->flags & AF_LATIN_BLUE_ACTIVE ) )
          continue;

        
        
        
        
        is_top_blue  = (FT_Byte)( ( blue->flags & AF_LATIN_BLUE_TOP ) != 0 );
        is_major_dir = FT_BOOL( edge->dir == axis->major_dir );

        
        
        
        if ( is_top_blue ^ is_major_dir )
        {
          FT_Pos  dist;


          
          dist = edge->fpos - blue->ref.org;
          if ( dist < 0 )
            dist = -dist;

          dist = FT_MulFix( dist, scale );
          if ( dist < best_dist )
          {
            best_dist = dist;
            best_blue = & blue->ref;
          }

          
          
          
          if ( edge->flags & AF_EDGE_ROUND && dist != 0 )
          {
            FT_Bool  is_under_ref = FT_BOOL( edge->fpos < blue->ref.org );


            if ( is_top_blue ^ is_under_ref )
            {
              blue = latin->blues + bb;
              dist = edge->fpos - blue->shoot.org;
              if ( dist < 0 )
                dist = -dist;

              dist = FT_MulFix( dist, scale );
              if ( dist < best_dist )
              {
                best_dist = dist;
                best_blue = & blue->shoot;
              }
            }
          }
        }
      }

      if ( best_blue )
        edge->blue_edge = best_blue;
    }
  }


  static FT_Error
  af_latin_hints_init( AF_GlyphHints    hints,
                       AF_LatinMetrics  metrics )
  {
    FT_Render_Mode  mode;
    FT_UInt32       scaler_flags, other_flags;
    FT_Face         face = metrics->root.scaler.face;


    af_glyph_hints_rescale( hints, (AF_ScriptMetrics)metrics );

    



    hints->x_scale = metrics->axis[AF_DIMENSION_HORZ].scale;
    hints->x_delta = metrics->axis[AF_DIMENSION_HORZ].delta;
    hints->y_scale = metrics->axis[AF_DIMENSION_VERT].scale;
    hints->y_delta = metrics->axis[AF_DIMENSION_VERT].delta;

    
    mode = metrics->root.scaler.render_mode;

#if 0 
    if ( mode == FT_RENDER_MODE_LCD || mode == FT_RENDER_MODE_LCD_V )
    {
      metrics->root.scaler.render_mode = mode = FT_RENDER_MODE_NORMAL;
    }
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

    



    if ( mode == FT_RENDER_MODE_LIGHT                    ||
         (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0 )
      scaler_flags |= AF_SCALER_FLAG_NO_HORIZONTAL;

    hints->scaler_flags = scaler_flags;
    hints->other_flags  = other_flags;

    return 0;
  }


  
  
  
  
  
  
  

  
  

  static FT_Pos
  af_latin_snap_width( AF_Width  widths,
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
  af_latin_compute_stem_width( AF_GlyphHints  hints,
                               AF_Dimension   dim,
                               FT_Pos         width,
                               AF_Edge_Flags  base_flags,
                               AF_Edge_Flags  stem_flags )
  {
    AF_LatinMetrics  metrics  = (AF_LatinMetrics) hints->metrics;
    AF_LatinAxis     axis     = & metrics->axis[dim];
    FT_Pos           dist     = width;
    FT_Int           sign     = 0;
    FT_Int           vertical = ( dim == AF_DIMENSION_VERT );


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) ||
          axis->extra_light                      )
      return width;

    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( (  vertical && !AF_LATIN_HINTS_DO_VERT_SNAP( hints ) ) ||
         ( !vertical && !AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) ) )
    {
      

      

      if ( ( stem_flags & AF_EDGE_SERIF ) && vertical && ( dist < 3 * 64 ) )
        goto Done_Width;

      else if ( ( base_flags & AF_EDGE_ROUND ) )
      {
        if ( dist < 80 )
          dist = 64;
      }
      else if ( dist < 56 )
        dist = 56;

      if ( axis->width_count > 0 )
      {
        FT_Pos  delta;


        
        delta = dist - axis->widths[0].cur;

        if ( delta < 0 )
          delta = -delta;

        if ( delta < 40 )
        {
          dist = axis->widths[0].cur;
          if ( dist < 48 )
            dist = 48;

          goto Done_Width;
        }

        if ( dist < 3 * 64 )
        {
          delta  = dist & 63;
          dist  &= -64;

          if ( delta < 10 )
            dist += delta;

          else if ( delta < 32 )
            dist += 10;

          else if ( delta < 54 )
            dist += 54;

          else
            dist += delta;
        }
        else
          dist = ( dist + 32 ) & ~63;
      }
    }
    else
    {
      
      FT_Pos  org_dist = dist;


      dist = af_latin_snap_width( axis->widths, axis->width_count, dist );

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
          {
            
            
            
            
            

            FT_Pos  delta;


            dist = ( dist + 22 ) & ~63;
            delta = dist - org_dist;
            if ( delta < 0 )
              delta = -delta;

            if (delta >= 16)
            {
              dist = org_dist;
              if ( dist < 48 )
                dist = ( dist + 64 ) >> 1;
            }
          }
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
  af_latin_align_linked_edge( AF_GlyphHints  hints,
                              AF_Dimension   dim,
                              AF_Edge        base_edge,
                              AF_Edge        stem_edge )
  {
    FT_Pos  dist = stem_edge->opos - base_edge->opos;

    FT_Pos  fitted_width = af_latin_compute_stem_width(
                             hints, dim, dist,
                             (AF_Edge_Flags)base_edge->flags,
                             (AF_Edge_Flags)stem_edge->flags );


    stem_edge->pos = base_edge->pos + fitted_width;

    AF_LOG(( "LINK: edge %d (opos=%.2f) linked to (%.2f), "
             "dist was %.2f, now %.2f\n",
             stem_edge-hints->axis[dim].edges, stem_edge->opos / 64.0,
             stem_edge->pos / 64.0, dist / 64.0, fitted_width / 64.0 ));
  }


  static void
  af_latin_align_serif_edge( AF_GlyphHints  hints,
                             AF_Edge        base,
                             AF_Edge        serif )
  {
    FT_UNUSED( hints );

    serif->pos = base->pos + (serif->opos - base->opos);
  }


  
  
  
  
  
  
  
  
  


  FT_LOCAL_DEF( void )
  af_latin_hint_edges( AF_GlyphHints  hints,
                       AF_Dimension   dim )
  {
    AF_AxisHints  axis       = &hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    FT_PtrDist    n_edges;
    AF_Edge       edge;
    AF_Edge       anchor     = 0;
    FT_Int        has_serifs = 0;


    
    

    if ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_BLUES( hints ) )
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

        AF_LOG(( "BLUE: edge %d (opos=%.2f) snapped to (%.2f), "
                 "was (%.2f)\n",
                 edge1-edges, edge1->opos / 64.0, blue->fit / 64.0,
                 edge1->pos / 64.0 ));

        edge1->pos    = blue->fit;
        edge1->flags |= AF_EDGE_DONE;

        if ( edge2 && !edge2->blue_edge )
        {
          af_latin_align_linked_edge( hints, dim, edge1, edge2 );
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
        has_serifs++;
        continue;
      }

      

      
      if ( edge2->blue_edge )
      {
        AF_LOG(( "ASSERTION FAILED for edge %d\n", edge2-edges ));

        af_latin_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
        continue;
      }

      if ( !anchor )
      {
        FT_Pos  org_len, org_center, cur_len;
        FT_Pos  cur_pos1, error1, error2, u_off, d_off;


        org_len = edge2->opos - edge->opos;
        cur_len = af_latin_compute_stem_width(
                    hints, dim, org_len,
                    (AF_Edge_Flags)edge->flags,
                    (AF_Edge_Flags)edge2->flags );
        if ( cur_len <= 64 )
          u_off = d_off = 32;
        else
        {
          u_off = 38;
          d_off = 26;
        }

        if ( cur_len < 96 )
        {
          org_center = edge->opos + ( org_len >> 1 );

          cur_pos1   = FT_PIX_ROUND( org_center );

          error1 = org_center - ( cur_pos1 - u_off );
          if ( error1 < 0 )
            error1 = -error1;

          error2 = org_center - ( cur_pos1 + d_off );
          if ( error2 < 0 )
            error2 = -error2;

          if ( error1 < error2 )
            cur_pos1 -= u_off;
          else
            cur_pos1 += d_off;

          edge->pos  = cur_pos1 - cur_len / 2;
          edge2->pos = edge->pos + cur_len;
        }
        else
          edge->pos = FT_PIX_ROUND( edge->opos );

        AF_LOG(( "ANCHOR: edge %d (opos=%.2f) and %d (opos=%.2f) "
                 "snapped to (%.2f) (%.2f)\n",
                 edge-edges, edge->opos / 64.0,
                 edge2-edges, edge2->opos / 64.0,
                 edge->pos / 64.0, edge2->pos / 64.0 ));
        anchor = edge;

        edge->flags |= AF_EDGE_DONE;

        af_latin_align_linked_edge( hints, dim, edge, edge2 );
      }
      else
      {
        FT_Pos  org_pos, org_len, org_center, cur_len;
        FT_Pos  cur_pos1, cur_pos2, delta1, delta2;


        org_pos    = anchor->pos + ( edge->opos - anchor->opos );
        org_len    = edge2->opos - edge->opos;
        org_center = org_pos + ( org_len >> 1 );

        cur_len = af_latin_compute_stem_width(
                   hints, dim, org_len,
                   (AF_Edge_Flags)edge->flags,
                   (AF_Edge_Flags)edge2->flags );

        if ( edge2->flags & AF_EDGE_DONE )
          edge->pos = edge2->pos - cur_len;

        else if ( cur_len < 96 )
        {
          FT_Pos  u_off, d_off;


          cur_pos1 = FT_PIX_ROUND( org_center );

          if (cur_len <= 64 )
            u_off = d_off = 32;
          else
          {
            u_off = 38;
            d_off = 26;
          }

          delta1 = org_center - ( cur_pos1 - u_off );
          if ( delta1 < 0 )
            delta1 = -delta1;

          delta2 = org_center - ( cur_pos1 + d_off );
          if ( delta2 < 0 )
            delta2 = -delta2;

          if ( delta1 < delta2 )
            cur_pos1 -= u_off;
          else
            cur_pos1 += d_off;

          edge->pos  = cur_pos1 - cur_len / 2;
          edge2->pos = cur_pos1 + cur_len / 2;

          AF_LOG(( "STEM: %d (opos=%.2f) to %d (opos=%.2f) "
                   "snapped to (%.2f) and (%.2f)\n",
                   edge-edges, edge->opos / 64.0,
                   edge2-edges, edge2->opos / 64.0,
                   edge->pos / 64.0, edge2->pos / 64.0 ));
        }
        else
        {
          org_pos    = anchor->pos + ( edge->opos - anchor->opos );
          org_len    = edge2->opos - edge->opos;
          org_center = org_pos + ( org_len >> 1 );

          cur_len    = af_latin_compute_stem_width(
                         hints, dim, org_len,
                         (AF_Edge_Flags)edge->flags,
                         (AF_Edge_Flags)edge2->flags );

          cur_pos1   = FT_PIX_ROUND( org_pos );
          delta1     = cur_pos1 + ( cur_len >> 1 ) - org_center;
          if ( delta1 < 0 )
            delta1 = -delta1;

          cur_pos2   = FT_PIX_ROUND( org_pos + org_len ) - cur_len;
          delta2     = cur_pos2 + ( cur_len >> 1 ) - org_center;
          if ( delta2 < 0 )
            delta2 = -delta2;

          edge->pos  = ( delta1 < delta2 ) ? cur_pos1 : cur_pos2;
          edge2->pos = edge->pos + cur_len;

          AF_LOG(( "STEM: %d (opos=%.2f) to %d (opos=%.2f) "
                   "snapped to (%.2f) and (%.2f)\n",
                   edge-edges, edge->opos / 64.0,
                   edge2-edges, edge2->opos / 64.0,
                   edge->pos / 64.0, edge2->pos / 64.0 ));
        }

        edge->flags  |= AF_EDGE_DONE;
        edge2->flags |= AF_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
        {
          AF_LOG(( "BOUND: %d (pos=%.2f) to (%.2f)\n",
                   edge-edges, edge->pos / 64.0, edge[-1].pos / 64.0 ));
          edge->pos = edge[-1].pos;
        }
      }
    }

    

    
    
    
    
    
    
    
    

    
    
    

    n_edges = edge_limit - edges;
    if ( dim == AF_DIMENSION_HORZ && ( n_edges == 6 || n_edges == 12 ) )
    {
      AF_Edge  edge1, edge2, edge3;
      FT_Pos   dist1, dist2, span, delta;


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

      if ( span < 8 )
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

    if ( has_serifs || !anchor )
    {
      



      for ( edge = edges; edge < edge_limit; edge++ )
      {
        FT_Pos  delta;


        if ( edge->flags & AF_EDGE_DONE )
          continue;

        delta = 1000;

        if ( edge->serif )
        {
          delta = edge->serif->opos - edge->opos;
          if ( delta < 0 )
            delta = -delta;
        }

        if ( delta < 64 + 16 )
        {
          af_latin_align_serif_edge( hints, edge->serif, edge );
          AF_LOG(( "SERIF: edge %d (opos=%.2f) serif to %d (opos=%.2f) "
                   "aligned to (%.2f)\n",
                   edge-edges, edge->opos / 64.0,
                   edge->serif - edges, edge->serif->opos / 64.0,
                   edge->pos / 64.0 ));
        }
        else if ( !anchor )
        {
          AF_LOG(( "SERIF_ANCHOR: edge %d (opos=%.2f) snapped to (%.2f)\n",
                   edge-edges, edge->opos / 64.0, edge->pos / 64.0 ));
          edge->pos = FT_PIX_ROUND( edge->opos );
          anchor    = edge;
        }
        else
        {
          AF_Edge  before, after;


          for ( before = edge - 1; before >= edges; before-- )
            if ( before->flags & AF_EDGE_DONE )
              break;

          for ( after = edge + 1; after < edge_limit; after++ )
            if ( after->flags & AF_EDGE_DONE )
              break;

          if ( before >= edges && before < edge   &&
               after < edge_limit && after > edge )
          {
            if ( after->opos == before->opos )
              edge->pos = before->pos;
            else
              edge->pos = before->pos +
                          FT_MulDiv( edge->opos - before->opos,
                                     after->pos - before->pos,
                                     after->opos - before->opos );
            AF_LOG(( "SERIF_LINK1: edge %d (opos=%.2f) snapped to (%.2f) "
                     "from %d (opos=%.2f)\n",
                     edge-edges, edge->opos / 64.0,
                     edge->pos / 64.0, before - edges,
                     before->opos / 64.0 ));
          }
          else
          {
            edge->pos = anchor->pos +
                        ( ( edge->opos - anchor->opos + 16 ) & ~31 );
            AF_LOG(( "SERIF_LINK2: edge %d (opos=%.2f) snapped to (%.2f)\n",
                     edge-edges, edge->opos / 64.0, edge->pos / 64.0 ));
          }
        }

        edge->flags |= AF_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;

        if ( edge + 1 < edge_limit        &&
             edge[1].flags & AF_EDGE_DONE &&
             edge->pos > edge[1].pos      )
          edge->pos = edge[1].pos;
      }
    }
  }


  static FT_Error
  af_latin_hints_apply( AF_GlyphHints    hints,
                        FT_Outline*      outline,
                        AF_LatinMetrics  metrics )
  {
    FT_Error  error;
    int       dim;


    error = af_glyph_hints_reload( hints, outline );
    if ( error )
      goto Exit;

    
#ifdef AF_USE_WARPER
    if ( metrics->root.scaler.render_mode == FT_RENDER_MODE_LIGHT ||
         AF_HINTS_DO_HORIZONTAL( hints ) )
#else
    if ( AF_HINTS_DO_HORIZONTAL( hints ) )
#endif
    {
      error = af_latin_hints_detect_features( hints, AF_DIMENSION_HORZ );
      if ( error )
        goto Exit;
    }

    if ( AF_HINTS_DO_VERTICAL( hints ) )
    {
      error = af_latin_hints_detect_features( hints, AF_DIMENSION_VERT );
      if ( error )
        goto Exit;

      af_latin_hints_compute_blue_edges( hints, metrics );
    }

    
    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
#ifdef AF_USE_WARPER
      if ( ( dim == AF_DIMENSION_HORZ &&
             metrics->root.scaler.render_mode == FT_RENDER_MODE_LIGHT ) )
      {
        AF_WarperRec  warper;
        FT_Fixed      scale;
        FT_Pos        delta;


        af_warper_compute( &warper, hints, dim, &scale, &delta );
        af_glyph_hints_scale_dim( hints, dim, scale, delta );
        continue;
      }
#endif

      if ( ( dim == AF_DIMENSION_HORZ && AF_HINTS_DO_HORIZONTAL( hints ) ) ||
           ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_VERTICAL( hints ) )   )
      {
        af_latin_hint_edges( hints, (AF_Dimension)dim );
        af_glyph_hints_align_edge_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_strong_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_weak_points( hints, (AF_Dimension)dim );
      }
    }
    af_glyph_hints_save( hints, outline );

  Exit:
    return error;
  }


  
  
  
  
  
  
  


  
  

  static const AF_Script_UniRangeRec  af_latin_uniranges[] =
  {
    AF_UNIRANGE_REC(  0x0020UL,  0x007FUL ),  
    AF_UNIRANGE_REC(  0x00A0UL,  0x00FFUL ),  
    AF_UNIRANGE_REC(  0x0100UL,  0x017FUL ),  
    AF_UNIRANGE_REC(  0x0180UL,  0x024FUL ),  
    AF_UNIRANGE_REC(  0x0250UL,  0x02AFUL ),  
    AF_UNIRANGE_REC(  0x02B0UL,  0x02FFUL ),  
    AF_UNIRANGE_REC(  0x0300UL,  0x036FUL ),  
    AF_UNIRANGE_REC(  0x0370UL,  0x03FFUL ),  
    AF_UNIRANGE_REC(  0x0400UL,  0x04FFUL ),  
    AF_UNIRANGE_REC(  0x0500UL,  0x052FUL ),  
    AF_UNIRANGE_REC(  0x1D00UL,  0x1D7FUL ),  
    AF_UNIRANGE_REC(  0x1D80UL,  0x1DBFUL ),  
    AF_UNIRANGE_REC(  0x1DC0UL,  0x1DFFUL ),  
    AF_UNIRANGE_REC(  0x1E00UL,  0x1EFFUL ),  
    AF_UNIRANGE_REC(  0x1F00UL,  0x1FFFUL ),  
    AF_UNIRANGE_REC(  0x2000UL,  0x206FUL ),  
    AF_UNIRANGE_REC(  0x2070UL,  0x209FUL ),  
    AF_UNIRANGE_REC(  0x20A0UL,  0x20CFUL ),  
    AF_UNIRANGE_REC(  0x2150UL,  0x218FUL ),  
    AF_UNIRANGE_REC(  0x2460UL,  0x24FFUL ),  
    AF_UNIRANGE_REC(  0x2C60UL,  0x2C7FUL ),  
    AF_UNIRANGE_REC(  0x2DE0UL,  0x2DFFUL ),  
    AF_UNIRANGE_REC(  0xA640UL,  0xA69FUL ),  
    AF_UNIRANGE_REC(  0xA720UL,  0xA7FFUL ),  
    AF_UNIRANGE_REC(  0xFB00UL,  0xFB06UL ),  
    AF_UNIRANGE_REC( 0x1D400UL, 0x1D7FFUL ),  
    AF_UNIRANGE_REC(       0UL,       0UL )
  };


  AF_DEFINE_SCRIPT_CLASS(af_latin_script_class,  
    AF_SCRIPT_LATIN,
    af_latin_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_latin_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_latin_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_latin_hints_init,
    (AF_Script_ApplyHintsFunc)  af_latin_hints_apply
  )



