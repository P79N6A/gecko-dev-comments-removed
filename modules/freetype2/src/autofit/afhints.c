

















#include "afhints.h"
#include "aferrors.h"
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_DEBUG_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_afhints


  

  FT_LOCAL_DEF( FT_Error )
  af_axis_hints_new_segment( AF_AxisHints  axis,
                             FT_Memory     memory,
                             AF_Segment   *asegment )
  {
    FT_Error    error   = FT_Err_Ok;
    AF_Segment  segment = NULL;


    if ( axis->num_segments >= axis->max_segments )
    {
      FT_Int  old_max = axis->max_segments;
      FT_Int  new_max = old_max;
      FT_Int  big_max = (FT_Int)( FT_INT_MAX / sizeof ( *segment ) );


      if ( old_max >= big_max )
      {
        error = FT_THROW( Out_Of_Memory );
        goto Exit;
      }

      new_max += ( new_max >> 2 ) + 4;
      if ( new_max < old_max || new_max > big_max )
        new_max = big_max;

      if ( FT_RENEW_ARRAY( axis->segments, old_max, new_max ) )
        goto Exit;

      axis->max_segments = new_max;
    }

    segment = axis->segments + axis->num_segments++;

  Exit:
    *asegment = segment;
    return error;
  }


  

  FT_LOCAL( FT_Error )
  af_axis_hints_new_edge( AF_AxisHints  axis,
                          FT_Int        fpos,
                          AF_Direction  dir,
                          FT_Memory     memory,
                          AF_Edge      *anedge )
  {
    FT_Error  error = FT_Err_Ok;
    AF_Edge   edge  = NULL;
    AF_Edge   edges;


    if ( axis->num_edges >= axis->max_edges )
    {
      FT_Int  old_max = axis->max_edges;
      FT_Int  new_max = old_max;
      FT_Int  big_max = (FT_Int)( FT_INT_MAX / sizeof ( *edge ) );


      if ( old_max >= big_max )
      {
        error = FT_THROW( Out_Of_Memory );
        goto Exit;
      }

      new_max += ( new_max >> 2 ) + 4;
      if ( new_max < old_max || new_max > big_max )
        new_max = big_max;

      if ( FT_RENEW_ARRAY( axis->edges, old_max, new_max ) )
        goto Exit;

      axis->max_edges = new_max;
    }

    edges = axis->edges;
    edge  = edges + axis->num_edges;

    while ( edge > edges )
    {
      if ( edge[-1].fpos < fpos )
        break;

      
      
      if ( edge[-1].fpos == fpos && dir == axis->major_dir )
        break;

      edge[0] = edge[-1];
      edge--;
    }

    axis->num_edges++;

    FT_ZERO( edge );
    edge->fpos = (FT_Short)fpos;
    edge->dir  = (FT_Char)dir;

  Exit:
    *anedge = edge;
    return error;
  }


#ifdef FT_DEBUG_AUTOFIT

#include FT_CONFIG_STANDARD_LIBRARY_H

  
#define AF_DUMP( varformat )          \
          do                          \
          {                           \
            if ( to_stdout )          \
              printf varformat;       \
            else                      \
              FT_TRACE7( varformat ); \
          } while ( 0 )


  static const char*
  af_dir_str( AF_Direction  dir )
  {
    const char*  result;


    switch ( dir )
    {
    case AF_DIR_UP:
      result = "up";
      break;
    case AF_DIR_DOWN:
      result = "down";
      break;
    case AF_DIR_LEFT:
      result = "left";
      break;
    case AF_DIR_RIGHT:
      result = "right";
      break;
    default:
      result = "none";
    }

    return result;
  }


#define AF_INDEX_NUM( ptr, base )  ( (ptr) ? ( (ptr) - (base) ) : -1 )


