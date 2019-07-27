

















  
  
  
  
  
  


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


#define FT_UPDATE_BBOX( p, bbox ) \
  FT_BEGIN_STMNT                  \
    if ( p->x < bbox.xMin )       \
      bbox.xMin = p->x;           \
    if ( p->x > bbox.xMax )       \
      bbox.xMax = p->x;           \
    if ( p->y < bbox.yMin )       \
      bbox.yMin = p->y;           \
    if ( p->y > bbox.yMax )       \
      bbox.yMax = p->y;           \
  FT_END_STMNT

#define CHECK_X( p, bbox )                         \
          ( p->x < bbox.xMin || p->x > bbox.xMax )

#define CHECK_Y( p, bbox )                         \
          ( p->y < bbox.yMin || p->y > bbox.yMax )


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Move_To( FT_Vector*  to,
                TBBox_Rec*  user )
  {
    FT_UPDATE_BBOX( to, user->bbox );

    user->last = *to;

    return 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Line_To( FT_Vector*  to,
                TBBox_Rec*  user )
  {
    user->last = *to;

    return 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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
    
    FT_UPDATE_BBOX( to, user->bbox );

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
  cubic_peak( FT_Pos  q1,
              FT_Pos  q2,
              FT_Pos  q3,
              FT_Pos  q4 )
  {
    FT_Pos  peak = 0;
    FT_Int  shift;

    
    
    
    
    
    
    
    

    shift = 27 -
      FT_MSB( FT_ABS( q1 ) | FT_ABS( q2 ) | FT_ABS( q3 ) | FT_ABS( q4 ) );

    if ( shift > 0 )
    {
      
      if ( shift > 2 )
        shift = 2;

      q1 <<=  shift;
      q2 <<=  shift;
      q3 <<=  shift;
      q4 <<=  shift;
    }
    else
    {
      q1 >>= -shift;
      q2 >>= -shift;
      q3 >>= -shift;
      q4 >>= -shift;
    }

    
    
    while ( q2 > 0 || q3 > 0 )
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
        peak = q1;
        break;
      }
      if ( q3 == q4 && q2 <= q4 )
      {
        peak = q4;
        break;
      }
    }

    if ( shift > 0 )
      peak >>=  shift;
    else
      peak <<= -shift;

    return peak;
  }


  static void
  BBox_Cubic_Check( FT_Pos   p1,
                    FT_Pos   p2,
                    FT_Pos   p3,
                    FT_Pos   p4,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    
    
    
    

    if ( p2 > *max || p3 > *max )
      *max += cubic_peak( p1 - *max, p2 - *max, p3 - *max, p4 - *max );

    
    if ( p2 < *min || p3 < *min )
      *min -= cubic_peak( *min - p1, *min - p2, *min - p3, *min - p4 );
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
    (FT_Outline_LineTo_Func) BBox_Line_To,
    (FT_Outline_ConicTo_Func)BBox_Conic_To,
    (FT_Outline_CubicTo_Func)BBox_Cubic_To,
    0, 0
  )


  

  FT_EXPORT_DEF( FT_Error )
  FT_Outline_Get_BBox( FT_Outline*  outline,
                       FT_BBox     *abbox )
  {
    FT_BBox     cbox = {  0x7FFFFFFFL,  0x7FFFFFFFL,
                         -0x7FFFFFFFL, -0x7FFFFFFFL };
    FT_BBox     bbox = {  0x7FFFFFFFL,  0x7FFFFFFFL,
                         -0x7FFFFFFFL, -0x7FFFFFFFL };
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

    for ( n = 0; n < outline->n_points; n++ )
    {
      FT_UPDATE_BBOX( vec, cbox);

      if ( FT_CURVE_TAG( outline->tags[n] ) == FT_CURVE_TAG_ON )
        FT_UPDATE_BBOX( vec, bbox);

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



