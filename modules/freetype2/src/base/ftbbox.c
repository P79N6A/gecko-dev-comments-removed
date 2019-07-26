

















  
  
  
  
  
  


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H

#include FT_BBOX_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_OBJECTS_H


  typedef struct  TBBox_Rec_
  {
    FT_Vector  last;
    FT_BBox    bbox;

  } TBBox_Rec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Move_To( FT_Vector*  to,
                TBBox_Rec*  user )
  {
    user->last = *to;

    return 0;
  }


#define CHECK_X( p, bbox )  \
          ( p->x < bbox.xMin || p->x > bbox.xMax )

#define CHECK_Y( p, bbox )  \
          ( p->y < bbox.yMin || p->y > bbox.yMax )


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  BBox_Conic_Check( FT_Pos   y1,
                    FT_Pos   y2,
                    FT_Pos   y3,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    
    
    
    

    y1 -= y2;
    y3 -= y2;
    y2 += FT_MulDiv( y1, y3, y1 + y3 );

    if ( y2 < *min )
      *min = y2;
    if ( y2 > *max )
      *max = y2;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Conic_To( FT_Vector*  control,
                 FT_Vector*  to,
                 TBBox_Rec*  user )
  {
    
    

    if ( CHECK_X( control, user->bbox ) )
      BBox_Conic_Check( user->last.x,
                        control->x,
                        to->x,
                        &user->bbox.xMin,
                        &user->bbox.xMax );

    if ( CHECK_Y( control, user->bbox ) )
      BBox_Conic_Check( user->last.y,
                        control->y,
                        to->y,
                        &user->bbox.yMin,
                        &user->bbox.yMax );

    user->last = *to;

    return 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Pos
  update_cubic_max( FT_Pos  q1,
                    FT_Pos  q2,
                    FT_Pos  q3,
                    FT_Pos  q4,
                    FT_Pos  max )
  {
    
    
    while ( q2 > max || q3 > max )
    {
      
      if ( q1 + q2 > q3 + q4 ) 
      {
        q4 = q4 + q3;
        q3 = q3 + q2;
        q2 = q2 + q1;
        q4 = q4 + q3;
        q3 = q3 + q2;
        q4 = ( q4 + q3 ) / 8;
        q3 = q3 / 4;
        q2 = q2 / 2;
      }
      else                     
      {
        q1 = q1 + q2;
        q2 = q2 + q3;
        q3 = q3 + q4;
        q1 = q1 + q2;
        q2 = q2 + q3;
        q1 = ( q1 + q2 ) / 8;
        q2 = q2 / 4;
        q3 = q3 / 2;
      }

      
      if ( q1 == q2 && q1 >= q3 )
      {
        max = q1;
        break;
      }
      if ( q3 == q4 && q2 <= q4 )
      {
        max = q4;
        break;
      }
    }

    return max;
  }


  static void
  BBox_Cubic_Check( FT_Pos   p1,
                    FT_Pos   p2,
                    FT_Pos   p3,
                    FT_Pos   p4,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    FT_Pos  nmin, nmax;
    FT_Int  shift;


    
    
    
    
    
    
    
    
    

    shift = 27 - FT_MSB( FT_ABS( p2 ) | FT_ABS( p3 ) );

    if ( shift > 0 )
    {
      
      if ( shift > 2 )
        shift = 2;

      p1 <<=  shift;
      p2 <<=  shift;
      p3 <<=  shift;
      p4 <<=  shift;
      nmin = *min << shift;
      nmax = *max << shift;
    }
    else
    {
      p1 >>= -shift;
      p2 >>= -shift;
      p3 >>= -shift;
      p4 >>= -shift;
      nmin = *min >> -shift;
      nmax = *max >> -shift;
    }

    nmax =  update_cubic_max(  p1,  p2,  p3,  p4,  nmax );

    
    nmin = -update_cubic_max( -p1, -p2, -p3, -p4, -nmin );

    if ( shift > 0 )
    {
      nmin >>=  shift;
      nmax >>=  shift;
    }
    else
    {
      nmin <<= -shift;
      nmax <<= -shift;
    }

    if ( nmin < *min )
      *min = nmin;
    if ( nmax > *max )
      *max = nmax;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Cubic_To( FT_Vector*  control1,
                 FT_Vector*  control2,
                 FT_Vector*  to,
                 TBBox_Rec*  user )
  {
    
    
    

    if ( CHECK_X( control1, user->bbox ) ||
         CHECK_X( control2, user->bbox ) )
      BBox_Cubic_Check( user->last.x,
                        control1->x,
                        control2->x,
                        to->x,
                        &user->bbox.xMin,
                        &user->bbox.xMax );

    if ( CHECK_Y( control1, user->bbox ) ||
         CHECK_Y( control2, user->bbox ) )
      BBox_Cubic_Check( user->last.y,
                        control1->y,
                        control2->y,
                        to->y,
                        &user->bbox.yMin,
                        &user->bbox.yMax );

    user->last = *to;

    return 0;
  }

FT_DEFINE_OUTLINE_FUNCS(bbox_interface,
    (FT_Outline_MoveTo_Func) BBox_Move_To,
    (FT_Outline_LineTo_Func) BBox_Move_To,
    (FT_Outline_ConicTo_Func)BBox_Conic_To,
    (FT_Outline_CubicTo_Func)BBox_Cubic_To,
    0, 0
  )

  

  FT_EXPORT_DEF( FT_Error )
  FT_Outline_Get_BBox( FT_Outline*  outline,
                       FT_BBox     *abbox )
  {
    FT_BBox     cbox;
    FT_BBox     bbox;
    FT_Vector*  vec;
    FT_UShort   n;


    if ( !abbox )
      return FT_THROW( Invalid_Argument );

    if ( !outline )
      return FT_THROW( Invalid_Outline );

    
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
    {
      abbox->xMin = abbox->xMax = 0;
      abbox->yMin = abbox->yMax = 0;
      return 0;
    }

    
    
    

    vec = outline->points;
    bbox.xMin = bbox.xMax = cbox.xMin = cbox.xMax = vec->x;
    bbox.yMin = bbox.yMax = cbox.yMin = cbox.yMax = vec->y;
    vec++;

    for ( n = 1; n < outline->n_points; n++ )
    {
      FT_Pos  x = vec->x;
      FT_Pos  y = vec->y;


      
      if ( x < cbox.xMin ) cbox.xMin = x;
      if ( x > cbox.xMax ) cbox.xMax = x;

      if ( y < cbox.yMin ) cbox.yMin = y;
      if ( y > cbox.yMax ) cbox.yMax = y;

      if ( FT_CURVE_TAG( outline->tags[n] ) == FT_CURVE_TAG_ON )
      {
        
        if ( x < bbox.xMin ) bbox.xMin = x;
        if ( x > bbox.xMax ) bbox.xMax = x;

        if ( y < bbox.yMin ) bbox.yMin = y;
        if ( y > bbox.yMax ) bbox.yMax = y;
      }

      vec++;
    }

    
    if ( cbox.xMin < bbox.xMin || cbox.xMax > bbox.xMax ||
         cbox.yMin < bbox.yMin || cbox.yMax > bbox.yMax )
    {
      
      

      FT_Error   error;
      TBBox_Rec  user;

#ifdef FT_CONFIG_OPTION_PIC
      FT_Outline_Funcs bbox_interface;
      Init_Class_bbox_interface(&bbox_interface);
#endif

      user.bbox = bbox;

      error = FT_Outline_Decompose( outline, &bbox_interface, &user );
      if ( error )
        return error;

      *abbox = user.bbox;
    }
    else
      *abbox = bbox;

    return FT_Err_Ok;
  }