#ifdef __cplusplus
  extern "C" {
#endif
  void
  af_glyph_hints_dump_points( AF_GlyphHints  hints,
                              FT_Bool        to_stdout )
  {
    AF_Point  points = hints->points;
    AF_Point  limit  = points + hints->num_points;
    AF_Point  point;


    AF_DUMP(( "Table of points:\n"
              "  [ index |  xorg |  yorg | xscale | yscale"
              " |  xfit |  yfit |  flags ]\n" ));

    for ( point = points; point < limit; point++ )
      AF_DUMP(( "  [ %5d | %5d | %5d | %6.2f | %6.2f"
                " | %5.2f | %5.2f | %c%c%c%c%c%c ]\n",
                point - points,
                point->fx,
                point->fy,
                point->ox / 64.0,
                point->oy / 64.0,
                point->x / 64.0,
                point->y / 64.0,
                ( point->flags & AF_FLAG_WEAK_INTERPOLATION ) ? 'w' : ' ',
                ( point->flags & AF_FLAG_INFLECTION )         ? 'i' : ' ',
                ( point->flags & AF_FLAG_EXTREMA_X )          ? '<' : ' ',
                ( point->flags & AF_FLAG_EXTREMA_Y )          ? 'v' : ' ',
                ( point->flags & AF_FLAG_ROUND_X )            ? '(' : ' ',
                ( point->flags & AF_FLAG_ROUND_Y )            ? 'u' : ' '));
    AF_DUMP(( "\n" ));
  }
#ifdef __cplusplus
  }
#endif


  static const char*
  af_edge_flags_to_string( AF_Edge_Flags  flags )
  {
    static char  temp[32];
    int          pos = 0;


    if ( flags & AF_EDGE_ROUND )
    {
      ft_memcpy( temp + pos, "round", 5 );
      pos += 5;
    }
    if ( flags & AF_EDGE_SERIF )
    {
      if ( pos > 0 )
        temp[pos++] = ' ';
      ft_memcpy( temp + pos, "serif", 5 );
      pos += 5;
    }
    if ( pos == 0 )
      return "normal";

    temp[pos] = '\0';

    return temp;
  }


  

#ifdef __cplusplus
  extern "C" {
#endif
  void
  af_glyph_hints_dump_segments( AF_GlyphHints  hints,
                                FT_Bool        to_stdout )
  {
    FT_Int  dimension;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AF_AxisHints  axis     = &hints->axis[dimension];
      AF_Point      points   = hints->points;
      AF_Edge       edges    = axis->edges;
      AF_Segment    segments = axis->segments;
      AF_Segment    limit    = segments + axis->num_segments;
      AF_Segment    seg;


      AF_DUMP(( "Table of %s segments:\n",
                dimension == AF_DIMENSION_HORZ ? "vertical"
                                               : "horizontal" ));
      if ( axis->num_segments )
        AF_DUMP(( "  [ index |  pos  |  dir  | from"
                  " |  to  | link | serif | edge"
                  " | height | extra |    flags    ]\n" ));
      else
        AF_DUMP(( "  (none)\n" ));

      for ( seg = segments; seg < limit; seg++ )
        AF_DUMP(( "  [ %5d | %5.2g | %5s | %4d"
                  " | %4d | %4d | %5d | %4d"
                  " | %6d | %5d | %11s ]\n",
                  seg - segments,
                  dimension == AF_DIMENSION_HORZ
                               ? (int)seg->first->ox / 64.0
                               : (int)seg->first->oy / 64.0,
                  af_dir_str( (AF_Direction)seg->dir ),
                  AF_INDEX_NUM( seg->first, points ),
                  AF_INDEX_NUM( seg->last, points ),
                  AF_INDEX_NUM( seg->link, segments ),
                  AF_INDEX_NUM( seg->serif, segments ),
                  AF_INDEX_NUM( seg->edge, edges ),
                  seg->height,
                  seg->height - ( seg->max_coord - seg->min_coord ),
                  af_edge_flags_to_string( (AF_Edge_Flags)seg->flags ) ));
      AF_DUMP(( "\n" ));
    }
  }
#ifdef __cplusplus
  }
#endif


  

#ifdef __cplusplus
  extern "C" {
#endif
  FT_Error
  af_glyph_hints_get_num_segments( AF_GlyphHints  hints,
                                   FT_Int         dimension,
                                   FT_Int*        num_segments )
  {
    AF_Dimension  dim;
    AF_AxisHints  axis;


    dim = ( dimension == 0 ) ? AF_DIMENSION_HORZ : AF_DIMENSION_VERT;

    axis          = &hints->axis[dim];
    *num_segments = axis->num_segments;

    return FT_Err_Ok;
  }
#ifdef __cplusplus
  }
#endif


  

