
















  






#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_CJK

#include "afcjk.h"
#include "aferrors.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  
  
  
  
  
  
  

  FT_LOCAL_DEF( FT_Error )
  af_cjk_metrics_init( AF_LatinMetrics  metrics,
                       FT_Face          face )
  {
    FT_CharMap  oldmap = face->charmap;


    metrics->units_per_em = face->units_per_EM;

    

    if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) )
      face->charmap = NULL;

    
    af_latin_metrics_init_widths( metrics, face, 0x7530 );

    FT_Set_Charmap( face, oldmap );

    return AF_Err_Ok;
  }


  static void
  af_cjk_metrics_scale_dim( AF_LatinMetrics  metrics,
                            AF_Scaler        scaler,
                            AF_Dimension     dim )
  {
    AF_LatinAxis  axis;


    axis = &metrics->axis[dim];

    if ( dim == AF_DIMENSION_HORZ )
    {
      axis->scale = scaler->x_scale;
      axis->delta = scaler->x_delta;
    }
    else
    {
      axis->scale = scaler->y_scale;
      axis->delta = scaler->y_delta;
    }
  }


  FT_LOCAL_DEF( void )
  af_cjk_metrics_scale( AF_LatinMetrics  metrics,
                        AF_Scaler        scaler )
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
    AF_LatinAxis  laxis  = &((AF_LatinMetrics)hints->metrics)->axis[dim];

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


  FT_LOCAL_DEF( FT_Error )
  af_cjk_hints_init( AF_GlyphHints    hints,
                     AF_LatinMetrics  metrics )
  {
    FT_Render_Mode  mode;
    FT_UInt32       scaler_flags, other_flags;


    af_glyph_hints_rescale( hints, (AF_ScriptMetrics)metrics );

    



    hints->x_scale = metrics->axis[AF_DIMENSION_HORZ].scale;
    hints->x_delta = metrics->axis[AF_DIMENSION_HORZ].delta;
    hints->y_scale = metrics->axis[AF_DIMENSION_VERT].scale;
    hints->y_delta = metrics->axis[AF_DIMENSION_VERT].delta;

    
    mode = metrics->root.scaler.render_mode;

#ifdef AF_USE_WARPER
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
    AF_LatinMetrics  metrics  = (AF_LatinMetrics) hints->metrics;
    AF_LatinAxis     axis     = & metrics->axis[dim];
    FT_Pos           dist     = width;
    FT_Int           sign     = 0;
    FT_Int           vertical = ( dim == AF_DIMENSION_VERT );

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

    offset = cur_len % 64;

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
    FT_Int        n_edges;
    AF_Edge       edge;
    AF_Edge       anchor   = 0;
    FT_Pos        delta    = 0;
    FT_Int        skipped  = 0;


    
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

      

      if ( edge2 < edge )
      {
        af_cjk_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
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
          edge->pos = before->pos +
            FT_MulDiv( edge->fpos - before->fpos,
                       after->pos - before->pos,
                       after->fpos - before->fpos );
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
  af_cjk_hints_apply( AF_GlyphHints    hints,
                      FT_Outline*      outline,
                      AF_LatinMetrics  metrics )
  {
    FT_Error  error;
    int       dim;

    FT_UNUSED( metrics );


    error = af_glyph_hints_reload( hints, outline, 0 );
    if ( error )
      goto Exit;

    
    if ( AF_HINTS_DO_HORIZONTAL( hints ) )
    {
      error = af_cjk_hints_detect_features( hints, AF_DIMENSION_HORZ );
      if ( error )
        goto Exit;
    }

    if ( AF_HINTS_DO_VERTICAL( hints ) )
    {
      error = af_cjk_hints_detect_features( hints, AF_DIMENSION_VERT );
      if ( error )
        goto Exit;
    }

    
    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
      if ( ( dim == AF_DIMENSION_HORZ && AF_HINTS_DO_HORIZONTAL( hints ) ) ||
           ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_VERTICAL( hints ) )   )
      {

#ifdef AF_USE_WARPER
        if ( dim == AF_DIMENSION_HORZ                                  &&
             metrics->root.scaler.render_mode == FT_RENDER_MODE_NORMAL )
        {
          AF_WarperRec  warper;
          FT_Fixed      scale;
          FT_Pos        delta;


          af_warper_compute( &warper, hints, dim, &scale, &delta );
          af_glyph_hints_scale_dim( hints, dim, scale, delta );
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
#if 0
    { 0x0100,  0xFFFF },  
#endif
    { 0x2E80,  0x2EFF },  
    { 0x2F00,  0x2FDF },  
    { 0x3000,  0x303F },  
    { 0x3040,  0x309F },  
    { 0x30A0,  0x30FF },  
    { 0x3100,  0x312F },  
    { 0x3130,  0x318F },  
    { 0x31A0,  0x31BF },  
    { 0x31C0,  0x31EF },  
    { 0x31F0,  0x31FF },  
    { 0x3200,  0x32FF },  
    { 0x3300,  0x33FF },  
    { 0x3400,  0x4DBF },  
    { 0x4DC0,  0x4DFF },  
    { 0x4E00,  0x9FFF },  
    { 0xF900,  0xFAFF },  
    { 0xFE30,  0xFE4F },  
    { 0xFF00,  0xFFEF },  
    { 0x20000, 0x2A6DF }, 
    { 0x2F800, 0x2FA1F }, 
    { 0,       0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_cjk_script_class =
  {
    AF_SCRIPT_CJK,
    af_cjk_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_cjk_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_cjk_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_cjk_hints_init,
    (AF_Script_ApplyHintsFunc)  af_cjk_hints_apply
  };

#else 

  static const AF_Script_UniRangeRec  af_cjk_uniranges[] =
  {
    { 0, 0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_cjk_script_class =
  {
    AF_SCRIPT_CJK,
    af_cjk_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  };

#endif 