#ifdef __cplusplus
  extern "C" {
#endif
  FT_Error
  af_glyph_hints_get_segment_offset( AF_GlyphHints  hints,
                                     FT_Int         dimension,
                                     FT_Int         idx,
                                     FT_Pos        *offset,
                                     FT_Bool       *is_blue,
                                     FT_Pos        *blue_offset )
  {
    AF_Dimension  dim;
    AF_AxisHints  axis;
    AF_Segment    seg;


    if ( !offset )
      return FT_THROW( Invalid_Argument );

    dim = ( dimension == 0 ) ? AF_DIMENSION_HORZ : AF_DIMENSION_VERT;

    axis = &hints->axis[dim];

    if ( idx < 0 || idx >= axis->num_segments )
      return FT_THROW( Invalid_Argument );

    seg      = &axis->segments[idx];
    *offset  = ( dim == AF_DIMENSION_HORZ ) ? seg->first->ox
                                            : seg->first->oy;
    if ( seg->edge )
      *is_blue = (FT_Bool)( seg->edge->blue_edge != 0 );
    else
      *is_blue = FALSE;

    if ( *is_blue )
      *blue_offset = seg->edge->blue_edge->cur;
    else
      *blue_offset = 0;

    return FT_Err_Ok;
  }
#ifdef __cplusplus
  }
#endif


  

#ifdef __cplusplus
  extern "C" {
#endif
  void
  af_glyph_hints_dump_edges( AF_GlyphHints  hints,
                             FT_Bool        to_stdout )
  {
    FT_Int  dimension;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AF_AxisHints  axis  = &hints->axis[dimension];
      AF_Edge       edges = axis->edges;
      AF_Edge       limit = edges + axis->num_edges;
      AF_Edge       edge;


      



      AF_DUMP(( "Table of %s edges:\n",
                dimension == AF_DIMENSION_HORZ ? "vertical"
                                               : "horizontal" ));
      if ( axis->num_edges )
        AF_DUMP(( "  [ index |  pos  |  dir  | link"
                  " | serif | blue | opos  |  pos  |    flags    ]\n" ));
      else
        AF_DUMP(( "  (none)\n" ));

      for ( edge = edges; edge < limit; edge++ )
        AF_DUMP(( "  [ %5d | %5.2g | %5s | %4d"
                  " | %5d |   %c  | %5.2f | %5.2f | %11s ]\n",
                  edge - edges,
                  (int)edge->opos / 64.0,
                  af_dir_str( (AF_Direction)edge->dir ),
                  AF_INDEX_NUM( edge->link, edges ),
                  AF_INDEX_NUM( edge->serif, edges ),
                  edge->blue_edge ? 'y' : 'n',
                  edge->opos / 64.0,
                  edge->pos / 64.0,
                  af_edge_flags_to_string( (AF_Edge_Flags)edge->flags ) ));
      AF_DUMP(( "\n" ));
    }
  }
#ifdef __cplusplus
  }
#endif

#undef AF_DUMP

#endif




  FT_LOCAL_DEF( AF_Direction )
  af_direction_compute( FT_Pos  dx,
                        FT_Pos  dy )
  {
    FT_Pos        ll, ss;  
    AF_Direction  dir;     


    if ( dy >= dx )
    {
      if ( dy >= -dx )
      {
        dir = AF_DIR_UP;
        ll  = dy;
        ss  = dx;
      }
      else
      {
        dir = AF_DIR_LEFT;
        ll  = -dx;
        ss  = dy;
      }
    }
    else 
    {
      if ( dy >= -dx )
      {
        dir = AF_DIR_RIGHT;
        ll  = dx;
        ss  = dy;
      }
      else
      {
        dir = AF_DIR_DOWN;
        ll  = dy;
        ss  = dx;
      }
    }

    
    
    ss *= 14;
    if ( FT_ABS( ll ) <= FT_ABS( ss ) )
      dir = AF_DIR_NONE;

    return dir;
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_init( AF_GlyphHints  hints,
                       FT_Memory      memory )
  {
    FT_ZERO( hints );
    hints->memory = memory;
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_done( AF_GlyphHints  hints )
  {
    FT_Memory  memory = hints->memory;
    int        dim;


    if ( !( hints && hints->memory ) )
      return;

    



    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
      AF_AxisHints  axis = &hints->axis[dim];


      axis->num_segments = 0;
      axis->max_segments = 0;
      FT_FREE( axis->segments );

      axis->num_edges = 0;
      axis->max_edges = 0;
      FT_FREE( axis->edges );
    }

    FT_FREE( hints->contours );
    hints->max_contours = 0;
    hints->num_contours = 0;

    FT_FREE( hints->points );
    hints->num_points = 0;
    hints->max_points = 0;

    hints->memory = NULL;
  }


  

  FT_LOCAL_DEF( void )
  af_glyph_hints_rescale( AF_GlyphHints    hints,
                          AF_StyleMetrics  metrics )
  {
    hints->metrics      = metrics;
    hints->scaler_flags = metrics->scaler.flags;
  }


  
  

  FT_LOCAL_DEF( FT_Error )
  af_glyph_hints_reload( AF_GlyphHints  hints,
                         FT_Outline*    outline )
  {
    FT_Error   error   = FT_Err_Ok;
    AF_Point   points;
    FT_UInt    old_max, new_max;
    FT_Fixed   x_scale = hints->x_scale;
    FT_Fixed   y_scale = hints->y_scale;
    FT_Pos     x_delta = hints->x_delta;
    FT_Pos     y_delta = hints->y_delta;
    FT_Memory  memory  = hints->memory;


    hints->num_points   = 0;
    hints->num_contours = 0;

    hints->axis[0].num_segments = 0;
    hints->axis[0].num_edges    = 0;
    hints->axis[1].num_segments = 0;
    hints->axis[1].num_edges    = 0;

    
    new_max = (FT_UInt)outline->n_contours;
    old_max = hints->max_contours;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 3 ) & ~3; 

      if ( FT_RENEW_ARRAY( hints->contours, old_max, new_max ) )
        goto Exit;

      hints->max_contours = new_max;
    }

    




    new_max = (FT_UInt)( outline->n_points + 2 );
    old_max = hints->max_points;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 2 + 7 ) & ~7; 

      if ( FT_RENEW_ARRAY( hints->points, old_max, new_max ) )
        goto Exit;

      hints->max_points = new_max;
    }

    hints->num_points   = outline->n_points;
    hints->num_contours = outline->n_contours;

    
    
    
    
    hints->axis[AF_DIMENSION_HORZ].major_dir = AF_DIR_UP;
    hints->axis[AF_DIMENSION_VERT].major_dir = AF_DIR_LEFT;

    if ( FT_Outline_Get_Orientation( outline ) == FT_ORIENTATION_POSTSCRIPT )
    {
      hints->axis[AF_DIMENSION_HORZ].major_dir = AF_DIR_DOWN;
      hints->axis[AF_DIMENSION_VERT].major_dir = AF_DIR_RIGHT;
    }

    hints->x_scale = x_scale;
    hints->y_scale = y_scale;
    hints->x_delta = x_delta;
    hints->y_delta = y_delta;

    hints->xmin_delta = 0;
    hints->xmax_delta = 0;

    points = hints->points;
    if ( hints->num_points == 0 )
      goto Exit;

    {
      AF_Point  point;
      AF_Point  point_limit = points + hints->num_points;


      
      {
        FT_Vector*  vec           = outline->points;
        char*       tag           = outline->tags;
        AF_Point    end           = points + outline->contours[0];
        AF_Point    prev          = end;
        FT_Int      contour_index = 0;


        for ( point = points; point < point_limit; point++, vec++, tag++ )
        {
          point->fx = (FT_Short)vec->x;
          point->fy = (FT_Short)vec->y;
          point->ox = point->x = FT_MulFix( vec->x, x_scale ) + x_delta;
          point->oy = point->y = FT_MulFix( vec->y, y_scale ) + y_delta;

          switch ( FT_CURVE_TAG( *tag ) )
          {
          case FT_CURVE_TAG_CONIC:
            point->flags = AF_FLAG_CONIC;
            break;
          case FT_CURVE_TAG_CUBIC:
            point->flags = AF_FLAG_CUBIC;
            break;
          default:
            point->flags = AF_FLAG_NONE;
          }

          point->prev = prev;
          prev->next  = point;
          prev        = point;

          if ( point == end )
          {
            if ( ++contour_index < outline->n_contours )
            {
              end  = points + outline->contours[contour_index];
              prev = end;
            }
          }
        }
      }

      
      {
        AF_Point*  contour       = hints->contours;
        AF_Point*  contour_limit = contour + hints->num_contours;
        short*     end           = outline->contours;
        short      idx           = 0;


        for ( ; contour < contour_limit; contour++, end++ )
        {
          contour[0] = points + idx;
          idx        = (short)( end[0] + 1 );
        }
      }

      
      {
        AF_Point      first  = points;
        AF_Point      prev   = NULL;
        FT_Pos        in_x   = 0;
        FT_Pos        in_y   = 0;
        AF_Direction  in_dir = AF_DIR_NONE;

        FT_Pos  last_good_in_x = 0;
        FT_Pos  last_good_in_y = 0;

        FT_UInt  units_per_em = hints->metrics->scaler.face->units_per_EM;
        FT_Int   near_limit   = 20 * units_per_em / 2048;


        for ( point = points; point < point_limit; point++ )
        {
          AF_Point  next;
          FT_Pos    out_x, out_y;


          if ( point == first )
          {
            prev = first->prev;

            in_x = first->fx - prev->fx;
            in_y = first->fy - prev->fy;

            last_good_in_x = in_x;
            last_good_in_y = in_y;

            if ( FT_ABS( in_x ) + FT_ABS( in_y ) < near_limit )
            {
              

              AF_Point  point_ = prev;


              while ( point_ != first )
              {
                AF_Point  prev_ = point_->prev;

                FT_Pos  in_x_ = point_->fx - prev_->fx;
                FT_Pos  in_y_ = point_->fy - prev_->fy;


                if ( FT_ABS( in_x_ ) + FT_ABS( in_y_ ) >= near_limit )
                {
                  last_good_in_x = in_x_;
                  last_good_in_y = in_y_;

                  break;
                }

                point_ = prev_;
              }
            }

            in_dir = af_direction_compute( in_x, in_y );
            first  = prev + 1;
          }

          point->in_dir = (FT_Char)in_dir;

          
          
          

          if ( FT_ABS( in_x ) + FT_ABS( in_y ) < near_limit )
            point->flags |= AF_FLAG_NEAR;
          else
          {
            last_good_in_x = in_x;
            last_good_in_y = in_y;
          }

          next  = point->next;
          out_x = next->fx - point->fx;
          out_y = next->fy - point->fy;

          in_dir         = af_direction_compute( out_x, out_y );
          point->out_dir = (FT_Char)in_dir;

          
          

          if ( point->flags & AF_FLAG_CONTROL )
          {
            
          Is_Weak_Point:
            point->flags |= AF_FLAG_WEAK_INTERPOLATION;
          }
          else if ( point->out_dir == point->in_dir )
          {
            if ( point->out_dir != AF_DIR_NONE )
            {
              
              
              goto Is_Weak_Point;
            }

            
            
            
            if ( ft_corner_is_flat(
                   point->flags & AF_FLAG_NEAR ? last_good_in_x : in_x,
                   point->flags & AF_FLAG_NEAR ? last_good_in_y : in_y,
                   out_x,
                   out_y ) )
            {
              
              
              goto Is_Weak_Point;
            }
          }
          else if ( point->in_dir == -point->out_dir )
          {
            
            goto Is_Weak_Point;
          }

          in_x = out_x;
          in_y = out_y;
        }
      }
    }

  Exit:
    return error;
  }


  

  FT_LOCAL_DEF( void )
  af_glyph_hints_save( AF_GlyphHints  hints,
                       FT_Outline*    outline )
  {
    AF_Point    point = hints->points;
    AF_Point    limit = point + hints->num_points;
    FT_Vector*  vec   = outline->points;
    char*       tag   = outline->tags;


    for ( ; point < limit; point++, vec++, tag++ )
    {
      vec->x = point->x;
      vec->y = point->y;

      if ( point->flags & AF_FLAG_CONIC )
        tag[0] = FT_CURVE_TAG_CONIC;
      else if ( point->flags & AF_FLAG_CUBIC )
        tag[0] = FT_CURVE_TAG_CUBIC;
      else
        tag[0] = FT_CURVE_TAG_ON;
    }
  }


  






  
  

  FT_LOCAL_DEF( void )
  af_glyph_hints_align_edge_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim )
  {
    AF_AxisHints  axis          = & hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Segment    seg;


    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( seg = segments; seg < segment_limit; seg++ )
      {
        AF_Edge   edge = seg->edge;
        AF_Point  point, first, last;


        if ( edge == NULL )
          continue;

        first = seg->first;
        last  = seg->last;
        point = first;
        for (;;)
        {
          point->x      = edge->pos;
          point->flags |= AF_FLAG_TOUCH_X;

          if ( point == last )
            break;

          point = point->next;
        }
      }
    }
    else
    {
      for ( seg = segments; seg < segment_limit; seg++ )
      {
        AF_Edge   edge = seg->edge;
        AF_Point  point, first, last;


        if ( edge == NULL )
          continue;

        first = seg->first;
        last  = seg->last;
        point = first;
        for (;;)
        {
          point->y      = edge->pos;
          point->flags |= AF_FLAG_TOUCH_Y;

          if ( point == last )
            break;

          point = point->next;
        }
      }
    }
  }


  






  
  

  FT_LOCAL_DEF( void )
  af_glyph_hints_align_strong_points( AF_GlyphHints  hints,
                                      AF_Dimension   dim )
  {
    AF_Point      points      = hints->points;
    AF_Point      point_limit = points + hints->num_points;
    AF_AxisHints  axis        = &hints->axis[dim];
    AF_Edge       edges       = axis->edges;
    AF_Edge       edge_limit  = edges + axis->num_edges;
    AF_Flags      touch_flag;


    if ( dim == AF_DIMENSION_HORZ )
      touch_flag = AF_FLAG_TOUCH_X;
    else
      touch_flag  = AF_FLAG_TOUCH_Y;

    if ( edges < edge_limit )
    {
      AF_Point  point;
      AF_Edge   edge;


      for ( point = points; point < point_limit; point++ )
      {
        FT_Pos  u, ou, fu;  
        FT_Pos  delta;


        if ( point->flags & touch_flag )
          continue;

        
        

        if (  ( point->flags & AF_FLAG_WEAK_INTERPOLATION ) &&
             !( point->flags & AF_FLAG_INFLECTION )         )
          continue;

        if ( dim == AF_DIMENSION_VERT )
        {
          u  = point->fy;
          ou = point->oy;
        }
        else
        {
          u  = point->fx;
          ou = point->ox;
        }

        fu = u;

        
        edge  = edges;
        delta = edge->fpos - u;
        if ( delta >= 0 )
        {
          u = edge->pos - ( edge->opos - ou );
          goto Store_Point;
        }

        
        edge  = edge_limit - 1;
        delta = u - edge->fpos;
        if ( delta >= 0 )
        {
          u = edge->pos + ( ou - edge->opos );
          goto Store_Point;
        }

        {
          FT_PtrDist  min, max, mid;
          FT_Pos      fpos;


          
          min = 0;
          max = edge_limit - edges;

#if 1
          
          if ( max <= 8 )
          {
            FT_PtrDist  nn;


            for ( nn = 0; nn < max; nn++ )
              if ( edges[nn].fpos >= u )
                break;

            if ( edges[nn].fpos == u )
            {
              u = edges[nn].pos;
              goto Store_Point;
            }
            min = nn;
          }
          else
#endif
          while ( min < max )
          {
            mid  = ( max + min ) >> 1;
            edge = edges + mid;
            fpos = edge->fpos;

            if ( u < fpos )
              max = mid;
            else if ( u > fpos )
              min = mid + 1;
            else
            {
              
              u = edge->pos;
              goto Store_Point;
            }
          }

          
          {
            AF_Edge  before = edges + min - 1;
            AF_Edge  after  = edges + min + 0;


            
            if ( before->scale == 0 )
              before->scale = FT_DivFix( after->pos - before->pos,
                                         after->fpos - before->fpos );

            u = before->pos + FT_MulFix( fu - before->fpos,
                                         before->scale );
          }
        }

      Store_Point:
        
        if ( dim == AF_DIMENSION_HORZ )
          point->x = u;
        else
          point->y = u;

        point->flags |= touch_flag;
      }
    }
  }


  






  
  
  

  static void
  af_iup_shift( AF_Point  p1,
                AF_Point  p2,
                AF_Point  ref )
  {
    AF_Point  p;
    FT_Pos    delta = ref->u - ref->v;


    if ( delta == 0 )
      return;

    for ( p = p1; p < ref; p++ )
      p->u = p->v + delta;

    for ( p = ref + 1; p <= p2; p++ )
      p->u = p->v + delta;
  }


  
  
  
  
  
  

  static void
  af_iup_interp( AF_Point  p1,
                 AF_Point  p2,
                 AF_Point  ref1,
                 AF_Point  ref2 )
  {
    AF_Point  p;
    FT_Pos    u;
    FT_Pos    v1 = ref1->v;
    FT_Pos    v2 = ref2->v;
    FT_Pos    d1 = ref1->u - v1;
    FT_Pos    d2 = ref2->u - v2;


    if ( p1 > p2 )
      return;

    if ( v1 == v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else
          u += d2;

        p->u = u;
      }
      return;
    }

    if ( v1 < v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else if ( u >= v2 )
          u += d2;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
    else
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v2 )
          u += d2;
        else if ( u >= v1 )
          u += d1;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
  }


  
  

  FT_LOCAL_DEF( void )
  af_glyph_hints_align_weak_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim )
  {
    AF_Point   points        = hints->points;
    AF_Point   point_limit   = points + hints->num_points;
    AF_Point*  contour       = hints->contours;
    AF_Point*  contour_limit = contour + hints->num_contours;
    AF_Flags   touch_flag;
    AF_Point   point;
    AF_Point   end_point;
    AF_Point   first_point;


    

    if ( dim == AF_DIMENSION_HORZ )
    {
      touch_flag = AF_FLAG_TOUCH_X;

      for ( point = points; point < point_limit; point++ )
      {
        point->u = point->x;
        point->v = point->ox;
      }
    }
    else
    {
      touch_flag = AF_FLAG_TOUCH_Y;

      for ( point = points; point < point_limit; point++ )
      {
        point->u = point->y;
        point->v = point->oy;
      }
    }

    for ( ; contour < contour_limit; contour++ )
    {
      AF_Point  first_touched, last_touched;


      point       = *contour;
      end_point   = point->prev;
      first_point = point;

      
      for (;;)
      {
        if ( point > end_point )  
          goto NextContour;

        if ( point->flags & touch_flag )
          break;

        point++;
      }

      first_touched = point;

      for (;;)
      {
        FT_ASSERT( point <= end_point                 &&
                   ( point->flags & touch_flag ) != 0 );

        
        while ( point < end_point                    &&
                ( point[1].flags & touch_flag ) != 0 )
          point++;

        last_touched = point;

        
        point++;
        for (;;)
        {
          if ( point > end_point )
            goto EndContour;

          if ( ( point->flags & touch_flag ) != 0 )
            break;

          point++;
        }

        
        af_iup_interp( last_touched + 1, point - 1,
                       last_touched, point );
      }

    EndContour:
      
      if ( last_touched == first_touched )
        af_iup_shift( first_point, end_point, first_touched );

      else 
      {
        if ( last_touched < end_point )
          af_iup_interp( last_touched + 1, end_point,
                         last_touched, first_touched );

        if ( first_touched > points )
          af_iup_interp( first_point, first_touched - 1,
                         last_touched, first_touched );
      }

    NextContour:
      ;
    }

    
    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( point = points; point < point_limit; point++ )
        point->x = point->u;
    }
    else
    {
      for ( point = points; point < point_limit; point++ )
        point->y = point->u;
    }
  }


#ifdef AF_CONFIG_OPTION_USE_WARPER

  

  FT_LOCAL_DEF( void )
  af_glyph_hints_scale_dim( AF_GlyphHints  hints,
                            AF_Dimension   dim,
                            FT_Fixed       scale,
                            FT_Pos         delta )
  {
    AF_Point  points       = hints->points;
    AF_Point  points_limit = points + hints->num_points;
    AF_Point  point;


    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( point = points; point < points_limit; point++ )
        point->x = FT_MulFix( point->fx, scale ) + delta;
    }
    else
    {
      for ( point = points; point < points_limit; point++ )
        point->y = FT_MulFix( point->fy, scale ) + delta;
    }
  }

#endif 


